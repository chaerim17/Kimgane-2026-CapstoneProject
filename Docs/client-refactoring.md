# 클라이언트 리팩토링 진행상황

- 브랜치: `refactor/client-cleanup` (2026-11-19, `develop` 2080953 기준)
- 범위: **Client만**. `Server/` 폴더와 서버 프로젝트 파일은 수정하지 않음.
- 1차가 끝나거나 범위가 넓어지면 Shared 리팩토링까지 이어서 진행.

상태 표기: ⬜ 대기 · 🟨 진행 중 · ✅ 완료 · ⏸ 보류

## 목표

1. 컴포넌트 시스템(`Component`, `AddComponent`/`GetComponent`)을 제거하고 전형적인 객체지향 구조로 전환.
2. 한국어가 아닌 주석과 출력 문자열(로드 실패, 로그 등)을 한국어로 변경.
3. 서버와 클라가 같이 쓰는 값을 Shared 기준으로 통일. 클라가 서버에 맞춤.
4. 레이어를 아래처럼 나눔.

| 영역 | 들어갈 것 |
|---|---|
| **Client** | 렌더링, 입력, 카메라, UI/HUD, 네트워크 송수신, 보간, 이동 예측 호출 |
| **Shared** | 양쪽 결과가 같아야 하는 계산: 이동, 충돌, 레이캐스트, 지형 높이, 맵 배치, 설정값, 프로토콜 |
| **Server** | 판정 권한: NPC AI, 데미지·루팅 결정, 세션 관리 |

## 원칙

- Shared에는 클라와 서버가 **둘 다** 쓰는 것만 넣음. Shared는 D3D12나 렌더링 코드에 의존하지 않음.
- 클라 이동 예측과 서버 보정 구조는 유지. 예측과 판정이 같은 Shared 함수를 쓰게 만드는 게 목적.
- 커밋 제목은 `refactor(client): ...` 형식. PR은 `develop`으로 올림.

---

## 단계

### 0단계: 폴더·필터·네임스페이스 정리 ✅

- ✅ `Client/src/Engine/` 단계 제거. 폴더 구성: `App`(←Application, GameClock, WindowSettings) · `Core`(←Math/VectorMath 합침) · `Camera` · `Input` · `Network` · `Rendering` · `Terrain` · `Scene` · `Gameplay` · `Debug`(←Diagnostics) · `ThirdParty`
- ✅ 네임스페이스 `Kimgane::Engine` → `Kimgane::Client` (하위 네임스페이스 없이 하나로. 설정 네임스페이스는 `Kimgane::Client::RenderSettings`처럼 앞부분만 교체)
- ✅ Shared 폴더에 있던 클라 전용 파일(`TerrainHeightMap.h/.cpp`, `TerrainSettings.h`, 네임스페이스가 Engine이던 것) → `Client/src/Terrain/`. 서버는 사용하지 않음
- ✅ include 경로 재계산, 클라·테스트 프로젝트 파일 경로 갱신, Docs 안의 옛 경로 갱신
- ✅ 클라 + TerrainTests + LunarKitTests 빌드 성공, 테스트 2종 통과
- ✅ 필터(`.vcxproj.filters`)를 실제 폴더와 1:1로 다시 작성. Header/Source 구분 없이 h·cpp를 같은 필터에 둠 (`Shared\...`, `Shaders`, `Assets` 필터 별도)
- ✅ 실행 테스트 (이후 단계 실행 확인으로 함께 확인)

`UI` 폴더는 2단계에서 실제 파일이 생길 때 만듦.

### 1단계: 충돌·물리를 Shared로 통일 ✅

로컬 플레이어 이동 예측은 원래 `SharedPhysics::StepCharacterMovementInWorld`와 Shared `CollisionWorld`(`TestMapCollision`)를 쓰고 있었음.
클라 전용 `CollisionManager`와 콜라이더 컴포넌트는 그 외 기능(조준 레이캐스트, 체력바 가림, 디버그 드로우)에만 남아 있어서 전부 Shared 월드로 옮김.

- ✅ 조준 레이캐스트를 Shared `RaycastQueries::RaycastClosest`(월드 전체) + NPC 캡슐로 교체. `SceneTerrainSampler` 어댑터 삭제
- ✅ TestHouse 접촉 검사(`CheckLocalPlayerHouseCollision`)는 원래 Shared 월드 조회였음. 상태만 `mLocalPlayerState`로 변경
- ✅ 집·정화기·상자의 클라 콜라이더 등록 제거. 모두 `mMapCollision` 월드 하나만 사용
- ✅ `ColliderDebugDrawSystem`이 Shared 월드 body를 그리도록 재작성 (B키를 처음 누를 때 메쉬 생성)
- ✅ `Dx12Renderer::IsLineOfSightBlocked`가 `Scene::GetCollisionWorld()` 레이캐스트를 쓰도록 변경
- ✅ 로컬 플레이어 상태를 `RigidbodyComponent` 대신 `GameScene::mLocalPlayerState`(Shared `RigidbodyState`)로 직접 보관
- ✅ `PlayerControllerComponent`의 Rigidbody 이동 경로(`ApplyMovement`, `ApplyJump`) 제거. 입력만 만들고 이동은 Shared Step이 처리
- ✅ 삭제: `Physics/` 폴더 전체(`CollisionManager`, `ColliderComponent`, `TerrainColliderComponent`, `RigidbodyComponent`, `PhysicsSettings.h`)
- ✅ 삭제: 빌드에서 빠져 있던 `Diagnostics/ComponentSmokeTests.*`
- ✅ `TestSceneSettings.h`에서 안 쓰게 된 상수(캡슐 크기, 집 위치, 정화기 경사로·충돌 경로) 제거
- ✅ Shared 변경: `CollisionWorld::GetBodies()` 읽기 전용 getter 추가 (헤더 inline 함수, 서버 동작 영향 없음)
- ✅ Debug x64 빌드 성공
- ✅ 실행 테스트 (2026-11-19 작업자 확인, 이상 없음)

**동작이 달라진 부분 (테스트 확인 항목)**

- 조준 사격: 이제 정화기(박스·경사로)와 아이템 상자에도 맞음. 로그에 `Purifier`, `ItemBox`가 나올 수 있음. 서버 판정과 같은 월드 기준.
- 체력바 가림: 정화기 경사로도 시야를 가림.
- 사격 방향 기준점: 클라 캡슐 컴포넌트 중심 → Shared 플레이어 캡슐 중심 (값은 같음).
- 콜라이더 디버그(B키): 지형·집·정화기·상자·로컬 플레이어 캡슐 표시. 지형 메쉬를 처음 켤 때 만들어서 잠깐 멈칫할 수 있음.
- NPC 히트박스 디버그(주석 처리돼 있던 코드)는 제거하고 TODO로 남김.

### 2단계: 나머지 컴포넌트 제거, 객체지향 구조로 전환 ✅

서버 `World/GameObject.h`, `Player`, `Npc`, `ItemBox`의 이름과 형태에 맞춤.

```
Core/GameObject          이름, 활성, Transform, Mesh, Material, virtual Update
 ├─ Gameplay/Player      변이 외형 전환
 │   ├─ LocalPlayer      입력(PlayerController 내용) + 예측 상태 + 카메라 연결
 │   └─ RemotePlayer     서버 위치 반영
 ├─ Gameplay/Npc         보간(NetworkSmoothing) + HP
 └─ Gameplay/ItemBox     본체 + 뚜껑
```

- ✅ 클래스 구조 확정 (LocalPlayer / RemotePlayer로 분리, RemotePlayer는 Player.h 안에 둠)
- ✅ **2-1** GameObject가 Mesh·Material을 멤버로 가짐, 조명·카메라를 Scene 멤버로 (커밋 9930826)
  - `MeshComponent`, `MaterialComponent`, `DirectionalLightComponent`, `CameraComponent` 삭제
  - 조명: `Scene::GetDirectionalLight()`가 멤버를 바로 반환 (오브젝트 검색 제거)
  - 카메라: `GameScene::mGameplayCamera`(SpringArmCamera)가 직접 보유, `UpdateGameplayCamera()`에서 회전·추적
  - 렌더링에 안 쓰이던 지형 GameObject와 `mTerrain` 제거 (지형은 LunarTerrainRenderer가 그림)
  - `ColliderDebugDrawSystem`, `LunarEnvironment`도 멤버 방식으로 변경
  - 안 쓰게 된 `Material::CreateSolidColor` 삭제
- ✅ **2-2** LocalPlayer / RemotePlayer / Npc / ItemBox 클래스, `PlayerControllerComponent`·`NetworkSmoothingComponent` 흡수 (커밋 10ddd7b)
  - `LocalPlayer`: 입력·회전·이동 패킷 + 예측 상태·서버 보정·화면 보간 (GameScene에 흩어져 있던 것)
  - `Player`: 로컬·원격 공통 부모, 변이 외형 전환 (앞으로 HP·팀·이름표·애니메이션 자리) / `RemotePlayer`: 서버 위치 반영
  - `Npc`: 목표 위치 보간, `ItemBox`: 본체+뚜껑 함께 관리
  - Scene: `mNetworkPlayers` → `mRemotePlayers` + `mNpcs`로 분리, `CreateObject<T>()` 템플릿
  - GameObject에 `GetYawRad`/`SetYawRad` 추가
- ✅ **2-3** UI 계열을 `UI/`로, 렌더러가 Scene의 UI 목록을 받도록 변경 (커밋 048c65b)
  - `TextComponent` → `UI/TextLabel` (Scene이 소유, 패널 GameObject를 가리킴)
  - `HealthBarComponent` → `UI/WorldHealthBar` (Npc 멤버), 렌더러는 `Scene::CollectWorldHealthBars()`로 받음
  - `DamageNumberComponent` → `UI/DamageNumber` (GameScene의 고정 개수 풀, GameObject 아님), 렌더러는 `Scene::GetDamageNumbers()`로 받음
  - `PlayerHud`, `PlayerHudSettings` → `UI/`로 이동
  - 렌더러가 매 프레임 모든 오브젝트에 `GetComponent`를 묻던 코드 제거
- ✅ **2-4** 컴포넌트 시스템 완전 삭제 (커밋 e2893db)
  - 삭제: `Core/Component.h`, `GameObject`의 `AddComponent`/`GetComponent`/`GetComponents`/`RemoveComponents`, 컴포넌트 목록
  - `GameObject::Update`는 기본 동작 없음(상속 클래스가 재정의)
  - 문서: `component-tests.md` 삭제, `architecture.md`·`rendering-pipeline.md`·`network-manager.md`·`shared-character-world.md`의 컴포넌트 설명을 현재 구조로 갱신 (task-board·변경 기록 같은 과거 기록은 유지)
  - 클라 + TerrainTests + LunarKitTests 빌드 성공

### 3단계: Scene.cpp 분할 ✅

시작 시 `Scene.cpp` 1402줄(클래스 5개), 그중 `GameScene` 약 900줄.

- ✅ **3-1** 씬 종류별 파일 분리, 코드 이동만 (커밋 629b3e6)
  - `Scene`(기본) / `TitleScene` / `SettingsOverlayScene`(+`OverlayScene`) / `GameScene`(+`Local`/`OnlineGameScene`)
  - 타이틀·설정 공용 UI 헬퍼 → `Scene/MenuWidgets` (UI 패널, 버튼 라벨, UI 카메라, `UI_ORTHOGRAPHIC_HEIGHT_M`)
  - 함수 73개 이동 전후 대조 일치
- ✅ **3-2** GameScene 책임을 클래스로 분리 (커밋 d497553)
  - `Scene/NetworkObjectSync`: 서버 오브젝트 → 원격 플레이어·NPC·상자 (생성·위치·제거·HP·원격 변이 외형·상자 충돌 body). 서버 `GameWorld`에 대응
  - `Scene/PlayerInteraction`: 퀵슬롯 선택, 조준 사격, F 상호작용 우선순위, 정화 요청 상태, 상자 열기 결과 로그
  - `GameScene`에 남은 것: Build, 매 프레임 순서 조율, 카메라, 내 캐릭터 물리·보정·표시 위치, 내 변이 외형·이동속도, HUD 상태 조립, 데미지 숫자, 정화기 배치
  - `Core/SharedVec3Conversion.h`: 여러 파일에 중복된 `ToSharedVec3`/`ToXMFloat3`를 하나로
  - GameScene.cpp 993줄 → 478줄

### 4단계: 서버·Shared 공통 값 정리 ✅

- ✅ 클라 설정 파일(`*Settings.h`)과 Shared/서버 값 대조
- ✅ 서버에만 있던 상자 공통값을 서버와 같은 이름으로 Shared에 둠 (커밋 7a545e2)
  - `Shared/Items/ItemBoxSettings.h`: `ITEM_BOX_OPEN_DISTANCE_M`, `ItemBoxAssetSettings`(본체·뚜껑 모델, 충돌 경로)
  - 클라는 이 값을 씀. 클라 쪽 복사본(`PlayerInteraction`의 열기 거리, `TestSceneSettings`의 상자 경로) 삭제
  - 서버는 아직 `ServerConfig.h`의 자기 값을 씀. 서버도 Shared를 쓸지는 나중에 서버 팀과 상의 (인계 목록에는 아직 안 적음)
- ✅ 변이 설정은 Shared에 두지 않음: 변이 수치·이동속도 배율은 서버가 계산해서 보내고, 클라는 게이지 비율만 계산함
  - `MUTATION_MAX_VALUE`를 `UI/PlayerHudSettings.h`로 옮김 (서버 `PlayerMutationSettings::MAX_VALUE`와 같게 유지)
- ✅ `TestSceneSettings`의 안 쓰는 별칭 `HOUSE_MODEL_PATH`·`HOUSE_COLLISION_PATH` 삭제
- ✅ `Shared/Terrain/TerrainConfig.h` 삭제: 아무도 안 씀. 서버 vcxproj·filters에서 등록만 뺌 (서버 코드는 안 바꿈, 서버 빌드 확인)

### 5단계: 한국어화 ✅

2~3단계에서 삭제되거나 다시 쓰이는 파일이 많아서 마지막에 진행.

- ✅ 한국어 출력 준비: `/utf-8` 빌드라 문자열이 UTF-8로 들어감
  - 디버그 콘솔을 `SetConsoleOutputCP(CP_UTF8)`로 맞춤
  - 오류 창은 `MessageBoxA`(CP949로 읽음) 대신 UTF-8 → 와이드 문자열로 바꿔 `MessageBoxW`로 띄움
- ✅ 출력·예외·로그 문자열 약 50개 (`ClientApplication`, `NetworkManager`, 렌더링·지형 예외, `GameScene`, `NetworkObjectSync`, `PlayerInteraction`의 아이템·상자 결과·사격 대상 이름)
  - `[Network]` 같은 로그 태그와 `playerId=` 같은 필드 이름은 코드·패킷 이름과 맞춰 검색하기 쉽게 영어로 둠
- ✅ 영어 주석 10줄 (`LunarEnvironment`, `Dx12Renderer`, `LunarTerrainRenderer`, `TerrainChunkGeometry`)
  - `// namespace` 닫는 표시와 주석 처리된 옛 코드는 그대로 둠
- ✅ 셰이더 `LunarTerrain.hlsl` 영어 주석 11줄 (BOM 없는 UTF-8, fxc 컴파일 확인)
- ✅ 테스트(`Client/Tests`) 실패·통과 메시지 50개와 영어 주석 5줄. 두 테스트 `main()`에 콘솔 UTF-8 설정 추가, 실행해서 통과 확인
  - `Shared/Tests/CharacterMovementTests.cpp`의 영어 메시지 53개는 Shared 리팩토링 때 처리

### TestHouse 정리 🟨 (나머지는 이번 리팩토링에서 미룸)

처음 텍스처 매핑 연습용으로 넣은 집. 지금 집 표시는 LunarKit 배치(`layout.txt`), 충돌은 `L02_LunarKit_collision.txt`라 TestHouse는 실제로 쓰는 곳이 없음.

- ✅ 벽돌 텍스처 `Assets/Textures/T_Brick_Clay_Beveled_D.dds` 삭제 (참조 없음), `asset-list.md` 항목 삭제
- ⬜ 주석·문구: `PlayerInteraction.h:48`, `Shared/Physics/RaycastQueries.h:32`, `Assets/P02_Cyborg_Player_Package/README_KO.md:26`
- ⬜ 문서: `architecture.md` 집 모델·충돌 표(옛 TestHouse 설명), `asset-list.md` 아이템 상자 설명
- ⬜ `Shared/Geometry/TestHouse_collision.txt`: 참조 없음. 서버 vcxproj·filters 등록만 빼면 삭제 가능 (확인 대기)
- ⬜ `Shared/Geometry/TestHouse.obj`, `TestMapSettings::HOUSE_MODEL_PATH`: 서버 `GameWorld::LoadMap()`이 읽기만 하고 안 씀. 서버 코드 수정이 필요 (인계 여부 확인 대기)

---

## 후속 과제 (이번 범위 밖)

- 100명 규모 대비 다른 플레이어 보간: `RemotePlayer`는 서버 위치로 바로 이동함. `Npc`의 목표 위치 보간을 `Player`로 올려 같이 쓰기
- 100명 규모 대비 렌더링: 같은 플레이어 모델 인스턴싱, 거리별 LOD (집 소품 LOD 방식 재사용)
- 팀 표시: 서버 팀 정보가 생기면 `Player`에 팀 번호를 두고 아군/적 색·이름표 구분

- `Shared/Protocol.h`의 안 쓰는 상수 `PLAYER_MOVE_SPEED`·`JUMP_POWER`·`GRAVITY`: 실제 값은 `PhysicsSettings`에 있어 헷갈림. 프로토콜 정리 때 같이 처리
- 집 위치 출처가 두 곳: 그리기는 `EnvironmentLayout` 배치 데이터, 충돌은 `HOUSE_POSITION_M` + 충돌 txt. 한쪽만 고치면 어긋남. 하나로 합칠지 검토
- 입력 순번 + 재시뮬레이션(replay) 방식 보정. 지금은 서버 위치로 바로 맞추고 화면 표시만 부드럽게 따라감. 프로토콜 변경이 필요해서 서버와 상의. -> 클라는 입력에 순번을 붙이고 서버가 처리했다고 알려줄때까지 기록, 서버는 몇번까지 처리했는지와 위치를 알려주고 클라는 그 위치에서 나머지 순번들을 재계산, 재계산 위치와 현재위치가 같으면 아무일 보정없음.