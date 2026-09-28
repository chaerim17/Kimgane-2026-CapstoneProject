#pragma once

#include <vector>
#include <DirectXMath.h>
#include <WinSock2.h>
#include <queue>
#include <unordered_map>

#include "../../Shared/Protocol.h"

constexpr int BUF_SIZE = 200;

namespace Kimgane::Engine
{
    struct ObjectState
    {
        bool mIsActive = false;
        Kimgane::Shared::World::ObjectType mType = Kimgane::Shared::World::ObjectType::Player;

        float mX = 0.0f;
        float mY = 0.0f;
        float mZ = 0.0f;

        float mYaw = 0.0f;

        int mMaxHp = 0;
        int mCurrentHp = 0;
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
        [[nodiscard]] bool IsConnected() const noexcept;
        [[nodiscard]] bool ConsumePlayerStateSyncTick(float deltaTimeSec) noexcept;

        void SendMoveInput(int direction);
        void SendMoveStart(int direction, float yaw);
        void SendMoveStop(int direction, float yaw);
        void SendRotate(float yaw);
        void SendJump();
        void SendOpenItemBox(int objectId);
        bool GetOpenItemBoxResult(S2C_OpenItemBoxResult& result);
        [[nodiscard]] std::uint64_t GetItemQuantity(Kimgane::Shared::Items::ItemId itemId) const noexcept
        {
            const auto it = mInventory.find(itemId);
            return it == mInventory.end() ? 0 : it->second;
        }
        void SendShoot(const DirectX::XMFLOAT3& direction);
        void SendPlayerState(const DirectX::XMFLOAT3& pos, float yaw, bool isJumping);

        bool GetPlayerLocation(int* id, float* x, float* y, float* z, float* yaw);
        bool GetRemovedPlayer(int* playerId);

        // 상자를 포함한 수신 상태 조회용 씬 생성 별도 필요
        [[nodiscard]] const ObjectState* FindObject(int objectId) const noexcept
        {
            const auto it = mObjects.find(objectId);
            return it == mObjects.end() ? nullptr : &it->second;
        }
        [[nodiscard]] const std::unordered_map<int, ObjectState>& GetObjects() const noexcept { return mObjects; }

        [[nodiscard]] int GetCurrentHp(int objectId) const noexcept
        {
            const auto* object = FindObject(objectId);
            return object ? object->mCurrentHp : 0;
        }

        [[nodiscard]] int GetMaxHp(int objectId) const noexcept
        {
            const auto* object = FindObject(objectId);
            return object ? object->mMaxHp : 0;
        }

        int GetMyPlayerId() const noexcept {
            return mMyPlayerId;
        }

        float GetPlayerStateSyncTimer() const noexcept {
            return mPlayerStateSyncTimer;
        }

    private:
        int mCurrentPacketSize = 0;
        int mSavedPacketSize = 0;

        char mPacketBuffer[BUF_SIZE] = {};

        SOCKET mSocket = INVALID_SOCKET;

        bool mIsConnected = false;

        std::queue<LocationUpdate> mLocationUpdates;
        std::queue<int> mRemovedPlayers;

        std::unordered_map<int, ObjectState> mObjects;
        // 서버가 알려 준 총 보유량을 저장
        std::unordered_map<Kimgane::Shared::Items::ItemId, std::uint64_t> mInventory;
        std::queue<S2C_OpenItemBoxResult> mOpenItemBoxResults;

        int mReadCursor = 0;

        int mMyPlayerId = -1;
        //디버그용: 응답 대기 중인 상자 ID. 응답 후 다음 상자를 자동으로 엽니다.
        int mDebugPendingItemBoxId = -1;

        float mPlayerStateSyncTimer = 0.0f;

    private:
        void ProcessPacket(unsigned char* packet);

        void ProcessData(char* buffer, size_t receivedBytes);
    };
} // namespace Kimgane::Engine
