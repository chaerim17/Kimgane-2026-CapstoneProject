#pragma once

#include "Player.h"
#include "../../Shared/Physics/CharacterMovement.h"
#include "../../Shared/Physics/CollisionWorld.h"
#include "../../Shared/Physics/RigidbodyTypes.h"

#include <DirectXMath.h>

namespace Kimgane::Client
{
class Camera;
class InputManager;
class NetworkManager;

// 내가 조작하는 플레이어.
// - 입력: 키보드 입력을 카메라 기준 이동 방향으로 바꾸고, 온라인이면 이동·회전·점프 패킷을 보냄.
// - 예측: 서버와 같은 Shared 이동 함수로 먼저 움직이고(mMovementState), 서버 위치가 오면 그 위치로 보정함.
// - 표시: 고정 스텝 사이 보간과 보정 차이 감쇠를 더한 화면용 위치를 제공함. 물리 상태에는 영향 없음.
class LocalPlayer final : public Player
{
public:
    LocalPlayer(std::string name,
                PlayerAppearance appearance,
                const InputManager& inputManager,
                NetworkManager& networkManager);

    // 매 렌더 프레임: 이동 입력을 만들고 바라보는 방향·네트워크 입력 패킷을 처리함.
    void Update(float deltaTimeSec) override;

    void SetCamera(const Camera* camera) noexcept;
    void SetNetworkInputEnabled(bool enabled) noexcept;
    void SetMoveSpeedMps(float moveSpeedMps) noexcept;
    [[nodiscard]] float GetMoveSpeedMps() const noexcept;

    // 스폰: 이동 상태를 통째로 바꾸고 보간·보정 기록을 지움.
    void ResetMovementState(const Kimgane::Shared::Physics::RigidbodyState& state) noexcept;
    // 고정 물리 스텝 1회: 쌓인 이동 입력을 소비해서 Shared 이동 함수로 위치를 갱신함.
    void StepMovement(float deltaTimeSec,
                      Kimgane::Shared::Physics::ObjectId colliderId,
                      const Kimgane::Shared::Physics::CollisionWorld& world);
    // 서버 권위 위치로 즉시 보정. 화면은 보정 직전 표시 위치에서 부드럽게 따라가게 함.
    void CorrectToServer(const DirectX::XMFLOAT3& authoritativePositionM,
                         float authoritativeYawRad,
                         float interpolationAlpha) noexcept;
    // 서버 보정으로 생긴 화면 위치 차이를 시간에 따라 줄임.
    void DecayRenderCorrection(double elapsedTimeSec) noexcept;

    [[nodiscard]] const Kimgane::Shared::Physics::RigidbodyState& GetMovementState() const noexcept;
    // 직전 스텝 위치와 현재 위치를 alpha로 보간한 뒤 보정 차이를 더한 화면용 위치.
    [[nodiscard]] DirectX::XMFLOAT3 GetRenderPositionM(float interpolationAlpha) const noexcept;

private:
    [[nodiscard]] DirectX::XMFLOAT3 BuildMovementDirection() const noexcept;
    void FaceMovementDirection(const DirectX::XMFLOAT3& direction) noexcept;
    void FaceCameraDirection() noexcept;
    void SendRotateIfChanged(float yawRad) noexcept;
    void SendMovementInputPackets(float yawRad);
    void SyncTransformToMovementState() noexcept;

    const InputManager& mInputManager;
    NetworkManager& mNetworkManager;
    const Camera* mCamera = nullptr;

    Kimgane::Shared::Physics::CharacterMotionInput mMovementInput = {};
    float mMoveSpeedMps = 0.0F;
    float mJumpVelocityMps = 0.0F;
    bool mJumpEnabled = false;
    bool mNetworkInputEnabled = true;
    float mLastSentYawRad = 0.0F;     // 서버에 마지막으로 보낸 바라보는 방향
    float mLastSentMoveYawRad = 0.0F; // 서버에 마지막으로 보낸 이동 방향

    Kimgane::Shared::Physics::RigidbodyState mMovementState = {};
    DirectX::XMFLOAT3 mPreviousPositionM = {};
    // 서버 보정 전 표시 위치와의 차이. 물리/충돌/송신 상태에는 적용하지 않음.
    DirectX::XMFLOAT3 mRenderCorrectionM = {};
};
} // namespace Kimgane::Client
