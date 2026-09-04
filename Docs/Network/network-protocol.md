# Network Protocol

클라이언트와 서버의 통신 규칙을 기록하는 문서입니다. 패킷 구조가 바뀌면 반드시 이 문서를 함께 수정합니다.

## 기준 구현: 'Shared/Protocol.h'
## 마지막 코드 대조일: 2026-09-04

## Policy

- 패킷 이름은 목적이 드러나게 작성합니다.
- Client -> Server, Server -> Client 방향을 명확히 표시합니다.
- 필드 타입, 단위, 좌표계 기준을 기록합니다.
- 임시 패킷도 문서에 남깁니다.
- 패킷 변경은 Client 담당자와 Server 담당자가 함께 리뷰합니다.
- 구현보다 Protocol.h를 기준 문서로 사용합니다.

## Connection

| 항목 | 값 |
| --- | --- |
| Network Environment | TCP/IP |
| Server IP | `127.0.0.1` |
| Port | `3500` (`protocol.h`의 `PORT`) |
| Serialization | Binary struct, `#pragma pack(push, 1)` |
| Endianness | Little-endian, Windows local MVP |
| Max Connections | `MAX_PLAYERS` = 50 |
| NPC Count | `NPC_COUNT` = 10 |
| Max Objects | `MAX_PLAYERS + NPC_COUNT` => 현재 60 |

## Common Header

모든 패킷은 아래 공통 헤더로 시작합니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 전체 크기 (바이트) |
| type | PACKET_TYPE | 패킷 종류 |

## Constants

| 이름 | 값 | 설명 |
| --- | --- | --- |
| WORLD_WIDTH | 20 | 월드 가로 크기 |
| WORLD_HEIGHT | 20 | 월드 세로 크기 |
| MAX_PLAYERS | 50 | 최대 동시 접속자 수 |
| MAX_NAME_LEN | 20 | 유저 이름 최대 길이 |
| PLAYER_MOVE_SPEED | 5.0f | 플레이어 이동 속도 (units/sec) |
| JUMP_POWER | 8.0f | 점프 힘 |
| GRAVITY | 20.0f | 중력 |

## Enums

### PACKET_TYPE

| 값 | 이름 | 방향 |
| --- | --- | --- |
| 0 | C2S_LOGIN | Client -> Server |
| 1 | C2S_MOVE | Client -> Server |
| 2 | C2S_MOVE_START | Client -> Server |
| 3 | C2S_MOVE_STOP | Client -> Server |
| 4 | C2S_ROTATE | Client -> Server |
| 5 | C2S_JUMP | Client -> Server |
| 6 | C2S_PLAYER_STATE | Client -> Server |
| 7 | S2C_LOGIN_RESULT | Server -> Client |
| 8 | S2C_AVATAR_INFO | Server -> Client |
| 9 | S2C_ADD_OBJECT | Server -> Client |
| 10 | S2C_REMOVE_OBJECT | Server -> Client |
| 11 | S2C_MOVE_OBJECT | Server -> Client |
| 12 | S2C_ROTATE | Server -> Client |

### DIRECTION

| 값 | 이름 |
| --- | --- |
| 0 | UP |
| 1 | DOWN |
| 2 | LEFT |
| 3 | RIGHT |

### MONSTER_TYPE
현재 정의되지 않음.

## Packet List

| Packet | Direction | Purpose |
| --- | --- | --- |
| C2S_Login | Client -> Server | 로그인 요청 |
| C2S_Move | Client -> Server | 이동 방향/yaw 전달 (START/STOP 공용) |
| C2S_Rotate | Client -> Server | plyaerID/yaw 전달 |
| C2S_Jump | Client -> Server | 점프 요청 |
| C2S_PlayerState | Client -> Server | 플레이어 위치, 회전, 애니메이션 상태를 주기적으로 전송하여 서버 충돌 판정 및 상태 동기화 수행 |
| S2C_LoginResult | Server -> Client | 로그인 성공/실패 결과 |
| S2C_AvatarInfo | Server -> Client | 본인 아바타 초기 정보 전달 |
| S2C_AddObject | Server -> Client | 플레이어 및 NPC 생성 |
| S2C_RemoveObject | Server -> Client | 플레이어 및 NPC 제거 |
| S2C_MoveObject | Server -> Client | 플레이어/npc 이동 결과(위치) 브로드캐스트 |
| S2C_Rotate | Server -> Client | 회전 갱신 |

## Packet Detail

### C2S_Login

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_LOGIN |
| username | char[MAX_NAME_LEN] | 유저 이름 |

### C2S_Move

`type` 필드로 `C2S_MOVE_START` / `C2S_MOVE_STOP`을 구분해서 재사용합니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_MOVE_START 또는 C2S_MOVE_STOP |
| direction | DIRECTION | 이동 방향 |
| yaw | float | 캐릭터가 바라보는 각도 |

### C2S_Rotate

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_ROTATE |
| playerId | int | 플레이어 ID |
| yaw | float | 회전 값 |

### C2S_JUMP

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_JUMP |

### C2S_PlayerState

플레이어의 현재 상태를 서버에 주기적으로 전달하는 패킷입니다.
애니메이션 상태 추가에 따라 전송 정보가 확장될 수 있습니다.

서버는 해당 정보를 이용하여:
플레이어 상태 동기화 / 이동 보정 / 충돌 판정 검증 / 애니메이션 상태 검증 /치트 및 위치 오차 검출 등을 수행합니다.

현재 구현 기준으로는 일정 Tick 간격(예: 0.2초)마다 전송되며, 서버 권위(Server Authoritative) 기반 위치 검증에 사용됩니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_PLAYER_STATE |
| x | float | X 위치 |
| y | float | Y 위치 |
| z | float | Z 위치 |
| yaw | float | 회전 값 |
| isJumping | bool | 점프 상태 |

### S2C_LoginResult

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_LOGIN_RESULT |
| success | bool | 로그인 성공 여부 |
| message | char[50] | 결과 메시지 |

### S2C_AvatarInfo

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_AVATAR_INFO |
| playerId | int | 플레이어 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |

### S2C_AddObject

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_ADD_PLAYER |
| playerId | int | 오브젝트 ID |
| username | char[MAX_NAME_LEN] | 이름 |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |

### S2C_RemoveObject

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_REMOVE_PLAYER |
| playerId | int | 제거 대상 ID |

### S2C_MoveObject

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_MOVE_OBJECT |
| playerId | int | 오브젝트 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |

### S2C_Rotate

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_MOVE_ROTATE |
| playerId | int | 오브젝트 ID |
| yaw | float | 바라보는 각도 |

## Client Network API

`NetworkManager` (`Kimgane::Engine`)가 제공하는 클라이언트 측 API는 network-manager.md를 참조하세요.

## Change Log

| 날짜 | 변경 내용 | 영향 범위 | 담당 |
| --- | --- | --- | --- |
| 2026-07-06 | TCP/IP 환경, TCP MVP, 기본 헤더 초안, 서버 권위 위치 기준 작성 | Client/Server 초기 구현 | 김채림 |
| 2026-07-21 | `protocol.h` / `NetworkManager.cpp` 기준으로 패킷 스펙 및 클라이언트 API 정리 | NetworkManager/ protocol.h | 김채림 |
| 2026-09-04 | Protocol.h 기준으로 문서 갱신, Object 기반 패킷 구조 반영, network-manager.md 파일과 분리 | 김채림 |