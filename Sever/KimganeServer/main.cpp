#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
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
};

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
    AcceptEx(serverSocket, clientSocket, acceptOver.mBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL,
             &acceptOver.mOver);

    for (;;)
    {
        DWORD numBytes;
        ULONG_PTR clientId;
        LPOVERLAPPED overLapped;
        GetQueuedCompletionStatus(hIocp, &numBytes, &clientId, &overLapped, INFINITE);
        /*if (overLapped == nullptr)
        {
            error_display(L"GQCS Errror: ", WSAGetLastError());
            if (clientId == -1)
            {
                exit(-1);
            }
            std::cout << "client[" << clientId << "] Disconnected.\n";
            clients[clientId].m_is_connected = false;
            for (auto& cl : clients)
                if (true == cl.m_is_connected)
                    cl.send_remove_player(clientId);
            closesocket(clients[clientId].m_client);
            clients[clientId].m_client = INVALID_SOCKET;
            continue;
        }*/

        ExpOver* expOver = reinterpret_cast<ExpOver*>(overLapped);
        switch (expOver->mIoType)
        {
        case IO_ACCEPT:
            break;
        case IO_RECV:
        break;
        case IO_SEND:
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
