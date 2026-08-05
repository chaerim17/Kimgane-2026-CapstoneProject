#include <algorithm>
#include <thread>
#include <cfloat>
#include "Server.h"
#include"../../../../Shared/Terrain/TerrainConfig.h"
#include "../NPC/NpcSetting.h"

std::array<std::unique_ptr<Session>, MAX_PLAYERS> clients;

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

// Todo : 타이머쓰레드 코드 분리 필요
void Server::TimerThread()
{
    constexpr float DELTA_TIME = 0.05f; // 50ms
    constexpr float MOVE_SPEED = 5.0f;

    while (true)
    {
        Sleep(50);

        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (!clients[i] || !clients[i]->IsConnected())
                continue;

            float nextX = clients[i]->mX;
            float nextZ = clients[i]->mZ;

            bool moved = false;

            if (clients[i]->mMoveUp)
            {
                nextZ += MOVE_SPEED * DELTA_TIME;
                moved = true;
            }

            if (clients[i]->mMoveDown)
            {
                nextZ -= MOVE_SPEED * DELTA_TIME;
                moved = true;
            }

            if (clients[i]->mMoveLeft)
            {
                nextX -= MOVE_SPEED * DELTA_TIME;
                moved = true;
            }

            if (clients[i]->mMoveRight)
            {
                nextX += MOVE_SPEED * DELTA_TIME;
                moved = true;
            }

            if (!moved)
                continue;

            // 충돌검사
            // 이동할 위치 기준 캡슐 생성
            Kimgane::Shared::Physics::Vec3 footPosM{nextX, clients[i]->mY, nextZ};

            Kimgane::Shared::Physics::CollisionBody playerBody{i,
                Kimgane::Shared::Physics::MakeCapsuleFromFootPosition(
                    footPosM, Kimgane::Shared::Physics::Settings::PLAYER_CAPSULE_RADIUS_M,
                    Kimgane::Shared::Physics::Settings::PLAYER_CAPSULE_HEIGHT_M),
                Kimgane::Shared::Physics::CollisionLayer::PLAYER, Kimgane::Shared::Physics::CollisionLayer::ALL, false};

            bool blocked = mCollisionWorld.HasBlockingContact(playerBody, i);

            float sampleX = clients[i]->mX + mTerrain->GetWorldWidthM() * 0.5f;
            float sampleZ = clients[i]->mZ + mTerrain->GetWorldLengthM() * 0.5f;
            clients[i]->mY = mTerrain->SampleHeightM(sampleX, sampleZ);


            if (!blocked)
            {
                clients[i]->mX = nextX;
                clients[i]->mZ = nextZ;
            }
            else
            {
                std::cout << "HOUSE HIT\n";
                std::cout << "player : " << nextX << ", " << nextZ << " blocked=" << blocked << '\n';
            }

            clients[i]->mY = mTerrain->SampleHeightM(sampleX, sampleZ);

            for (int p = 0; p < MAX_PLAYERS; ++p)
            {
                if (clients[p] && clients[p]->IsConnected())
                {
                    clients[p]->SendMoveObject(i);
                }
            }
        }
        NpcSetting::Update(*mTerrain);
    }
}

Server::Server()
    : mListenSocket(INVALID_SOCKET), mClientSocket(INVALID_SOCKET), mIocp(nullptr), mAcceptOver(IO_ACCEPT){}
Server::~Server()
{
    if (mListenSocket != INVALID_SOCKET)
        closesocket(mListenSocket);

    WSACleanup();
}

bool Server::Initialize()
{
    WSADATA wasData;
    WSAStartup(MAKEWORD(2, 2), &wasData);

    // Terrain 로드
    try
    {
        mTerrain = TerrainHeightMap::LoadRawAuto(TerrainConfig::TERRAIN_RAW_PATH, TerrainConfig::CELL_SPACING,
                                                 TerrainConfig::HEIGHT_SCALE);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }

    // ObjLoader
    auto mesh = Kimgane::Shared::Geometry::ObjLoader::Load("Shared/Geometry/TestHouse");
    //std::cout << "Vertex Count : " << mesh.positionsM.size() << std::endl;

    float minX = FLT_MAX;
    float minY = FLT_MAX;
    float minZ = FLT_MAX;

    float maxX = -FLT_MAX;
    float maxY = -FLT_MAX;
    float maxZ = -FLT_MAX;

    for (const auto& p : mesh.positionsM)
    {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        minZ = std::min(minZ, p.z);

        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
        maxZ = std::max(maxZ, p.z);
    }

    std::cout << "Min : " << minX << ", " << minY << ", " << minZ << '\n';
    std::cout << "Max : " << maxX << ", " << maxY << ", " << maxZ << '\n';

    //// 2층집 offset
    ////왼쪽 벽
    //Kimgane::Shared::Physics::Box leftWall{{6.6f, 10.91f, 2.18f}, {0.2f, 6.2f, 3.6f}};
    //// 오른쪽 벽
    //Kimgane::Shared::Physics::Box rightWall{{14.2f, 10.91f, 2.18f}, {0.2f, 6.2f, 3.6f}};
    //// 뒤 벽
    //Kimgane::Shared::Physics::Box rearWall{{10.4f, 10.91f, 5.6f}, {4.0f, 6.2f, 0.2f}};
    ////2층바닥
    //Kimgane::Shared::Physics::Box secondFloor{{10.4f, 10.71f, 2.18f}, {4.0f, 0.2f, 3.6f}};

    //mCollisionWorld.AddOrUpdateBody(
    //    {10000, leftWall, Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD, Kimgane::Shared::Physics::CollisionLayer::ALL, false});

    //mCollisionWorld.AddOrUpdateBody({10001, rightWall, Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD, Kimgane::Shared::Physics::CollisionLayer::ALL, false});

    //mCollisionWorld.AddOrUpdateBody({10002, rearWall, Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD, Kimgane::Shared::Physics::CollisionLayer::ALL, false});
    //mCollisionWorld.AddOrUpdateBody({10003, secondFloor, Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD, Kimgane::Shared::Physics::CollisionLayer::ALL, false});

    const float housePosX = 0.0f;
    const float housePosY = 4.71f;
    const float housePosZ = 0.0f;

    Kimgane::Shared::Physics::Box debugBox{
        {housePosX + (minX + maxX) * 0.5f, housePosY + (minY + maxY) * 0.5f, housePosZ + (minZ + maxZ) * 0.5f},
        {(maxX - minX) * 0.5f * 0.5f, (maxY - minY) * 0.5f, (maxZ - minZ) * 0.5f * 0.5f}};

    mCollisionWorld.AddOrUpdateBody({10000, debugBox, Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD,
                                     Kimgane::Shared::Physics::CollisionLayer::ALL, false});

    NpcSetting::Initialize(*mTerrain);

    mListenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    bind(mListenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
    listen(mListenSocket, SOMAXCONN);

    mIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    // CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(TimerThread), nullptr, 0, nullptr);
    CreateIoCompletionPort((HANDLE)mListenSocket, mIocp, -1, 0);
    std::thread(&Server::TimerThread, this).detach();
    mClientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    ExpOver acceptOver(IO_ACCEPT);
    AcceptEx(mListenSocket, mClientSocket, mAcceptOver.mBuffer, 0, sizeof(SOCKADDR_IN) + 16,
             sizeof(SOCKADDR_IN) + 16, NULL, &mAcceptOver.mOver);

    return true;
}

void Server::Run()
{
    for (int playerID = 0;;)
    {
        DWORD numBytes;
        ULONG_PTR clientId;
        LPOVERLAPPED overLapped;
        BOOL result = GetQueuedCompletionStatus(mIocp, &numBytes, &clientId, &overLapped, INFINITE);

        if (overLapped == nullptr)
        {
            error_display(L"GQCS Errror: ", WSAGetLastError());

            if (clientId == -1)
            {
                exit(-1);
            }

            std::cout << "client[" << clientId << "] Disconnected.\n";

            clients[clientId]->Disconnect();
            clients[clientId].reset();

            continue;
        }

        ExpOver* expOver = reinterpret_cast<ExpOver*>(overLapped);

        switch (expOver->mIoType)
        {
        case IO_ACCEPT:
        {
            HandleAccept(playerID);
            break;
        }
        case IO_RECV:
        {
            HandleRecv(static_cast<int>(clientId), numBytes, expOver);
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
}

void Server::HandleAccept(int& playerID)
{
    std::cout << "Client connected." << std::endl;
    auto session = std::make_unique<Session>();
    session->Connect(mClientSocket, playerID);
    clients[playerID] = std::move(session);

    CreateIoCompletionPort((HANDLE)mClientSocket, mIocp, playerID, 0);
    std::cout << "Register Recv\n";
    clients[playerID]->DoRecv();
    std::cout << "Recv Registered\n";
    if (playerID >= MAX_PLAYERS)
    {
        closesocket(mClientSocket);
        return;
    }
    ++playerID;

    mClientSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    ZeroMemory(&mAcceptOver.mOver, sizeof(mAcceptOver.mOver));

    AcceptEx(mListenSocket, mClientSocket, mAcceptOver.mBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
             NULL, &mAcceptOver.mOver);
}

void Server::HandleRecv(int playerId, DWORD numBytes, ExpOver* expOver)
{
    if (numBytes == 0)
    {
        HandleDisconnect(playerId);
        return;
    }
    //std::cout << "Client[" << playerId << "] Recv: " << numBytes << std::endl;

    Session* session = clients[playerId].get();
    unsigned char* packetPtr = reinterpret_cast<unsigned char*>(expOver->mBuffer);
    int dataSize = numBytes + session->mPrevRecv;

    // 패킷 재조립
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
}

void Server::HandleDisconnect(int objectId)
{
    std::cout << "Client[" << objectId << "] Disconnected\n";

    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (!clients[i] || !clients[i]->IsConnected() || i == objectId)
        {
            continue;
        }

        clients[i]->SendRemoveObject(objectId);
    }

    clients[objectId]->Disconnect();
    clients[objectId].reset();
}
