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
| CLIENT-CORRECTION-01 | `refactor/client` | 로컬 캐릭터의 서버 위치 보정을 표시 오프셋으로 완화; 연속 보정/큰 이동/카메라 확인 | Debug x64 빌드 통과; 게임 실행 확인 대기; 구현 커밋: `ef04a2b`, `e2918b5` |
| REFACTOR-PHYSICS-05 | `refactor/client` | Client/Server 물리 60Hz, 점프 입력 보관, 로컬 표시 보간; 서버 전송 50ms 유지 | Debug x64 빌드 통과; 게임 실행 확인 대기; 구현 커밋: `ef04a2b`, `e2918b5` |
| REFACTOR-PHYSICS-04 | `refactor/client` | Shared 강체 상태/지형·집 충돌 연결, 서버 물리 60Hz 및 전송 50ms 분리 | Debug x64 빌드 및 공용 테스트 통과; 온라인 게임 확인 대기 |
| REFACTOR-PHYSICS-03 | `refactor/client` | Shared 캐릭터 입력/적분/접촉 보정 공통화 및 Client/Server 연결 | Debug x64 빌드 통과; 게임 실행 확인 대기; 구현 커밋: `ef04a2b`, `e2918b5` |
| REFACTOR-PHYSICS-02 | `refactor/client` | 씬의 캐릭터 충돌 보정을 `CharacterCollisionSolver`로 추출; 이동/착지/벽 슬라이딩/온라인 보정 확인 | Debug x64 빌드 통과; 게임 실행 확인 대기; 구현 커밋: `f1a7a1a` |
| REFACTOR-PHYSICS-01 | `refactor/client` | 미사용 충돌 이벤트 큐 및 전체 쌍 검사 제거; 로컬 이동, 지형/집 충돌, 레이캐스트 확인 | Debug x64 빌드 통과; 게임 실행 확인 대기; 구현 커밋: `f1a7a1a` |
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

## develop 기준 클라이언트 보정 이식 (2026-09-12)

- 기준: `origin/develop`의 `f596f72`, 작업 브랜치: `refactor/client`.
- 최신 조준 카메라, 이동 yaw와 시선 yaw 분리를 유지하면서 충돌 Solver 분리, 로컬 60Hz 물리, 점프 입력 보관, 모델/카메라 표시 보간을 이식했습니다.
- 서버 위치는 물리 상태에 즉시 반영하고 표시 오프셋만 반감기 0.05초로 줄입니다. 2m 이상 보정은 즉시 표시합니다.
- `Server/`는 develop과 동일합니다. 서버 연결용 CharacterMovementWorld 및 CollisionWorld 확장은 제외했습니다. Shared에는 클라이언트가 사용하는 추가 이동 API와 고정 시간 계산만 남기며 기존 서버 이동 함수는 유지합니다.
- 검증: Client/Server Debug x64 빌드 성공, Shared 이동·힘 소비·점프·착지·벽 슬라이딩·30/60/144 FPS·긴 프레임 테스트 통과, diff 공백 오류 없음.
- 실제 온라인 조준 이동, 연속 서버 보정, 카메라 흔들림은 게임 실행 확인이 남아 있습니다. 서버는 기존 50ms 물리와 계산을 유지하므로 클라이언트 예측과의 차이 자체를 제거한 것은 아닙니다.
- 이전 미커밋 작업은 `backup before refactor/client develop migration` stash에 보존했습니다.
## 서버 캐릭터 강체 연결 (2026-09-12)

- 후속 요청에 따라 서버 수평 이동/점프의 별도 계산을 클라이언트와 동일한 Shared 강체 계산으로 교체했습니다. 서버의 50ms 위치 전송 및 NPC 갱신 구조는 유지합니다.
- 세션별 강체 상태를 보관하고, 입력 잠금 아래 이동 방향/점프 요청을 가져와 60Hz로 계산합니다. 접속 시 충돌 접지 상태를 초기화하며 점프 요청은 물리 스텝마다 한 번 소비합니다.
- 검증: Client/Server Debug x64 빌드 성공. 공용 테스트에서 스폰 접지, 점프/착지, 공중 점프 차단, 벽 슬라이딩, 발판 이탈 후 낙하, 자기 자신/트리거/마스크 제외, 지형 경계 및 프레임 주기 검증 통과.
- 실제 TCP 검증은 기존 3500 포트 사용과 테스트 작업 경로의 RAW 로딩 실패로 완료하지 못했습니다. 게임에서 조준 이동과 연속 서버 보정 확인이 남아 있습니다.
- 앞 절의 서버 변경 제외 기록은 이전 이식 시점의 범위입니다. 이번 후속 작업은 서버 연결 변경을 포함합니다.

## 맵 충돌 월드와 캐릭터 이동 공통화 (2026-09-15)

- `HeightMapData`로 지형 로딩·높이·법선 계산을 통일하고, `TestMapCollision`으로 지형과 집의 배치·충돌 등록을 공통화했습니다.
- Client/Server는 같은 `RigidbodyState` 초기화와 `StepCharacterMovementInWorld`를 사용합니다. 클라이언트 전용 캐릭터 충돌 Solver와 구형 수평·수직 이동 API는 제거했습니다.
- 온라인 씬은 공통 맵만 사용하며 회전 테스트 큐브는 로컬 테스트 씬에 남겼습니다.
- 서버 타이머·동기화는 `origin/develop`의 `383a51b` 기준으로 복원했습니다. 기존 `Sleep(50)`·`mWorldMutex`·IOCP 구조를 유지하고, 브랜치에서 추가한 서버 60Hz 누적 시간 계산·별도 전송 주기 제어·세션별 입력 mutex는 제거했습니다. 앞 절의 서버 60Hz·입력 잠금 기록은 과거 작업에 해당합니다.
- 서버는 각 50ms 틱에서 공통 이동 함수를 호출합니다. 클라이언트는 기존 60Hz이므로 시간 간격에 따른 결과 차이는 남습니다. 입력 기록·네트워크 보정과 패킷 형식은 변경하지 않았습니다.
- 검증: 프로젝트 등록·include·잔여 참조·diff 정적 검토. 사용자 요청으로 빌드·실행·테스트 실행은 하지 않았으며 이전 절의 빌드 통과 기록은 이번 변경에 적용되지 않습니다.
- 구조와 사용자 검증 항목: `Docs/Physics/shared-character-world.md`.

## 전체 아키텍처 문서 갱신 (2026-09-15)

- `DOCS-ARCH-01`: Client·Server·Shared의 현재 코드 기준으로 `Docs/architecture.md`를 갱신했습니다.
- 실행 파일·모듈 경계, 객체 소유, 클라 프레임과 서버 스레드, 패킷과 상태 권한, 공통 물리·맵, NPC·사격·HP, 카메라·렌더링·에셋 경로를 정리했습니다.
- 미연결 코드와 기능별 수정 시작 지점을 구분하고, README의 아키텍처 설명과 네트워크 문서 링크를 연결했습니다.
