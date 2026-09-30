#include "Pch.h"
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "NetworkManager.h"

#pragma comment(lib, "ws2_32.lib")

namespace Kimgane::Engine
{
    namespace
    {
        constexpr float PLAYER_STATE_SYNC_INTERVAL_SEC = 0.2f;
        using ObjectType = Kimgane::Shared::World::ObjectType;

        bool IsCharacter(ObjectType type) noexcept
        {
            return type == ObjectType::Player || type == ObjectType::Npc;
        }

        bool IsValidObjectId(int id, ObjectType type) noexcept
        {
            switch (type)
            {
            case ObjectType::Player: return id >= 0 && id < MAX_PLAYERS;
            case ObjectType::Npc: return id >= MAX_PLAYERS && id < MAX_OBJECTS;
            case ObjectType::ItemBox: return id >= MAX_OBJECTS;
            default: return false;
            }
        }
    }

    bool NetworkManager::Initialize()
    {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);

        mSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (INVALID_SOCKET == mSocket)
        {
            WSACleanup();
            return false;
        }

        SOCKADDR_IN serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);

        InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);

        if (connect(mSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
        {
            std::cout << "[Network] Connect Failed.\n";
            closesocket(mSocket);
            mSocket = INVALID_SOCKET;
            WSACleanup();

            return false;
        }

        mIsConnected = true;

        std::cout << "[Network] Connect Success to Server.\n";

        u_long nonBlocking = 1;

        ioctlsocket(mSocket, FIONBIO, &nonBlocking);

        C2S_Login loginPacket{};

        loginPacket.size = sizeof(loginPacket);
        loginPacket.type = C2S_LOGIN;

        send(mSocket, reinterpret_cast<char*>(&loginPacket), sizeof(loginPacket), 0);

        std::cout << "[Network] Send Login Packet: " << sizeof(C2S_Login) << " byte.\n";

        return true;
    }

    void NetworkManager::Shutdown()
    {
        if (mSocket != INVALID_SOCKET)
        {
            closesocket(mSocket);
            mSocket = INVALID_SOCKET;
        }
        mIsConnected = false;
        mObjects.clear();
        mInventory.clear();
        mOpenItemBoxResults = {};
        mLocationUpdates = {};
        mRemovedPlayers = {};
        mMyPlayerId = -1;
        mCurrentPacketSize = mSavedPacketSize = mReadCursor = 0;
        mPlayerStateSyncTimer = 0.0f;

        WSACleanup();
    }

    void NetworkManager::Update(float deltaTime)
    {
        (void)deltaTime;

        if (!mIsConnected)
            return;

        char recvBuffer[BUF_SIZE];

        int receivedBytes = recv(mSocket, recvBuffer, BUF_SIZE, 0);

        if (receivedBytes > 0)
        {
            //std::cout << "receivedBytes = " << receivedBytes << '\n';
            ProcessData(recvBuffer, receivedBytes);
        } 
        else if (receivedBytes == 0)
        {
            std::cout << "[Network] Server Disconnected.\n";

            Shutdown();
        }
        else
        {
            int error = WSAGetLastError();

            if (error != WSAEWOULDBLOCK)
            {
                Shutdown();
            }
        }
    }

    bool NetworkManager::IsConnected() const noexcept
    {
        return mIsConnected && mSocket != INVALID_SOCKET;
    }

    bool NetworkManager::ConsumePlayerStateSyncTick(float deltaTimeSec) noexcept
    {
        if (!IsConnected())
        {
            mPlayerStateSyncTimer = 0.0f;
            return false;
        }

        mPlayerStateSyncTimer += std::max(deltaTimeSec, 0.0f);
        if (mPlayerStateSyncTimer < PLAYER_STATE_SYNC_INTERVAL_SEC)
        {
            return false;
        }

        mPlayerStateSyncTimer = 0.0f;
        return true;
    }

    void NetworkManager::SendMoveStart(int direction, float yaw)
    {
        if (!IsConnected())
        {
            return;
        }

        //std::cout << "[Network] Send Move Start:"<< direction << std::endl;
        C2S_Move packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_MOVE_START;
        packet.direction = static_cast<DIRECTION>(direction);
        packet.yaw = yaw;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    void NetworkManager::SendMoveStop(int direction, float yaw)
    {
        if (!IsConnected())
        {
            return;
        }

        std::cout << "[Network] Send Move Stop: " << direction << std::endl;
        C2S_Move packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_MOVE_STOP;
        packet.direction = static_cast<DIRECTION>(direction);
        packet.yaw = yaw;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    bool NetworkManager::GetPlayerLocation(int* id, float* x, float* y, float* z, float* yaw)
    {
        if (mLocationUpdates.empty())
        {
            return false;
        }

        LocationUpdate update = mLocationUpdates.front();
        mLocationUpdates.pop();

        // 회전 패킷 업데이트 디버깅
        //std::cout << "[POP] " << update.playerId << " (" << update.x << ", " << update.y << ", " << update.z << ", "
        //          << update.yaw << ")\n";

        *id = update.playerId;
        *x = update.x;
        *y = update.y;
        *z = update.z;
        *yaw = update.yaw;

        return true;
    }

    bool NetworkManager::GetRemovedPlayer(int* playerId)
    {
        if (mRemovedPlayers.empty())
        {
            return false;
        }

        *playerId = mRemovedPlayers.front();
        mRemovedPlayers.pop();
        std::cout << "[REMOVE POP] " << *playerId << '\n';

        return true;
    }

    void NetworkManager::SendOpenItemBox(int objectId)
    {
        if (!IsConnected() || objectId < 0)
            return;
        C2S_OpenItemBox packet{};
        packet.size = sizeof(packet);
        packet.type = C2S_OPEN_ITEM_BOX;
        packet.objectId = objectId;
        send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
        //std::cout << "[OPEN_BOX SEND] objectId=" << objectId << " size=" << sizeof(packet) << '\n';
    }

    bool NetworkManager::GetOpenItemBoxResult(S2C_OpenItemBoxResult& result)
    {
        if (mOpenItemBoxResults.empty())
            return false;
        result = mOpenItemBoxResults.front();
        mOpenItemBoxResults.pop();
        return true;
    }

    void NetworkManager::ProcessPacket(unsigned char* packet)
    {
        PACKET_TYPE type{};
        memcpy(&type, packet + 1, sizeof(type));
        std::size_t expectedSize = 0;
        switch (type)
        {
        case S2C_LOGIN_RESULT: expectedSize = sizeof(S2C_LoginResult); break;
        case S2C_AVATAR_INFO: expectedSize = sizeof(S2C_AvatarInfo); break;
        case S2C_ADD_OBJECT: expectedSize = sizeof(S2C_AddObject); break;
        case S2C_MOVE_OBJECT: expectedSize = sizeof(S2C_MoveObject); break;
        case S2C_DAMAGE: expectedSize = sizeof(S2C_Damage); break;
        case S2C_REMOVE_OBJECT: expectedSize = sizeof(S2C_RemoveObject); break;
        case S2C_ROTATE: expectedSize = sizeof(S2C_Rotate); break;
        case S2C_OPEN_ITEM_BOX_RESULT: expectedSize = sizeof(S2C_OpenItemBoxResult); break;
        default: return;
        }

        if (packet[0] != expectedSize)
        {
            std::cout << "[Network] Packet size mismatch: packetType=" << static_cast<int>(type)
                      << " received=" << static_cast<int>(packet[0])
                      << " expected=" << expectedSize << '\n';
            Shutdown();
            return;
        }

        switch (type)
        {
        case S2C_LOGIN_RESULT:
        {
            const auto& message = *reinterpret_cast<const S2C_LoginResult*>(packet);
            if (message.success)
                std::cout << "[Network] Login Success.\n";
            else
                Shutdown();
            break;
        }
        case S2C_OPEN_ITEM_BOX_RESULT:
        {
            const auto& message = *reinterpret_cast<const S2C_OpenItemBoxResult*>(packet);
            using ItemId = Kimgane::Shared::Items::ItemId;
            switch (message.status)
            {
            case OpenItemBoxStatus::Success:
                if (message.quantity == 0 || message.totalQuantity < message.quantity)
                    return;

                switch (message.itemId)
                {
                case ItemId::HpPotion:
                case ItemId::Chip:
                case ItemId::Armor:
                    break;

                default:
                    return;
                }
                mInventory[message.itemId] = message.totalQuantity;
                break;

            case OpenItemBoxStatus::NotFound:
            case OpenItemBoxStatus::NotItemBox:
            case OpenItemBoxStatus::TooFar:
            case OpenItemBoxStatus::RewardFailed:
                break;

            default:
                return;
            }
            //std::cout << "[OPEN_BOX RECV] objectId=" << message.objectId
                      //<< " status=" << static_cast<int>(message.status)
                      //<< " item=" << static_cast<int>(message.itemId)
                      //<< " quantity=" << message.quantity << " serverTotal=" << message.totalQuantity
                      //<< " clientTotal=" << GetItemQuantity(message.itemId) << '\n';
            mOpenItemBoxResults.push(message);
            break;
        }
        case S2C_AVATAR_INFO:
        {
            const auto& message = *reinterpret_cast<const S2C_AvatarInfo*>(packet);
            if (!IsValidObjectId(message.playerId, ObjectType::Player))
                return;
            mMyPlayerId = message.playerId;
            auto& state = mObjects[message.playerId];
            state = {true, ObjectType::Player, message.x, message.y, message.z, message.yaw, 0, 0};
            mLocationUpdates.push({message.playerId, state.mX, state.mY, state.mZ, state.mYaw});
            break;
        }
        case S2C_ADD_OBJECT:
        {
            const auto& message = *reinterpret_cast<const S2C_AddObject*>(packet);
            // 디버깅
            //std::cout << "[ADD_OBJECT RECV] size=" << static_cast<int>(message.size)
            //          << " packetType=" << static_cast<int>(message.type)
            //          << " objectId=" << message.objectId
            //          << " objectType=" << static_cast<int>(message.objectType)
            //          << " pos=(" << message.x << ", " << message.y << ", " << message.z << ')'
            //          << " yaw=" << message.yaw
            //          << " maxHp=" << message.maxHp << " currentHp=" << message.currentHp << '\n';
            if (!IsValidObjectId(message.objectId, message.objectType))
            {
                std::cout << "[ADD_OBJECT REJECT] Invalid object ID/type.\n";
                return;
            }
            auto& state = mObjects[message.objectId];
            state = {true, message.objectType, message.x, message.y, message.z, message.yaw,
                     message.maxHp, message.currentHp};
            // 상자는 상태만 저장 씬 연결은 별도로 필요
            if (IsCharacter(state.mType))
                mLocationUpdates.push({message.objectId, state.mX, state.mY, state.mZ, state.mYaw});
            break;
        }
        case S2C_MOVE_OBJECT:
        {
            const auto& message = *reinterpret_cast<const S2C_MoveObject*>(packet);
            auto it = mObjects.find(message.objectId);
            if (it == mObjects.end())
                return;
            auto& state = it->second;
            state.mX = message.x;
            state.mY = message.y;
            state.mZ = message.z;
            state.mYaw = message.yaw;
            if (IsCharacter(state.mType))
                mLocationUpdates.push({message.objectId, state.mX, state.mY, state.mZ, state.mYaw});
            break;
        }
        case S2C_DAMAGE:
        {
            const auto& message = *reinterpret_cast<const S2C_Damage*>(packet);
            auto it = mObjects.find(message.targetId);
            if (it == mObjects.end())
                return;
            it->second.mMaxHp = message.maxHp;
            it->second.mCurrentHp = message.currentHp;
            std::cout << "[DAMAGE RECV] attackerId=" << message.attackerId
                      << " targetId=" << message.targetId << " damage=" << message.damage
                      << " maxHp=" << message.maxHp << " currentHp=" << message.currentHp << '\n';
            break;
        }
        case S2C_REMOVE_OBJECT:
        {
            const auto& message = *reinterpret_cast<const S2C_RemoveObject*>(packet);
            auto it = mObjects.find(message.objectId);
            if (it == mObjects.end())
                return;
            if (IsCharacter(it->second.mType))
                mRemovedPlayers.push(message.objectId);
            //const bool wasItemBox = it->second.mType == ObjectType::ItemBox;
            mObjects.erase(it);
            //if (wasItemBox)
                //std::cout << "[ITEM_BOX REMOVE RECV] objectId=" << message.objectId
                          //<< " remaining=" << (FindObject(message.objectId) != nullptr) << '\n';
            break;
        }
        case S2C_ROTATE:
        {
            const auto& message = *reinterpret_cast<const S2C_Rotate*>(packet);
            auto it = mObjects.find(message.objectId);
            if (it == mObjects.end())
                return;
            auto& state = it->second;
            state.mYaw = message.yaw;
            if (IsCharacter(state.mType))
                mLocationUpdates.push({message.objectId, state.mX, state.mY, state.mZ, state.mYaw});
            break;
        }
        default:
            break;
        }
    }

    void NetworkManager::ProcessData(char* buffer, size_t receivedBytes)
    {
        unsigned char* packet = reinterpret_cast<unsigned char*>(buffer);

        int dataSize = static_cast<int>(receivedBytes);

        while (dataSize > 0)
        {
            if (mCurrentPacketSize == 0)
            {
                if (packet[0] < 1 + sizeof(PACKET_TYPE) || packet[0] > BUF_SIZE)
                {
                    Shutdown();
                    return;
                }
                mCurrentPacketSize = packet[0];
                mSavedPacketSize = mCurrentPacketSize;
                mReadCursor = 0;
            }

            int bytesToCopy = std::min(dataSize, mCurrentPacketSize);

            memcpy(mPacketBuffer + mReadCursor, packet, bytesToCopy);

            mReadCursor += bytesToCopy;
            mCurrentPacketSize -= bytesToCopy;

            dataSize -= bytesToCopy;
            packet += bytesToCopy;

            if (mCurrentPacketSize == 0)
            {
                ProcessPacket(reinterpret_cast<unsigned char*>(mPacketBuffer));
                if (!mIsConnected)
                    return;

                mReadCursor = 0;
                mSavedPacketSize = 0;
            }
        }
    }

    void NetworkManager::SendRotate(float yaw)
    {
        if (!IsConnected())
        {
            return;
        }

        C2S_Rotate packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_ROTATE;
        packet.playerId = mMyPlayerId;
        packet.yaw = yaw;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    void NetworkManager::SendJump()
    {
        if (!IsConnected())
        {
            return;
        }

        //std::cout << " [Jump Send] " << '\n';
        C2S_Jump packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_JUMP;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    void NetworkManager::SendShoot(const DirectX::XMFLOAT3& direction)
    {
        if (!IsConnected())
        {
            return;
        }

        C2S_Shoot packet{};
        packet.size = sizeof(packet);
        packet.type = C2S_SHOOT;
        packet.playerId = mMyPlayerId;
        packet.direction.x = direction.x;
        packet.direction.y = direction.y;
        packet.direction.z = direction.z;

        int sentBytes = send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);

        std::cout << "[SHOOT SEND] playerId=" << packet.playerId << " direction=(" << packet.direction.x << ", "
                  << packet.direction.y << ", " << packet.direction.z << ')' << " bytes=" << sentBytes << '/'
                  << static_cast<int>(packet.size) << '\n';
        
    }

    void NetworkManager::SendPlayerState(const DirectX::XMFLOAT3& pos, float yaw, bool isJumping)
    {
        if (!IsConnected())
        {
            return;
        }

        C2S_PlayerState packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_PLAYER_STATE;

        packet.x = pos.x;
        packet.y = pos.y;
        packet.z = pos.z;

        packet.yaw = yaw;
        packet.isJumping = isJumping;

        send(mSocket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
    }

} // namespace Kimgane::Engine
