# DirectX 12 Bootstrap

이 문서는 DirectX 12 첫 화면을 띄우기 위한 초기화 순서와 완료 기준을 정리합니다. 목표는 복잡한 렌더러를 한 번에 만드는 것이 아니라, `Clear Color`가 안정적으로 출력되는 최소 DX12 루프를 만드는 것입니다.

## Goal

MVP의 첫 렌더링 목표:

```text
Win32 Window
DirectX 12 Device
Command Queue
Swap Chain
RTV Descriptor Heap
Command Allocator
Command List
Fence
Clear Color
Present
```

## First Output

첫 성공 화면은 모델 렌더링이 아니라 Clear Color로 판단합니다.

| 항목 | 기준 |
| --- | --- |
| Window | 1280x720 창 모드 |
| Clear Color | TODO |
| Buffer Count | 2 or 3, TODO |
| Swap Chain Format | TODO |
| Depth Buffer | 첫 단계에서는 Optional |
| Debug Layer | Debug 빌드에서 Enable |

## Initialization Order

권장 초기화 순서:

```text
1. Enable Debug Layer, Debug only
2. Create DXGI Factory
3. Select Adapter
4. Create D3D12 Device
5. Create Command Queue
6. Create Swap Chain
7. Create RTV Descriptor Heap
8. Create Render Target Views
9. Create Command Allocator
10. Create Command List
11. Create Fence
12. Create Fence Event
13. Enter Render Loop
```

## Render Loop

최소 렌더 루프:

```text
1. Reset Command Allocator
2. Reset Command List
3. Transition Back Buffer: Present -> Render Target
4. Set Render Target
5. Clear Render Target View
6. Transition Back Buffer: Render Target -> Present
7. Close Command List
8. Execute Command List
9. Present
10. Signal Fence
11. Wait if needed
12. Update Frame Index
```

## Resource Ownership

| 리소스 | 소유 모듈 | 비고 |
| --- | --- | --- |
| DXGI Factory | Renderer Device | Adapter, Swap Chain 생성 |
| D3D12 Device | Renderer Device | 전체 DX12 객체 생성 기준 |
| Command Queue | Renderer Device | Direct command queue |
| Swap Chain | Renderer Device | Window handle 기준 |
| RTV Heap | Descriptor Manager TODO | Back Buffer RTV |
| Command Allocator | Frame Resource TODO | Buffer count와 수명 주의 |
| Command List | Renderer Device TODO | Reset/Close 규칙 명확히 |
| Fence | Frame Sync TODO | GPU 완료 동기화 |

## Debug Layer Rule

- Debug 빌드에서는 D3D12 Debug Layer를 켭니다.
- GPU validation 사용 여부는 성능과 로그량을 보고 결정합니다. TODO
- Debug Layer 경고는 `Docs/troubleshooting.md`에 기록합니다.
- 경고를 무시해야 할 경우 이유를 주석 또는 문서에 남깁니다.

## Common Failure Points

| 증상 | 우선 확인 |
| --- | --- |
| 창은 뜨지만 화면이 안 바뀜 | Command List Close/Execute, Present 호출 |
| Present에서 실패 | Swap Chain 생성 인자, Window Handle, Buffer Count |
| GPU 경고 발생 | Resource Barrier, Descriptor Handle, Fence |
| Command Allocator Reset 오류 | GPU가 아직 해당 allocator를 사용 중인지 확인 |
| RTV Clear 실패 | RTV Descriptor Heap과 Back Buffer RTV 생성 확인 |

## Completion Criteria

`CLIENT-02` 완료 기준:

- Debug x64 빌드 성공
- 창 생성 후 Clear Color 출력
- 프레임마다 Present 호출
- 창 닫기 정상 처리
- Debug Layer에서 치명적인 경고 없음
- DX12 초기화 순서가 이 문서와 크게 어긋나지 않음
- 문제 발생 시 `Docs/troubleshooting.md`에 기록

## Next Steps

Clear Color 이후 순서:

```text
1. Depth Buffer 추가
2. Root Signature 추가
3. Pipeline State Object 추가
4. Vertex Buffer 추가
5. 삼각형 출력
6. Constant Buffer 추가
7. Camera Matrix 추가
8. 20x20m 테스트 공간 출력
```

## Change Log

| 날짜 | 변경 내용 | 이유 |
| --- | --- | --- |
| 2026-07-06 | DX12 부트스트랩 문서 작성 | Client 첫 렌더링 작업 기준 통일 |
