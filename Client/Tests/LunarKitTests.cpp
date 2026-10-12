#include "../../Shared/Geometry/ObjLoader.h"
#include "../../Shared/World/EnvironmentLayout.h"
#include "../../Shared/World/TestMapCollision.h"
#include "../../Shared/Physics/CharacterMovementWorld.h"
#include "../../Shared/Physics/RaycastQueries.h"
#include "../src/ThirdParty/DirectXTex/DDSTextureLoader12.h"
#include <wrl/client.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace P = Kimgane::Shared::Physics;
namespace W = Kimgane::Shared::World;
namespace G = Kimgane::Shared::Geometry;

void Require(bool ok, const std::string& message)
{
    if (!ok) throw std::runtime_error(message);
}

void Walk(const W::TestMapCollision& map, P::Vec3 start, P::Vec3 end)
{
    auto state = P::MakePlayerMovementState(start);
    P::ResolveCharacterContactsInWorld(state, -3, map.GetWorld());
    const auto delta = P::Subtract(end, start);
    const float lengthM = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    const P::CharacterMotionInput input{{delta.x / lengthM, 0, delta.z / lengthM}, 3, 0, false};
    for (int frame = 0; frame < static_cast<int>(std::ceil(lengthM / 3 * 60)); ++frame)
        P::StepCharacterMovementInWorld(state, input, 1.0F / 60.0F, -3, map.GetWorld());
    std::cout << "walk " << start.x << ',' << start.z << " -> " << state.positionM.x << ',' << state.positionM.z << '\n';
    Require(std::abs(state.positionM.x - end.x) < 0.15F && std::abs(state.positionM.z - end.z) < 0.15F,
            "Door/breach route blocked by a collider or an entry step");
    Require(state.isGrounded && std::isfinite(state.positionM.y), "Route must remain grounded");
}

int main()
{
    try
    {
        const auto placements = W::LoadEnvironmentLayout(L"Assets/LunarKit/layout.txt");
        Require(placements.size() >= 7, "Missing prefab placements");
        std::size_t files = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("Assets/LunarKit/obj"))
        {
            if (entry.path().extension() != ".obj") continue;
            const auto mesh = G::ObjLoader::Load(entry.path());
            Require(!mesh.indices.empty() && mesh.indices.size() % 3 == 0, "Empty/nontriangular OBJ");
            Require(mesh.positionsM.size() == mesh.uvs.size(), "UVs missing");
            for (std::size_t i = 0; i < mesh.positionsM.size(); ++i)
            {
                const auto& p = mesh.positionsM[i]; const auto& n = mesh.normals[i]; const auto& uv = mesh.uvs[i];
                Require(std::isfinite(p.x + p.y + p.z + n.x + n.y + n.z), "Nonfinite mesh data");
                Require(uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1, "Atlas UV outside image");
                Require(std::abs(n.x*n.x+n.y*n.y+n.z*n.z-1) < 0.001F, "Invalid normals");
            }
            for (std::size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                const auto& a = mesh.positionsM[mesh.indices[i]];
                const auto& b = mesh.positionsM[mesh.indices[i+1]];
                const auto& c = mesh.positionsM[mesh.indices[i+2]];
                const P::Vec3 u{b.x-a.x,b.y-a.y,b.z-a.z}, v{c.x-a.x,c.y-a.y,c.z-a.z};
                const P::Vec3 cross{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
                Require(P::LengthSquared(cross) > 1e-14F, "Degenerate triangle");
                const auto& n = mesh.normals[mesh.indices[i]];
                Require(cross.x*n.x+cross.y*n.y+cross.z*n.z > 0, "Winding differs from exported normal");
            }
            ++files;
        }
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Require(SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))), "D3D12 device");
        for (const auto* name : {L"BaseColor", L"NormalDX", L"ORM"})
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> texture;
            std::unique_ptr<uint8_t[]> bytes;
            std::vector<D3D12_SUBRESOURCE_DATA> subresources;
            const auto path = std::filesystem::path(L"Assets/LunarKit/textures") / (std::wstring(L"L02_")+name+L".dds");
            Require(SUCCEEDED(DirectX::LoadDDSTextureFromFile(device.Get(), path.c_str(), &texture, bytes, subresources)), "DDS loader");
            Require(subresources.size() == (std::wstring(name) == L"BaseColor" ? 12U : 11U), "Incomplete DDS mip chain");
        }
        Require(W::SelectEnvironmentLod(23,0)==0 && W::SelectEnvironmentLod(23,1)==1, "LOD hysteresis");
        Require(W::SelectEnvironmentLod(200,0)==3 && W::SelectEnvironmentLod(0,3)==0, "LOD jumps/teleports");
        W::TestMapCollision client, server;
        const auto terrain = W::LoadTestMapTerrain(); client.Load(terrain); server.Load(terrain);
        Require(client.GetWorld().GetBodyCount()==server.GetWorld().GetBodyCount(), "Client/server map mismatch");
        Require(client.GetHouseBoxes().size()>300, "Missing modular colliders");
        for (const auto& route : {std::pair{P::Vec3{6,4.71F,-6},P::Vec3{6,4.71F,6}},
                                 std::pair{P::Vec3{2,4.71F,-6},P::Vec3{2,4.71F,-2}},
                                 std::pair{P::Vec3{14,4.71F,-2},P::Vec3{9,4.71F,-2}},
                                 std::pair{P::Vec3{-4,4.71F,-6},P::Vec3{-4,4.71F,6}}})
        {
            Walk(client, route.first, route.second); Walk(server, route.second, route.first);
        }
        // A solid side panel must continue to block players and bullets at every visual LOD.
        auto state = P::MakePlayerMovementState({13,4.71F,2});
        const P::CharacterMotionInput intoWall{{-1,0,0},3,0,false};
        for(int i=0;i<90;++i) P::StepCharacterMovementInWorld(state,intoWall,1.0F/60,-3,client.GetWorld());
        Require(state.positionM.x > 12.5F, "Solid wall did not block movement");
        P::RaycastQueries::RaycastHit hit;
        bool wallHit=false;
        for(const auto& box:client.GetHouseBoxes())
            wallHit |= P::RaycastQueries::RaycastBox({{13,6.5F,2},{-1,0,0},1},box.box,hit);
        Require(wallHit, "Solid wall did not block raycast");
        std::cout << "Lunar kit tests passed: " << files << " OBJ files, 3 DDS maps, LOD, shared collision and traversal\n";
        return 0;
    }
    catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
