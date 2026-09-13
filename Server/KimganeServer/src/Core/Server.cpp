#include <algorithm>
#include <thread>
#include <cmath>
#include <limits>
#include "../../../../Shared/Physics/RaycastQueries.h"
#include "../Npc/Npc.h"
#include "../../../../Shared/Physics/CharacterMovement.h"
#include "Server.h"
#include "../Network/PacketHandler.h"
#include"../../../../Shared/Terrain/TerrainConfig.h"
#include "../NPC/NpcSetting.h"

std::array<std::unique_ptr<Session>, MAX_PLAYERS> clients;

namespace
{
namespace Physics = Kimgane::Shared::Physics;
namespace Raycast = Physics::RaycastQueries;

// 서버 높이맵의 중앙 원점 좌표를 Shared 지형 조회 인터페이스에 연결합니다.
class ShootTerrainSampler final : public Physics::TerrainSampler
{
public:
    explicit ShootTerrainSampler(const TerrainHeightMap& terrain) : mTerrain(terrain) {}

    bool SampleHeightAtWorld(const Physics::Vec3& position, Physics::TerrainSample& sample) const noexcept override
    {
        const float x = position.x + mTerrain.GetWorldWidthM() * 0.5F;
        const float z = position.z + mTerrain.GetWorldLengthM() * 0.5F;
        if (!mTerrain.ContainsSamplePositionM(x, z))
            return false;
        sample.heightM = mTerrain.SampleHeightM(x, z);
        return true;
    }

private:
    const TerrainHeightMap& mTerrain;
};
}

void Server::HandleShoot(Session& attacker, const Vec3& direction)
{
    // 패킷의 playerId 대신 실제 발사 세션을 사용하고, 방향은 서버에서 정규화합니다.
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z))
        return;
    const double length = std::hypot(static_cast<double>(direction.x),
                                    static_cast<double>(direction.y), static_cast<double>(direction.z));
    if (length <= 0.000001)
        return;

    const auto playerCapsule = Physics::MakeCapsuleFromFootPosition(
        {attacker.mX, attacker.mY, attacker.mZ}, Physics::Settings::PLAYER_CAPSULE_RADIUS_M,
        Physics::Settings::PLAYER_CAPSULE_HEIGHT_M);
    Raycast::Ray ray{playerCapsule.centerM,
                     {static_cast<float>(direction.x / length), static_cast<float>(direction.y / length),
                      static_cast<float>(direction.z / length)},
                     std::numeric_limits<float>::infinity()};
    if (!std::isfinite(ray.originM.x) || !std::isfinite(ray.originM.y) || !std::isfinite(ray.originM.z))
        return;

    // 무제한 Ray로 살아 있는 NPC 중 가장 가까운 한 개를 찾습니다.
    Npc* target = nullptr;
    for (const auto& npc : NpcSetting::gNpcs)
    {
        if (npc->mCurrentHp <= 0)
            continue;
        const auto capsule = Physics::MakeCapsuleFromFootPosition(
            {npc->mX, npc->mY, npc->mZ}, Physics::Settings::NPC_CAPSULE_RADIUS_M,
            Physics::Settings::NPC_CAPSULE_HEIGHT_M);
        Raycast::RaycastHit hit{};
        if (Raycast::RaycastCapsule(ray, capsule, hit) && std::isfinite(hit.distanceM)
            && hit.distanceM < ray.maxDistanceM)
        {
            target = npc.get();
            ray.maxDistanceM = hit.distanceM;
        }
    }
    if (!target)
        return;

    // 선택한 NPC까지의 구간만 장애물 검사합니다. 지형에 무한 길이 Ray를 순회시키지 않습니다.
    for (const auto& collisionBox : mHouseCollisionBoxes)
    {
        auto box = collisionBox.box;
        box.centerM.y += TEST_HOUSE_WORLD_OFFSET_Y;
        Raycast::RaycastHit hit{};
        if (Raycast::RaycastBox(ray, box, hit))
            return;
    }
    const ShootTerrainSampler sampler(*mTerrain);
    Physics::TerrainSample originSample{};
    if (!sampler.SampleHeightAtWorld(ray.originM, originSample) || ray.originM.y <= originSample.heightM)
        return;
    Raycast::RaycastHit terrainHit{};
    if (Raycast::RaycastTerrain(ray, Physics::TerrainSurface{&sampler}, terrainHit))
        return;

    // HP를 먼저 확정하고, 모든 클라이언트에 같은 결과를 보냅니다.
    const int damage = std::min(SHOOTING_DAMAGE, target->mCurrentHp);
    target->mCurrentHp -= damage;
    for (const auto& recipient : clients)
    {
        if (recipient && recipient->IsConnected())
            recipient->SendDamage(attacker.GetId(), target->mId, damage, target->mMaxHp, target->mCurrentHp);
    }
}

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
        std::lock_guard worldLock(mWorldMutex);
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (!clients[i] || !clients[i]->IsConnected())
                continue;

            // 실제 세션을 참조해서 shared movement state에서 이동계산
            auto& session = *clients[i];
            CharacterMovementState state{{session.mX, session.mY, session.mZ},
                                         session.mVelocityY, session.mIsJumping};
            const CharacterMovementInput input{session.mMoveYaw, session.mMoveUp, session.mMoveDown,
                                               session.mMoveRight, session.mMoveLeft};
            // 지형 높이 조회
            const float sampleX = state.positionM.x + mTerrain->GetWorldWidthM() * 0.5f;
            const float sampleZ = state.positionM.z + mTerrain->GetWorldLengthM() * 0.5f;
            const float terrainHeight = mTerrain->SampleHeightM(sampleX, sampleZ);

            const float groundHeight = StepCharacterHorizontalMovement(
                state, input, i, mCollisionWorld, terrainHeight, groundBoxes, MOVE_SPEED, DELTA_TIME);

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

            state.velocityYMps = session.mVelocityY;
            state.isJumping = session.mIsJumping;
            StepCharacterVerticalMovement(state, groundHeight, GRAVITY, DELTA_TIME);
            session.mY = state.positionM.y;
            session.mVelocityY = state.velocityYMps;
            session.mIsJumping = state.isJumping;
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

        // 접속/해제와 패킷 처리 중에 보호
        std::lock_guard worldLock(mWorldMutex);

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

        PacketHandler::HandlePacket(*this, session, packetPtr);
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
