# Network Protocol

클라이언트와 서버의 통신 규칙을 기록하는 문서입니다. 패킷 구조가 바뀌면 반드시 이 문서를 함께 수정합니다.

## 기준 구현: 'Shared/Protocol.h'
## 마지막 코드 대조일: 2026-11-20 (현재 Shared/Client/Server 코드 정적 대조)

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
| 7 | C2S_SHOOT | Client -> Server |
| 8 | C2S_OPEN_ITEM_BOX | Client -> Server |
| 9 | S2C_LOGIN_RESULT | Server -> Client |
| 10 | S2C_AVATAR_INFO | Server -> Client |
| 11 | S2C_ADD_OBJECT | Server -> Client |
| 12 | S2C_REMOVE_OBJECT | Server -> Client |
| 13 | S2C_MOVE_OBJECT | Server -> Client |
| 14 | S2C_ROTATE | Server -> Client |
| 15 | S2C_DAMAGE | Server -> Client |
| 16 | S2C_OPEN_ITEM_BOX_RESULT | Server -> Client |
| 17 | C2S_START_PURIFICATION | Client -> Server |
| 18 | C2S_REQUEST_PURI_SYRINGE | Client -> Server |
| 19 | S2C_PURI_SYRING_RESULT | Server -> Client |

### ObjectType / ItemId

| enum | 기반 타입 | 값 |
| --- | --- | --- |
| `Kimgane::Shared::World::ObjectType` | `std::uint8_t` | Player=0, Npc=1, ItemBox=2 |
| `Kimgane::Shared::Items::ItemId` | `std::uint16_t` | None=0, HpPotion=1, Chip=2, Armor=3, PuriSyringe=4 |

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
| C2S_PlayerState | Client -> Server | 플레이어 위치, 회전, 접지 상태 전달 |
| C2S_Shoot | Client -> Server | 플레이어 ID와 정규화된 발사 방향 전달 |
| C2S_OpenItemBox | Client -> Server | 대상 상자 ID로 수령 요청 |
| S2C_LoginResult | Server -> Client | 로그인 성공/실패 결과 |
| S2C_AvatarInfo | Server -> Client | 본인 아바타 초기 정보 전달 |
| S2C_AddObject | Server -> Client | 플레이어, NPC 및 아이템 박스 생성 |
| S2C_RemoveObject | Server -> Client | 플레이어, NPC 및 아이템 박스 제거 |
| S2C_MoveObject | Server -> Client | 플레이어/npc 이동 결과(위치) 브로드캐스트 |
| S2C_Rotate | Server -> Client | 회전 갱신 |
| S2C_Damage | Server -> Client | 데미지 결과와 대상의 최대/현재 HP 전달 |
| S2C_OpenItemBoxResult | Server -> Client | 수령 결과, 지급 아이템·수량 및 해당 아이템 총 보유량 전달 |
| C2S_StartPurification | Client -> Server | 발판에서 지속 정화 시작 요청 |
| C2S_RequestPuriSyringe | Client -> Server | 정화 주사기 1개 사용 요청 |
| S2C_PuriSyringResult | Server -> Client | 서버가 확정한 주사기 총 보유량 전달 |

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

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | C2S_PLAYER_STATE |
| x | float | X 위치 |
| y | float | Y 위치 |
| z | float | Z 위치 |
| yaw | float | 회전 값 |
| isGrounded | bool | 클라이언트 접지 상태. 바닥 접촉 시 true, 공중에서는 false |

기존 isJumping 필드를 isGrounded로 변경했습니다. bool의 위치·크기는 동일하지만 의미가 달라지므로 양쪽은 같은 프로토콜 버전을 사용해야 합니다.

### C2S_Shoot

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 21바이트 (4바이트 enum/int/float, 1바이트 정렬 기준) |
| type | PACKET_TYPE | C2S_SHOOT (7) |
| playerId | int | 발사 요청 플레이어 ID |
| direction | Vec3 | 월드 좌표계의 정규화된 발사 방향 (단위 없음) |

`Vec3`는 `float x, y, z` 순서의 12바이트 데이터 구조체이며 연산 함수는 포함하지 않습니다. 클라이언트는 계산하고 정규화한 방향을 `direction`에 `Vec3` 형식으로 채워야 합니다.

### C2S_OpenItemBox

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 9바이트 (enum/int 4바이트, 1바이트 정렬 기준) |
| type | PACKET_TYPE | C2S_OPEN_ITEM_BOX (8) |
| objectId | int | 수령할 아이템 박스 ID |

요청 플레이어는 접속 세션으로 식별합니다.

### S2C_LoginResult

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_LOGIN_RESULT |
| success | bool | 로그인 성공 여부 |

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
| mutation | float | 서버 변이 수치. 현재 0~100 |
| moveSpeed | float | 기본 이동속도에 곱할 무단위 배율 |

### S2C_AddObject

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_ADD_OBJECT |
| objectId | int | 오브젝트 ID |
| objectType | Kimgane::Shared::World::ObjectType | Player / Npc / ItemBox 구분 (1바이트) |
| username | char[MAX_NAME_LEN] | 이름 |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |
| maxHp | int | 해당 객체의 최대 체력 |
| currentHp | int | 전송 시점의 해당 객체 현재 체력 |
| mutation | float | 서버 변이 수치. 현재 0~100 |
| moveSpeed | float | 기본 이동속도에 곱할 무단위 배율 |

플레이어 이외의 오브젝트는 `mutation=0`, `moveSpeed=1`을 전달합니다.
`maxHp=0`은 체력을 사용하지 않는 오브젝트를 나타냅니다.

### S2C_RemoveObject

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_REMOVE_OBJECT |
| objectId | int | 제거 대상 ID |

### S2C_MoveObject

위치·회전·변이·속도 배율을 전달하며 HP 필드는 포함하지 않습니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 패킷 크기 |
| type | PACKET_TYPE | S2C_MOVE_OBJECT |
| objectId | int | 오브젝트 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
| yaw | float | 바라보는 각도 |
| mutation | float | 서버 변이 수치. 현재 0~100 |
| moveSpeed | float | 기본 이동속도에 곱할 무단위 배율 |

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

### S2C_OpenItemBoxResult

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 24바이트 (enum/int 4바이트, 1바이트 정렬 기준) |
| type | PACKET_TYPE | S2C_OPEN_ITEM_BOX_RESULT (16) |
| objectId | int | 요청한 상자 ID |
| status | OpenItemBoxStatus (`std::uint8_t`) | 수령 결과 |
| itemId | Kimgane::Shared::Items::ItemId | 지급 아이템. 실패 시 None |
| quantity | std::uint32_t | 이번 지급량. 실패 시 0 |
| totalQuantity | std::uint64_t | 지급 후 해당 아이템의 서버 총 보유량. 실패 시 0 |

| status 값 | 의미 |
| --- | --- |
| Success=0 | 보상 지급 및 상자 제거 완료 |
| NotFound=1 | 상자가 없음. 다른 플레이어의 선점·중복 요청 포함 |
| NotItemBox=2 | 대상이 아이템 박스가 아님 |
| TooFar=3 | 서버의 상호작용 거리 조건을 충족하지 않음 |
| RewardFailed=4 | 보상 선택 또는 인벤토리 추가 실패 |

`quantity`는 이번 증가량, `totalQuantity`는 해당 아이템의 누적 보유량입니다. 총 보유량은 증가량처럼 다시 더하지 않습니다. 실패 패킷의 0은 인벤토리를 초기화하라는 뜻이 아닙니다.

### C2S_StartPurification

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 5바이트 |
| type | PACKET_TYPE | C2S_START_PURIFICATION (17) |

추가 payload는 없습니다. 요청 플레이어는 접속 세션으로 식별합니다.
정화 시작 결과 및 종료 요청/응답 패킷은 정의되어 있지 않습니다.

### C2S_RequestPuriSyringe

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 5바이트 |
| type | PACKET_TYPE | C2S_REQUEST_PURI_SYRINGE (18) |

추가 payload는 없습니다. 요청 플레이어는 접속 세션으로 식별합니다.
처리 후 수량은 `S2C_PuriSyringResult`로 전달합니다.

### S2C_PuriSyringResult

구조체 이름의 `Syring` 철자는 현재 코드와 동일합니다.

| Field | Type | Description |
| --- | --- | --- |
| size | unsigned char | 13바이트 |
| type | PACKET_TYPE | S2C_PURI_SYRING_RESULT (19) |
| quantity | std::uint64_t | 처리 후 정화 주사기 총 보유량 |

서버는 로그인 처리 중 아바타 정보 직후, 그리고 주사기 사용 요청 처리 후 요청자에게 전송합니다.
성공 여부나 실패 사유 필드는 없습니다. 사용에 실패해도 현재 수량을 보냅니다.
`quantity`는 증가·감소량이 아닌 총 보유량이며 0은 보유 수량이 없음을 뜻합니다.
변이·속도 결과는 `S2C_MOVE_OBJECT`의 기존 필드로 전달됩니다.

## 호환성과 크기 제한

필드 추가로 패킷 레이아웃이 달라졌으므로 서버와 클라이언트는 같은 `Protocol.h`로 빌드해야 합니다.
위 크기는 PACKET_TYPE이 4바이트인 Windows 빌드와 1바이트 패킹 기준입니다.
수신 버퍼 한도는 200바이트이며 프로토콜 버전 협상은 없습니다.

## Client Network API

`NetworkManager` (`Kimgane::Client`)가 제공하는 클라이언트 측 API는 network-manager.md를 참조하세요.

## Change Log

| 날짜 | 변경 내용 | 영향 범위 | 담당 |
| --- | --- | --- | --- |
| 2026-11-20 | 변이·속도 필드와 정화·주사기 패킷 명세 반영. 상세 처리 흐름은 network-manager.md로 이동 | Shared/Client/Server 정적 대조 | 서버 담당 |
| 2026-10-12 | 아이템 박스 생성·제거 및 수령 요청/결과, 인벤토리 저장 계약, 패킷 번호·objectType 반영 | Shared/Client/Server 문서 | 서버 담당 |
| 2026-09-11 | S2C_AddObject에 maxHp/currentHp 추가 (전송부 연결은 후속 작업), 발사/데미지 송수신과 NPC 체력 계약 및 미구현 범위 갱신 | Shared/Client/Server 문서 | 김채림 |
| 2026-09-04 | Protocol.h 기준으로 문서 갱신, Object 기반 패킷 구조 반영, network-manager.md 파일과 분리 | 김채림 |
| 2026-07-21 | `protocol.h` / `NetworkManager.cpp` 기준으로 패킷 스펙 및 클라이언트 API 정리 | NetworkManager/ protocol.h | 김채림 |
| 2026-07-06 | TCP/IP 환경, TCP MVP, 기본 헤더 초안, 서버 권위 위치 기준 작성 | Client/Server 초기 구현 | 김채림 |
