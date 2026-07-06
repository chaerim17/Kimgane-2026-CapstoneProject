# Component Test Guide

## Purpose

This document defines lightweight tests for checking whether the current client framework components work after a change.

## Debug Smoke Tests

Debug builds run component smoke tests automatically after the DirectX 12 renderer creates the device.

Entry point:

- `Client/src/Engine/Diagnostics/ComponentSmokeTests.h`
- `Client/src/Engine/Diagnostics/ComponentSmokeTests.cpp`
- Called from `Client/src/Main.cpp`

If a smoke test fails, the app throws a runtime error during startup and the existing error message box shows the failed test name.

## Current Coverage

| Area | Tested Behavior |
| --- | --- |
| Core | `GameObject` add/get/remove component, active update skip, `Transform` translate/world matrix |
| Camera | `SpringArmCamera` eye placement and view-projection generation |
| Material | Base color, metallic/roughness clamp, emissive color/intensity clamp |
| Light | `DirectionalLightComponent` direction normalization, color/intensity/ambient clamp |
| Scene | Active light component lookup and fallback light behavior |
| Mesh | Cube mesh creation, index usage, local triangles, local AABB, `MeshComponent` reference storage |
| Physics | `RigidbodyComponent`, `BoxColliderComponent`, `CollisionManager`, `TerrainHeightMap`, `TerrainColliderComponent` |
| Shader | `Assets/Shaders/LitColor.hlsl` vertex/pixel compile and root constant count |

## Visual Scene Probes

`TestScene` includes visual probe cubes for manual rendering checks.

| Probe | Purpose |
| --- | --- |
| `Visual Test Matte` | Rough surface, weak specular |
| `Visual Test Specular` | Low roughness, clear Phong specular |
| `Visual Test Emissive` | Emissive material contribution |
| `Test Cube` | Rotating component stack with mesh, material, collider |

## Rule

- Add a smoke test when adding a new engine component.
- Keep smoke tests deterministic and fast.
- Use these tests for local confidence only; full gameplay tests should be added separately when game rules are defined.
