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
        if (!mIsConnected)
            return;

        char recvBuffer[BUF_SIZE];

        int receivedBytes = recv(mSocket, recvBuffer, BUF_SIZE, 0);

        if (receivedBytes > 0)
        {
            std::cout << "receivedBytes = " << receivedBytes << '\n';
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
        std::cout << "[Network] Send Move Start:"<< direction << std::endl;
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

        std::cout << "[POP] " << update.playerId << " (" << update.x << ", " << update.y << ", "<< update.z << ")\n";

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
            mLocationUpdates.push({playerId, avatarPacket->x, avatarPacket->y, avatarPacket->z, avatarPacket->yaw});

            std::cout << "[Network] Player " << playerId << " Avatar Info: (" << avatarPacket->x << ", "
                      << avatarPacket->y << ", " << avatarPacket->z << ')' << std::endl;

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
                mLocationUpdates.push({objectId, movePacket->x, movePacket->y, movePacket->z, movePacket->yaw});

                std::cout << "[Network] Move Object " << objectId << " : (" << movePacket->x << ", " << movePacket->y
                          << ", " << movePacket->z << ")\n";
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
} // namespace Kimgane::Engine
