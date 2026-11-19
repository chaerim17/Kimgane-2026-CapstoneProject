#include "../../Shared/Geometry/ObjLoader.h"
#include "../../Shared/World/EnvironmentLayout.h"
#include "../../Shared/World/TestMapCollision.h"
#include "../../Shared/Physics/CharacterMovementWorld.h"
#include "../../Shared/Physics/RaycastQueries.h"
#include "../src/ThirdParty/DirectXTex/DDSTextureLoader12.h"
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <Windows.h>

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
    std::cout << "이동 " << start.x << ',' << start.z << " -> " << state.positionM.x << ',' << state.positionM.z << '\n';
    Require(std::abs(state.positionM.x - end.x) < 0.15F && std::abs(state.positionM.z - end.z) < 0.15F,
            "문·통로 경로가 충돌 박스나 입구 턱에 막힘");
    // 지형을 따라 내려가면 마지막 이동 프레임에 바닥과 살짝 떠 있을 수 있어서 착지할 때까지 몇 프레임 더 돌림.
    const P::CharacterMotionInput stop{{0,0,0},0,0,false};
    for (int frame = 0; frame < 30 && !state.isGrounded; ++frame)
        P::StepCharacterMovementInWorld(state, stop, 1.0F / 60.0F, -3, map.GetWorld());
    Require(state.isGrounded && std::isfinite(state.positionM.y), "경로 끝에서 지형이나 바닥에 착지해야 함");
}

void FollowRoad(const W::TestMapCollision& map, const std::vector<P::Vec3>& points)
{
    Require(points.size() > 1, "도로 경로가 비어 있음");
    auto state = P::MakePlayerMovementState(points.front());
    P::ResolveCharacterContactsInWorld(state, -3, map.GetWorld());
    for (std::size_t i = 1; i < points.size(); ++i)
    {
        const auto target = points[i];
        const auto delta = P::Subtract(target, state.positionM);
        const int budget = static_cast<int>(std::ceil(std::hypot(delta.x,delta.z)/3*60))+90;
        bool reached = false;
        for (int frame = 0; frame < budget; ++frame)
        {
            const auto remaining = P::Subtract(target,state.positionM);
            const auto distance = std::hypot(remaining.x,remaining.z);
            if (distance < .09F) { reached = true; break; }
            const P::CharacterMotionInput input{{remaining.x/distance,0,remaining.z/distance},3,0,false};
            P::StepCharacterMovementInWorld(state,input,1.0F/60,-3,map.GetWorld());
        }
        Require(reached, "도로가 다음 위치 근처에서 막힘: "+std::to_string(target.x)+","+std::to_string(target.z));
    }
}

int main()
{
    // /utf-8 빌드라 문자열이 UTF-8로 들어감. 콘솔도 UTF-8로 맞춰야 한국어 메시지가 깨지지 않음.
    SetConsoleOutputCP(CP_UTF8);
    try
    {
        const auto placements = W::LoadEnvironmentLayout(L"Assets/LunarKit/layout.txt");
        Require(placements.size() >= 7, "프리팹 배치 수가 부족함");
        std::size_t files = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator("Assets/LunarKit/obj"))
        {
            if (entry.path().extension() != ".obj") continue;
            const auto mesh = G::ObjLoader::Load(entry.path());
            Require(!mesh.indices.empty() && mesh.indices.size() % 3 == 0, "OBJ가 비어 있거나 삼각형으로 이뤄지지 않음");
            Require(mesh.positionsM.size() == mesh.uvs.size(), "UV가 없음");
            for (std::size_t i = 0; i < mesh.positionsM.size(); ++i)
            {
                const auto& p = mesh.positionsM[i]; const auto& n = mesh.normals[i]; const auto& uv = mesh.uvs[i];
                Require(std::isfinite(p.x + p.y + p.z + n.x + n.y + n.z), "메쉬 데이터에 유한하지 않은 값이 있음");
                Require(uv.x >= 0 && uv.x <= 1 && uv.y >= 0 && uv.y <= 1, "아틀라스 UV가 이미지 범위를 벗어남");
                Require(std::abs(n.x*n.x+n.y*n.y+n.z*n.z-1) < 0.001F, "노멀 길이가 1이 아님");
            }
            for (std::size_t i = 0; i < mesh.indices.size(); i += 3)
            {
                const auto& a = mesh.positionsM[mesh.indices[i]];
                const auto& b = mesh.positionsM[mesh.indices[i+1]];
                const auto& c = mesh.positionsM[mesh.indices[i+2]];
                const P::Vec3 u{b.x-a.x,b.y-a.y,b.z-a.z}, v{c.x-a.x,c.y-a.y,c.z-a.z};
                const P::Vec3 cross{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
                Require(P::LengthSquared(cross) > 1e-14F, "넓이가 0인 삼각형이 있음");
                const auto& n = mesh.normals[mesh.indices[i]];
                Require(cross.x*n.x+cross.y*n.y+cross.z*n.z > 0, "삼각형 감는 방향이 내보낸 노멀과 다름");
            }
            ++files;
        }
        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Require(SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))), "D3D12 디바이스 생성 실패");
        for (const auto* name : {L"BaseColor", L"NormalDX", L"ORM"})
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> texture;
            std::unique_ptr<uint8_t[]> bytes;
            std::vector<D3D12_SUBRESOURCE_DATA> subresources;
            const auto path = std::filesystem::path(L"Assets/LunarKit/textures") / (std::wstring(L"L02_")+name+L".dds");
            Require(SUCCEEDED(DirectX::LoadDDSTextureFromFile(device.Get(), path.c_str(), &texture, bytes, subresources)), "DDS 텍스처 로드 실패");
            Require(subresources.size() == (std::wstring(name) == L"BaseColor" ? 12U : 11U), "DDS 밉맵 단계가 부족함");
        }
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> mask;
            std::unique_ptr<uint8_t[]> bytes;
            std::vector<D3D12_SUBRESOURCE_DATA> subresources;
            Require(SUCCEEDED(DirectX::LoadDDSTextureFromFile(device.Get(),
                L"Assets/Terrain/lunar_surface_mask.dds", &mask, bytes, subresources)), "월드 표면 마스크 DDS 로드 실패");
            Require(mask->GetDesc().Format == DXGI_FORMAT_R8G8B8A8_UNORM && subresources.size() == 11,
                    "월드 마스크는 선형 RGBA8이고 밉맵 단계가 모두 있어야 함");
        }
        Require(W::SelectEnvironmentLod(23,0)==0 && W::SelectEnvironmentLod(23,1)==1, "LOD 경계에서 단계가 자꾸 바뀌지 않게 하는 여유 구간(히스테리시스)이 동작하지 않음");
        Require(W::SelectEnvironmentLod(200,0)==3 && W::SelectEnvironmentLod(0,3)==0, "멀리 이동하거나 순간이동했을 때 LOD 선택이 잘못됨");
        W::TestMapCollision client, server;
        const auto terrain = W::LoadTestMapTerrain(); client.Load(terrain); server.Load(terrain);
        Require(client.GetWorld().GetBodyCount()==server.GetWorld().GetBodyCount(), "클라와 서버의 맵 충돌 body 수가 다름");
        Require(client.GetHouseBoxes().size()>300, "모듈 건물 충돌 박스 수가 부족함");
        for (const auto& route : {std::pair{P::Vec3{6,4.71F,-6},P::Vec3{6,4.71F,6}},
                                 std::pair{P::Vec3{2,4.71F,-6},P::Vec3{2,4.71F,-2}},
                                 std::pair{P::Vec3{14,4.71F,-2},P::Vec3{9,4.71F,-2}},
                                 std::pair{P::Vec3{-4,4.71F,-6},P::Vec3{-4,4.71F,6}}})
        {
            Walk(client, route.first, route.second); Walk(server, route.second, route.first);
        }
        // 배치된 모든 건물 입구를 실제 지형과 Shared 충돌 월드로 통과해 봄.
        std::ifstream routes("Assets/LunarKit/traversal-routes.txt");
        Require(routes.is_open(), "정착지 통과 경로 파일이 없음");
        std::size_t routeCount = 0;
        for (std::string line; std::getline(routes, line);)
        {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream values(line);
            std::string identity;
            P::Vec3 start{}, end{};
            Require(static_cast<bool>(values >> identity >> start.x >> start.y >> start.z >> end.x >> end.y >> end.z),
                    "정착지 통과 경로 형식이 잘못됨");
            std::cout << identity << '\n';
            Walk(client, start, end);
            Walk(server, end, start);
            ++routeCount;
        }
        Require(routeCount >= 10, "외곽 정착지 문 경로 수가 부족함");
        std::ifstream roads("Assets/LunarKit/transport-routes.txt");
        Require(roads.is_open(), "도로 경로 파일이 없음");
        std::vector<P::Vec3> roadPoints;
        std::size_t roadCount = 0;
        const auto checkRoad = [&]()
        {
            if (roadPoints.empty()) return;
            FollowRoad(client,roadPoints);
            std::reverse(roadPoints.begin(),roadPoints.end());
            FollowRoad(server,roadPoints);
            roadPoints.clear(); ++roadCount;
        };
        for (std::string line; std::getline(roads,line);)
        {
            if (line.empty() || line[0]=='#') continue;
            std::istringstream values(line); std::string keyword; values >> keyword;
            if (keyword=="road") { checkRoad(); continue; }
            P::Vec3 point;
            Require(keyword=="point" && static_cast<bool>(values >> point.x >> point.y >> point.z), "도로 경유점 형식이 잘못됨");
            roadPoints.push_back(point);
        }
        checkRoad(); Require(roadCount==5,"기지 네 곳을 잇는 도로 수가 맞지 않음");
        // 막힌 벽면은 어떤 LOD로 보이든 플레이어와 총알을 계속 막아야 함.
        auto state = P::MakePlayerMovementState({13,4.71F,2});
        const P::CharacterMotionInput intoWall{{-1,0,0},3,0,false};
        for(int i=0;i<90;++i) P::StepCharacterMovementInWorld(state,intoWall,1.0F/60,-3,client.GetWorld());
        Require(state.positionM.x > 12.5F, "막힌 벽이 이동을 막지 못함");
        P::RaycastQueries::RaycastHit hit;
        bool wallHit=false;
        for(const auto& box:client.GetHouseBoxes())
            wallHit |= P::RaycastQueries::RaycastBox({{13,6.5F,2},{-1,0,0},1},box.box,hit);
        Require(wallHit, "막힌 벽이 레이캐스트를 막지 못함");
        std::cout << "LunarKit 테스트 통과: OBJ " << files << "개, 아틀라스 DDS 3장 + 월드 마스크, LOD, 공용 충돌, 문, 양방향 도로 5개\n";
        return 0;
    }
    catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
