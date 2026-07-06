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
| Client/Engine/Core | `GameObject`, `Component`, `Transform` 등 클라이언트 게임 오브젝트 기본 구조 | 김영목 |
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
Input -> Client Game State -> Packet -> Server State -> Broadcast -> Client Render State
```

## Main Loop

TODO: 클라이언트와 서버의 루프 구조를 작성합니다.

```text
Client:
  ProcessInput
  Update
  SendPacket
  ReceivePacket
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

## Open Questions

| 항목 | 내용 | 담당 | 마감 |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |
