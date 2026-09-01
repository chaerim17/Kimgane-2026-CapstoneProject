# Task Board

GitHub Projects를 사용하기 전까지 이 문서를 임시 태스크보드로 사용합니다. GitHub Issues를 사용하기 시작하면 이 문서는 전체 흐름 요약용으로 유지합니다.

## Board Rule

- 작업은 `Todo`, `In Progress`, `Review`, `Done` 중 하나의 상태를 가집니다.
- 한 사람이 동시에 진행하는 작업은 1~2개로 제한합니다.
- 작업이 막히면 `Blocked`를 표시하고 이유를 남깁니다.
- 완료된 작업은 관련 PR, 커밋, 문서를 연결합니다.

## Status

| 상태 | 의미 |
| --- | --- |
| Todo | 아직 시작하지 않음 |
| In Progress | 담당자가 작업 중 |
| Review | 리뷰 또는 통합 확인 필요 |
| Done | 완료 |
| Blocked | 외부 결정이나 다른 작업 대기 |

## First Week Tasks

| ID | 작업 | 담당 | 상태 | 완료 기준 |
| --- | --- | --- | --- | --- |
| SETUP-01 | Visual Studio 솔루션 및 Client 프로젝트 생성 | 김영목 | Done | `KimganeCapstone.sln`, `Kimgane.Client.vcxproj` 생성 |
| SETUP-02 | Debug/Release x64 설정 확인 | 김영목 | Done | Debug/Release x64 빌드 성공 |
| CLIENT-01 | Win32 창 생성 | 김영목 | Done | 창 생성, 메시지 루프, 종료 처리 |
| CLIENT-02 | DirectX 12 디바이스/스왑체인 초기화 | 김영목 | Done | Clear Color 출력 및 Present 성공 |
| CLIENT-CORE-01 | 최소 GameObject/Component/Transform 구조 추가 | 김영목 | Done | 컴포넌트 추가/조회/삭제, Transform 월드 행렬 계산 가능 |
| CLIENT-03 | 게임 루프와 deltaTime 계산 | 김영목 | Done | Update/Render 흐름 분리 |
| CLIENT-CAMERA-01 | Camera/SpringArmCamera 기본 구조 추가 | 김영목 | Done | 카메라 계층 분리, ViewProjection 전달 |
| CLIENT-CONFIG-01 | 기본 설정 헤더 분리 | 김영목 | Done | Window/Render/Camera/TestScene Settings 헤더 분리 |
| CLIENT-PCH-01 | Client PCH 구성 | 김영목 | Done | `Pch.h/Pch.cpp`, `/Yu`, `/Yc` 빌드 성공 |
| CLIENT-04 | WASD 입력 처리 | 김영목 | Done | 이동 입력 상태를 매 프레임 갱신 |
| CLIENT-05 | 3인칭 카메라 초안 | 김영목 | Done | 플레이어 기준 카메라 위치 계산 |
| RENDER-CUBE-01 | 단색 큐브 테스트 렌더링 | 김영목 | Done | MeshComponent + MaterialComponent 조합 렌더링 |
| PHYSICS-COLLIDER-01 | BoxColliderComponent 기본 구조 추가 | 김영목 | Done | 월드 OBB/AABB 갱신, Raycast 함수 |
| RENDER-01 | 20x20m 테스트 공간 표시 | TODO | Todo | 임시 평면 또는 Height Map 렌더링 |
| RENDER-02 | Directional Light 기본 적용 | TODO | Todo | 조명 방향에 따라 기본 음영 확인 |
| RENDER-03 | HeightMap 지형 Mesh 이식 | TODO | Todo | 원본 구조 참고, 렌더/충돌 클래스 분리 |
| RENDER-04 | ModelMesh 이식 | TODO | Todo | OBJ/FBX/Binary Mesh 후보 분리 |
| SERVER-01 | TCP/IP 서버 부팅 | TODO | Todo | 로컬 포트에서 listen 가능 |
| SERVER-02 | 클라이언트 접속/해제 처리 | TODO | Todo | 2개 클라이언트 접속 로그 확인 |
| SERVER-03 | 플레이어 ID와 위치 상태 저장 | TODO | Todo | 접속별 playerId와 position 관리 |
| NETWORK-01 | 공용 패킷 헤더 정의 | TODO | Todo | `Docs/network-protocol.md`와 코드 기준 일치 |
| NETWORK-02 | PlayerPosition 송수신 | 김영목 | In Progress | Client -> Server 위치 전달 |
| NETWORK-03 | PlayerSnapshot 브로드캐스트 | 김영목 | In Progress | Server -> Clients 위치 전달 |
| DOCS-01 | MVP 진행 결과 기록 | TODO | Todo | 구현 후 README와 Docs 업데이트 |

## Completed Engine Tasks

| ID | Task | Owner | Status | Note |
| --- | --- | --- | --- | --- |
| CLIENT-CLOCK-01 | GameClock engine timing split | Codex | Done | `Main.cpp` no longer owns delta time policy |
| RENDER-MESH-01 | Mesh vertex/index geometry base | Codex | Done | Vertex normal/color, optional index buffer, local triangles |
| RENDER-TERRAIN-01 | RAW HeightMap and TerrainMesh | Codex | Done | RAW8/RAW16 loader, wave fallback, indexed terrain mesh |
| PHYSICS-TERRAIN-01 | TerrainColliderComponent | Codex | Done | World height query and terrain raycast |
| PHYSICS-COLLISION-01 | CollisionManager | Codex | Done | Collider registry, raycast, Box-Box and Terrain-Box contact checks |
| PHYSICS-RIGIDBODY-01 | RigidbodyComponent | Codex | Done | Force, impulse, velocity change, gravity, drag |
| RENDER-FBX-01 | FBX converted text model mesh loader | Codex | Done | `.fbx.txt` / `.txt` converted mesh path, returns shared `Mesh` |
| RENDER-SHADER-01 | Shader/Material/Light structure split | Codex | Done | ShaderCompiler, Material, DirectionalLight, lit color shader |
| RENDER-SHADER-02 | HLSL file split | Codex | Done | `Assets/Shaders/LitColor.hlsl`, runtime `CompileFromFile()` path |
| RENDER-LIGHT-01 | DirectionalLightComponent and Phong shader | Codex | Done | Light as GameObject component, ambient/diffuse/specular/emissive HLSL |
| TEST-CLIENT-01 | Client component smoke tests | Codex | Done | Debug startup tests for Core, Camera, Rendering, Physics, Shader |
| REFACTOR-CLIENT-01 | Client coding style naming migration | Codex | Done | Member variables use `mName`, constants use `UPPER_SNAKE_CASE` |
| CLIENT-INPUT-01 | InputManager and InputState | Codex | Done | WASD/Jump key state, pressed edge, normalized movement axis |
| CLIENT-PLAYER-01 | PlayerControllerComponent | Codex | Done | Camera-relative movement through Rigidbody velocity |
| PHYSICS-CAPSULE-01 | CapsuleColliderComponent | Codex | Done | Player capsule AABB/raycast and collision manager checks |
| PHYSICS-SHARED-RIGIDBODY-01 | Shared Rigidbody core | Codex | Done | `RigidbodyState` and `RigidbodyIntegrator` moved physics formulas into `Shared/Physics` |
| REFACTOR-CLIENT-02 | ClientApplication framework split | Codex | Done | `Main.cpp` only owns WinMain; boot, loop, input, scene, camera, render steps moved to `ClientApplication` |
| NETWORK-CLIENT-01 | Client network location API placeholder | Codex | Done | `ClientNetworkFacade`, `SendLocalPlayerPosition`, `get_player_location`, scene position apply hook |
| CLIENT-SCENE-01 | Client scene flow scaffold | Codex | Done | `TitleScene`, `GameScene`, `LocalGameScene`, `OnlineGameScene`, `OverlayScene`, `SettingsOverlayScene` added |

## Suggested Ownership

| 담당 영역 | 추천 첫 작업 |
| --- | --- |
| 김영목 | `CLIENT-01`, `CLIENT-02`, `CLIENT-03` |
| 김준해 | `CLIENT-04`, `CLIENT-05`, `RENDER-01`, `RENDER-02` |
| 김채림 | `SERVER-01`, `SERVER-02`, `SERVER-03`, `NETWORK-01` |
| Together | `NETWORK-02`, `NETWORK-03`, MVP 통합 테스트 |

## Review Queue

| ID | PR/Branch | 확인할 내용 | 상태 |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |

## Done

| ID | 완료 내용 | 관련 문서/PR |
| --- | --- | --- |
| DOCS-SETUP-01 | README, 협업 규칙, 개발환경, MVP/기술결정/태스크보드 문서 추가 | TODO |
| CLIENT-BOOTSTRAP-01 | Kimgane.Client Windows Desktop Application 및 DX12 Clear Color 부트스트랩 생성 | `Docs/dx12-bootstrap.md` |
| CLIENT-CORE-01 | NexonGameJam 구조를 참고한 최소 클라이언트 컴포넌트 코어 추가 | `Docs/architecture.md` |
| CLIENT-RENDER-CORE-01 | Mesh/Material/Collider/Scene/Dx12Renderer 및 단색 큐브 렌더링 구조 추가 | `Docs/rendering-pipeline.md` |
| CLIENT-CAMERA-01 | Camera, FirstPerson, ThirdPerson, SpringArm, Spectator 카메라 계층 추가 | `Docs/architecture.md` |
| CLIENT-CONFIG-01 | Window/Render/Camera/TestScene 기본 설정 헤더 추가 | `Docs/architecture.md` |
| CLIENT-PCH-01 | PCH 구성 및 Windows `NOMINMAX` 기준 추가 | `Docs/development-environment.md` |
