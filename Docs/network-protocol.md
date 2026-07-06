# Network Protocol

클라이언트와 서버의 통신 규칙을 기록하는 문서입니다. 패킷 구조가 바뀌면 반드시 이 문서를 함께 수정합니다.

## Policy

- 패킷 이름은 목적이 드러나게 작성합니다.
- Client -> Server, Server -> Client 방향을 명확히 표시합니다.
- 필드 타입, 단위, 좌표계 기준을 기록합니다.
- 임시 패킷도 문서에 남깁니다.
- 패킷 변경은 Client 담당자와 Server 담당자가 함께 리뷰합니다.

## Connection

| 항목 | 값 |
| --- | --- |
| Network Environment | TCP/IP |
| Protocol | TCP for MVP |
| Server IP | `127.0.0.1` |
| Port | `27015` |
| Serialization | Binary struct draft |
| Endianness | Little-endian, Windows local MVP |
| Tick Rate | TODO |

## Common Header

TODO: 공통 패킷 헤더를 확정하면 작성합니다.

| Field | Type | Description |
| --- | --- | --- |
| packetId | uint16 | 패킷 종류 |
| size | uint16 | 전체 크기 |

## Packet List

| Packet | Direction | Purpose | Status |
| --- | --- | --- | --- |
| PlayerPosition | Client -> Server | 플레이어 위치 전송 | Draft |
| PlayerSnapshot | Server -> Client | 서버 기준 플레이어 위치 전달 | Draft |

## Packet Detail

### PlayerPosition

- Direction: Client -> Server
- Purpose: 플레이어의 현재 위치 전송
- Status: Draft

| Field | Type | Description |
| --- | --- | --- |
| playerId | int | 플레이어 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |

### PlayerSnapshot

- Direction: Server -> Client
- Purpose: 서버가 관리하는 플레이어 위치 전달
- Status: Draft

| Field | Type | Description |
| --- | --- | --- |
| playerId | int | 플레이어 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |

## Synchronization Rule

| 항목 | 기준 |
| --- | --- |
| Authority | Server authoritative position |
| Position Sync Rate | TODO |
| Interpolation | Post-MVP |
| Prediction | Post-MVP |
| Reconciliation | Post-MVP |

## Change Log

| 날짜 | 변경 내용 | 영향 범위 | 담당 |
| --- | --- | --- | --- |
| 2026-07-06 | TCP/IP 환경, TCP MVP, 기본 헤더 초안, 서버 권위 위치 기준 작성 | Client/Server 초기 구현 | TODO |
