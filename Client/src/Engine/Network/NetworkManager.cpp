#include "Pch.h"
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "NetworkManager.h"

#pragma comment(lib, "ws2_32.lib")

namespace Kimgane::Engine
{
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

        WSACleanup();
    }

    void NetworkManager::Update(float deltaTime)
    {
        (void)deltaTime;

        if (!mIsConnected)
            return;

        mPlayerStateSyncTimer += deltaTime;
        if (mPlayerStateSyncTimer >= 0.2f)
        {
            mPlayerStateSyncTimer = 0.0f;

            DirectX::XMFLOAT3 pos{0.0f, 0.0f, 0.0f};
            float yaw = 0.0f;
            bool isJumping = false;
            SendPlayerState(pos, yaw, isJumping);
        }

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

    void NetworkManager::SendMoveStart(int direction, float yaw)
    {
        //std::cout << "[Network] Send Move Start:"<< direction << std::endl;
        C2S_Move packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_MOVE_START;
        packet.direction = static_cast<DIRECTION>(direction);
        packet.yaw = yaw;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    void NetworkManager::SendMoveStop(int direction)
    {
        std::cout << "[Network] Send Move Stop: " << direction << std::endl;
        C2S_Move packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_MOVE_STOP;
        packet.direction = static_cast<DIRECTION>(direction);

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

    void NetworkManager::ProcessPacket(unsigned char* packet)
    {
        PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&packet[1]);

        switch (type)
        {
        case S2C_LOGIN_RESULT:
        {
            auto* loginPacket = reinterpret_cast<S2C_LoginResult*>(packet);

            if (loginPacket->success)
            {
                std::cout << "[Network] Login Success: " << loginPacket->message << std::endl;
            }
            else
            {
                Shutdown();
            }

            break;
        }

        case S2C_AVATAR_INFO:
        {
            auto* avatarPacket = reinterpret_cast<S2C_AvatarInfo*>(packet);

            int playerId = avatarPacket->playerId;
            mMyPlayerId = playerId;

            mObjects[playerId].mIsActive = true;

            mObjects[playerId].mX = avatarPacket->x;
            mObjects[playerId].mY = avatarPacket->y;
            mObjects[playerId].mZ = avatarPacket->z;
            mObjects[playerId].mYaw = avatarPacket->yaw;
            mLocationUpdates.push({playerId, avatarPacket->x, avatarPacket->y, avatarPacket->z, avatarPacket->yaw});

            /*std::cout << "[Network] Player " << playerId << " Avatar Info: (" << avatarPacket->x << ", "
                      << avatarPacket->y << ", " << avatarPacket->z << ')' << std::endl;*/

            break;
        }

        case S2C_ADD_OBJECT:
        {
            auto* addPlayerPacket = reinterpret_cast<S2C_AddObject*>(packet);
            int objectId = addPlayerPacket->objectId;
            mObjects[objectId].mIsActive = true;
            mObjects[objectId].mX = addPlayerPacket->x;
            mObjects[objectId].mY = addPlayerPacket->y;
            mObjects[objectId].mZ = addPlayerPacket->z;
            mObjects[objectId].mYaw = addPlayerPacket->yaw;
            mLocationUpdates.push({objectId, addPlayerPacket->x, addPlayerPacket->y, addPlayerPacket->z, addPlayerPacket->yaw});

            std::cout << "[Network] Object " << objectId << " Added: (" << addPlayerPacket->x << ", "
                      << addPlayerPacket->y << ", " << addPlayerPacket->z << ')' << std::endl;
        }
            break;

        case S2C_MOVE_OBJECT:
            {
                auto* movePacket = reinterpret_cast<S2C_MoveObject*>(packet);
                int objectId = movePacket->objectId;
                mObjects[objectId].mX = movePacket->x;
                mObjects[objectId].mY = movePacket->y;
                mObjects[objectId].mZ = movePacket->z;
                mObjects[objectId].mYaw = movePacket->yaw;
                mLocationUpdates.push({objectId, movePacket->x, movePacket->y, movePacket->z, movePacket->yaw});

                /*std::cout << "[Network] Move Object " << objectId << " : (" << movePacket->x << ", " << movePacket->y
                          << ", " << movePacket->z << ")\n";*/
            }
            break;

        case S2C_REMOVE_OBJECT:
            {
                auto* removePacket = reinterpret_cast<S2C_RemoveObject*>(packet);
                int objectId = removePacket->objectId;
                mObjects[objectId].mIsActive = false;
                mRemovedPlayers.push(objectId);
                std::cout << "[Network] Object " << objectId << " Removed\n";

                break;
            }

        case S2C_ROTATE:
            {
                auto* rotatePacket = reinterpret_cast<S2C_Rotate*>(packet);
                int objectId = rotatePacket->objectId;
                mObjects[objectId].mYaw = rotatePacket->yaw;
                mLocationUpdates.push(
                    {objectId, mObjects[objectId].mX, mObjects[objectId].mY, mObjects[objectId].mZ, rotatePacket->yaw});
            }
            break;

        default:
            std::cout << "[Network] Unknown Packet\n";
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

                mReadCursor = 0;
                mSavedPacketSize = 0;
            }
        }
    }

    void NetworkManager::SendRotate(float yaw)
    {
        C2S_Rotate packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_ROTATE;
        packet.playerId = mMyPlayerId;
        packet.yaw = yaw;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    void NetworkManager::SendJump()
    {
        //std::cout << " [Jump Send] " << '\n';
        C2S_Jump packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_JUMP;

        send(mSocket, reinterpret_cast<char*>(&packet), packet.size, 0);
    }

    void NetworkManager::SendPlayerState(const DirectX::XMFLOAT3& pos, float yaw, bool isJumping)
    {
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
