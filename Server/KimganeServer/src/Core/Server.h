#pragma once

#include "Session.h"
#include "../Terrain/TerrainHeightMap.h"

extern std::array<std::unique_ptr<Session>, MAX_PLAYERS> clients;

void error_display(const wchar_t* msg, int err_no);
void TimerThread();

class Server
{
public:
    Server();
    ~Server();

    bool Initialize();
    void Run();

private:
    SOCKET mListenSocket;
    HANDLE mIocp;

    SOCKET mClientSocket;
    ExpOver mAcceptOver;

    void HandleAccept(int& playerID);
    void HandleRecv(int playerId, DWORD numBytes, ExpOver* expOver);


    void HandleDisconnect(int playerId);

    void TimerThread();

    std::shared_ptr<TerrainHeightMap> mTerrain;
};
