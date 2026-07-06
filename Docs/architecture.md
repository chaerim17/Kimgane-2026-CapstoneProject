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
| Client/Engine | TODO | TODO |
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
| TODO | TODO | TODO | TODO |

## Open Questions

| 항목 | 내용 | 담당 | 마감 |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |
