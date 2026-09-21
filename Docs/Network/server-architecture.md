# 김가네 서버 아키텍처 — 클라이언트 개발자 공유 초안

> 기준: 2026-09-18 작업 트리의 소스 코드
> 목적: 서버 전체 구조를 이해하고, 협업 대상으로 허용한 **터레인 로직과 캐릭터 이동 로직**의 작업 경계를 파악한다

## 목차

1. [협업 범위](#1-협업-범위)
2. [전체 구조](#2-전체-구조)
3. [실행 흐름과 상태 소유권](#3-실행-흐름과-상태-소유권)
4. [터레인 로직](#4-터레인-로직)
5. [캐릭터 이동 로직](#5-캐릭터-이동-로직)
6. [클라이언트 연동 시 확인할 사항](#6-클라이언트-연동-시-확인할-사항)

## 1. 협업 범위

| 구분 | 대상 | 작업 경계 |
| --- | --- | --- |
| 클라이언트 직접 수정 가능 | `Server/KimganeServer/src/Terrain/ServerTerrainCalculation.h/.cpp` | 지형 로드·조회, 월드 좌표 변환, 이동 계산 연결, 지형에 의한 사격 차단 판정 |
| 클라이언트 직접 수정 가능 | `Server/KimganeServer/src/Terrain/TerrainHeightMap.h/.cpp` | 서버 높이맵 로드·보간·범위 검사 |
| 클라이언트 직접 수정 가능 | `Shared/Physics/CharacterMovement.h/.cpp` | 캐릭터 수평 이동, 바닥 선택, 수직 이동·착지 계산 |
| 참조용 — 직접 수정 범위 밖 | `Core/Server.h/.cpp`, `Core/Session.h/.cpp`, `Network/PacketHandler.h/.cpp` | 호출 위치·입력·출력 확인용. 시그니처 변경에 따른 호출부 수정은 서버 담당자와 조율 |
| 참조용 — 직접 수정 범위 밖 | `Npc/`, `Shared/Protocol.h`, 공통 충돌·레이캐스트 구현, 프로젝트 설정 |

허용 범위는 파일 이름만으로 확인하지 않음. 예를 들어 `BlocksShot()`의 지형 판정을 수정할 수 있어도 NPC 선택, 데미지, HP, 사망 처리를 변경하는 작업까지 포함하지 않는다.

## 2. 전체 구조

| 모듈 | 역할 |
| --- | --- |
| `Core/Server` | 서버 초기화, 접속·수신·송신 완료 처리, 월드 갱신, 사격·오브젝트 제거 조정 |
| `Core/Session` | 소켓과 비동기 I/O 정보, 수신 버퍼, 플레이어 상태, 송신 패킷 구성 |
| `Network/PacketHandler` | 패킷 종류별 처리. 이동 입력을 Session에 저장하고 사격 요청은 Server에 전달 |
| `Terrain/ServerTerrainCalculation` | Server와 지형·이동 계산 사이의 연결 함수 모음 |
| `Terrain/TerrainHeightMap` | 서버용 RAW 높이맵 데이터와 높이 조회 |
| `Npc/Npc`, `Npc/NpcSetting` | NPC 상태, 생성, 주기적 이동 및 관련 전송 |
| `Shared/` | Shared 파일에 있는 것들은 Shared filter 내부에 있게끔 한다|

`Terrain/ServerTerrain.h/.cpp`도 프로젝트에 등록되어 있으나, 현재 `Server.cpp`의 실행 경로는 이 래퍼가 아니라 **`ServerTerrainCalculation`과 서버 `TerrainHeightMap`**을 사용한다.

## 3. 실행 흐름과 상태 소유권

### 초기화

`Server::Initialize()`는 지형 로드 → 집 충돌 박스 로드 및 충돌 월드 등록 → NPC 초기화 → 네트워크 준비와 타이머 시작을 수행한다. 지형 로드 예외는 Server에서 처리하고 초기화를 실패시킨다.

### 실행 중

- `Server::Run()`은 IOCP(비동기 입출력 완료 통지)에서 이벤트를 받아 접속·수신·송신 완료를 처리한다.
- `HandleRecv()`는 TCP 수신 바이트를 패킷 길이에 따라 나누고, 완성된 패킷을 `PacketHandler`에 전달한다.
- 별도 `TimerThread()`는 `Sleep(50)` 후 플레이어 이동과 NPC 갱신을 수행한다. 계산에는 고정 `0.05초`를 전달하며 실제 경과 시간을 측정하지 않는다.
- `Run()`의 이벤트 처리와 타이머 갱신은 `mWorldMutex`를 사용한다. 분리된 계산 함수는 자체 잠금을 잡지 않으며, 호출자가 상태 접근을 보호한다.

| 상태 | 소유·관리 위치 |
| --- | --- |
| 플레이어 세션 | 전역 `clients` 배열의 `unique_ptr<Session>` |
| 플레이어 위치·방향·점프 상태 | 각 `Session` |
| 높이맵 | `Server::mTerrain`의 `shared_ptr<TerrainHeightMap>` |
| 충돌 월드 / 원본 집 충돌 박스 | `Server::mCollisionWorld` / `mHouseCollisionBoxes` |
| 이동용 월드 좌표 박스 | 타이머 시작 시 만든 로컬 `groundBoxes` |
| NPC 목록 | `NpcSetting::gNpcs` |

계산 함수에 전달되는 높이맵·충돌 월드·박스 목록은 참조 또는 `span`으로 읽는다. 해당 함수가 데이터를 소유하거나 네트워크 전송을 수행하지 않는다.

## 4. 터레인 로직

### 4.1 실제 사용 파일

서버는 `src/Terrain/TerrainHeightMap`의 전역 클래스 `TerrainHeightMap`을 사용한다. `Shared/Terrain/TerrainHeightMap`의 엔진 종속성에 의해 사용하지 않으나 구조는 동일하다.

서버 로드 경로는 다음과 같다.

`Server::Initialize()` → `ServerTerrainCalculation::LoadTerrain()` → 서버 `TerrainHeightMap::LoadRawAuto()`

### 4.2 좌표와 높이 조회

- 캐릭터 월드 좌표는 지형 중앙을 원점으로 사용한다.
- 높이맵 조회 좌표는 지형 모서리에서 시작하는 양수 좌표다.
- `GetWorldWidthM()`는 `(샘플 수 - 1) × 셀 간격`이며 길이도 같은 방식이다.

```cpp
sampleX = worldX + terrain.GetWorldWidthM() * 0.5f;
sampleZ = worldZ + terrain.GetWorldLengthM() * 0.5f;
height  = terrain.SampleHeightM(sampleX, sampleZ);
```

### 4.3 ServerTerrainCalculation의 함수 계약

| 함수 | 입력 | 출력·변경 상태 |
| --- | --- | --- |
| `LoadTerrain()` | `TerrainConfig` 설정 | 높이맵 공유 포인터. 로드 실패는 예외로 전달 |
| `BuildGroundBoxes()` | 원본 충돌 박스 목록, 월드 Y 오프셋 | Y 오프셋을 적용한 `vector<Box>` 사본 |
| `UpdateHorizontal()` | Session, 오브젝트 ID, 높이맵, 충돌 월드, 바닥 박스, 속도, dt | Session의 x/y/z 갱신, 바닥 높이 반환 |
| `UpdateVertical()` | Session, 바닥 높이, 중력, dt | Session의 y, 수직 속도, 점프 상태 갱신 |
| `BlocksShot()` | 높이맵, 대상까지 길이가 제한된 ray | 지형이 사격을 막으면 `true` |

`BuildGroundBoxes()`는 현재 집의 Y 오프셋 `4.71m`를 적용한다. 타이머 시작 때 한 번 생성되므로 이후 원본 박스를 바꿔도 자동으로 재생성되지 않는다. `CollisionWorld`에 등록된 박스와 이동용 `groundBoxes`는 같은 월드 변환을 사용해야 한다.

### 4.4 지형에 의한 사격 차단

Server가 대상 NPC를 선택하고 ray의 최대 거리를 대상까지로 제한한 뒤 `BlocksShot()`을 호출한다.

1. `ShootTerrainSampler`가 월드 좌표를 높이맵 좌표로 변환한다.
2. 발사점이 지형 범위 밖이거나 높이가 지형 이하이면 차단한다.
3. `RaycastTerrain()`으로 대상까지의 구간이 지형과 만나는지 검사한다.

NPC는 `NpcSetting`에서 같은 서버 높이맵을 직접 조회한다. 플레이어와 달리 `ServerTerrainCalculation::UpdateHorizontal()`을 거치지 않는다. 높이맵 조회 규칙 변경 시 NPC에도 영향이 있으므로 변경 시 NPC의 움직임에 이상이 생길 수 있다.

## 5. 캐릭터 이동 로직

### 5.1 입력에서 결과 전송까지

현재 순서는 **수평 계산 → Session 반영 → 이동 패킷 구성·송신 요청 → 수직 계산**이다. 따라서 해당 틱의 패킷에 수직 계산까지 끝난 위치가 담긴다고 가정하면 안 된다. 전송 순서 변경은 Server 호출 흐름 변경이므로 현 순서를 고정한다.

### 5.2 상태와 입력의 의미

| 값 | 의미 |
| --- | --- |
| `CharacterMovementState::positionM` | 발 기준 월드 위치, 단위 m |
| `velocityYMps` | 수직 속도, 단위 m/s |
| `isJumping` | 점프 계산 활성 여부. 일반적인 접지 판정 결과가 아님 |
| `CharacterMovementInput::yawRad` | 최종 이동 방향, 단위 rad. 서버에서는 `Session::mMoveYaw` 전달 |
| 네 방향 bool | 현재 이동 입력이 있는지 판단하는 게이트 |
| `Session::mYaw` | 캐릭터 정면/전송용 방향. 이동 계산의 `mMoveYaw`와 구분 |

`HandleMoveStart/Stop()`은 해당 방향 bool과 `mMoveYaw`를 갱신한다. Shared 함수는 방향키를 다시 조합하지 않고, 하나라도 눌렸다면 `sin(yaw), cos(yaw)`를 이동 벡터로 사용한다.

`HandleJump()`는 이미 점프 중이면 무시하고, 아니면 점프 상태와 초기 속도 `JUMP_POWER`를 설정한다.

### 5.3 수평 이동과 바닥 선택

`UpdateHorizontal()`은 다음 순서로 서버 상태를 Shared 계산에 연결한다.

1. Session에서 위치·수직 속도·점프 여부와 이동 입력을 복사한다.
2. **이동 전 x/z**에서 지형 높이를 조회한다.
3. `StepCharacterHorizontalMovement()`를 호출한다.
4. 결과 x/y/z를 Session에 반영하고 바닥 높이를 반환한다.

현재 호출은 충돌 차단 여부를 검사한다. 별도의 슬라이딩·계단 오르기 해결 함수를 이 경로에서 호출하지 않는다. 지형 높이는 이동 전 위치에서 조회하지만 박스 윗면은 후보 위치에서 찾으며, **이동이 막혀도 후보 위치에서 선택한 바닥 높이를 사용**한다.

### 5.4 수직 이동과 착지

점프 중에는 아래 순서로 갱신한다.

```cpp
positionY += velocityY * dt;
velocityY -= gravity * dt;
```

하강 중이고 y가 바닥 이하이면 바닥에 맞추고 수직 속도를 0, 점프 상태를 false로 만든다. 점프 중이 아니면 항상 바닥 높이에 맞춘다. 낙하 상태와 접지 상태를 별도로 관리하는 구조는 아니다.

| 설정 | 현재 사용 값 / 위치 |
| --- | --- |
| 이동 dt | `0.05초`, `Server::TimerThread()`의 `DELTA_TIME` |
| 수평 속도 | `5m/s`, 동일 함수의 로컬 `MOVE_SPEED` |
| 중력 | `20m/s²`, `Shared/Protocol.h`의 `GRAVITY` |
| 점프 초기 속도 | `8m/s`, `Shared/Protocol.h`의 `JUMP_POWER` |
| 캡슐 크기 | `Shared/Physics/PhysicsSettings.h`의 플레이어 캡슐 설정 |

## 6. 클라이언트 연동 시 확인할 사항

- 지형 높이 조회 시점, 박스 윗면 선택 조건, 수평·수직 계산 순서를 비교한다.
- 계산 함수의 변경과 패킷 전송 순서 변경을 구분한다.
- `C2S_PLAYER_STATE`는 현재 위치 차이를 계산할 뿐 서버 위치를 덮어쓰거나 보정을 구현하지 않는다.
- `S2C_MOVE_OBJECT`는 objectId, 위치, yaw를 전달한다. 현재 구조에 입력 확인 번호나 입력 재실행 절차가 있다고 가정하지 않는다.

## Change Log

| 날짜 | 변경 내용 | 검증 범위 |
| --- | --- | --- |
| 2026-09-18 | 클라이언트 공유용 초안 작성. 전체 구조와 터레인·이동 협업 경계 정리 | 현재 소스 대조 |
