#pragma once

#include "Session.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../../../../Shared/Physics/CollisionWorld.h"
#include "../../../../Shared/Geometry/ObjLoader.h"
#include "../../../../Shared/Physics/CollisionTypes.h"
#include "../../../../Shared/Physics/CollisionQueries.h"
#include "../../../../Shared/Geometry/CollisionBoxLoader.h"

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
    Kimgane::Shared::Physics::CollisionWorld mCollisionWorld;

};
