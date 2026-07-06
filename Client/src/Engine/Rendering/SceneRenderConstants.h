#pragma once

#include <DirectXMath.h>

#include <cstdint>

namespace Kimgane::Engine
{
struct SceneShaderConstants
{
    DirectX::XMFLOAT4X4 viewProjection = {};
    DirectX::XMFLOAT4 lightDirectionIntensity = {0.35F, -1.0F, 0.25F, 1.0F};
    DirectX::XMFLOAT4 lightColorAmbient = {1.0F, 0.96F, 0.86F, 0.18F};
    DirectX::XMFLOAT4 cameraPositionSpecularPower = {0.0F, 0.0F, 0.0F, 32.0F};
};

struct ObjectShaderConstants
{
    DirectX::XMFLOAT4X4 world = {};
    DirectX::XMFLOAT4 baseColor = {1.0F, 1.0F, 1.0F, 1.0F};
    DirectX::XMFLOAT4 surface = {0.0F, 0.5F, 0.0F, 0.0F};
    DirectX::XMFLOAT4 emission = {0.0F, 0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine

namespace Kimgane::Engine::RenderRootParameter
{
inline constexpr unsigned int SCENE = 0;
inline constexpr unsigned int OBJECT = 1;

inline constexpr unsigned int MATRIX_32BIT_COUNT = 16;
inline constexpr unsigned int SCENE_CONSTANTS_32BIT_COUNT =
    static_cast<unsigned int>(sizeof(SceneShaderConstants) / sizeof(std::uint32_t));
inline constexpr unsigned int OBJECT_WORLD_32BIT_COUNT = MATRIX_32BIT_COUNT;
inline constexpr unsigned int OBJECT_WORLD_32BIT_OFFSET = 0;
inline constexpr unsigned int OBJECT_COLOR_32BIT_OFFSET = 16;
inline constexpr unsigned int OBJECT_CONSTANTS_32BIT_COUNT =
    static_cast<unsigned int>(sizeof(ObjectShaderConstants) / sizeof(std::uint32_t));
} // namespace Kimgane::Engine::RenderRootParameter
