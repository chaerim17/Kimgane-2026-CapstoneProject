# Network Protocol

클라이언트와 서버의 통신 규칙을 기록하는 문서입니다. 패킷 구조가 바뀌면 반드시 이 문서를 함께 수정합니다.

## 기준 구현: 'Shared/Protocol.h'
## 마지막 코드 대조일: 2026-09-11

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
| 13 | C2S_SHOOT | Client -> Server |
| 14 | S2C_DAMAGE | Server -> Client |

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
| C2S_Rotate | Client -> Server | playerId/yaw 전달 |
| C2S_Jump | Client -> Server | 점프 요청 |
| C2S_PlayerState | Client -> Server | 플레이어 위치, 회전, 점프 상태 전달 (현재 서버는 위치 오차 계산) |
| C2S_Shoot | Client -> Server | 플레이어 ID와 정규화된 발사 방향 전달 |
| S2C_LoginResult | Server -> Client | 로그인 성공/실패 결과 |
| S2C_AvatarInfo | Server -> Client | 본인 아바타 초기 정보 전달 |
| S2C_AddObject | Server -> Client | 플레이어 및 NPC 생성 |
| S2C_RemoveObject | Server -> Client | 플레이어 및 NPC 제거 |
| S2C_MoveObject | Server -> Client | 플레이어/npc 이동 결과(위치) 브로드캐스트 |
| S2C_Rotate | Server -> Client | 회전 갱신 |
| S2C_Damage | Server -> Client | 데미지 결과와 대상의 최대/현재 HP 전달 |

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

### C2S_Jump

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_JUMP |

### C2S_PlayerState

플레이어의 현재 상태를 서버에 주기적으로 전달하는 패킷입니다.
애니메이션 상태 추가에 따라 전송 정보가 확장될 수 있습니다.

현재 클라이언트 Application은 0.2초 간격으로 전송하며 isJumping에는 false를 전달합니다. 서버 HandlePlayerState()는 서버 위치와 수신 위치의 차이를 계산합니다. 실제 이동 보정, 애니메이션 검증 및 치트 판정을 수행하는 것으로 확장시킬 예정입니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_PLAYER_STATE |
| x | float | X 위치 |
| y | float | Y 위치 |
| z | float | Z 위치 |
| yaw | float | 회전 값 |
| isJumping | bool | 점프 상태 |

### C2S_Shoot

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 21바이트 (4바이트 enum/int/float, 1바이트 정렬 기준) |
| type | PACKET_TYPE | C2S_SHOOT (13) |
| playerId | int | 발사 요청 플레이어 ID |
| direction | Vec3 | 월드 좌표계의 정규화된 발사 방향 (단위 없음) |

`Vec3`는 `float x, y, z` 순서의 12바이트 데이터 구조체이며 연산 함수는 포함하지 않습니다. 클라이언트는 계산하고 정규화한 방향을 `direction`에 `Vec3` 형식으로 채워야 합니다.

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
| type | PACKET_TYPE | S2C_ADD_OBJECT |
| objectId | int | 오브젝트 ID |
| username | char[MAX_NAME_LEN] | 이름 |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |
| maxHp | int | 해당 객체의 최대 체력 |
| currentHp | int | 전송 시점의 해당 객체 현재 체력 |

`maxHp`, `currentHp`는 플레이어와 NPC 공용 필드이지만 현재 체력은 NPC에만 적용합니다. 
현재 `maxHp == 0`은 체력을 사용하지 않는 객체를 뜻합니다. `maxHp > 0`일 때만 HP Bar 비율을 계산해야 합니다.

서버 SendAddObject()는 NPC의 mMaxHp/mCurrentHp를 읽고, 플레이어는 0/0을 채웁니다. NPC 생성 시에는 100/100으로 초기화하지만 중도 접속용 패킷을 만들 때 현재 체력을 다시 초기화하지 않습니다. 클라이언트는 수신값을 ObjectState에 저장합니다. 게임 객체와 HP Bar 연결은 후속 작업입니다. S2C_AvatarInfo에는 HP 필드가 없습니다.

### S2C_RemoveObject

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_REMOVE_OBJECT |
| objectId | int | 제거 대상 ID |

### S2C_MoveObject

HP 필드는 없습니다. 클라이언트는 체력을 유지하며 위치와 회전만 갱신합니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_MOVE_OBJECT |
| objectId | int | 오브젝트 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |

### S2C_Rotate

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_ROTATE |
| objectId | int | 오브젝트 ID |
| yaw | float | 바라보는 각도 |

### S2C_Damage

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 25바이트 (4바이트 enum/int, 1바이트 정렬) |
| type | PACKET_TYPE | S2C_DAMAGE |
| attackerId | int | 공격자 ID |
| targetId | int | 피격 대상 NPC의 객체 ID |
| damage | int | 이번에 실제 적용된 데미지 |
| maxHp | int | 대상의 최대 HP |
| currentHp | int | 데미지 적용 후 현재 HP |

## Client Network API

`NetworkManager` (`Kimgane::Engine`)가 제공하는 클라이언트 측 API는 network-manager.md를 참조하세요.

## Change Log

| 날짜 | 변경 내용 | 영향 범위 | 담당 |
| --- | --- | --- | --- |
| 2026-07-06 | TCP/IP 환경, TCP MVP, 기본 헤더 초안, 서버 권위 위치 기준 작성 | Client/Server 초기 구현 | 김채림 |
| 2026-07-21 | `protocol.h` / `NetworkManager.cpp` 기준으로 패킷 스펙 및 클라이언트 API 정리 | NetworkManager/ protocol.h | 김채림 |
| 2026-09-04 | Protocol.h 기준으로 문서 갱신, Object 기반 패킷 구조 반영, network-manager.md 파일과 분리 | 김채림 |
| 2026-09-11 | S2C_AddObject에 maxHp/currentHp 추가 (전송부 연결은 후속 작업), 발사/데미지 송수신과 NPC 체력 계약 및 미구현 범위 갱신 | Shared/Client/Server 문서 | 김채림 |
