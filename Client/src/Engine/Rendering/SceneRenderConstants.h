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
};

struct ObjectShaderConstants
{
    DirectX::XMFLOAT4X4 world = {};
    DirectX::XMFLOAT4 baseColor = {1.0F, 1.0F, 1.0F, 1.0F};
};
} // namespace Kimgane::Engine

namespace Kimgane::Engine::RenderRootParameter
{
inline constexpr unsigned int kScene = 0;
inline constexpr unsigned int kObject = 1;

inline constexpr unsigned int kMatrix32BitCount = 16;
inline constexpr unsigned int kSceneConstants32BitCount =
    static_cast<unsigned int>(sizeof(SceneShaderConstants) / sizeof(std::uint32_t));
inline constexpr unsigned int kObjectWorld32BitCount = kMatrix32BitCount;
inline constexpr unsigned int kObjectWorld32BitOffset = 0;
inline constexpr unsigned int kObjectColor32BitOffset = 16;
inline constexpr unsigned int kObjectConstants32BitCount =
    static_cast<unsigned int>(sizeof(ObjectShaderConstants) / sizeof(std::uint32_t));
} // namespace Kimgane::Engine::RenderRootParameter
