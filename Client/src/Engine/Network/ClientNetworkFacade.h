#pragma once

#include <DirectXMath.h>

#include <cstddef>
#include <vector>

namespace Kimgane::Engine
{
// 서버와 클라이언트가 공유해야 할 플레이어 위치 데이터
// 서버 프로젝트가 생기면 Shared 영역으로 이동할 후보
struct PlayerNetworkLocation
{
    int mId = 0;
    DirectX::XMFLOAT3 mPositionM = {0.0F, 0.0F, 0.0F};
};

// 클라이언트 코드에서 네트워크 구현 세부 사항을 직접 보지 않도록 감싸는 Facade
// 현재는 서버 연동 전 동기화 테스트용 placeholder이며, 내부 구현만 실제 소켓 코드로 교체
class ClientNetworkFacade final
{
public:
    // 서버 연결 초기화 지점
    void InitializeNetwork();

    // 서버 연결 해제 지점
    void ShutdownNetwork() noexcept;

    // 매 프레임 네트워크 송수신을 갱신하는 지점
    void UpdateNetwork(float deltaTimeSec);

    // 로컬 플레이어 위치를 서버로 보내는 지점
    void SendLocalPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM);

    // 교수님 회의에서 정한 수신 함수 형태를 유지한 placeholder
    // 서버에서 받은 플레이어 위치가 있으면 true를 반환하고, 없으면 false를 반환
    [[nodiscard]] bool get_player_location(int* id, float* x, float* y, float* z) noexcept;
    [[nodiscard]] bool IsInitialized() const noexcept;
    [[nodiscard]] const PlayerNetworkLocation& GetLastSentLocation() const noexcept;
    [[nodiscard]] bool HasLastSentLocation() const noexcept;

private:
    // 서버가 붙기 전까지 다른 플레이어가 움직이는 것처럼 보이게 만드는 임시 데이터
    // 실제 서버 수신 코드가 들어오면 이 함수와 mock 상태값은 제거
    void BuildMockReceivedLocations(float deltaTimeSec);

    bool mIsInitialized = false;
    bool mHasLastSentLocation = false;
    float mMockElapsedTimeSec = 0.0F;
    std::size_t mReadCursor = 0U;
    PlayerNetworkLocation mLastSentLocation = {};
    std::vector<PlayerNetworkLocation> mReceivedLocations;
};
} // namespace Kimgane::Engine
