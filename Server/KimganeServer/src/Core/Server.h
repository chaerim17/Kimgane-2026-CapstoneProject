#pragma once

#include <mutex>

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
    // 호출자는 mWorldMutex를 보유해야 합니다. PacketHandler에서 사용합니다.
    GameWorld& GetWorld() noexcept { return mWorld; }
    const std::array<std::unique_ptr<Session>, MAX_PLAYERS>& GetSessions() const noexcept { return mClients; }

private:
    std::mutex mWorldMutex; // 타이머와 패킷 처리의 게임 상태 접근 lock
    SOCKET mListenSocket;
    HANDLE mIocp;

    SOCKET mClientSocket;
    ExpOver mAcceptOver;

    void HandleAccept(int& playerID);
    void HandleRecv(int playerId, DWORD numBytes, ExpOver* expOver);


    void HandleDisconnect(int playerId);
    void RemoveObject(int objectId);

    void TimerThread();

    std::array<std::unique_ptr<Session>, MAX_PLAYERS> mClients;
    GameWorld mWorld;
};
