# Architecture

프로젝트 전체 구조와 모듈 책임을 정리하는 문서입니다.

## Goal

TODO: 게임의 핵심 경험과 기술 목표를 작성합니다.

## High-Level Structure

```text
Client
  -> Input
  -> Game Logic
  -> Renderer
  -> Network

Server
  -> Session
  -> Packet Handler
  -> Game State
  -> Sync

Shared
  -> Packet Definition
  -> Common Types
```

## Module Responsibility

| 모듈 | 책임 | 담당 |
| --- | --- | --- |
| Client/Engine/Application | `ClientApplication` 기반 창 생성, 클라이언트 초기화, 메인 루프 단계 관리 | 김영목 |
| Client/Engine/Camera | `Camera`, `FirstPersonCamera`, `ThirdPersonCamera`, `SpringArmCamera`, `SpectatorCamera` | 김영목 |
| Client/Engine/Core | `GameObject`, `Component`, `Transform`, `WindowSettings` 등 클라이언트 기본 구조 | 김영목 |
| Client/Engine/Math | DirectXMath 기반 벡터 보조 함수 | 김영목 |
| Client/Engine/Gameplay | `PlayerControllerComponent` 등 플레이어 조작 컴포넌트 | 김영목 |
| Client/Engine/Input | `InputManager`, `InputState` 등 프레임 입력 상태 | 김영목 |
| Client/Engine/Network | `ClientNetworkFacade` 기반 클라 측 위치 송신/수신 API와 서버 연동 placeholder | 김영목 |
| Client/Engine/Physics | `BoxColliderComponent`, `CapsuleColliderComponent` 등 충돌 컴포넌트 | 김영목 |
| Client/Engine/Rendering | `Dx12Renderer`, `Mesh`, `MeshComponent`, `MaterialComponent` | 김영목 |
| Client/Engine/Scene | `Scene`, `TestScene`, `TestSceneSettings` 등 오브젝트 생명주기와 렌더 목록 관리 | 김영목 |
| Client/Pch | Windows, DirectX, STL 등 안정적인 공통 헤더 PCH | 김영목 |
| Client/Engine | 엔진 공통 기능의 상위 영역. 세부 모듈이 늘어나면 하위 폴더로 분리 | TODO |
| Client/Game | TODO | TODO |
| Client/Renderer | TODO | TODO |
| Client/Network | TODO | TODO |
| Server/Core | TODO | TODO |
| Server/Network | TODO | TODO |
| Server/GameLogic | TODO | TODO |
| Shared | TODO | TODO |

## Data Flow

TODO: 입력, 게임 상태, 렌더링, 서버 동기화 흐름을 작성합니다.

```text
Input -> Client Game State -> Camera/Scene Update -> Renderer -> Packet -> Server State -> Broadcast
```

## Main Loop

TODO: 클라이언트와 서버의 루프 구조를 작성합니다.

```text
Client:
  ProcessInput
  SendLocalPlayerPosition
  ReceivePlayerLocations
  UpdateScene
  UpdateCamera
  Render

Server:
  Accept
  ReceivePacket
  UpdateWorld
  BroadcastState
```

## Design Decisions

| 날짜 | 결정 | 이유 | 영향 |
| --- | --- | --- | --- |
| 2026-07-06 | NexonGameJam의 컴포넌트 구조를 참고해 최소 `GameObject/Component/Transform` 구조만 포팅 | 기존 코드 전체를 가져오면 렌더링, 물리, 머티리얼 의존성이 함께 들어와 초기 DX12 부트스트랩이 무거워짐 | 이후 Renderer/Physics/Scene 작업은 `Client/Engine/Core` 위에 단계적으로 연결 |
| 2026-07-06 | `Dx12Renderer`를 분리하고 Camera/Scene/Mesh/Collider를 독립 모듈로 구성 | `Main.cpp`에 렌더링 책임이 집중되는 것을 줄이고 SOLID 방향으로 확장하기 위함 | HeightMap, ModelMesh, 조명, 입력을 단계적으로 추가 가능 |
| 2026-07-06 | PCH는 외부/표준/DirectX 헤더만 포함하고 프로젝트 헤더는 각 파일에서 명시 include | 빌드 속도와 의존성 가독성을 함께 유지하기 위함 | `Pch.h` 변경 시 전체 재빌드 발생 |
| 2026-07-09 | `Main.cpp`는 WinMain 진입점만 담당하고, 앱 생명주기는 `ClientApplication`으로 분리 | 네트워크 송수신, 서버 좌표 반영, 씬 업데이트 단계가 추가될 때 진입점이 비대해지는 것을 방지 | 후속 `NetworkClient`/위치 API 작업을 `ProcessInput -> UpdateScene -> UpdateCamera -> Render` 흐름에 삽입 가능 |
| 2026-07-09 | 서버 구현 전 클라 네트워크 접점은 `ClientNetworkFacade`의 placeholder로 먼저 고정 | 서버 담당자가 실제 TCP 송수신으로 내부 구현만 교체할 수 있게 하기 위함 | `SendLocalPlayerPosition`, `get_player_location` 호출부를 먼저 검증 가능 |

## Engine Import Notes

| Date | Note | Impact |
| --- | --- | --- |
| 2026-07-06 | `Mesh` now supports optional index buffers and local triangle data. | Terrain, FBX model, and mesh-collider features can share one geometry path. |
| 2026-07-06 | `GameClock` owns per-frame delta time. | Application bootstrapping stays separate from engine timing policy. |
| 2026-07-06 | `TerrainHeightMap`, `TerrainMeshBuilder`, and `TerrainColliderComponent` added. | Terrain rendering and collision share RAW/procedural height data. |
| 2026-07-06 | `CollisionManager` and `RigidbodyComponent` added. | Physics responsibilities are split into broad collision queries and motion integration. |
| 2026-07-06 | `FbxModelMesh` added as converted FBX text loader. | FBX assets can enter the shared `Mesh` path while direct FBX SDK support remains optional. |
| 2026-07-06 | `ShaderCompiler`, `ShaderLibrary`, `Material`, and `DirectionalLight` added. | Rendering state is no longer embedded only inside `Dx12Renderer`. |
| 2026-07-06 | `InputManager`, `PlayerControllerComponent`, and `CapsuleColliderComponent` added. | Local player movement, third-person camera follow, and player-body collision can be tested before networking. |
| 2026-07-09 | `ClientApplication` added. | Win32 bootstrapping and frame loop orchestration are separated from `Main.cpp`. |
| 2026-07-09 | `ClientNetworkFacade` placeholder added. | Client can send local position, consume professor-defined player location API, and render received player positions without a real server. |

## Open Questions

| 항목 | 내용 | 담당 | 마감 |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |
