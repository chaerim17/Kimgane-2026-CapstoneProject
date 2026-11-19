#pragma once

#include <DirectXMath.h>

#include <memory>
#include <string>

namespace Kimgane::Client
{
class GameObject;
class Mesh;
class OrthographicCamera;
class Scene;
class TextLabel;

// 타이틀·설정 화면이 같이 쓰는 메뉴 UI 헬퍼.
// UI 카메라를 화면 비율에 맞추고, 버튼 패널과 그 위의 글자를 만듦.

// UI 카메라가 보는 세로 높이(월드 단위). 가로 폭은 화면 비율에 따라 GetUiOrthographicWidthM()으로 구함.
inline constexpr float UI_ORTHOGRAPHIC_HEIGHT_M = 9.0F;

[[nodiscard]] float GetUiOrthographicWidthM(float cameraAspectRatio) noexcept;
void ConfigureUiCamera(OrthographicCamera& camera, float cameraAspectRatio) noexcept;

GameObject& CreateUiPanel(Scene& scene,
                          const std::shared_ptr<Mesh>& uiMesh,
                          std::string name,
                          const DirectX::XMFLOAT3& positionM,
                          const DirectX::XMFLOAT3& scale,
                          const DirectX::XMFLOAT4& colorLinear);

TextLabel& AddUiButtonLabel(Scene& scene, const GameObject& button, std::wstring text, float fontSizeDip);
} // namespace Kimgane::Client
