#pragma once
#include <array>
namespace Kimgane::Shared::LunarMap {
struct MeshBatch { const wchar_t* path; float metallic; float roughness; float emissionR; float emissionG; float emissionB; float intensity; };
inline constexpr std::array<MeshBatch, 13> MESH_BATCHES = {{
    {L"Assets/Models/LunarOutpost/amber.txt", 0.200000F, 0.300000F, 1.000000F, 0.540000F, 0.120000F, 6.000000F},
    {L"Assets/Models/LunarOutpost/compacted_dust.txt", 0.000000F, 0.940000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/edge_dark.txt", 0.650000F, 0.430000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/glass.txt", 0.750000F, 0.220000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/hull_ceramic.txt", 0.500000F, 0.360000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/ice_light.txt", 0.200000F, 0.250000F, 0.240000F, 0.790000F, 1.000000F, 5.000000F},
    {L"Assets/Models/LunarOutpost/lettering.txt", 0.100000F, 0.500000F, 0.800000F, 0.910000F, 1.000000F, 0.400000F},
    {L"Assets/Models/LunarOutpost/lunar_rock.txt", 0.000000F, 0.940000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/safety_orange.txt", 0.300000F, 0.500000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/signal_red.txt", 0.000000F, 0.500000F, 1.000000F, 0.025000F, 0.012000F, 4.000000F},
    {L"Assets/Models/LunarOutpost/solar_cells.txt", 0.650000F, 0.250000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/titanium.txt", 0.750000F, 0.350000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
    {L"Assets/Models/LunarOutpost/warm_alloy.txt", 0.700000F, 0.310000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F},
}};
} // namespace Kimgane::Shared::LunarMap
