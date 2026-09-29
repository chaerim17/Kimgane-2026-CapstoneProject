#include <algorithm>
#include <thread>
#include <cmath>
#include <limits>
#include "../../../../Shared/Physics/RaycastQueries.h"
#include "../Npc/Npc.h"
#include "../../../../Shared/Physics/CharacterMovementWorld.h"
#include "../Terrain/ServerTerrainCalculation.h"
#include "Server.h"
#include "../Items/LootTable.h"
#include "../Network/PacketHandler.h"
#include "../Npc/NpcSetting.h"

namespace
{
namespace Physics = Kimgane::Shared::Physics;
namespace Raycast = Physics::RaycastQueries;

}

void Server::HandleShoot(Player& attacker, const Vec3& direction)
{
    // 패킷의 playerId 대신 실제 발사 세션을 사용하고, 방향은 서버에서 정규화합니다.
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) || !std::isfinite(direction.z))
        return;
    const double length = std::hypot(static_cast<double>(direction.x),
                                    static_cast<double>(direction.y), static_cast<double>(direction.z));
    if (length <= 0.000001)
        return;

    const auto playerCapsule = Physics::MakeCapsuleFromFootPosition(
        attacker.GetPositionM(), Physics::Settings::PLAYER_CAPSULE_RADIUS_M,
        Physics::Settings::PLAYER_CAPSULE_HEIGHT_M);
    Raycast::Ray ray{playerCapsule.centerM,
                     {static_cast<float>(direction.x / length), static_cast<float>(direction.y / length),
                      static_cast<float>(direction.z / length)},
                     std::numeric_limits<float>::infinity()};
    if (!std::isfinite(ray.originM.x) || !std::isfinite(ray.originM.y) || !std::isfinite(ray.originM.z))
        return;

    // 무제한 Ray로 살아 있는 NPC 중 가장 가까운 한 개를 찾습니다.
    Npc* target = nullptr;
    for (const auto& [id, object] : mWorld.GetObjects())
    {
        if (object->GetType() != GameObject::ObjectType::Npc)
            continue;
        auto* npc = static_cast<Npc*>(object.get());
        if (npc->mCurrentHp <= 0)
            continue;
        const auto capsule = Physics::MakeCapsuleFromFootPosition(
            npc->GetPositionM(), Physics::Settings::NPC_CAPSULE_RADIUS_M,
            Physics::Settings::NPC_CAPSULE_HEIGHT_M);
        Raycast::RaycastHit hit{};
        if (Raycast::RaycastCapsule(ray, capsule, hit) && std::isfinite(hit.distanceM)
            && hit.distanceM < ray.maxDistanceM)
        {
            target = npc;
            ray.maxDistanceM = hit.distanceM;
        }
    }
    if (!target)
        return;

    // 선택한 NPC까지의 구간만 장애물 검사합니다. 지형에 무한 길이 Ray를 순회시키지 않습니다.
    for (const auto& collisionBox : mWorld.GetMapCollision().GetHouseBoxes())
    {
        const auto& box = collisionBox.box;
        Raycast::RaycastHit hit{};
        if (Raycast::RaycastBox(ray, box, hit))
            return;
    }
    if (ServerTerrainCalculation::BlocksShot(mWorld.GetMapCollision().GetTerrainSampler(), ray))
        return;

    // HP를 먼저 확정하고, 모든 클라이언트에 같은 결과를 보냅니다.
    const int damage = std::min(SHOOTING_DAMAGE, target->mCurrentHp);
    target->mCurrentHp -= damage;
    for (const auto& recipient : mClients)
    {
        if (recipient && recipient->IsConnected())
            recipient->SendDamage(attacker.GetId(), target->GetId(), damage, target->mMaxHp, target->mCurrentHp);
    }

    // 마지막 데미지 결과를 먼저 보내고 오브젝트 제거
    if (target->mCurrentHp == 0)
        RemoveObject(target->GetId());
}

void Server::HandleOpenItemBox(Session& session, int objectId)
{
    // 중복 요청은 NotFound
    auto* player = mWorld.FindPlayer(session.GetId());
    if (!session.IsConnected() || !player)
        return;
    //std::cout << "[OPEN_BOX REQUEST] playerId=" << session.GetId() << " objectId=" << objectId << '\n';
    const auto* object = mWorld.FindObject(objectId);
    if (!object)
    {
        session.SendOpenItemBoxResult(objectId, OpenItemBoxStatus::NotFound);
        return;
    }
    if (object->GetType() != GameObject::ObjectType::ItemBox)
    {
        session.SendOpenItemBoxResult(objectId, OpenItemBoxStatus::NotItemBox);
        return;
    }

    const auto& from = player->GetPositionM();
    const auto& to = object->GetPositionM();
    const double distance = std::hypot(static_cast<double>(from.x) - to.x,
                                      static_cast<double>(from.y) - to.y,
                                      static_cast<double>(from.z) - to.z);
    //std::cout << "[OPEN_BOX DISTANCE] playerId=" << session.GetId() << " objectId=" << objectId
              //<< " distance=" << distance << " limit=" << ITEM_BOX_OPEN_DISTANCE_M << '\n';
    if (!std::isfinite(distance) || distance > ITEM_BOX_OPEN_DISTANCE_M)
    {
        session.SendOpenItemBoxResult(objectId, OpenItemBoxStatus::TooFar);
        return;
    }

    Kimgane::Shared::Items::ItemReward reward;
    try
    {
        reward = ItemBoxLoot::DrawReward(mLootRandom);
    }
    catch (const std::invalid_argument&)
    {
        session.SendOpenItemBoxResult(objectId, OpenItemBoxStatus::RewardFailed);
        return;
    }
    auto& inventory = player->GetInventory();
    if (!inventory.TryAdd(reward))
    {
        session.SendOpenItemBoxResult(objectId, OpenItemBoxStatus::RewardFailed);
        return;
    }

    const auto totalQuantity = inventory.GetQuantity(reward.itemId);
    // 아이템 상자 수령 후 제거
    RemoveObject(objectId);
    //std::cout << "[ITEM_BOX REMOVED] objectId=" << objectId
              //<< " objectRemaining=" << (mWorld.FindObject(objectId) != nullptr)
              //<< " colliderRemaining=" << mWorld.GetMapCollision().GetWorld().ContainsBody(objectId) << '\n';
    session.SendOpenItemBoxResult(objectId, OpenItemBoxStatus::Success, reward, totalQuantity);
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
    constexpr float DELTA_TIME = 0.05f; // 50ms

    while (true)
    {
        Sleep(50);
        std::lock_guard worldLock(mWorldMutex);
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (!mClients[i] || !mClients[i]->IsConnected())
                continue;

            auto* player = mWorld.FindPlayer(mClients[i]->GetId());
            if (!player)
                continue;
            ServerTerrainCalculation::UpdateCharacter(*player, mWorld.GetMapCollision().GetWorld(), DELTA_TIME);

            // 브로드캐스트
            for (int p = 0; p < MAX_PLAYERS; ++p)
            {
                if (mClients[p] && mClients[p]->IsConnected())
                    mClients[p]->SendMoveObject(*player);
            }
        }
        for (const auto* npc : NpcSetting::Update(mWorld))
        {
            for (const auto& recipient : mClients)
            {
                if (recipient && recipient->IsConnected())
                    recipient->SendMoveObject(*npc);
            }
        }
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
        mWorld.LoadMap();
        NpcSetting::Initialize(mWorld);
        mWorld.SpawnItemBoxes(ItemBoxSpawnSettings::COUNT);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return false;
    }

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

            HandleDisconnect(static_cast<int>(clientId));

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
    // 기존 순차 ID 정책을 유지하되 배열 접근 전에 수용 한계를 확인합니다.
    if (playerID >= MAX_PLAYERS)
    {
        closesocket(mClientSocket);
    }
    else
    {
        std::cout << "Client connected." << std::endl;
        auto session = std::make_unique<Session>();
        session->Connect(mClientSocket, playerID);
        auto& player = mWorld.CreatePlayer(playerID);
        // 첫 물리 틱 전에도 로그인 위치와 접지 상태가 지형/집 충돌에 맞도록 초기화합니다.
        namespace Physics = Kimgane::Shared::Physics;
        Physics::ResolveCharacterContactsInWorld(player.mMovementState, player.GetId(), mWorld.GetMapCollision().GetWorld());
        mClients[playerID] = std::move(session);

        CreateIoCompletionPort((HANDLE)mClientSocket, mIocp, playerID, 0);
        std::cout << "Register Recv\n";
        mClients[playerID]->DoRecv();
        std::cout << "Recv Registered\n";
        ++playerID;
    }

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

    Session* session = mClients[playerId].get();
    unsigned char* packetPtr = reinterpret_cast<unsigned char*>(expOver->mBuffer);
    int dataSize = numBytes + session->mPrevRecv;

    // 패킷 재조립
    while (dataSize > 0)
    {
        const int packetSize = packetPtr[0];
        if (packetSize < 1 + sizeof(PACKET_TYPE) || packetSize > BUF_SIZE)
        {
            HandleDisconnect(playerId);
            return;
        }
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
    RemoveObject(objectId);
}

void Server::RemoveObject(int objectId)
{
    auto* object = mWorld.FindObject(objectId);
    if (!object)
        return;
    const auto type = object->GetType();
    if (type == GameObject::ObjectType::Player && mClients[objectId])
    {
        mClients[objectId]->Disconnect();
        mClients[objectId].reset();
    }
    mWorld.RemoveObject(objectId);

    for (const auto& recipient : mClients)
    {
        if (recipient && recipient->IsConnected())
            recipient->SendRemoveObject(objectId);
    }
}
