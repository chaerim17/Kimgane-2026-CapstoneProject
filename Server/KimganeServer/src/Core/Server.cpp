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

            float moveX = 0.0f;
            float moveZ = 0.0f;

            float yaw = clients[i]->mYaw;

            float forwardX = sinf(yaw);
            float forwardZ = cosf(yaw);

            float rightX = cosf(yaw);
            float rightZ = -sinf(yaw);

            if (clients[i]->mMoveUp)
            {
                moveX += forwardX;
                moveZ += forwardZ;
                moved = true;
            }

            if (clients[i]->mMoveDown)
            {
                moveX -= forwardX;
                moveZ -= forwardZ;
                moved = true;
            }

            if (clients[i]->mMoveRight)
            {
                moveX += rightX;
                moveZ += rightZ;
                moved = true;
            }

            if (clients[i]->mMoveLeft)
            {
                moveX -= rightX;
                moveZ -= rightZ;
                moved = true;
            }

            // 대각선 보정
            float len = sqrtf(moveX * moveX + moveZ * moveZ);
            if (len > 0.0f)
            {
                moveX /= len;
                moveZ /= len;
            }

            nextX += moveX * MOVE_SPEED * DELTA_TIME;
            nextZ += moveZ * MOVE_SPEED * DELTA_TIME;

            //이동패킷 값 디버깅
            //std::cout << "yaw=" << yaw << " moveX=" << moveX << " moveZ=" << moveZ << '\n';
            static bool test = false;

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
            float terrainHeight = mTerrain->SampleHeightM(sampleX, sampleZ);

            
            if (!blocked)
            {
                clients[i]->mX = nextX;
                clients[i]->mZ = nextZ;
            }
            else
            {
                /*std::cout << "[HOUSE HIT] "
                          << "Player=" << i << " Pos(" << nextX << ", " << clients[i]->mY << ", " << nextZ << ")\n";
                std::cout << "player : " << nextX << ", " << nextZ << " blocked=" << blocked << '\n';*/
            }

            if (not clients[i]->mIsJumping)
            {
                clients[i]->mY = terrainHeight;
            }

            // std::cout << "[TIMER] " << i << " Jump=" << clients[i]->mIsJumping << '\n';

            for (int p = 0; p < MAX_PLAYERS; ++p)
            {
                if (clients[p] && clients[p]->IsConnected())
                {
                    clients[p]->SendMoveObject(i);
                }
            }

            // 점프 처리
            if (clients[i]->mIsJumping)
            {
                clients[i]->mY += clients[i]->mVelocityY * DELTA_TIME;

                clients[i]->mVelocityY -= GRAVITY * DELTA_TIME;

                if (clients[i]->mVelocityY < 0.0f && clients[i]->mY <= terrainHeight)
                {
                    //std::cout << "Land\n";

                    clients[i]->mY = terrainHeight;
                    clients[i]->mVelocityY = 0.0f;
                    clients[i]->mIsJumping = false;
                }
            }

            //점프 디버깅
            /*if (clients[i]->mIsJumping)
            {
                std::cout << "[Player " << i << "] "
                          << "Y=" << clients[i]->mY << " VelY=" << clients[i]->mVelocityY << '\n';
            }*/

            if (not clients[i]->mIsJumping)
            {
                clients[i]->mY = terrainHeight;
            }

            if (!moved && !clients[i]->mIsJumping)
                continue;
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

    auto houseCollisionBoxes = Kimgane::Shared::Geometry::CollisionBoxLoader::Load("Shared/Geometry/TestHouse_collision.txt");

    int colliderId = 10000;

    for (const auto& collisionBox : houseCollisionBoxes)
    {
        // 충돌 시 디버깅용 로그 (어느 물체와 충돌했는지 확인)
        /*std::cout << "Register ID=" << colliderId << " Center(" << collisionBox.box.centerM.x << ", "
                  << collisionBox.box.centerM.y << ", " << collisionBox.box.centerM.z << ") "
                  << "Extent(" << collisionBox.box.halfExtentsM.x << ", " << collisionBox.box.halfExtentsM.y << ", "
                  << collisionBox.box.halfExtentsM.z << ")\n";*/

        mCollisionWorld.AddOrUpdateBody({colliderId, collisionBox.box,
                                         Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD,
                                         Kimgane::Shared::Physics::CollisionLayer::ALL, false});

        ++colliderId;
    }

    //std::cout << "Loaded house boxes = " << houseCollisionBoxes.size() << '\n';

    for (size_t i = 0; i < std::min<size_t>(3, houseCollisionBoxes.size()); ++i)
    {
        const auto& box = houseCollisionBoxes[i].box;

        /*std::cout << "[House Box " << i << "] "
                  << "Center(" << box.centerM.x << ", " << box.centerM.y << ", " << box.centerM.z << ") "
                  << "Extent(" << box.halfExtentsM.x << ", " << box.halfExtentsM.y << ", " << box.halfExtentsM.z
                  << ")\n";*/
    }

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
