# 공통 맵 충돌과 캐릭터 이동

## 현재 구현

Client와 Server는 `Shared/World/TestMapCollision`으로 각자의 맵 충돌 월드를 구성하고,
`Shared/Physics/CharacterMovementWorld.h`의 동일한 함수를 호출합니다.

```text
TestMapSettings + HeightMapData + 집 충돌 파일
                    ↓
             TestMapCollision
                    ↓
MakePlayerMovementState → ResolveCharacterContactsInWorld (스폰)
                    ↓
         StepCharacterMovementInWorld (이동)
                    ↓
          RigidbodyState 위치·속도·접지
```

- `Shared/Terrain/HeightMapData`가 RAW 로딩, 높이 보간, 법선 계산을 담당합니다.
  서버의 `TerrainHeightMap`은 이 타입의 별칭입니다. 기존 클라이언트 `TerrainHeightMap` API는
  공통 데이터를 보관하고 DirectX 벡터·바운딩 박스로 변환하는 연결부입니다.
- `TestMapSettings`에서 RAW 경로·크기·간격·높이 배율, 지형과 집의 위치, 플레이어 스폰 위치를 정의합니다.
  현재 배치는 평행 이동을 지원하며 집 충돌 형상은 월드 축에 정렬된 박스입니다.
- `TestMapCollision`은 지형을 먼저, 집의 각 박스를 파일 순서대로 등록합니다.
  집 위치는 여기서 한 번 적용합니다. 클라 모델과 디버그 콜라이더도 같은 설정과 박스를 사용합니다.
- 지형 sampler는 맵 객체가 소유합니다. 맵 복사·이동 시 sampler 수명은 공유하고 충돌 registry는 각각 유지합니다.
- 맵의 콜라이더 ID는 박스별로 고유합니다. 하나의 집 오브젝트에 여러 콜라이더가 연결될 수 있습니다.
  현재는 기존 정수 ID registry를 공통 사용하며, 세대 번호가 있는 범용 핸들 시스템은 추가하지 않았습니다.
- 온라인 맵은 서버와 같은 지형·집으로 구성합니다. 회전 테스트 큐브는 로컬 테스트 씬에만 생성합니다.
  로컬 큐브의 컴포넌트 형상을 Shared 월드에 갱신한 뒤 같은 이동 함수를 사용합니다.
- `RigidbodyState`와 `MakePlayerMovementState`가 양쪽 캐릭터의 상태와 초기값을 정의합니다.
  스폰 시 강제로 접지시키지 않고 동일한 월드 접촉 조회로 결정합니다.
- 클라이언트는 강체의 Shared 상태를 이동 함수에 전달하고 결과를 컴포넌트에 반영합니다.
  물리 위치를 화면 보간 위치로 덮어쓰지 않으며, 컴포넌트 자동 적분을 끈 캐릭터만 이 경로로 갱신합니다.
- 별도 `CharacterCollisionSolver`와 사용되지 않는 구형 수평·수직 이동 API는 제거했습니다.
  컴포넌트용 `CollisionManager`는 기존 조회·레이캐스트·진단 API를 유지하지만 캐릭터 이동 판정에는 사용하지 않습니다.

## 이번 범위에서 유지한 동작

서버 실행·동기화 구조는 `origin/develop`의 `383a51b`를 기준으로 합니다.
TimerThread는 기존 `Sleep(50)` 후 플레이어마다 이동 계산·전송을 하고, 마지막에 NPC를 갱신합니다.
develop에 원래 있던 `mWorldMutex`와 IOCP 실행 구조를 유지하고,
브랜치에서 추가했던 서버 60Hz 누적 시간 계산·별도 전송 주기 제어·세션별 입력 mutex는 제거했습니다.
각 틱의 이동·충돌 계산은 Shared 함수에 `0.05`초를 전달합니다.
점프 패킷은 요청을 기록하고 공통 이동 함수가 접지 상태에 따라 처리합니다. 패킷 형식은 유지합니다.
사격의 사거리·데미지 정책도 유지하며, 맵 장애물과 지형 데이터 접근만 공통 맵으로 연결했습니다.

클라이언트의 기존 60Hz 갱신은 유지합니다. 양쪽 계산 로직과 맵은 같지만 시간 간격은 다르므로,
이번 변경만으로 점프 궤적과 충돌 결과가 항상 같아지는 것은 아닙니다.
입력 번호, 입력 재실행, 서버 스냅샷 확장 등 네트워크 예측·보정 변경은 포함하지 않습니다.
따라서 네트워크 지연이나 현재 위치 보정 방식에서 발생하는 오차까지 해결한 것은 아닙니다.

## 검토와 사용자 검증

사용자 요청에 따라 빌드, 게임·서버 실행, C++ 테스트 실행은 수행하지 않았습니다.
프로젝트 XML과 파일 등록, 로컬 include, 삭제 API 잔여 참조, 소스 구문 구분자와 diff 공백을 정적으로 검토했습니다.
서버 실행·잠금 관련 코드는 `origin/develop`과 비교했습니다. 정적 검토는 컴파일 검증을 대신하지 않습니다.

`Shared/Tests/CharacterMovementTests.cpp`는 공통 월드 API에 맞춰 수정하고 다음 회귀 검증을 추가했습니다.

- 집의 여러 박스 등록과 배치가 한 번만 적용되는지 확인
- 맵 복사·재구성·이동 후 지형 sampler 수명 확인
- 지형 월드 좌표, 높이 보간, 경사 법선, 경계 확인
- 양쪽 캐릭터 초기 상태의 공통 설정 확인

독립 테스트를 빌드할 때는 기존 Physics 소스에 `Shared/Terrain/HeightMapData.cpp`,
`Shared/World/TestMapCollision.cpp`, `Shared/Geometry/CollisionBoxLoader.cpp`,
`Shared/IO/AssetPathResolver.cpp`를 함께 연결해야 합니다.

사용자 실행 확인 항목: 양쪽 빌드, 스폰·점프·착지·집 벽 슬라이딩·발판 이탈,
온라인에서 회전 테스트 큐브 제외, 로컬 큐브 충돌과 기존 조준·체력바 동작.
