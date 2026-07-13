#pragma once

#include <vector>
#include <DirectXMath.h>
#include <WinSock2.h>

#include "../../Shared/protocol.h"

constexpr int BUF_SIZE = 200;

namespace Kimgane::Engine
{
    struct PlayerState
    {
        bool mIsActive = false;

        float mX = 0.0f;
        float mY = 0.0f;
        float mZ = 0.0f;
    };

    class NetworkManager
    {
    public:
        bool Initialize();
        void Shutdown();
        void Update(float deltaTime);

        void SendMoveInput(int direction);

        bool GetPlayerLocation(int* id, float* x, float* y, float* z);

    private:
        int mCurrentPacketSize = 0;
        int mSavedPacketSize = 0;

        char mPacketBuffer[BUF_SIZE] = {};

        SOCKET mSocket = INVALID_SOCKET;

        bool mIsConnected = false;

        PlayerState mPlayers[MAX_PLAYERS];

        int mReadCursor = 0;

    private:
        void ProcessPacket(unsigned char* packet);

        void ProcessData(char* buffer, size_t receivedBytes);
    };
} // namespace Kimgane::Engine
