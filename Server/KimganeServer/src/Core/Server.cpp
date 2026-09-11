#include <algorithm>
#include <thread>
#include "../../../../Shared/Physics/CharacterMovement.h"
#include "Server.h"
#include "../../../../Shared/Maps/LunarOutpost/LunarMapSettings.h"
#include "../../../../Shared/Maps/LunarOutpost/LunarMovement.h"
#include "../../../../Shared/IO/AssetPathResolver.h"
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

void Server::TimerThread()
{
    using namespace Kimgane::Shared::Physics;
    constexpr float DELTA_TIME = 0.05f; // 50ms
    constexpr float MOVE_SPEED = 5.0f;

    // 서버가 로드한 충돌 박스를 Shared 계산에 필요한 월드 좌표로 변환 (y값 보정해주기)
    std::vector<Box> groundBoxes;
    groundBoxes.reserve(mHouseCollisionBoxes.size());
    for (const auto& collisionBox : mHouseCollisionBoxes)
    {
        Box worldBox = collisionBox.box;
        worldBox.centerM.y += TEST_HOUSE_WORLD_OFFSET_Y;
        groundBoxes.push_back(worldBox);
    }

    while (true)
    {
        Sleep(50);
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (!clients[i] || !clients[i]->IsConnected())
                continue;

            // 실제 세션을 참조해서 shared movement state에서 이동계산
            auto& session = *clients[i];
            CharacterMovementState state{{session.mX, session.mY, session.mZ},
                                         session.mVelocityY, session.mIsJumping};
            //const CharacterMovementInput input{session.mYaw, session.mMoveUp, session.mMoveDown,
            //                                   session.mMoveRight, session.mMoveLeft};
            const CharacterMovementInput input{session.mMoveYaw, session.mMoveUp, session.mMoveDown, session.mMoveRight,
                                               session.mMoveLeft};
            // 지형 높이 조회
            const float sampleX = state.positionM.x + mTerrain->GetWorldWidthM() * 0.5f;
            const float sampleZ = state.positionM.z + mTerrain->GetWorldLengthM() * 0.5f;
            const float terrainHeight = mTerrain->SampleHeightM(sampleX, sampleZ);

            const auto heightAt = [this](float x, float z) {
                return mTerrain->SampleHeightM(x + mTerrain->GetWorldWidthM() * 0.5F,
                                               z + mTerrain->GetWorldLengthM() * 0.5F);
            };
            const float groundHeight = Kimgane::Shared::LunarMap::ENABLED
                ? Kimgane::Shared::LunarMap::StepHorizontal(state, input, i, mCollisionWorld,
                    mLunarColliders, heightAt, MOVE_SPEED, DELTA_TIME)
                : StepCharacterHorizontalMovement(state, input, i, mCollisionWorld, terrainHeight,
                                                   groundBoxes, MOVE_SPEED, DELTA_TIME);

            // Keep the falling state produced when walking off a lunar platform.
            StepCharacterVerticalMovement(state, groundHeight, GRAVITY, DELTA_TIME);
            session.mVelocityY = state.velocityYMps;
            session.mIsJumping = state.isJumping;

            // 계산 위치 세션에 반영
            session.mX = state.positionM.x;
            session.mY = state.positionM.y;
            session.mZ = state.positionM.z;

            // 브로드캐스트
            for (int p = 0; p < MAX_PLAYERS; ++p)
            {
                if (clients[p] && clients[p]->IsConnected())
                    clients[p]->SendMoveObject(i);
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
        if (Kimgane::Shared::LunarMap::ENABLED)
        {
            namespace LunarMap = Kimgane::Shared::LunarMap;
            const auto path = Kimgane::Shared::IO::ResolveAssetPath(LunarMap::HEIGHTMAP_PATH);
            mTerrain = TerrainHeightMap::LoadRaw16(path, LunarMap::SAMPLE_WIDTH, LunarMap::SAMPLE_LENGTH,
                                                  LunarMap::CELL_SPACING_M, LunarMap::HEIGHT_SCALE_M);
        }
        else mTerrain = TerrainHeightMap::LoadRawAuto(TerrainConfig::TERRAIN_RAW_PATH,
                                                     TerrainConfig::CELL_SPACING, TerrainConfig::HEIGHT_SCALE);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }

    if (Kimgane::Shared::LunarMap::ENABLED)
    {
        try
        {
            mLunarColliders = Kimgane::Shared::LunarMap::LoadCollision(Kimgane::Shared::LunarMap::COLLISION_PATH);
            Kimgane::Shared::LunarMap::RegisterCollision(mCollisionWorld, mLunarColliders);
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << '\n';
            return false;
        }
    }
    else
    {
    mHouseCollisionBoxes = Kimgane::Shared::Geometry::CollisionBoxLoader::Load("Shared/Geometry/TestHouse_collision.txt");

    int colliderId = 10000;

    for (const auto& collisionBox : mHouseCollisionBoxes)
    {
        // 충돌박스를 월드 좌표로 변환
        auto worldBox = collisionBox.box;
        worldBox.centerM.y += TEST_HOUSE_WORLD_OFFSET_Y;

        mCollisionWorld.AddOrUpdateBody({colliderId++, worldBox, Kimgane::Shared::Physics::CollisionLayer::STATIC_WORLD,
                                         Kimgane::Shared::Physics::CollisionLayer::ALL, false});
        // 충돌 시 디버깅용 로그 (어느 물체와 충돌했는지 확인)
        /*std::cout << "Register ID=" << colliderId << " Center(" << collisionBox.box.centerM.x << ", "
                  << collisionBox.box.centerM.y << ", " << collisionBox.box.centerM.z << ") "
                  << "Extent(" << collisionBox.box.halfExtentsM.x << ", " << collisionBox.box.halfExtentsM.y << ", "
                  << collisionBox.box.halfExtentsM.z << ")\n";*/
        ++colliderId;
    }

    //std::cout << "Loaded house boxes = " << mHouseCollisionBoxes.size() << '\n';

    for (size_t i = 0; i < std::min<size_t>(3, mHouseCollisionBoxes.size()); ++i)
    {
        const auto& box = mHouseCollisionBoxes[i].box;

        /*std::cout << "[House Box " << i << "] "
                  << "Center(" << box.centerM.x << ", " << box.centerM.y << ", " << box.centerM.z << ") "
                  << "Extent(" << box.halfExtentsM.x << ", " << box.halfExtentsM.y << ", " << box.halfExtentsM.z
                  << ")\n";*/
    }

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
