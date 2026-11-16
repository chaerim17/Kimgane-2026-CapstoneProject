#pragma once

#include <mutex>
#include <random>
#include <array>

#include "../Config/ServerConfig.h"
#include "Session.h"
#include "../World/GameWorld.h"

void error_display(const wchar_t* msg, int err_no);

class Server
{
public:
    Server();
    ~Server();

    bool Initialize();
    void Run();
    // Run()이 월드 잠금을 보유한 상태에서 호출
    void HandleShoot(Player& attacker, const Vec3& direction);
    void HandleOpenItemBox(Session& session, int objectId);
    void StartPurification(Player& player) noexcept;
    // 호출자는 mWorldMutex를 보유해야 합니다. PacketHandler에서 사용합니다.
    GameWorld& GetWorld() noexcept { return mWorld; }
    using MatchSessions = std::array<std::unique_ptr<Session>, PLAYERS_PER_MATCH>;
    const MatchSessions& GetSessions() const noexcept { return mClients[CURRENT_MATCH_SLOT]; }

private:
    std::mutex mWorldMutex; // 타이머와 패킷 처리의 게임 상태 접근 lock
    SOCKET mListenSocket;
    HANDLE mIocp;

    SOCKET mClientSocket;
    ExpOver mAcceptOver;

    void HandleAccept(bool accepted);
    Session* FindSessionByConnection(ULONG_PTR connectionId) noexcept;
    void HandleRecv(Session& session, DWORD numBytes, ExpOver* expOver);


    void HandleDisconnect(int playerId);
    void RemoveObject(int objectId);

    void TimerThread();

    static constexpr int CURRENT_MATCH_SLOT = 0; // The existing server owns one GameWorld.
    static constexpr ULONG_PTR CONNECTION_SLOT_COUNT = MATCH_CAPACITY * PLAYERS_PER_MATCH;
    static_assert(MATCH_CAPACITY > CURRENT_MATCH_SLOT);
    static_assert(MAX_PLAYERS <= PLAYERS_PER_MATCH);
    std::array<MatchSessions, MATCH_CAPACITY> mClients;
    ULONG_PTR mNextConnectionGeneration = 1;
    GameWorld mWorld;
    std::mt19937 mLootRandom{std::random_device{}()};
};
