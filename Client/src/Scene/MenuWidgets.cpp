#include "Pch.h"

#include "MenuWidgets.h"

#include "Scene.h"
#include "../Camera/Camera.h"

#include <algorithm>
#include <utility>

namespace Kimgane::Client
{
namespace
{
constexpr float UI_CAMERA_DISTANCE_M = 10.0F;
constexpr float UI_NEAR_CLIP_M = 0.1F;
constexpr float UI_FAR_CLIP_M = 50.0F;
} // namespace

float GetUiOrthographicWidthM(float cameraAspectRatio) noexcept
{
    return UI_ORTHOGRAPHIC_HEIGHT_M * std::max(cameraAspectRatio, 1.0F);
}

void ConfigureUiCamera(OrthographicCamera& camera, float cameraAspectRatio) noexcept
{
    camera.SetView({0.0F, 0.0F, -UI_CAMERA_DISTANCE_M}, {0.0F, 0.0F, 0.0F});
    camera.SetOrthographic(GetUiOrthographicWidthM(cameraAspectRatio),
                           UI_ORTHOGRAPHIC_HEIGHT_M,
                           UI_NEAR_CLIP_M,
                           UI_FAR_CLIP_M);
}

GameObject& CreateUiPanel(Scene& scene,
                          const std::shared_ptr<Mesh>& uiMesh,
                          std::string name,
                          const DirectX::XMFLOAT3& positionM,
                          const DirectX::XMFLOAT3& scale,
                          const DirectX::XMFLOAT4& colorLinear)
{
    GameObject& panel = scene.CreateObject(std::move(name));
    panel.GetTransform().SetPositionM(positionM);
    panel.GetTransform().SetScale(scale);
    panel.SetMesh(uiMesh);
    Material& material = panel.GetMaterial();
    material.SetBaseColorLinear(colorLinear);
    material.SetSurface(0.0F, 0.8F);
    material.SetEmissionLinear({colorLinear.x, colorLinear.y, colorLinear.z}, 0.15F);
    return panel;
}

TextLabel& AddUiButtonLabel(Scene& scene, const GameObject& button, std::wstring text, float fontSizeDip)
{
    TextLabel& label = scene.CreateTextLabel(button, std::move(text), fontSizeDip);
    label.SetColorLinear({0.92F, 0.96F, 1.0F, 1.0F});
    label.SetInsetRatio(0.08F, 0.12F);
    label.SetAlignment(TextHorizontalAlignment::Center, TextVerticalAlignment::Center);
    return label;
}
} // namespace Kimgane::Client
