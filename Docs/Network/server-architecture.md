# 김가네 서버 아키텍처 — 클라이언트 개발자 공유 초안

> 기준: 2026-11-27 작업 트리의 소스 코드
> 목적: 서버 전체 구조를 이해하고, 협업 대상으로 허용한 **터레인 로직과 캐릭터 이동 로직**의 작업 경계를 파악한다

## 목차

1. [협업 범위](#1-협업-범위)
2. [전체 구조](#2-전체-구조)
3. [실행 흐름과 상태 소유권](#3-실행-흐름과-상태-소유권)
4. [터레인 로직](#4-터레인-로직)
5. [캐릭터 이동 로직](#5-캐릭터-이동-로직)
6. [클라이언트 연동 시 확인할 사항](#6-클라이언트-연동-시-확인할-사항)
7. [변이·정화기·정화 주사기](#7-변이정화기정화-주사기)

## 1. 협업 범위

| 구분 | 대상 | 작업 경계 |
| --- | --- | --- |
| 클라이언트 직접 수정 가능 | `Server/KimganeServer/src/Terrain/ServerTerrainCalculation.h/.cpp` | 지형 로드·조회, 월드 좌표 변환, 이동 계산 연결, 지형에 의한 사격 차단 판정 |
| 클라이언트 직접 수정 가능 | `Server/KimganeServer/src/Terrain/TerrainHeightMap.h/.cpp` | 서버 높이맵 로드·보간·범위 검사 |
| 클라이언트 직접 수정 가능 | `Shared/Physics/CharacterMovement.h/.cpp` | 캐릭터 수평 이동, 바닥 선택, 수직 이동·착지 계산 |
| 참조용 — 직접 수정 범위 밖 | `Core/Server.h/.cpp`, `Core/Session.h/.cpp`, `Network/PacketHandler.h/.cpp` | 호출 위치·입력·출력 확인용. 시그니처 변경에 따른 호출부 수정은 서버 담당자와 조율 |
| 참조용 — 직접 수정 범위 밖 | `Npc/`, `Shared/Protocol.h`, 공통 충돌·레이캐스트 구현, 프로젝트 설정 |

허용 범위는 파일 이름만으로 확인하지 않음. 예를 들어 `BlocksShot()`의 지형 판정을 수정할 수 있어도 NPC 선택, 데미지, HP, 사망 처리를 변경하는 작업까지 포함하지 않는다.

### 현재 코드 경로와 협업 범위 해석

위 표는 기존 문서의 협업 범위를 보존한 내용이다.
기존 서버 전용 Terrain 경로는 현재 실행 경로에서 Shared 지형·물리 구현으로 대체되었다.
현재 사용하는 파일은 아래와 같다.

| 기능 | 현재 코드 |
| --- | --- |
| 높이맵 로드·월드 높이 조회 | `Shared/Terrain/HeightMapData.h/.cpp`, `HeightMapSampler.h` |
| 캐릭터 이동·접촉 보정 | `Shared/Physics/CharacterMovement.h/.cpp`, `CharacterMovementWorld.h` |
| 공통 맵 배치·충돌 등록 | `Shared/World/TestMapSettings.h`, `TestMapCollision.h/.cpp` |

이 경로 표는 현재 구현을 찾기 위한 안내다. Shared로 이동하거나 새로 추가된 파일의 직접 수정 범위는 서버 담당자와 조율한다.

## 2. 전체 구조

| 모듈 | 역할 |
| --- | --- |
| `Core/Server` | 서버 초기화, 접속·수신·송신 완료 처리, 월드 갱신, 사격·오브젝트 제거 조정 |
| `Core/Session` | 소켓과 비동기 I/O 정보, 수신 버퍼 및 송신 패킷 구성. 플레이어 상태는 GameWorld의 Player가 소유 |
| `Network/PacketHandler` | 패킷 종류별 처리. 이동 입력을 Player에 저장하고 사격 요청은 Server에 전달 |
| `Shared/World/TestMapCollision` | 지형·집·정화기 충돌 등록과 TerrainSampler 연결 |
| `Shared/Terrain/HeightMapData`, `HeightMapSampler` | RAW 높이맵 로드·보간과 월드 높이 조회 |
| `Npc/Npc`, `Npc/NpcSetting` | NPC 상태, 생성, 주기적 이동 및 관련 전송 |
| `Shared/` | Shared 파일에 있는 것들은 Shared filter 내부에 있게끔 한다|

### 아이템 박스·인벤토리 관련 파일

서버 파일 경로는 `Server/KimganeServer/src/` 기준이다. 패킷 필드와 수령 결과는 [network-protocol.md](network-protocol.md)를 참조한다.

| 파일 | 역할 |
| --- | --- |
| `World/GameObject.h` | ID, 종류 및 위치·회전 조회를 제공하는 월드 오브젝트 공통 인터페이스 |
| `World/GameWorld.h/.cpp` | 플레이어·NPC·상자와 맵 데이터 소유. OBJ·충돌 정보 로딩, 오브젝트 생성·조회·제거 및 충돌 월드 등록 |
| `World/Player.h` | 플레이어 이동 상태·변이·정화 상태와 Inventory 소유 |
| `World/ItemBox.h` | 상자 위치와 월드 충돌 박스 보관. 파일의 로컬 중심에 상자 위치를 더해 적용 |
| `Items/Inventory.h` | ItemId별 uint64 보유량을 메모리에 누적·조회·1개 소모. 유효하지 않은 아이템과 0개 지급 거부 |
| `Items/LootTable.h` | 보상 후보·가중치·수량 범위 및 보상 선택 함수 |
| `Config/ServerConfig.h` | 서버 상자 모델·충돌 경로, 수령 거리·배치 설정 및 변이·정화·속도 배율 설정 |
| `Shared/World/ObjectTypes.h` | Player, Npc, ItemBox 종류 식별자 |
| `Shared/Items/ItemTypes.h` | PuriSyringe를 포함한 공통 아이템 식별자와 ItemReward 데이터 |
| `Shared/Geometry/ObjLoader.h/.cpp`, `ObjGeometry.h` | 집·상자 몸체·뚜껑 OBJ를 순수 지오메트리 데이터로 로딩·보관. GPU 생성이나 메시 충돌 계산은 수행하지 않음 |
| `Shared/Geometry/CollisionBoxLoader.h/.cpp` | 공통 충돌 파일의 로컬 박스 정보 로딩 |
| `Shared/Geometry/S07_Supply_Crate_collision.txt` | 닫힌 상자 몸체·뚜껑을 감싸는 단일 박스의 중심·크기 정의 |

`Core/Server`는 수령 검증·보상 지급·제거를 조정하고, `Network/PacketHandler`는 요청을 전달하며, `Core/Session`은 결과 패킷을 구성한다. `CrateDefinition.h`는 제거했으며 충돌체 규격은 Geometry의 충돌 파일을 사용한다. 인벤토리는 Player 수명 동안만 유지되며 영속 저장은 없다.

기존 서버 전용 Terrain 파일은 현재 실행 경로에서 Shared 지형·물리 구현으로 대체되었다. 해당 함수의 현재 연결은 4절과 5절에서 설명한다.

## 3. 실행 흐름과 상태 소유권

### 초기화

`Server::Initialize()`는 GameWorld 맵 로드 → 지형·집·정화기 충돌 등록 → NPC 100개 초기화 → 상자 최대 300개 배치 → 네트워크 준비와 타이머 시작을 수행한다. 지형 로드 예외는 Server에서 처리하고 초기화를 실패시킨다.

### 실행 중

- `Server::Run()`은 IOCP(비동기 입출력 완료 통지)에서 이벤트를 받아 접속·수신·송신 완료를 처리한다.
- `HandleRecv()`는 TCP 수신 바이트를 패킷 길이에 따라 나누고, 완성된 패킷을 `PacketHandler`에 전달한다.
- 별도 `TimerThread()`는 `Sleep(50)` 후 플레이어 이동과 NPC 갱신을 수행한다. 계산에는 고정 `0.05초`를 전달하며 실제 경과 시간을 측정하지 않는다.
- `Run()`의 이벤트 처리와 타이머 갱신은 `mWorldMutex`를 사용한다. 분리된 계산 함수는 자체 잠금을 잡지 않으며, 호출자가 상태 접근을 보호한다.

| 상태 | 소유·관리 위치 |
| --- | --- |
| 플레이어 세션 | `Server::mClients`의 `unique_ptr<Session>` |
| 플레이어·NPC·아이템 박스 | `Server::mWorld`의 `GameWorld::mObjects` |
| 플레이어 이동·변이·정화 상태 / 인벤토리 | 각 `Player` |
| 높이맵 | `GameWorld::mTerrain` |
| 충돌 월드 / 집 충돌 박스 | `GameWorld::mMapCollision` |
| 집·상자 몸체·뚜껑 OBJ 데이터 | `GameWorld`의 지오메트리 멤버 |
| 상자 로컬 충돌 규격 / 배치 후 충돌 박스 | `GameWorld::mItemBoxLocalCollision` / 각 `ItemBox` |

계산 함수에 전달되는 높이맵·충돌 월드·박스 목록은 참조 또는 `span`으로 읽는다. 해당 함수가 데이터를 소유하거나 네트워크 전송을 수행하지 않는다.

### 접속 식별과 로그인

Session은 `mClients[matchSlot][playerSlot]`에 저장하고 IOCP 완료 키에는 세대를 포함한 연결 ID를 사용한다.
`FindSessionByConnection()`은 슬롯을 복원한 뒤 현재 Session의 연결 ID와 비교한다.
현재 Player 조회에는 `Session::GetId()`를 사용하며 Player ID와 슬롯은 같은 값을 쓴다.
NPC·상자는 50부터 충돌 월드의 기존 ID를 건너뛰며 할당한다.

접속 시 Player를 생성하고 `ResolveCharacterContactsInWorld()`로 초기 위치·접지를 보정한다.
현재 `GameWorld::CreatePlayer()`에는 시연용 정화 주사기 1개 지급 코드가 활성화되어 있다.
로그인 시 본인 아바타 정보와 주사기 수량을 보내고 NPC·상자 및 다른 플레이어 정보를 교환한다.
재접속하면 새 Player가 생성되어 시연용 주사기를 다시 지급한다.

## 4. 터레인 로직

### 4.1 실제 사용 파일

서버는 `Shared/Terrain/HeightMapData.h/.cpp`와 `HeightMapSampler.h`를 사용한다.
`HeightMapData`가 RAW 데이터와 보간을 맡고 `HeightMapSampler`가 물리 쿼리에 월드 높이와 normal을 제공한다.

서버 로드 경로는 다음과 같다.

`Server::Initialize()` → `GameWorld::LoadMap()` → `LoadTestMapTerrain()` → `HeightMapData::LoadRaw16()`

`TestMapCollision::Load()`는 집·정화기 충돌 파일을 읽어 지형, 월드 박스, 정화기 경사로를 등록한다.
배치 좌표와 파일 경로는 `Shared/World/TestMapSettings.h`에서 공유한다.

### 4.2 좌표와 높이 조회

- 캐릭터 월드 좌표는 발 위치 기준이며 단위는 m다.
- 높이맵 조회 좌표는 지형 모서리에서 시작하는 양수 좌표다.
- `GetWorldWidthM()`는 `(샘플 수 - 1) × 셀 간격`이며 길이도 같은 방식이다.
- `HeightMapSampler`는 지형의 월드 위치를 반영해 조회 좌표와 높이를 변환한다.

```cpp
sampleX = worldX - terrainPosition.x + terrain.GetWorldWidthM() * 0.5f;
sampleZ = worldZ - terrainPosition.z + terrain.GetWorldLengthM() * 0.5f;
height  = terrain.SampleHeightM(sampleX, sampleZ) + terrainPosition.y;
```

### 4.3 지형·이동 연결 함수 계약

기존 `ServerTerrainCalculation`의 함수 역할은 현재 다음 Shared 경로로 연결된다.

| 기존 함수 | 현재 코드 | 입력과 결과 |
| --- | --- | --- |
| `LoadTerrain()` | `LoadTestMapTerrain()` | TestMapSettings의 RAW 설정으로 HeightMapData 공유 포인터 생성. 실패는 예외로 전달 |
| `BuildGroundBoxes()` | `TestMapCollision::Build()` | 집·정화기 로컬 박스에 각 원점을 더해 CollisionWorld에 등록 |
| `UpdateHorizontal()` | `StepCharacterMovementInWorld()` → `StepCharacterMovement()` | Player RigidbodyState, 입력, dt를 받아 수평 속도 설정·적분·접촉 보정 |
| `UpdateVertical()` | `RigidbodyIntegrator::Integrate()`와 `ResolveCharacterContacts()` | 중력·수직 속도·위치 갱신 후 충돌과 접지 보정 |
| `BlocksShot()` | `Server::HandleShoot()`의 `RaycastTerrain()` 호출 | 대상까지 길이가 제한된 ray로 지형 차단 판정 |

집의 월드 Y 오프셋은 TestMapSettings의 4.71m를 사용한다.
집 박스와 지형은 이동·사격 쿼리에 같은 월드 변환으로 제공된다.

### 4.4 지형에 의한 사격 차단

Server가 가장 가까운 유효 NPC hitbox를 선택하고 ray의 최대 거리를 대상까지로 제한한다.

1. 집 월드 박스와 ray의 교차를 검사한다.
2. 공통 TerrainSampler가 발사점의 월드 높이를 조회한다.
3. 발사점이 지형 범위 밖이거나 지형 이하이면 차단한다.
4. `RaycastTerrain()`으로 대상까지 지형과 교차하는지 검사한다.
5. 차단되지 않았으면 데미지·HP·제거 처리를 수행한다.

NPC도 같은 충돌 월드를 사용한다.
`NpcSetting::Update()` → `Npc::UpdateMovement()` → Shared 이동 계산으로 연결되므로 지형·접촉 규칙 변경은 NPC에도 영향을 준다.

## 5. 캐릭터 이동 로직

### 5.1 입력에서 결과 전송까지

현재 순서는 변이 갱신 → 이동 입력과 속도 배율 구성 → 이동·적분·접촉 보정 → 정화 종료 판정 → 이동 패킷 송신이다.
해당 틱의 이동 패킷에는 수직 이동과 접촉 보정까지 끝난 위치가 담긴다.
실제 실행 경로는 `Server::TimerThread()` → `StepCharacterMovementInWorld()` → `StepCharacterMovement()`다.

### 5.2 상태와 입력의 의미

| 값 | 의미 |
| --- | --- |
| `Player::mMovementState.positionM` | 발 기준 월드 위치, m |
| `RigidbodyState::velocityMps` | 수평·수직 속도, m/s |
| `RigidbodyState::isGrounded` | 공통 접촉 보정으로 결정하는 서버 접지 상태 |
| `CharacterMotionInput::direction` | 월드 XZ 이동 방향 |
| `Player::mMoveYaw` | 입력에서 계산한 이동 방향, rad |
| 네 방향 bool | 서로 반대인 입력을 상쇄해 이동 여부를 판단하는 게이트 |
| `Player::mYaw` | 캐릭터 정면/전송용 방향, rad |
| `mJumpRequested` | 다음 타이머 틱에서 소비할 점프 요청 |

`HandleMoveStart/Stop()`은 방향 bool과 mMoveYaw를 갱신한다.
타이머는 이동 입력이 있으면 `sin(mMoveYaw), 0, cos(mMoveYaw)`를 이동 방향으로 설정한다.
`HandleJump()`는 점프 요청만 기록하고, 다음 틱의 Shared 이동 계산이 접지 여부로 실제 점프를 허용한다.

### 5.3 수평 이동과 바닥 선택

`CharacterMovement.cpp`의 `StepCharacterMovement()`는 다음 순서로 처리한다.

1. 입력 방향에 속도를 곱해 수평 velocity를 설정한다.
2. 점프 요청과 접지 조건이 맞으면 수직 속도 변화를 적용한다.
3. RigidbodyIntegrator로 상태를 적분한다.
4. 후보 위치에서 접촉을 조회하고 위치와 속도를 보정한다.

`CharacterMovementWorld.h`의 쿼리는 발 위치에서 캡슐을 만들고 CollisionWorld의 지형·박스·경사로 접촉을 조회한다.
`ResolveCharacterContacts()`는 접촉을 반복 조회하고 보행 가능한 지면 또는 이동을 막는 접촉을 처리한다.
위치 침투를 보정하고 SlideMovement로 충돌면 방향의 속도를 조정하며 보행 가능한 지면 접촉으로 접지를 결정한다.

### 5.4 수직 이동과 착지

점프는 접지 중에 요청을 받았을 때 VelocityChange로 적용한다.
이후 RigidbodyIntegrator가 중력과 속도를 적분하고, 접촉 보정이 지형·박스·경사로와의 위치 및 속도를 조정한다.
기존의 고정 바닥 높이에 y를 맞추는 분리된 UpdateVertical 경로는 Shared Rigidbody와 접촉 보정으로 대체되었다.

| 설정 | 현재 사용 값 / 위치 |
| --- | --- |
| 이동 dt | 0.05초, ServerConfig의 TIMER_INTERVAL_MS=50으로 계산 |
| 기본 수평 속도 | 5m/s, PhysicsSettings의 PLAYER_MOVE_SPEED_MPS |
| 변이 속도 배율 | 1.0~3.0, ServerConfig의 PlayerMutationSettings |
| 중력 | -20m/s², PhysicsSettings의 GRAVITY_Y_MPS2 |
| 점프 초기 속도 | 8m/s, PhysicsSettings의 PLAYER_JUMP_VELOCITY_MPS |
| 캡슐 크기 | PhysicsSettings의 플레이어 캡슐 설정 |

## 6. 클라이언트 연동 시 확인할 사항

- 공통 맵 배치·캡슐 설정과 이동·접촉 보정 순서를 비교한다.
- 계산 함수의 변경과 패킷 전송 순서 변경을 구분한다.
- `C2S_PLAYER_STATE`는 현재 위치 차이를 계산할 뿐 서버 위치를 덮어쓰거나 보정을 구현하지 않는다.
- `S2C_MOVE_OBJECT`는 objectId, 위치, yaw, 변이 수치와 이동속도 배율을 전달한다. 현재 구조에 입력 확인 번호나 입력 재실행 절차가 있다고 가정하지 않는다.

## 7. 변이·정화기·정화 주사기

### 변이와 속도 배율

`Player::UpdateMutation()`는 변이를 0~100으로 제한한다.
평상시 초당 1 증가하고, 정화 중에는 증가 대신 초당 5 감소한다.
`GetMoveSpeedMultiplier()`는 현재 변이를 5구간으로 나누어 서버 이동과 패킷 전송에 같은 배율을 제공한다.

| 변이 범위 | 배율 | 기본 5m/s 적용 |
| --- | --- | --- |
| 0 이상 20 미만 | 1.0 | 5m/s |
| 20 이상 40 미만 | 1.4 | 7m/s |
| 40 이상 60 미만 | 1.8 | 9m/s |
| 60 이상 80 미만 | 2.2 | 11m/s |
| 80 이상 100 미만 | 2.6 | 13m/s |
| 100 | 3.0 | 15m/s |

밸런스 값은 `ServerConfig.h`의 `PlayerMutationSettings`에서 조정한다.
`moveSpeed` 패킷 필드는 위 배율을 전달한다.

### 정화기

정화기는 Shared 맵의 고정 배치이며 오브젝트 생성 패킷으로 배치하지 않는다.
원점은 (-16, 4.40, 4)m이고 발 위치 판정 중심은 원점에 (0, 0.642, 0.035)m를 더한다.
XZ 반경 0.73m, Y 오차 0.14m 이내이며 서버 접지가 true여야 한다.
좌표·충돌·판정 범위는 `Shared/World/TestMapSettings.h`에서 공유한다.

F 입력으로 `C2S_START_PURIFICATION`을 받으면 `Server::StartPurification()`이 조건을 확인하고 `mIsPurifying`을 켠다.
한 번 시작하면 범위 안에 머무는 동안 계속 정화하고, 0에서는 0을 유지한다.
서버가 이동 후 범위 이탈·접지 해제를 확인하면 종료하므로 돌아온 뒤 다시 요청해야 한다.
시작 승인 결과와 종료 요청 패킷은 없다.

### 정화 주사기

`ItemId::PuriSyringe=4`는 인벤토리에서 1개씩 소모하는 아이템이다.
`Player::UsePuriSyringe()`는 변이가 0보다 높고 `Inventory::UseItem()`이 성공한 경우 변이를 최대 25 줄인다.
실패하면 변이와 보유량을 유지한다. 효과 이후 이동속도는 현재 변이 단계로 다시 계산한다.

`C2S_RequestPuriSyringe`를 받으면 서버는 접속 세션의 Player에 효과를 적용하고 `Session::SendPuriSyringeResult()`를 호출한다.
`S2C_PuriSyringResult`는 남은 수량만 포함하며 로그인 중에도 전송한다.
변이·속도는 기존 이동 패킷으로 동기화한다. 송수신 및 정화 진행 디버그 로그는 주석 처리되어 있다.

## Change Log

| 날짜 | 변경 내용 | 검증 범위 |
| --- | --- | --- |
| 2026-11-27 | 원문 목차·협업 범위·파일 설명과 절 구조를 보존하고 현재 Shared 물리·변이·정화 경로로 갱신 | 소스 정적 대조. 빌드·실행 미수행 |
| 2026-10-12 | 아이템 박스·인벤토리 관련 파일 역할 및 상태 소유권 갱신. 랜덤 배치 상세는 제외 | 관련 소스·패킷 정적 대조 |
| 2026-09-18 | 클라이언트 공유용 초안 작성. 전체 구조와 터레인·이동 협업 경계 정리 | 현재 소스 대조 |
