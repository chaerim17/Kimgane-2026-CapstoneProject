# Component Test Guide

## Purpose

This document defines lightweight tests for checking whether the current client framework components work after a change.

## Debug Smoke Tests

Debug builds run component smoke tests automatically after the DirectX 12 renderer creates the device.

Entry point:

- `Client/src/Debug/ComponentSmokeTests.h`
- `Client/src/Debug/ComponentSmokeTests.cpp`
- Called from `Client/src/Main.cpp`

If a smoke test fails, the app throws a runtime error during startup and the existing error message box shows the failed test name.

## Current Coverage

| Area | Tested Behavior |
| --- | --- |
| Core | `GameObject` add/get/remove component, active update skip, `Transform` translate/world matrix |
| Camera | `SpringArmCamera` eye placement and view-projection generation |
| Input | `InputManager` key state, pressed edge, diagonal movement normalization |
| Gameplay | `PlayerControllerComponent` Rigidbody velocity control and jump hook |
| Material | Base color, metallic/roughness clamp, emissive color/intensity clamp |
| Light | `DirectionalLightComponent` direction normalization, color/intensity/ambient clamp |
| Scene | Active light component lookup and fallback light behavior |
| Mesh | Cube mesh creation, index usage, local triangles, local AABB, `MeshComponent` reference storage |
| Network | `ClientNetworkFacade` initialize/shutdown, sent local position storage, `get_player_location(...)` mock receive API |
| Physics | `RigidbodyComponent`, `BoxColliderComponent`, `CapsuleColliderComponent`, `CollisionManager`, `TerrainHeightMap`, `TerrainColliderComponent` |
| Shader | `Assets/Shaders/LitColor.hlsl` vertex/pixel compile and root constant count |

## Gameplay Scene Checks

The rotating test obstacle and three material probe cubes have been removed from gameplay scenes.
Local and online scenes use the shared lunar map without additional probe objects or cube collision bodies.
Cube mesh generation remains available for UI geometry, model fallbacks and component tests.

Manual scene checks cover the local player's movement and collisions with terrain, buildings and the purifier.

## Rule

- Add a smoke test when adding a new engine component.
- Keep smoke tests deterministic and fast.
- Use these tests for local confidence only; full gameplay tests should be added separately when game rules are defined.
