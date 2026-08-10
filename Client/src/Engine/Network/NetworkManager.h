#pragma once

#include <vector>
#include <DirectXMath.h>
#include <WinSock2.h>
#include <queue>

#include "../../Shared/Protocol.h"

constexpr int BUF_SIZE = 200;

namespace Kimgane::Engine
{
    struct ObjectState
    {
        bool mIsActive = false;

        float mX = 0.0f;
        float mY = 0.0f;
        float mZ = 0.0f;
    };

    struct LocationUpdate 
    {
        int playerId;
        float x;
        float y;
        float z;
        float yaw;
    };

    class NetworkManager
    {
    public:
        bool Initialize();
        void Shutdown();
        void Update(float deltaTime);

        void SendMoveInput(int direction);
        void SendMoveStart(int direction, float yaw);
        void SendMoveStop(int direction);
        void SendRotate(float yaw);

        bool GetPlayerLocation(int* id, float* x, float* y, float* z, float* yaw);
        bool GetRemovedPlayer(int* playerId);

        int GetMyPlayerId() const noexcept {
            return mMyPlayerId;
        }

    private:
        int mCurrentPacketSize = 0;
        int mSavedPacketSize = 0;

        char mPacketBuffer[BUF_SIZE] = {};

        SOCKET mSocket = INVALID_SOCKET;

        bool mIsConnected = false;

        std::queue<LocationUpdate> mLocationUpdates;
        std::queue<int> mRemovedPlayers;

        ObjectState mObjects[MAX_OBJECTS];

        int mReadCursor = 0;

        int mMyPlayerId = -1;

    private:
        void ProcessPacket(unsigned char* packet);

        void ProcessData(char* buffer, size_t receivedBytes);
    };
} // namespace Kimgane::Engine
