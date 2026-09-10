# Network Manager

클라이언트 `Kimgane::Engine::NetworkManager`의 현재 구현과 게임 연동 방식을 기록하는 문서입니다.
프로토콜이 변경되면 송수신 처리, 공개 API, 미구현 항목을 함께 갱신합니다.
패킷 필드의 상세 명세는 [Network Protocol](network-protocol.md)에서 관리합니다.

- 문서 상태: 초안
- 문서 담당자: 김채림
- 마지막 코드 대조일: 2026-09-11
- 기준 코드: [NetworkManager.h](../../Client/src/Engine/Network/NetworkManager.h), [NetworkManager.cpp](../../Client/src/Engine/Network/NetworkManager.cpp), [Shared/Protocol.h](../../Shared/Protocol.h)

## 1. 역할과 현재 상태

`NetworkManager`는 서버 연결, 입력 및 상태 패킷 전송, TCP 수신 데이터 조립을 담당합니다.
수신한 위치·회전과 제거 이벤트는 큐에 저장하고, 게임에서 꺼내 적용합니다.
실제 클라이언트의 게임 오브젝트 생성/제거, 로컬 플레이어 보정, NPC 위치 보간은 Scene 및 Gameplay 영역에서 처리합니다.

| 구분 | 현재 구현 |
| --- | --- |
| 연결 | WinSock TCP, `127.0.0.1:3500` (`PORT`) |
| 초기화 | 동기 `connect()` 성공 후 논블로킹 소켓으로 전환, 로그인 요청 전송 |
| 수신 | 프레임마다 `Update()`에서 `recv()` 1회 호출 |
| 패킷 조립 | 첫 바이트의 `size` 기준으로 분할 수신 및 여러 패킷이 합쳐진 수신 처리 |
| 송신 | 각 API에서 구조체를 구성해 `send()` 직접 호출 |
| 상태 동기화 | 온라인 게임에서 0.2초마다 로컬 상태 전송 |
| 오브젝트 관리 | `mObjects[MAX_OBJECTS]`에 활성 여부, 위치, yaw, 최대/현재 HP 저장. 현재 최대 60개 슬롯 |
| 연결 종료 | 서버 종료 또는 수신 오류 시 `Shutdown()` 호출. 자동 재접속 없음 |

`ClientNetworkFacade`는 별도로 남아 있는 mock 기반 placeholder입니다.
현재 게임 연결 흐름은 `NetworkManager`를 사용하며, 두 클래스는 같은 구현이 아닙니다.

## 2. 게임 연동 흐름

관련 코드: [ClientApplication.cpp](../../Client/src/Engine/Application/ClientApplication.cpp),
[PlayerControllerComponent.cpp](../../Client/src/Engine/Gameplay/PlayerControllerComponent.cpp),
[Scene.cpp](../../Client/src/Engine/Scene/Scene.cpp)

1. `Initialize()`에서 서버에 연결하고 `C2S_LOGIN`을 보냅니다. 현재 `username`은 빈 값입니다.
2. `S2C_LOGIN_RESULT` 실패 시 연결을 종료합니다. 성공 시 결과 메시지를 출력합니다.
3. `S2C_AVATAR_INFO`를 받으면 본인 ID를 저장하고 초기 위치를 수신 큐에 넣습니다.
4. 입력에 따라 이동 시작 정지, 회전, 점프 패킷을 보냅니다.
5. 매 프레임 Scene 갱신 → 로컬 상태 주기 송신 → 네트워크 수신 → 수신 큐의 게임 반영 순서로 처리합니다.
6. `ApplyNetworkPlayerLocations()`에서 위치 큐를 모두 소비한 뒤 제거 큐를 모두 소비합니다.
7. Scene은 본인 ID의 위치를 보정에 사용하고, 다른 오브젝트를 생성하거나 갱신합니다. 기존 NPC 위치 갱신에는 `NetworkSmoothingComponent`를 사용합니다.

`IsConnected()`는 소켓 연결 상태를 반환합니다. 로그인 완료 또는 본인 아바타 준비 완료를 나타내는 별도 상태는 없습니다.

## 3. 공개 API

### 연결 및 갱신

| API | 역할 및 호출 시 주의점 |
| --- | --- |
| `Initialize()` | 연결 및 로그인 요청. 소켓 생성/연결 실패 시 `false` 반환 |
| `Shutdown()` | 소켓 종료, 연결 플래그 해제, `WSACleanup()` 호출 |
| `Update(float deltaTime)` | 수신 데이터 처리. 현재 `deltaTime`은 사용하지 않음 |
| `IsConnected()` | 연결 플래그와 유효한 소켓 여부 확인 |
| `ConsumePlayerStateSyncTick(float deltaTimeSec)` | 연결 중 누적 시간이 0.2초 이상이면 타이머를 0으로 초기화하고 `true` 반환 |
| `GetMyPlayerId()` | 본인 ID 반환. 아바타 정보 수신 전 초기값은 `-1` |
| `GetPlayerStateSyncTimer()` | 상태 송신용 누적 시간 조회 |

동기화 타이머는 초과 시간을 다음 주기로 이월하지 않으며, 호출 한 번에 최대 한 번의 송신 시점만 반환합니다.
`Update()` 자체가 상태 패킷을 보내지는 않습니다. Application에서 타이머를 확인한 뒤 `SendPlayerState()`를 호출합니다.

### 송신

| API | 패킷 타입 | 현재 전달 정보 |
| --- | --- | --- |
| `SendMoveStart(int direction, float yaw)` | `C2S_MOVE_START` | 방향, yaw |
| `SendMoveStop(int direction)` | `C2S_MOVE_STOP` | 방향. 구조체 초기화로 yaw는 0 |
| `SendRotate(float yaw)` | `C2S_ROTATE` | 본인 `playerId`, yaw |
| `SendJump()` | `C2S_JUMP` | 점프 요청 |
| `SendShoot(const DirectX::XMFLOAT3& direction)` | `C2S_SHOOT` | 본인 플레이어 ID와 전달받은 발사 방향 전송. 정규화 계산 및 입력 연결은 호출부 담당 |
| `SendPlayerState(const DirectX::XMFLOAT3& pos, float yaw, bool isJumping)` | `C2S_PLAYER_STATE` | 위치, yaw, 점프 여부 |
| `SendMoveInput(int direction)` | 미연결 | 헤더 선언만 있으며 현재 구현 정의 없음, 현재의 SendMoveStart와 SendMoveStop를 이쪽으로 통합 예정 |

구현된 공개 송신 함수는 연결되지 않았으면 즉시 반환합니다.
이동 시작/정지는 `C2S_Move` 구조체를 공유합니다. `C2S_MOVE` enum은 남아 있지만 현재 매니저에서 송신하지 않습니다.
현재 Application은 `SendPlayerState()`의 `isJumping`에 항상 `false`를 전달합니다.
클라이언트 호출부의 위치는 미터(`PositionM`), yaw는 라디안(`rotationRad.y`) 기준입니다.

### 수신 결과 조회

| API | 동작 |
| --- | --- |
| `GetPlayerLocation(int* id, float* x, float* y, float* z, float* yaw)` | 위치 큐에서 가장 오래된 항목 하나를 꺼내 출력. 비어 있으면 `false` |
| `GetRemovedPlayer(int* playerId)` | 제거 큐에서 ID 하나를 꺼내 출력. 비어 있으면 `false` |

두 함수는 큐를 소비하는 API이므로, 최신 상태를 반복 조회하는 getter가 아닙니다.
출력 포인터의 null 검사는 없으므로 유효한 포인터를 전달해야 합니다.
API와 `LocationUpdate.playerId`에는 Player 명칭이 남아 있지만 실제로는 NPC를 포함한 오브젝트 ID도 전달합니다.

## 4. 수신 패킷 처리 현황

| 패킷 타입 | 처리 내용 |
| --- | --- |
| `S2C_LOGIN_RESULT` | 성공 로그 출력 또는 실패 시 연결 종료 |
| `S2C_AVATAR_INFO` | 본인 ID 설정, 활성 상태 및 위치·yaw 저장, 위치 큐 추가 |
| `S2C_ADD_OBJECT` | 대상 활성화, 위치/yaw 및 최대/현재 HP 저장, 위치 큐 추가. `username`은 현재 저장하지 않음 |
| `S2C_MOVE_OBJECT` | 위치/yaw 갱신, 위치 큐 추가. 저장된 HP는 유지 |
| `S2C_DAMAGE` | targetId의 최대·현재 HP를 수신값으로 덮어쓰기, 데미지 수신 로그 출력 |
| `S2C_REMOVE_OBJECT` | 대상 비활성화, 제거 큐 추가 |
| `S2C_ROTATE` | yaw 갱신, 캐시된 위치와 새 yaw를 위치 큐에 추가 |
| 그 외 | Unknown Packet 로그 출력 |

`S2C_AVATAR_INFO`는 `playerId`를 사용하고, 생성/이동/제거/회전 패킷은 `objectId`, 데미지 패킷은 `targetId`와 `attackerId`를 사용합니다.
위치와 제거가 서로 다른 큐이므로 두 종류 이벤트 사이의 원래 수신 순서는 게임 반영 시 유지되지 않습니다.

## 5. 프로토콜과 수신 버퍼

- 직렬화는 `#pragma pack(push, 1)`이 적용된 구조체의 메모리를 직접 전송하는 방식입니다.
- 첫 필드 `size`는 `unsigned char`이며 전체 패킷 크기를 바이트로 담습니다.
- `type`은 `PACKET_TYPE` enum입니다. 패킹은 enum 자체를 1바이트로 만들지 않으므로 빌드 환경의 타입 크기를 함께 확인해야 합니다.
- `BUF_SIZE`는 200바이트이며 임시 수신 버퍼와 패킷 조립 버퍼에 사용됩니다.
- `ProcessData()`는 조립 중인 패킷의 남은 길이와 복사 위치를 유지하고, 완성되면 `ProcessPacket()`에 전달합니다.

패킷 확장 시 `size`의 표현 범위뿐 아니라 200바이트 조립 버퍼도 확인해야 합니다.

## 6. 프로토콜 변경 시 갱신 절차

1. 변경 목적, 방향, 필드 타입·단위와 호환성 영향을 `network-protocol.md`에 정리합니다.
2. `Shared/Protocol.h`와 Client/Server 송수신 구현을 함께 대조합니다.
3. 이 문서의 API 표, 수신 처리 표, 게임 연동 흐름을 갱신합니다.
4. 해당 변경의 빌드, 실행 결과를 기록하고 마지막 코드 대조일 및 변경 이력을 갱신합니다.

## 7. 변경 이력

| 날짜 | 변경 내용 | 영향 범위 | 담당 |
| --- | --- | --- | --- |
| 2026-07-09 | 서버 구현 전 클라 `ClientNetworkFacade` placeholder와 `get_player_location(...)` API 추가 | Client 네트워크 접점 | 김영목 |
| 2026-09-07 | 현재 코드 기준 연결·송수신 API, 게임 연동, 갱신 절차 등 정의 | NetworkManager 문서 | 김채림 |
| 2026-09-11 | 발사 송신·데미지 수신·HP 저장, 게임 연결 미구현 범위와 실제 디버깅 코드 상태 반영 | NetworkManager 문서 | 김채림 |
