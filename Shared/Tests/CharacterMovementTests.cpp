#include "../Physics/CharacterMovement.h"
#include "../Physics/CharacterMovementWorld.h"
#include "../Physics/FixedStepClock.h"
#include "../World/TestMapCollision.h"

#include <array>
#include <utility>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace Physics = Kimgane::Shared::Physics;

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

bool Near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.00001F;
}

// The character position is its foot. A floor lies at y=0 and a wall at x=1.
class TestEnvironment final : public Physics::CharacterContactQuery
{
public:
    bool floorEnabled = false;
    bool wallEnabled = false;
    int queryCount = 0;

    void QueryContacts(const Physics::Vec3& positionM, std::vector<Physics::ContactInfo>& contacts) override
    {
        ++queryCount;
        contacts.clear();
        if (wallEnabled && positionM.x >= 1.0F)
        {
            Physics::ContactInfo contact = {};
            contact.normal = {-1.0F, 0.0F, 0.0F};
            contact.surfaceNormal = contact.normal;
            contact.penetrationM = positionM.x - 1.0F;
            contacts.push_back(contact);
        }
        if (floorEnabled && positionM.y <= 0.0F)
        {
            Physics::ContactInfo contact = {};
            contact.normal = {0.0F, 1.0F, 0.0F};
            contact.surfaceNormal = contact.normal;
            contact.penetrationM = -positionM.y;
            contact.isGroundCandidate = true;
            contact.isWalkable = true;
            contacts.push_back(contact);
        }
    }
};

Physics::RigidbodyState MakeState()
{
    Physics::RigidbodyState state = {};
    state.dragPerSec = 0.0F;
    state.groundFrictionPerSec = 0.0F;
    return state;
}

void TestMovementAndForceConsumption()
{
    TestEnvironment world;
    auto state = MakeState();
    state.useGravity = false;
    state.massKg = 2.0F;
    state.accumulatedForceN = {0.0F, 4.0F, 0.0F};
    const Physics::CharacterMotionInput input{{1.0F, 0.0F, 0.0F}, 5.0F, 8.0F, false};
    Physics::StepCharacterMovement(state, input, 0.1F, world);
    Require(Near(state.positionM.x, 0.5F), "Movement must be integrated exactly once");
    Require(Near(state.velocityMps.y, 0.2F), "Force must be applied once");
    Require(Near(state.accumulatedForceN.y, 0.0F), "Consumed force must be cleared");
    Physics::StepCharacterMovement(state, input, 0.1F, world);
    Require(Near(state.positionM.x, 1.0F), "Consecutive steps must use the latest state");
    Require(Near(state.velocityMps.y, 0.2F), "Force must not be applied again");
}

void TestJumpAndLanding()
{
    TestEnvironment world;
    world.floorEnabled = true;
    auto state = MakeState();
    state.isGrounded = true;
    Physics::CharacterMotionInput input{{}, 5.0F, 8.0F, true};
    Physics::StepCharacterMovement(state, input, 0.01F, world);
    Require(state.positionM.y > 0.0F && !state.isGrounded, "Grounded jump must leave the floor");
    const float firstVelocity = state.velocityMps.y;
    Physics::StepCharacterMovement(state, input, 0.01F, world);
    Require(state.velocityMps.y < firstVelocity, "Jump requests in the air must not add another impulse");
    input.jumpRequested = false;
    for (int step = 0; step < 400 && !state.isGrounded; ++step)
    {
        Physics::StepCharacterMovement(state, input, 0.01F, world);
    }
    Require(state.isGrounded && state.positionM.y >= -0.00101F, "Landing must correct penetration");
    Require(Near(state.velocityMps.y, 0.0F), "Landing must remove downward velocity");
    world.floorEnabled = false;
    Physics::StepCharacterMovement(state, input, 0.01F, world);
    Require(!state.isGrounded, "Losing the floor must clear grounding");
    Physics::StepCharacterMovement(state, input, 0.01F, world);
    Require(state.velocityMps.y < 0.0F, "Gravity must resume after losing the floor");
}

void TestWallSlideAndRequery()
{
    TestEnvironment world;
    world.wallEnabled = true;
    auto state = MakeState();
    state.useGravity = false;
    state.positionM.x = 0.9F;
    const Physics::CharacterMotionInput input{{0.6F, 0.0F, 0.8F}, 5.0F, 0.0F, false};
    Physics::StepCharacterMovement(state, input, 0.1F, world);
    Require(Near(state.positionM.x, 1.001F), "Wall correction must retain only allowed penetration");
    Require(Near(state.velocityMps.x, 0.0F), "Wall must remove inward velocity");
    Require(Near(state.velocityMps.z, 4.0F) && Near(state.positionM.z, 0.4F), "Wall must preserve tangential motion");
    Require(world.queryCount >= 2 && world.queryCount <= 4, "Corrected positions must be queried with bounded iterations");
}

void TestWorldContactOrientationAndFiltering()
{
    Physics::CollisionWorld world;
    const Physics::Box box{{0.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 1.0F}};
    const Physics::CollisionBody character{7,
        Physics::MakeCapsuleFromFootPosition({1.2F, 0.0F, 0.0F}, 0.45F, 1.8F),
        Physics::CollisionLayer::PLAYER, Physics::CollisionLayer::ALL, false};
    world.AddOrUpdateBody({1, box, Physics::CollisionLayer::STATIC_WORLD, Physics::CollisionLayer::ALL, false});
    world.AddOrUpdateBody({2, box, Physics::CollisionLayer::STATIC_WORLD, Physics::CollisionLayer::ALL, true});
    world.AddOrUpdateBody({3, box, Physics::CollisionLayer::STATIC_WORLD, 0U, false});
    world.AddOrUpdateBody(character);
    const auto contacts = world.QueryCharacterContacts(character);
    Require(contacts.size() == 1, "Character queries must exclude self, triggers and masked bodies");
    Require(contacts.front().objectA == 1 && contacts.front().objectB == 7,
            "Character contact must use environment A and character B");
    Require(contacts.front().normal.x > 0.9F, "Character B normal must point out of the wall");
}

void TestHeightMapWorldEntryPoint()
{
    auto terrain = Kimgane::Shared::Terrain::HeightMapData::CreateFlat(21, 21, 1.0F);
    Kimgane::Shared::World::TestMapCollision map;
    map.Build(terrain, {});
    auto& world = map.GetWorld();
    auto state = MakeState();
    state.positionM.y = -0.1F;
    Physics::ResolveCharacterContactsInWorld(state, 7, world);
    Require(state.isGrounded && state.positionM.y >= -0.00101F,
            "Spawn must resolve the floor before the first physics tick");
    Physics::StepCharacterMovementInWorld(state, {}, 0.05F, 7, world);
    Require(state.isGrounded && state.positionM.y >= -0.00101F,
            "Generic world entry must resolve terrain with the shared movement calculation");
    Require(Near(state.velocityMps.y, 0.0F), "World entry must remove falling velocity on landing");
    Physics::CharacterMotionInput input{{}, Physics::Settings::PLAYER_MOVE_SPEED_MPS,
        Physics::Settings::PLAYER_JUMP_VELOCITY_MPS, true};
    Physics::StepCharacterMovementInWorld(state, input, Physics::FIXED_STEP_DELTA_SEC, 7, world);
    Require(!state.isGrounded && state.positionM.y > 0.0F, "World entry must jump from grounded state");
    const float jumpVelocity = state.velocityMps.y;
    Physics::StepCharacterMovementInWorld(state, input, Physics::FIXED_STEP_DELTA_SEC, 7, world);
    Require(state.velocityMps.y < jumpVelocity, "World entry must reject an airborne jump");
    input.jumpRequested = false;
    for (int step = 0; step < 180 && !state.isGrounded; ++step)
    {
        Physics::StepCharacterMovementInWorld(state, input, Physics::FIXED_STEP_DELTA_SEC, 7, world);
    }
    Require(state.isGrounded && Near(state.velocityMps.y, 0.0F), "World entry must land after a jump");

    // The wall starts at x=1. The same capsule must slide along it while retaining tangential velocity.
    world.AddOrUpdateBody({1, Physics::Box{{1.5F, 2.0F, 0.0F}, {0.5F, 2.0F, 8.0F}},
        Physics::CollisionLayer::STATIC_WORLD, Physics::CollisionLayer::ALL, false});
    input.direction = {0.6F, 0.0F, 0.8F};
    for (int step = 0; step < 60; ++step)
    {
        Physics::StepCharacterMovementInWorld(state, input, Physics::FIXED_STEP_DELTA_SEC, 7, world);
    }
    Require(state.positionM.x <= 1.0F - Physics::Settings::PLAYER_CAPSULE_RADIUS_M + 0.002F,
            "World entry must keep the player capsule outside the wall");
    Require(Near(state.positionM.z, 4.0F) && Near(state.velocityMps.z, 4.0F),
            "World entry must preserve tangential movement along the wall");

    // Walk off a raised box: support must be lost and gravity must bring the player to the terrain.
    map.Build(terrain, {});
    world.AddOrUpdateBody({1, Physics::Box{{0.0F, 0.5F, 0.0F}, {1.0F, 0.5F, 1.0F}},
        Physics::CollisionLayer::STATIC_WORLD, Physics::CollisionLayer::ALL, false});
    state = MakeState();
    state.positionM.y = 1.0F;
    Physics::StepCharacterMovementInWorld(state, {}, Physics::FIXED_STEP_DELTA_SEC, 7, world);
    Require(state.isGrounded, "Raised box must support the character");
    input.direction = {1.0F, 0.0F, 0.0F};
    bool fell = false;
    for (int step = 0; step < 90; ++step)
    {
        Physics::StepCharacterMovementInWorld(state, input, Physics::FIXED_STEP_DELTA_SEC, 7, world);
        fell = fell || state.velocityMps.y < 0.0F;
    }
    Require(fell && state.isGrounded && state.positionM.y < 0.01F,
            "Walking off a box must fall and land on terrain");
    state.positionM = {100.0F, 2.0F, 100.0F};
    Physics::StepCharacterMovementInWorld(state, {}, 0.05F, 7, world);
    Require(!state.isGrounded, "Height-map boundaries must not create an infinite floor");
}

void TestSharedMapOwnershipAndPlacement()
{
    namespace World = Kimgane::Shared::World;
    namespace Terrain = Kimgane::Shared::Terrain;
    const std::array<Kimgane::Shared::Geometry::NamedCollisionBox, 2> boxes = {{
        {"wall", {{1.5F, -2.71F, 0.0F}, {0.5F, 2.0F, 8.0F}}},
        {"platform", {{-4.0F, -4.21F, 0.0F}, {1.0F, 0.5F, 1.0F}}}}};
    auto terrain = Terrain::HeightMapData::CreateFlat(21, 21, 1.0F);
    World::TestMapCollision clientMap;
    clientMap.Build(terrain, boxes);
    Require(clientMap.GetWorld().GetBodyCount() == 3, "Each house box needs its own collider ID");
    const auto* wall = clientMap.GetWorld().FindBody(World::TestMapSettings::FIRST_HOUSE_COLLIDER_ID);
    Require(wall != nullptr && Near(std::get<Physics::Box>(wall->shape).centerM.y, 2.0F),
            "House placement must be applied exactly once");
    Require(clientMap.IsHouseCollider(World::TestMapSettings::FIRST_HOUSE_COLLIDER_ID + 1) &&
            !clientMap.IsHouseCollider(World::TestMapSettings::TERRAIN_COLLIDER_ID),
            "Terrain and multiple house boxes must remain distinguishable");

    auto serverMap = clientMap;
    clientMap.Build(Terrain::HeightMapData::CreateFlat(21, 21, 1.0F, 10.0F), {});
    terrain.reset();
    Physics::TerrainSample sample;
    Require(serverMap.GetTerrainSampler().SampleHeightAtWorld({}, sample) && Near(sample.heightM, 0.0F),
            "Copied map must retain its own terrain lifetime after the original is rebuilt");
    auto movedMap = std::move(serverMap);
    auto state = Physics::MakePlayerMovementState({0.0F, -0.1F, 0.0F});
    Physics::ResolveCharacterContactsInWorld(state, 7, movedMap.GetWorld());
    Require(state.isGrounded && state.positionM.y < 0.01F,
            "Moved map must retain valid terrain pointers in registered bodies");
}

void TestSharedTerrainSamplingAndPlayerState()
{
    namespace Terrain = Kimgane::Shared::Terrain;
    auto data = std::make_shared<Terrain::HeightMapData>(2, 2, 1.0F,
        std::vector<float>{0.0F, 1.0F, 0.0F, 1.0F});
    Terrain::HeightMapSampler sampler(data, {10.0F, 3.0F, 20.0F});
    Physics::TerrainSample sample;
    Require(sampler.SampleHeightAtWorld({10.0F, 0.0F, 20.0F}, sample) && Near(sample.heightM, 3.5F),
            "Centered terrain coordinates must include world translation and bilinear sampling");
    Require(sample.normal.x < 0.0F && sample.normal.y > 0.0F,
            "Both hosts must receive the same sloped ground normal");
    Require(sampler.SampleHeightAtWorld({9.5F, 0.0F, 19.5F}, sample) && Near(sample.heightM, 3.0F),
            "The terrain boundary must remain queryable");
    Require(!sampler.SampleHeightAtWorld({9.49F, 0.0F, 19.5F}, sample),
            "Terrain sampling must reject positions outside its bounds");
    const auto state = Physics::MakePlayerMovementState({1.0F, 2.0F, 3.0F});
    Require(Near(state.positionM.y, 2.0F) && !state.isGrounded && state.useGravity &&
            Near(state.dragPerSec, Physics::Settings::PLAYER_DRAG_PER_SEC) &&
            Near(state.groundFrictionPerSec, Physics::Settings::PLAYER_GROUND_FRICTION_PER_SEC),
            "Client and server state initialization must share settings without assuming grounding");
}

void TestFixedStepFrameRates()
{
    for (const int framesPerSecond : {30, 60, 144})
    {
        Physics::FixedStepClock clock;
        TestEnvironment world;
        auto state = MakeState();
        state.useGravity = false;
        int totalSteps = 0;
        for (int frame = 0; frame < framesPerSecond; ++frame)
        {
            const int steps = clock.Advance(1.0 / framesPerSecond);
            totalSteps += steps;
            for (int step = 0; step < steps; ++step)
            {
                Physics::StepCharacterMovement(state, {{1.0F, 0.0F, 0.0F}, 5.0F, 0.0F, false},
                                               Physics::FIXED_STEP_DELTA_SEC, world);
            }
        }
        Require(totalSteps == 60, "One second must produce 60 physics steps at each render rate");
        Require(Near(state.positionM.x, 5.0F), "Movement distance must not depend on rendering frame rate");
    }
}

void TestFixedStepStallsAndRemainder()
{
    Physics::FixedStepClock clock;
    Require(clock.Advance(Physics::FIXED_STEP_SEC * 0.5) == 0, "Partial frames must not advance physics");
    Require(Near(clock.GetInterpolationAlpha(), 0.5F), "Interpolation must use fractional time");
    Require(clock.Advance(Physics::FIXED_STEP_SEC * 0.5) == 1, "Fractional time must carry across frames");
    Require(clock.Advance(10.0) == Physics::FixedStepClock::MAX_STEPS_PER_UPDATE, "Long stalls must have bounded work");
    Require(clock.GetInterpolationAlpha() >= 0.0F && clock.GetInterpolationAlpha() < 1.0F,
            "Dropped backlog must not become interpolation extrapolation");
    Require(clock.Advance(Physics::FIXED_STEP_SEC) == 1, "Stall backlog must not leak into following updates");
    clock.Reset();
    Require(clock.Advance(-1.0) == 0 && clock.Advance(std::numeric_limits<double>::infinity()) == 0,
            "Invalid elapsed times must not advance physics");
    Require(Near(clock.GetInterpolationAlpha(), 0.0F), "Reset must clear interpolation history");
}

void TestContactOnlyAndZeroTime()
{
    TestEnvironment world;
    world.floorEnabled = true;
    auto state = MakeState();
    state.positionM.y = -0.2F;
    state.velocityMps = {2.0F, -1.0F, 3.0F};
    Physics::ResolveCharacterContacts(state, world);
    Require(Near(state.positionM.x, 0.0F) && Near(state.positionM.z, 0.0F), "Contact-only path must not integrate");
    Require(state.isGrounded && Near(state.velocityMps.y, 0.0F), "Contact-only path must resolve grounding");
    const auto before = state.positionM;
    Physics::StepCharacterMovement(state, {{1.0F, 0.0F, 0.0F}, 5.0F, 0.0F, false}, 0.0F, world);
    Require(Near(state.positionM.x, before.x) && Near(state.positionM.y, before.y), "Zero time must not advance motion");
}
} // namespace

int main()
{
    try
    {
        TestMovementAndForceConsumption();
        TestJumpAndLanding();
        TestWallSlideAndRequery();
        TestWorldContactOrientationAndFiltering();
        TestContactOnlyAndZeroTime();
        TestFixedStepFrameRates();
        TestHeightMapWorldEntryPoint();
        TestFixedStepStallsAndRemainder();
        TestSharedMapOwnershipAndPlacement();
        TestSharedTerrainSamplingAndPlayerState();
        std::cout << "Character movement tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
