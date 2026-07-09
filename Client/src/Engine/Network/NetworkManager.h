#pragma once

#include <vector>
#include <DirectXMath.h>
#include <WinSock2.h>

namespace Kimgane::Engine
{
    struct PlayerLocation
    {
        int mId;

        float mX;
        float mY;
        float mZ;
    };

    class NetworkManager
    {
    public:
        bool Initialize();

        void Shutdown();

        void Update(float deltaTime);

        void SendMoveInput(int direction);

        bool get_player_location(int* id, float* x, float* y, float* z);

    private:
        void ProcessPacket(unsigned char* packet);

        void ProcessData(char* buffer, size_t receivedBytes);

    private:
        SOCKET mSocket;

        std::vector<PlayerLocation> mPlayerLocations;

        int mReadCursor;
    };
} // namespace Kimgane::Engine
