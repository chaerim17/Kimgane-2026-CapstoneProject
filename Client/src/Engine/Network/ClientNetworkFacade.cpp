#include "Pch.h"

#include "ClientNetworkFacade.h"

#include "../../Shared/protocol.h"

#include <cmath>

namespace Kimgane::Engine
{
void ClientNetworkFacade::InitializeNetwork()
{
    // 임시: 실제 서버 주소/포트가 정해지면 이 위치에서 소켓을 생성하고 연결?
    mIsInitialized = true;
    mHasLastSentLocation = false;
    mMockElapsedTimeSec = 0.0F;
    mReadCursor = 0U;
    mLastSentLocation = {};
    mReceivedLocations.clear();
}

void ClientNetworkFacade::ShutdownNetwork() noexcept
{
    // 임시: 실제 네트워크 연결이 생기면 이 위치에서 송수신 버퍼와 소켓을 정리?
    mIsInitialized = false;
    mHasLastSentLocation = false;
    mReadCursor = 0U;
    mReceivedLocations.clear();
}

void ClientNetworkFacade::UpdateNetwork(float deltaTimeSec)
{
    if (!mIsInitialized)
    {
        return;
    }

    // 임시: 서버 수신 처리가 들어오면 임시 대신 수신 패킷을 mReceivedLocations에 채웁니다.
    BuildMockReceivedLocations(deltaTimeSec);
    mReadCursor = 0U;
}

void ClientNetworkFacade::SendLocalPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM)
{
    if (!mIsInitialized)
    {
        return;
    }

    // 임시: 실제 통신에서는 이 위치에서 playerId와 positionM을 패킷으로 직렬화해 서버로 전송?
    mLastSentLocation = {playerId, positionM};
    mHasLastSentLocation = true;
}

bool ClientNetworkFacade::get_player_location(int* id, float* x, float* y, float* z) noexcept
{
    // 서버 수신 큐를 하나씩 꺼내는 형태로 맞춘 함수
    // 현재는 임시 벡터를 순회
    if (id == nullptr || x == nullptr || y == nullptr || z == nullptr || mReadCursor >= mReceivedLocations.size())
    {
        return false;
    }

    const PlayerNetworkLocation& location = mReceivedLocations[mReadCursor];
    ++mReadCursor;

    *id = location.mId;
    *x = location.mPositionM.x;
    *y = location.mPositionM.y;
    *z = location.mPositionM.z;
    return true;
}

bool ClientNetworkFacade::IsInitialized() const noexcept
{
    return mIsInitialized;
}

const PlayerNetworkLocation& ClientNetworkFacade::GetLastSentLocation() const noexcept
{
    return mLastSentLocation;
}

bool ClientNetworkFacade::HasLastSentLocation() const noexcept
{
    return mHasLastSentLocation;
}

void ClientNetworkFacade::BuildMockReceivedLocations(float deltaTimeSec)
{
    // 서버가 없을 때도 클라이언트 렌더링 동기화 흐름을 확인하기 위한 임시 원형 이동 데이터입니다.
    mMockElapsedTimeSec += deltaTimeSec;
    const float angleRad = mMockElapsedTimeSec * NetworkSettings::MOCK_REMOTE_PLAYER_SPEED_RAD_PER_SEC;
    const DirectX::XMFLOAT3 mockPositionM = {
        std::cos(angleRad) * NetworkSettings::MOCK_REMOTE_PLAYER_RADIUS_M,
        NetworkSettings::MOCK_REMOTE_PLAYER_HEIGHT_M,
        std::sin(angleRad) * NetworkSettings::MOCK_REMOTE_PLAYER_RADIUS_M};

    mReceivedLocations.clear();
    mReceivedLocations.push_back({NetworkSettings::MOCK_REMOTE_PLAYER_ID, mockPositionM});
}
} // namespace Kimgane::Engine
