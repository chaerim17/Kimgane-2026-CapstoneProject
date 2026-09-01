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
| Server IP | `127.0.0.1` |
| Port | `3500` (`protocol.h`의 `PORT`) |
| Serialization | Binary struct, `#pragma pack(push, 1)` |
| Endianness | Little-endian, Windows local MVP |
| Max Connections | `MAX_PLAYERS` = 50 |

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

## Enums

### PACKET_TYPE

| 값 | 이름 | 방향 |
| --- | --- | --- |
| 0 | C2S_LOGIN | Client -> Server |
| 1 | C2S_MOVE | Client -> Server |
| 2 | C2S_MOVE_START | Client -> Server |
| 3 | C2S_MOVE_STOP | Client -> Server |
| 4 | S2C_LOGIN_RESULT | Server -> Client |
| 5 | S2C_AVATAR_INFO | Server -> Client |
| 6 | S2C_ADD_PLAYER | Server -> Client |
| 7 | S2C_REMOVE_PLAYER | Server -> Client |
| 8 | S2C_MOVE_PLAYER | Server -> Client |

### DIRECTION

| 값 | 이름 |
| --- | --- |
| 0 | UP |
| 1 | DOWN |
| 2 | LEFT |
| 3 | RIGHT |

## Packet List

| Packet | Direction | Purpose |
| --- | --- | --- |
| C2S_Login | Client -> Server | 로그인 요청 |
| C2S_Move | Client -> Server | 이동 방향/yaw 전달 (START/STOP 공용) |
| S2C_LoginResult | Server -> Client | 로그인 성공/실패 결과 |
| S2C_AvatarInfo | Server -> Client | 본인 아바타 초기 정보 전달 |
| S2C_AddPlayer | Server -> Client | 다른 플레이어 입장 알림 |
| S2C_RemovePlayer | Server -> Client | 플레이어 퇴장 알림 |
| S2C_MovePlayer | Server -> Client | 플레이어 이동 결과(위치) 브로드캐스트 |

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

### S2C_AddPlayer

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_ADD_PLAYER |
| playerId | int | 플레이어 ID |
| username | char[MAX_NAME_LEN] | 유저 이름 |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |

### S2C_RemovePlayer

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_REMOVE_PLAYER |
| playerId | int | 퇴장한 플레이어 ID |

### S2C_MovePlayer

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_MOVE_PLAYER |
| playerId | int | 이동한 플레이어 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |

## Client Network API

`NetworkManager` (`Kimgane::Engine`)가 제공하는 클라이언트 측 API입니다.

| API | Purpose |
| --- | --- |
| `Initialize()` | 서버 접속 및 `C2S_Login` 전송 |
| `Shutdown()` | 연결 종료 |
| `Update(float deltaTime)` | 수신 폴링 및 패킷 처리 |
| `SendMoveStart(int direction, float yaw)` | `C2S_MOVE_START` 전송 |
| `SendMoveStop(int direction)` | `C2S_MOVE_STOP` 전송 |
| `GetPlayerLocation(int* id, float* x, float* y, float* z, float* yaw)` | 위치 갱신 큐에서 1건 꺼내기 |
| `GetRemovedPlayer(int* playerId)` | 퇴장 플레이어 큐에서 1건 꺼내기 |

## Change Log

| 날짜 | 변경 내용 | 영향 범위 | 담당 |
| --- | --- | --- | --- |
| 2026-07-06 | TCP/IP 환경, TCP MVP, 기본 헤더 초안, 서버 권위 위치 기준 작성 | Client/Server 초기 구현 | TODO |
| 2026-07-09 | 서버 구현 전 클라 `ClientNetworkFacade` placeholder와 `get_player_location(...)` API 추가 | Client 네트워크 접점 | 김영목 |
| 2026-07-21 | `protocol.h` / `NetworkManager.cpp` 기준으로 패킷 스펙 및 클라이언트 API 정리 | NetworkManager/ protocol.h | 김채림 |