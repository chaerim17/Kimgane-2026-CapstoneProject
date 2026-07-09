#include "Pch.h"
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "NetworkManager.h"

#include "../../../Shared/protocol.h"

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

        if (connect(mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            std::cout << "[Network] Connect Failed.\n ";
            return false;
        }
        std::cout << "[Network] Connect Success to Server.\n";

        u_long nonBlocking = 1;
        ioctlsocket(mSocket, FIONBIO, &nonBlocking);

        C2S_Login loginPacket{};
        loginPacket.size = sizeof(loginPacket);
        loginPacket.type = C2S_LOGIN;
        send(mSocket, reinterpret_cast<char*>(&loginPacket), loginPacket.size, 0);
        std::cout << "[Network] Send Login Packet: " << loginPacket.size << " byte.\n";

        return true;
    }

    void NetworkManager::Shutdown() {}

    void NetworkManager::Update(float deltaTime) {}

    void NetworkManager::SendMoveInput(int direction) {}

    bool NetworkManager::get_player_location(int* id, float* x, float* y, float* z)
    {
        return false;
    }

    void NetworkManager::ProcessPacket(unsigned char* packet) {}

    void NetworkManager::ProcessData(char* buffer, size_t receivedBytes) {}
} // namespace Kimgane::Engine
