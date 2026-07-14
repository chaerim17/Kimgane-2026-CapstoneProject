#include "Pch.h"
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "NetworkManager.h"

#include "../../Shared/protocol.h"

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

    void NetworkManager::SendMoveStart(int direction)
    {
        std::cout << "[Network] Send Move Start:"<< direction << std::endl;
        C2S_Move packet{};

        packet.size = sizeof(packet);
        packet.type = C2S_MOVE_START;
        packet.direction = static_cast<DIRECTION>(direction);

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

    bool NetworkManager::GetPlayerLocation(int* id, float* x, float* y, float* z)
    {
        if (mLocationUpdates.empty())
        {
            return false;
        }

        LocationUpdate update = mLocationUpdates.front();
        mLocationUpdates.pop();

        *id = update.playerId;
        *x = update.x;
        *y = update.y;
        *z = update.z;

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
            //------------------------------
            std::cout << "packet size = " << static_cast<int>(packet[0]) << '\n';

            for (int i = 0; i < packet[0]; ++i)
            {
                printf("%02X ", packet[i]);
            }
            printf("\n");
            //------------------------------
            auto* avatarPacket = reinterpret_cast<S2C_AvatarInfo*>(packet);

            int playerId = avatarPacket->playerId;
            mMyPlayerId = playerId;

            mPlayers[playerId].mIsActive = true;

            mPlayers[playerId].mX = avatarPacket->x;
            mPlayers[playerId].mY = avatarPacket->y;
            mPlayers[playerId].mZ = avatarPacket->z;
            mLocationUpdates.push({playerId, avatarPacket->x, avatarPacket->y, avatarPacket->z});

            std::cout << "[Network] Player " << playerId << " Avatar Info: (" << avatarPacket->x << ", "
                      << avatarPacket->y << ", " << avatarPacket->z << ')' << std::endl;

            break;
        }

        case S2C_ADD_PLAYER:
        {
            auto* addPlayerPacket = reinterpret_cast<S2C_AddPlayer*>(packet);
            int playerId = addPlayerPacket->playerId;
            mPlayers[playerId].mIsActive = true;
            mPlayers[playerId].mX = addPlayerPacket->x;
            mPlayers[playerId].mY = addPlayerPacket->y;
            mPlayers[playerId].mZ = addPlayerPacket->z;
            mLocationUpdates.push({playerId, addPlayerPacket->x, addPlayerPacket->y, addPlayerPacket->z});

            std::cout << "[Network] Player " << playerId << " Added: (" << addPlayerPacket->x << ", "
                      << addPlayerPacket->y << ", " << addPlayerPacket->z << ')' << std::endl;
        }
            break;

        case S2C_MOVE_PLAYER:
            {
                auto* movePacket = reinterpret_cast<S2C_MovePlayer*>(packet);
                int playerId = movePacket->playerId;
                mPlayers[playerId].mX = movePacket->x;
                mPlayers[playerId].mY = movePacket->y;
                mPlayers[playerId].mZ = movePacket->z;
                mLocationUpdates.push({playerId, movePacket->x, movePacket->y, movePacket->z});

                std::cout << "[Network] Move Player " << playerId << " : (" << movePacket->x << ", " << movePacket->y
                          << ", " << movePacket->z << ")\n";
            }
            break;

        case S2C_REMOVE_PLAYER:
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
} // namespace Kimgane::Engine
