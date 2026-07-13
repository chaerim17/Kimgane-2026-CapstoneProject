#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <array>
#include "../../Shared/protocol.h"

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")

constexpr int BUF_SIZE = 200;

enum IOType
{
    IO_SEND,
    IO_RECV,
    IO_ACCEPT
};

class ExpOver
{
public:
    WSAOVERLAPPED mOver;
    IOType mIoType;
    WSABUF mWsa;
    char mBuffer[BUF_SIZE];

    ExpOver()
    {
        ZeroMemory(&mOver, sizeof(mOver));
        mWsa.buf = mBuffer;
        mWsa.len = BUF_SIZE;
    }

    ExpOver(IOType ioType) : mIoType(ioType)
    {
        ZeroMemory(&mOver, sizeof(mOver));
        mWsa.buf = mBuffer;
        mWsa.len = BUF_SIZE;
    }
};

void error_display(const wchar_t* msg, int err_no)
{
    WCHAR* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_no,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    std::wcout << msg;
    std::wcout << L" === 에러 " << lpMsgBuf << std::endl;
    while (true)
        ; // 디버깅 용
    LocalFree(lpMsgBuf);
}

class Session
{
public:
    SOCKET mClient;
    int mId;
    bool mIsConnected;

    ExpOver mRecvOver;
    int mPrevRecv{};
    char mUserName[MAX_NAME_LEN];

    float mX, mY, mZ;

    Session()
    {
        mClient = INVALID_SOCKET;
        mId = -1;
        mIsConnected = false;
    }
    ~Session()
    {
        if (mIsConnected)
            closesocket(mClient);
    }

    void DoRecv()
    {
        std::cout << "DoRecv called for client[" << mId << "]\n";
        DWORD recv_flag = 0;
        ZeroMemory(&mRecvOver.mOver, sizeof(mRecvOver.mOver));
        mRecvOver.mIoType = IO_RECV;
        // 패킷 재조립
        mRecvOver.mWsa.len = BUF_SIZE - mPrevRecv;
        mRecvOver.mWsa.buf = mRecvOver.mBuffer + mPrevRecv;
        WSARecv(mClient, &mRecvOver.mWsa, 1, 0, &recv_flag, &mRecvOver.mOver, nullptr);
    }

    void DoSend(int size, char* buffer)
    {
        ExpOver* o = new ExpOver(IO_SEND);
        o->mWsa.len = size;
        memcpy(o->mBuffer, buffer, size);
        WSASend(mClient, &o->mWsa, 1, 0, 0, &o->mOver, nullptr);
    }

    void SendAvatarInfo()
    {
        S2C_AvatarInfo avatarPacket;
        avatarPacket.size = sizeof(S2C_AvatarInfo);
        avatarPacket.type = S2C_AVATAR_INFO;
        avatarPacket.playerId = mId;
        avatarPacket.x = mX;
        avatarPacket.y = mY;
        avatarPacket.z = mZ;
        DoSend(avatarPacket.size, reinterpret_cast<char*>(&avatarPacket));
    }

    void SendMovePlayer(int moverId);
    void SendAddPlayer(int playerId);

    void SendLoginSuccess()
    {
        S2C_LoginResult loginResultPacket;
        loginResultPacket.size = sizeof(S2C_LoginResult);
        loginResultPacket.type = S2C_LOGIN_RESULT;
        loginResultPacket.success = true;
        strncpy_s(loginResultPacket.message, "Login successful.", sizeof(loginResultPacket.message));
        DoSend(loginResultPacket.size, reinterpret_cast<char*>(&loginResultPacket));
    }
    void SendRemovePlayer(int playerId)
    {
        S2C_RemovePlayer removePlayerPacket;
        removePlayerPacket.size = sizeof(S2C_RemovePlayer);
        removePlayerPacket.type = S2C_REMOVE_PLAYER;
        removePlayerPacket.playerId = playerId;
        DoSend(removePlayerPacket.size, reinterpret_cast<char*>(&removePlayerPacket));
    }

    void ProcessPacket(unsigned char* packet);
};

std::array<std::unique_ptr<Session>, MAX_PLAYERS> clients;

void Session::ProcessPacket(unsigned char* packet)
{
    PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&packet[1]);
    switch (type)
    {
    case C2S_LOGIN:
    {
       /* C2S_Login* loginPacket = reinterpret_cast<C2S_Login*>(packet);
        std::cout << "username = " << loginPacket->username << '\n';
        strncpy_s(mUserName, loginPacket->username, MAX_NAME_LEN);
        */
        std::cout << "Client[" << mId << "] Login: " << mUserName << std::endl;

        SendAvatarInfo();
        SendLoginSuccess();

        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (clients[i] && clients[i]->mIsConnected && i != mId)
            {
                SendAddPlayer(i);
                clients[i]->SendAddPlayer(mId);
            }
        }

        break;
    }

    case C2S_MOVE:
    {
        C2S_Move* movePacket = reinterpret_cast<C2S_Move*>(packet);
        DIRECTION dir = movePacket->direction;

        // TODO: Update player position 계산
        switch (dir)
        {
        case UP:
            break;
        case DOWN:
            break;
        case LEFT:
            break;
        case RIGHT:
            break;
        }

        std::cout << "Player[" << mId << "] moved \n";
        break;
    }
    default:
        std::cout << "Unknown packet type received from player[" << mId << "].\n";
        break;
    }
}

int main()
{
    WSADATA wasData;
    WSAStartup(MAKEWORD(2, 2), &wasData);
    SOCKET serverSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);
    HANDLE hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    CreateIoCompletionPort((HANDLE)serverSocket, hIocp, -1, 0);

    SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    ExpOver acceptOver(IO_ACCEPT);
    AcceptEx(serverSocket, clientSocket, acceptOver.mBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
             NULL, &acceptOver.mOver);

    for (int playerID = 0;;)
    {
        DWORD numBytes;
        ULONG_PTR clientId;
        LPOVERLAPPED overLapped;
        GetQueuedCompletionStatus(hIocp, &numBytes, &clientId, &overLapped, INFINITE);
        if (overLapped == nullptr)
        {
            error_display(L"GQCS Errror: ", WSAGetLastError());
            if (clientId == -1)
            {
                exit(-1);
            }
            std::cout << "client[" << clientId << "] Disconnected.\n";
            clients[clientId]->mIsConnected = false;
            for (auto& cl : clients)
                if (true == cl->mIsConnected)
                    // cl->send_remove_player(clientId);
                    closesocket(clients[clientId]->mClient);
            clients[clientId]->mClient = INVALID_SOCKET;
            continue;
        }

        ExpOver* expOver = reinterpret_cast<ExpOver*>(overLapped);

        switch (expOver->mIoType)
        {
        case IO_ACCEPT:
        {
            std::cout << "Client connected." << std::endl;
            auto session = std::make_unique<Session>();
            session->mIsConnected = true;
            session->mClient = clientSocket;
            session->mId = playerID;
            clients[playerID] = std::move(session);

            CreateIoCompletionPort((HANDLE)clientSocket, hIocp, playerID, 0);

            std::cout << "Register Recv\n";
            clients[playerID]->DoRecv();
            std::cout << "Recv Registered\n";
            ++playerID;

            clientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

            ZeroMemory(&acceptOver.mOver, sizeof(acceptOver.mOver));

            AcceptEx(serverSocket, clientSocket, acceptOver.mBuffer, 0, sizeof(SOCKADDR_IN) + 16,
                       sizeof(SOCKADDR_IN) + 16, NULL, &acceptOver.mOver);
            break;
        }
        case IO_RECV:
        {
            int playerId = static_cast<int>(clientId);

            if (numBytes == 0)
            {
                std::cout << "Client[" << playerId << "] Disconnected\n";
                clients[playerId]->mIsConnected = false;
                closesocket(clients[playerId]->mClient);
                clients[playerId].reset();
                break;
            }
            std::cout << "Client[" << playerId << "] Recv: " << numBytes << std::endl;

            Session* session = clients[playerId].get();
            unsigned char* packetPtr = reinterpret_cast<unsigned char*>(expOver->mBuffer);
            int dataSize = numBytes + session->mPrevRecv;

            while (dataSize > 0)
            {
                const int packetSize = packetPtr[0];
                if (packetSize > dataSize)
                    break;

                session->ProcessPacket(packetPtr);
                packetPtr += packetSize;
                dataSize -= packetSize;
            }

            if (dataSize > 0)
            {
                memmove(session->mRecvOver.mBuffer, packetPtr, dataSize);
                session->mPrevRecv = dataSize;
            }
            else
            {
                session->mPrevRecv = 0;
            }
            session->DoRecv();

            break;
        }
        case IO_SEND:
            delete expOver;
            break;
        default:
            std::cout << "Unknown IO type." << std::endl;
            exit(-1);
            break;
        }
    }
    closesocket(serverSocket);
    WSACleanup();
}

void Session::SendAddPlayer(int playerId)
{
    S2C_AddPlayer addPlayerPacket;
    addPlayerPacket.size = sizeof(addPlayerPacket);
    addPlayerPacket.type = S2C_ADD_PLAYER;
    addPlayerPacket.playerId = playerId;
    addPlayerPacket.x = clients[playerId]->mX;
    addPlayerPacket.y = clients[playerId]->mY;
    addPlayerPacket.z = clients[playerId]->mZ;

    DoSend(addPlayerPacket.size, reinterpret_cast<char*>(&addPlayerPacket));
}
