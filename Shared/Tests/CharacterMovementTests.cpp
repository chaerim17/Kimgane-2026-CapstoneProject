#include "../Physics/CharacterMovement.h"

#include "../Physics/FixedStepClock.h"

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

        TestContactOnlyAndZeroTime();
        TestFixedStepFrameRates();

        TestFixedStepStallsAndRemainder();
        std::cout << "Character movement tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
