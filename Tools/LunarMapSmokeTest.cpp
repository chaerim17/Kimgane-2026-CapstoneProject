#include "../Shared/Maps/LunarOutpost/LunarMapSettings.h"
#include "../Shared/Maps/LunarOutpost/LunarMovement.h"
#include "../Shared/Terrain/TerrainHeightMap.h"
#include "../Server/KimganeServer/src/Terrain/TerrainHeightMap.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace L = Kimgane::Shared::LunarMap;
namespace P = Kimgane::Shared::Physics;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main()
{
    try
    {
        auto client = Kimgane::Engine::TerrainHeightMap::LoadRaw16(L::HEIGHTMAP_PATH, L::SAMPLE_WIDTH,
            L::SAMPLE_LENGTH, L::CELL_SPACING_M, L::HEIGHT_SCALE_M);
        auto server = TerrainHeightMap::LoadRaw16(L::HEIGHTMAP_PATH, L::SAMPLE_WIDTH,
            L::SAMPLE_LENGTH, L::CELL_SPACING_M, L::HEIGHT_SCALE_M);
        Require(client->GetHeightsM() == server->GetHeightsM(), "client/server RAW16 mismatch");
        for (float z = 0.25F; z < 280; z += 0.75F)
            for (float x = 0.25F; x < 280; x += 0.75F)
                Require(std::abs(client->SampleHeightM(x,z)-server->SampleHeightM(x,z)) < 0.00001F,
                        "client/server interpolation mismatch");
        const auto height = [&](float x, float z) { return server->SampleHeightM(x+140,z+140); };
        auto colliders = L::LoadCollision(L::COLLISION_PATH);
        P::CollisionWorld world;
        L::RegisterCollision(world, colliders);
        Require(colliders.size() == 259 && world.GetBodyCount() == 259, "collider count mismatch");
        const auto body = [](P::Vec3 foot) { return P::CollisionBody{1,
            P::MakeCapsuleFromFootPosition(foot,P::Settings::PLAYER_CAPSULE_RADIUS_M,
                P::Settings::PLAYER_CAPSULE_HEIGHT_M),P::CollisionLayer::PLAYER,P::CollisionLayer::ALL,false}; };
        Require(!world.HasBlockingContact(body({L::SPAWN_X_M,L::SPAWN_Y_M,L::SPAWN_Z_M})), "spawn obstructed");
        Require(std::abs(height(0,88)-L::SPAWN_Y_M)<0.002F, "spawn height mismatch");
        Require(std::abs(L::GroundHeight({0,19.425F,42},19.425F,colliders,height)-19.425F)<0.002F,
                "bridge not supporting feet");
        const float under = height(0,42);
        Require(L::GroundHeight({0,under,42},under,colliders,height)<19.0F, "teleport onto bridge from below");
        unsigned ramps=0;
        for (const auto& c : colliders)
            if (const auto* ramp = std::get_if<P::Ramp>(&c.shape))
            {
                ++ramps;
                auto p = ramp->centerM;
                Require(!world.HasBlockingContact(body(p)), "ramp midpoint blocks walking");
                Require(L::GroundHeight(p,p.y,colliders,height)>=p.y-0.002F, "ramp has no support");
            }
        Require(ramps==6, "missing ramps");
        P::CharacterMovementState state{{0,L::SPAWN_Y_M,88},0,false};
        P::CharacterMovementInput input{3.14159265F,true,false,false,false};
        for (int i=0;i<720;++i)
        {
            const float ground=L::StepHorizontal(state,input,1,world,colliders,height,6,1.0F/60);
            P::StepCharacterVerticalMovement(state,ground,9.8F,1.0F/60);
        }
        std::cout << "route endpoint: " << state.positionM.x << ',' << state.positionM.y << ',' << state.positionM.z << '\n';
        Require(state.positionM.z<20, "spawn-to-bridge route blocked");
        P::CharacterMovementState falling{{4.2F,19.425F,42},0,false};
        input.yawRad=1.5707963F;
        const float floor=L::StepHorizontal(falling,input,1,world,colliders,height,6,1.0F/60);
        Require(falling.isJumping && falling.positionM.y>floor+0.3F, "walking off bridge must start falling");
        P::StepCharacterVerticalMovement(falling,floor,9.8F,1.0F/60);
        Require(falling.positionM.y>floor+0.3F, "fall snapped to crater floor");
        P::CharacterMovementState wall{{119, height(119,0),0},0,false};
        input.yawRad=1.5707963F;
        for(int i=0;i<120;++i) (void)L::StepHorizontal(wall,input,1,world,colliders,height,6,1.0F/60);
        Require(wall.positionM.x<120, "playable boundary leaked");
        // A typo must fail explicitly, rather than silently dropping a wall on one peer.
        auto fixture=std::filesystem::temp_directory_path()/"kimgane_lunar_invalid_collision.txt";
        {std::ofstream out(fixture); out<<"ramp bad 0 0 0 1 1 1 Diagonal\n";}
        bool rejected=false;
        try {(void)L::LoadCollision(fixture);} catch (const std::runtime_error&) {rejected=true;}
        std::filesystem::remove(fixture);
        Require(rejected,"malformed collision accepted");
        std::cout << "PASS: terrain parity, spawn, bridge above/below, six ramps, route, boundary, invalid data\n";
        return 0;
    }
    catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1;}
}
