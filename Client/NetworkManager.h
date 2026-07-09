#pragma once

#include <vector>
#include <DirectXMath.h>
#include <WinSock2.h>

namespace Kimgane::Engine
{
    struct PlayerState
    {
        bool mIsActive;
        
        float mX;
        float mY;
        float mZ;
    };

    class NetworkManager
    {
    public:
        bool Initialize(); // ClientApplication, Scene에서 호출
        void Shutdown();
        void Update(float deltaTime);
        void SendMoveInput(int direction);  // wasd 입력 서버로 전송

        bool GetPlayerLocation(int* id, float* x, float* y, float* z);

    private:
        int mCurrentPacketSize;
        int mSavedPacketSize;
        char mPacketBuffer[BUF_SIZE];

        SOCKET mSocket;
        PlayerState mPlayers[MAX_PLAYERS];  //수신한 플레이어 저장
        int mReadCursor;

        void ProcessPacket(unsigned char* packet);
        void ProcessData(char* buffer, size_t receivedBytes);
    };
} // namespace Kimgane::Engine
