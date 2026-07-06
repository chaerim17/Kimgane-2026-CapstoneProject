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
| Dx12Renderer | Device, Swap Chain, RTV/DSV, Command List, Root Signature, PSO, Present | `Client/src/Engine/Rendering` |
| RenderSettings | Frame Count, Clear Color, Render Target/Depth Format | 기본 렌더 설정 모음 |
| Mesh | Vertex Buffer와 기본 도형 생성 | 현재 Cube Mesh만 구현 |
| MaterialComponent | 오브젝트 단색 머티리얼 데이터 | Root Constants로 전달 |
| MeshComponent | `GameObject`와 `Mesh` 연결 | 컴포넌트 구조 |
| Camera | View, Projection, SpringArm Camera | `Client/src/Engine/Camera` |
| Light | TODO | Directional Light 등 |
| Shader | 현재 런타임 문자열 컴파일 | 추후 `Assets/Shaders/` 분리 |
| Debug UI | TODO | ImGui 사용 여부 TODO |

## Shader Rule

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
