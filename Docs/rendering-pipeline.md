# Rendering Pipeline

DirectX 렌더링 구조와 리소스 관리 기준을 기록하는 문서입니다.

## Graphics API

| 항목 | 값 |
| --- | --- |
| DirectX Version | DirectX 12 |
| Shader Model | `vs_5_0`, `ps_5_0` for bootstrap |
| Swap Chain Format | `DXGI_FORMAT_R8G8B8A8_UNORM` |
| Depth Format | `DXGI_FORMAT_D32_FLOAT` |
| Coordinate System | Left-handed, Y-up, meters |
| Math Library | DirectXMath |

## Pipeline Overview

TODO: 실제 구현 후 단계별로 업데이트합니다.

```text
Initialize Device
Create Command Queue
Create Command Allocator
Create Command List
Create Descriptor Heaps
Create Swap Chain
Create Render Target
Create Depth Stencil
Create Fence
Create Root Signature
Create Pipeline State
Load Mesh / Shader
Update Camera ViewProjection
Update Root Constants
Record Command List
Draw
Execute Command List
Present
Wait For Fence
```

## Renderer Module

| 모듈 | 책임 | 비고 |
| --- | --- | --- |
| Dx12Renderer | Device, Swap Chain, RTV/DSV, Command List, Root Signature, PSO, Present | `Client/src/Rendering` |
| RenderSettings | Frame Count, Clear Color, Render Target/Depth Format | 기본 렌더 설정 모음 |
| Mesh | Vertex Buffer와 기본 도형 생성 | 현재 Cube Mesh만 구현 |
| Material | 오브젝트 색·표면·발광·텍스처 데이터 | `GameObject`가 값으로 보유, Root Constants로 전달 |
| GameObject::Mesh | 그릴 `Mesh`(공유) | `GameObject::SetMesh`, 없으면 그리지 않음 |
| Camera | View, Projection, SpringArm Camera | `Client/src/Camera` |
| Light | TODO | Directional Light 등 |
| Shader | 현재 런타임 문자열 컴파일 | 추후 `Assets/Shaders/` 분리 |
| Debug UI | TODO | ImGui 사용 여부 TODO |

## TRAPPIST-1e 스카이박스

- 로컬·온라인 게임 프레임에서 깊이 버퍼를 지운 뒤 `RenderSkybox()`를 한 번 호출합니다. 다음 월드 패스의 지형과 오브젝트가 하늘을 가리고, 설정 오버레이와 HUD가 위에 그려집니다.
- `Skybox.hlsl`은 버텍스 버퍼 없는 전체 화면 삼각형을 그리고 카메라의 회전·투영으로 복원한 월드 방향으로 `TextureCube`를 샘플링합니다. 카메라가 이동해도 별 배치가 밀리지 않습니다.
- DDS 로더의 큐브맵 정보를 사용해 기존 SRV 힙에 `TEXTURECUBE` 뷰 한 개를 할당합니다. 스카이박스 PSO가 기존 루트 시그니처의 `b0`과 `t0`를 사용합니다. 이후 월드 패스가 씬 상수를 갱신하고 각 오브젝트가 자기 텍스처를 바인딩합니다.
- 소스와 6면 방향은 [스카이박스 에셋 안내](../Assets/Textures/Skybox/Trappist-1e-2K/README_KO.md)에 기록합니다.

## 탐지 펄스

- 로컬 테스트에서는 시작 위치 앞의 벽 뒤에 NPC가 있고, 1번 퀵슬롯에 테스트용 칩이 있다. 온라인에서는 상자에서 획득한 칩을 선택한다. 두 모드 모두 `F`로 발동한다.
- 발동 순간 렌더링용 카메라의 뷰-투영 행렬과 위치를 사용한다. 플레이어로부터 12m 안에 있고 가슴 기준점이 화면에 투영되며 지형이나 벽에 가린 대상만 고른다. 1.5초 표시, 15초 쿨타임은 테스트 가안이다.
- 이번 펄스에서 탐지한 적만 별도 깊이 버퍼에 그린다. 월드 깊이와 비교해 가려진 표면에 빨간색 반투명 채움과 2px 윤곽을 합성한다. 같은 모델의 뒷면으로 인해 앞에 드러난 부분까지 하이라이트되는 문제를 막기 위해 적의 가장 가까운 표면만 비교한다.
- 깊이 리소스는 `R32_TYPELESS`로 만들고 그릴 때는 `D32_FLOAT`, 합성할 때는 `R32_FLOAT`로 읽는다. 대상이 있는 프레임에만 합성하며, 이후 월드 깊이 쓰기 상태와 기존 루트 시그니처를 복구한다.
- 온라인에서도 기존 인벤토리와 동기화된 플레이어·NPC 정보로 클라이언트에서 표시를 검증한다. 발동·쿨타임은 로컬에서 관리하며 새 패킷은 없다. 현재 팀 ID가 없으므로 다른 플레이어를 테스트 대상으로 취급한다.

후속 서버 연동에서는 발동자의 칩 장착, 스태미나, 쿨타임을 확인한다. 서버 위치로 대상의 사거리·생존·적대 관계를 확인하고, 승인된 대상 ID와 표시 기간을 발동자에게 반환한다. 클라이언트는 그 목록을 발동 당시 카메라로 선별한 목록과 교차한 뒤 기존 하이라이트 패스에 넘긴다.

## Shader Rule

- Runtime HLSL source files live under `Assets/Shaders/`.
- Bootstrap lit color shader path: `Assets/Shaders/LitColor.hlsl`.
- `ShaderCompiler::CompileFromFile()` is the default path for runtime shader compilation.

- 셰이더 파일은 `Assets/Shaders/` 또는 확정된 경로에 둡니다.
- 컴파일된 셰이더 산출물은 Git에 올릴지 여부를 결정합니다. TODO
- 상수 버퍼 구조체는 C++ 코드와 HLSL 레이아웃을 함께 관리합니다.
- 셰이더 변경 시 입력 레이아웃과 상수 버퍼 문서를 함께 확인합니다.
- DirectX 12에서는 DXC 사용 여부를 먼저 결정합니다. TODO

## DirectX 12 Rule

- 리소스 상태 전환은 명시적인 Resource Barrier로 관리합니다.
- Descriptor Heap 인덱스 정책은 문서화 후 코드에 반영합니다.
- Command Allocator는 GPU 실행 완료 시점 이후에 Reset합니다.
- Fence 값과 Frame Resource 관리는 렌더러 공통 규칙으로 둡니다.
- Debug Layer 경고는 가능한 한 즉시 원인을 기록하고 해결합니다.

## Resource Rule

| 리소스 | 포맷 | 경로 | 비고 |
| --- | --- | --- | --- |
| Basic Mesh | C++ generated vertex buffer | Code | Cube Mesh 구현 |
| Height Map | TODO | `Assets/Models/` 또는 `Assets/Textures/` | 다음 단계 |
| Model | OBJ/FBX/Binary TODO | `Assets/Models/` | 다음 단계 |
| Texture | TODO | `Assets/Textures/` | TODO |
| Shader | TODO | `Assets/Shaders/` | TODO |
| Sound | TODO | `Assets/Sounds/` | TODO |

## Mesh Rule

- `Mesh` stores position, normal, vertex color, optional index buffer, and local triangle data.
- Terrain, FBX model, and mesh collider features must reuse this mesh path before adding custom draw code.

## Terrain Rule

- `TerrainHeightMap` owns sample data in meters and supports `RAW8`, little-endian `RAW16`, and square-size auto detection.
- `TerrainMeshBuilder` converts height samples into indexed mesh geometry centered at the owner transform.
- 지형 높이 조회·레이캐스트는 같은 높이 데이터를 쓰는 Shared `TestMapCollision`의 지형 body가 담당합니다.

## Model Mesh Rule

- `FbxModelMesh` currently loads converted FBX text files (`.fbx.txt` or `.txt`) and returns the shared `Mesh` type.
- Direct binary FBX loading requires an explicit FBX SDK or asset-pipeline decision before linking external libraries.

## Shader And Lighting Rule

- `ShaderCompiler` owns HLSL compilation and `ShaderLibrary` owns bootstrap shader path lookup.
- `Material` stores object render properties, and each `GameObject` holds one `Material` by value.
- `Scene` owns a single `DirectionalLight`. `GameScene` configures it in `Build`; UI scenes use the default.
- `Material` stores base color, surface values, and emissive color/intensity.
- `Assets/Shaders/LitColor.hlsl` uses a Phong-style ambient + diffuse + specular + emissive lighting model.

## NPC Damage Numbers

- 미팅일지의 2026-08-26 사냥 항목과 2026-09-28 ToDo에 따라, NPC 피격 시 실제 데미지 숫자를 머리 위에 표시합니다.
- `S2C_DAMAGE` 수신 결과만 표시합니다. 서버의 발사 판정, HP 계산, 기존 패킷 구조는 그대로 사용합니다.
- `ClientApplication`이 위치 → 데미지 → 제거 순서로 반영하고, `GameScene::ShowDamageNumber()`가 NPC와 독립된 `DamageNumber`를 풀에서 꺼내 띄웁니다. NPC가 사라지는 마지막 공격도 숫자의 수명이 끝날 때까지 표시됩니다.
- `UI/DamageNumber`가 1.1초 동안 상승량과 불투명도를 계산합니다. 마지막 0.45초에 페이드아웃하고, 수명이 끝나면 비활성으로 돌아가 재사용됩니다.
- `Dx12Renderer`는 월드 위치를 화면으로 투영한 뒤 기존 Direct2D/DirectWrite 텍스트 경로로 그립니다. 글자는 30 DIP 크기의 노란색에 검은 그림자를 더하고, 연속 피격은 좌우 3개 위치를 번갈아 사용합니다.
- 벽/지형에 가려지거나 카메라의 표시 범위를 벗어난 숫자는 그리지 않습니다. 동시 표시 숫자는 최대 64개이며, 비활성 숫자 또는 가장 오래된 숫자를 재사용합니다.
- 수명, 상승 속도, 글자 크기, 색상, 동시 표시 수는 `Client/src/Rendering/RenderSettings.h`의 `DAMAGE_NUMBER_*`, `MAX_DAMAGE_NUMBER_COUNT`에서 조절합니다.

### 수동 확인 항목

빌드와 게임 실행은 사용자 요청에 따라 수행하지 않았습니다. 사용자 빌드 후 온라인 모드에서 확인합니다.

1. 조준 후 NPC를 좌클릭으로 공격하면 서버가 보낸 데미지 숫자가 위로 올라가며 사라지는지 확인합니다.
2. NPC를 연속 공격했을 때 각 피격 숫자가 따로 표시되는지 확인합니다.
3. NPC가 죽는 공격에서도 남은 HP만큼의 마지막 데미지 숫자가 표시되는지 확인합니다.
4. 빗나간 공격과 벽/지형에 막힌 공격에는 숫자가 생기지 않는지 확인합니다.
5. 카메라를 돌리거나 이동했을 때 숫자가 피격 당시 위치에 유지되고, 가려진 숫자는 보이지 않는지 확인합니다.

## Performance Targets

| 항목 | 목표 | 측정 방법 |
| --- | --- | --- |
| FPS | TODO | TODO |
| Frame Time | TODO | TODO |
| Draw Calls | TODO | TODO |
| VRAM Usage | TODO | TODO |
| Loading Time | TODO | TODO |

## Debugging

| 문제 | 확인 항목 |
| --- | --- |
| 화면이 검은색 | Swap Chain, RTV, Clear Color, Command List, Execute, Present 확인 |
| 모델이 안 보임 | 카메라, 좌표계, 월드 행렬, Depth, Cull Mode, Resource Barrier 확인 |
| 텍스처가 안 나옴 | 경로, SRV, Descriptor Heap, Root Signature, 샘플러, UV, 포맷 확인 |
| 조명이 이상함 | Normal, Light Direction, 좌표계, 상수 버퍼 정렬 확인 |
| GPU가 멈추거나 경고 발생 | Fence, Command Allocator Reset 시점, Resource Lifetime 확인 |

## Change Log

| 날짜 | 변경 내용 | 이유 | 담당 |
| --- | --- | --- | --- |
| 2026-07-06 | DirectX 12 기준으로 렌더링 파이프라인 문서 갱신 | 실제 개발환경 반영 | TODO |
| 2026-07-06 | `Dx12Renderer`, Depth Buffer, Root Signature, PSO, Cube Mesh, MaterialComponent 기준 반영 | 단색 큐브 렌더링 구조 추가 | Codex |
| 2026-07-06 | `RenderSettings`, `CameraSettings`, `WindowSettings`, `TestSceneSettings` 기준 추가 | 상수/기본 설정값 위치 통일 | Codex |
| 2026-10-11 | 서버 데미지 결과를 상승·페이드아웃 숫자로 표시, 객체 재사용 및 수동 확인 항목 추가 | 미팅일지의 데미지 숫자 표시 요구 반영 | Codex |
