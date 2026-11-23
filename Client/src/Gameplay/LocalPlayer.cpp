#include "Pch.h"

#include "LocalPlayer.h"

#include "PlayerControllerSettings.h"
#include "../Camera/Camera.h"
#include "../Core/SharedVec3Conversion.h"
#include "../Core/VectorMath.h"
#include "../Input/InputManager.h"
#include "../Network/NetworkManager.h"
#include "../Scene/TestSceneSettings.h"
#include "../../Shared/Physics/CharacterMovementWorld.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Kimgane::Client
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;

constexpr float YAW_SEND_THRESHOLD_RAD = 0.001F;
constexpr float ZERO_AXIS_EPSILON = 0.000001F;

DirectX::XMFLOAT3 ProjectPlanar(const DirectX::XMFLOAT3& value, const DirectX::XMFLOAT3& fallback) noexcept
{
    return VectorMath::NormalizeOrFallback({value.x, 0.0F, value.z}, fallback);
}

float DistanceSquaredM(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    const DirectX::XMFLOAT3 deltaM = VectorMath::Subtract(lhs, rhs);
    return VectorMath::Dot(deltaM, deltaM);
}
} // namespace

LocalPlayer::LocalPlayer(std::string name,
                         PlayerAppearance appearance,
                         const InputManager& inputManager,
                         NetworkManager& networkManager)
    : Player(std::move(name), std::move(appearance)),
      mInputManager(inputManager),
      mNetworkManager(networkManager),
      mMoveSpeedMps(PlayerControllerSettings::DEFAULT_MOVE_SPEED_MPS),
      mJumpVelocityMps(PlayerControllerSettings::DEFAULT_JUMP_VELOCITY_MPS),
      mJumpEnabled(PlayerControllerSettings::DEFAULT_JUMP_ENABLED)
{
}

void LocalPlayer::Update(float deltaTimeSec)
{
    if (!IsActive())
    {
        return;
    }

    // 여기서는 입력만 준비함. 실제 이동·점프·충돌은 고정 스텝마다 StepMovement가 처리.
    const DirectX::XMFLOAT3 movementDirection = BuildMovementDirection();
    mMovementInput = {{movementDirection.x, movementDirection.y, movementDirection.z},
                      mMoveSpeedMps, mJumpVelocityMps,
                      mJumpEnabled && (mMovementInput.jumpRequested || mInputManager.WasKeyPressed(InputKey::Jump))};
    if (mNetworkInputEnabled && mInputManager.WasKeyPressed(InputKey::Jump))
    {
        mNetworkManager.SendJump();
    }
    if (mInputManager.IsKeyDown(InputKey::Aim))
    {
        FaceCameraDirection();
    }
    else
    {
        FaceMovementDirection(movementDirection);
    }
    SendMovementInputPackets(std::atan2(movementDirection.x, movementDirection.z));

    GameObject::Update(deltaTimeSec);
}

void LocalPlayer::SetCamera(const Camera* camera) noexcept
{
    mCamera = camera;
}

void LocalPlayer::SetNetworkInputEnabled(bool enabled) noexcept
{
    mNetworkInputEnabled = enabled;
}

void LocalPlayer::SetMoveSpeedMps(float moveSpeedMps) noexcept
{
    mMoveSpeedMps = std::max(0.0F, moveSpeedMps);
}

float LocalPlayer::GetMoveSpeedMps() const noexcept
{
    return mMoveSpeedMps;
}

void LocalPlayer::ResetMovementState(const SharedPhysics::RigidbodyState& state) noexcept
{
    mMovementState = state;
    SyncTransformToMovementState();
    mPreviousPositionM = GetTransform().GetPositionM();
    mRenderCorrectionM = {};
}

void LocalPlayer::StepMovement(float deltaTimeSec,
                               SharedPhysics::ObjectId colliderId,
                               const SharedPhysics::CollisionWorld& world)
{
    mPreviousPositionM = GetTransform().GetPositionM();
    // 점프 요청은 한 번 소비하고 방향은 유지함.
    const SharedPhysics::CharacterMotionInput input = mMovementInput;
    mMovementInput.jumpRequested = false;
    SharedPhysics::StepCharacterMovementInWorld(mMovementState, input, deltaTimeSec, colliderId, world);
    SyncTransformToMovementState();
}

void LocalPlayer::CorrectToServer(const DirectX::XMFLOAT3& authoritativePositionM,
                                  float authoritativeYawRad,
                                  float interpolationAlpha) noexcept
{
    const float errorSqM = DistanceSquaredM(GetTransform().GetPositionM(), authoritativePositionM);
    if (errorSqM > 0.0F)
    {
        // 반복 보정 중에도 현재 표시 위치를 이어감. 실제 물리 위치는 즉시 서버에 맞춤.
        const DirectX::XMFLOAT3 displayedPositionM = GetRenderPositionM(interpolationAlpha);
        const DirectX::XMFLOAT3 renderCorrectionM = VectorMath::Subtract(displayedPositionM, authoritativePositionM);
        constexpr float SNAP_DISTANCE_SQ_M = TestSceneSettings::PLAYER_CORRECTION_SNAP_DISTANCE_M *
                                             TestSceneSettings::PLAYER_CORRECTION_SNAP_DISTANCE_M;
        mRenderCorrectionM =
            errorSqM < SNAP_DISTANCE_SQ_M && VectorMath::Dot(renderCorrectionM, renderCorrectionM) < SNAP_DISTANCE_SQ_M
                ? renderCorrectionM
                : DirectX::XMFLOAT3{};
        mPreviousPositionM = authoritativePositionM;
        mMovementState.positionM = ToSharedVec3(authoritativePositionM);
        SyncTransformToMovementState();
    }

    SetYawRad(authoritativeYawRad);
}

void LocalPlayer::DecayRenderCorrection(double elapsedTimeSec) noexcept
{
    if (!std::isfinite(elapsedTimeSec) || elapsedTimeSec <= 0.0)
    {
        return;
    }
    const float decay =
        static_cast<float>(std::exp2(-elapsedTimeSec / TestSceneSettings::PLAYER_CORRECTION_HALF_LIFE_SEC));
    mRenderCorrectionM = VectorMath::Scale(mRenderCorrectionM, decay);
    constexpr float MIN_OFFSET_SQ_M =
        TestSceneSettings::PLAYER_CORRECTION_MIN_OFFSET_M * TestSceneSettings::PLAYER_CORRECTION_MIN_OFFSET_M;
    if (VectorMath::Dot(mRenderCorrectionM, mRenderCorrectionM) <= MIN_OFFSET_SQ_M)
    {
        mRenderCorrectionM = {};
    }
}

const SharedPhysics::RigidbodyState& LocalPlayer::GetMovementState() const noexcept
{
    return mMovementState;
}

DirectX::XMFLOAT3 LocalPlayer::GetRenderPositionM(float interpolationAlpha) const noexcept
{
    const DirectX::XMFLOAT3 currentPositionM = GetTransform().GetPositionM();
    const DirectX::XMFLOAT3 interpolatedPositionM = VectorMath::Add(
        mPreviousPositionM,
        VectorMath::Scale(VectorMath::Subtract(currentPositionM, mPreviousPositionM), interpolationAlpha));
    return VectorMath::Add(interpolatedPositionM, mRenderCorrectionM);
}

DirectX::XMFLOAT3 LocalPlayer::BuildMovementDirection() const noexcept
{
    const InputState inputState = mInputManager.GetState();
    if (std::fabs(inputState.mMoveAxis.x) <= ZERO_AXIS_EPSILON && std::fabs(inputState.mMoveAxis.y) <= ZERO_AXIS_EPSILON)
    {
        return {0.0F, 0.0F, 0.0F};
    }

    const DirectX::XMFLOAT3 forward =
        ProjectPlanar(mCamera != nullptr ? mCamera->GetForward() : PlayerControllerSettings::DEFAULT_FALLBACK_FORWARD,
                      PlayerControllerSettings::DEFAULT_FALLBACK_FORWARD);
    const DirectX::XMFLOAT3 right =
        ProjectPlanar(mCamera != nullptr ? mCamera->GetRight() : PlayerControllerSettings::DEFAULT_FALLBACK_RIGHT,
                      PlayerControllerSettings::DEFAULT_FALLBACK_RIGHT);
    const DirectX::XMFLOAT3 forwardMove = VectorMath::Scale(forward, inputState.mMoveAxis.y);
    const DirectX::XMFLOAT3 rightMove = VectorMath::Scale(right, inputState.mMoveAxis.x);
    return VectorMath::NormalizeOrFallback(VectorMath::Add(forwardMove, rightMove), {0.0F, 0.0F, 0.0F});
}

void LocalPlayer::FaceMovementDirection(const DirectX::XMFLOAT3& direction) noexcept
{
    if (std::fabs(direction.x) <= ZERO_AXIS_EPSILON && std::fabs(direction.z) <= ZERO_AXIS_EPSILON)
    {
        return;
    }

    const float yawRad = std::atan2(direction.x, direction.z);
    SetYawRad(yawRad);
    SendRotateIfChanged(yawRad);
}

void LocalPlayer::FaceCameraDirection() noexcept
{
    if (mCamera == nullptr)
    {
        return;
    }

    const DirectX::XMFLOAT3 cameraForward =
        ProjectPlanar(mCamera->GetForward(), PlayerControllerSettings::DEFAULT_FALLBACK_FORWARD);
    const float yawRad = std::atan2(cameraForward.x, cameraForward.z);
    SetYawRad(yawRad);
    SendRotateIfChanged(yawRad);
}

void LocalPlayer::SendRotateIfChanged(float yawRad) noexcept
{
    if (std::fabs(yawRad - mLastSentYawRad) <= YAW_SEND_THRESHOLD_RAD)
    {
        return;
    }

    if (mNetworkInputEnabled)
    {
        mNetworkManager.SendRotate(yawRad);
    }
    mLastSentYawRad = yawRad;
}

void LocalPlayer::SendMovementInputPackets(float yawRad)
{
    if (!mNetworkInputEnabled)
    {
        return;
    }

    struct MoveKeyDirection
    {
        InputKey key;
        int direction;
    };
    // 이동 키를 누르면 시작, 떼면 정지 패킷을 보냄.
    const MoveKeyDirection moveKeys[] = {
        {InputKey::MoveForward, UP}, {InputKey::MoveBackward, DOWN}, {InputKey::MoveRight, RIGHT}, {InputKey::MoveLeft, LEFT}};
    for (const MoveKeyDirection& moveKey : moveKeys)
    {
        if (mInputManager.WasKeyPressed(moveKey.key))
        {
            mNetworkManager.SendMoveStart(moveKey.direction, yawRad);
        }
        if (mInputManager.WasKeyReleased(moveKey.key))
        {
            mNetworkManager.SendMoveStop(moveKey.direction, yawRad);
        }
    }

    // 키를 누른 채 카메라를 돌려 이동 방향이 바뀌면, 눌린 키 하나(앞>뒤>오른쪽>왼쪽 순)로 시작 패킷을 다시 보냄.
    for (const MoveKeyDirection& moveKey : moveKeys)
    {
        if (!mInputManager.IsKeyDown(moveKey.key))
        {
            continue;
        }
        if (std::fabs(yawRad - mLastSentMoveYawRad) > YAW_SEND_THRESHOLD_RAD)
        {
            mNetworkManager.SendMoveStart(moveKey.direction, yawRad);
            mLastSentMoveYawRad = yawRad;
        }
        break;
    }
}

void LocalPlayer::SyncTransformToMovementState() noexcept
{
    GetTransform().SetPositionM(ToXMFloat3(mMovementState.positionM));
}
} // namespace Kimgane::Client
