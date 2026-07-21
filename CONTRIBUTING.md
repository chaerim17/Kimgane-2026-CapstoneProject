# Contributing Guide

이 문서는 팀원이 같은 방식으로 개발하기 위한 협업 기준입니다. 규칙은 개발 중 계속 개선할 수 있지만, 변경 시 팀원 모두가 알고 있어야 합니다.

## Core Rule

- `main`은 항상 발표, 제출, 시연 가능한 상태를 목표로 합니다.
- 기능 개발은 별도 브랜치에서 진행합니다.
- 한 작업은 가능한 한 하나의 목적만 가집니다.
- 빌드가 깨진 상태로 공유 브랜치에 병합하지 않습니다.
- 코드보다 먼저 인터페이스를 맞춥니다. 특히 Client/Server 패킷은 문서화 후 구현합니다.
- 결정되지 않은 내용은 임의로 확정하지 않고 `TODO`로 남깁니다.

## Work Unit

모든 작업은 이슈, 작업 카드, 회의록의 Action Item 중 하나로 추적합니다.

좋은 작업 단위:

```text
Client - WASD 이동 입력 처리
Server - 플레이어 위치 동기화 패킷 추가
Renderer - Directional Light 기본 구조 구현
Docs - 개발환경 설정 문서 추가
```

피해야 할 작업 단위:

```text
이것저것 수정
버그 수정
클라 작업
서버 작업
```

작업 완료 기준:

- 구현이 요구사항대로 동작합니다.
- 로컬에서 빌드가 성공합니다.
- 직접 실행하여 기본 시나리오를 확인했습니다.
- 관련 문서가 필요한 경우 업데이트했습니다.
- 불필요한 로그, 임시 코드, 테스트 파일을 정리했습니다.
- 팀원이 이해할 수 있는 단위로 커밋했습니다.

## Branch Strategy

| 브랜치 | 용도 |
| --- | --- |
| `main` | 발표, 제출, 시연 가능한 안정 버전 |
| `develop` | 기능 통합 브랜치 |
| `feature/*` | 신규 기능 개발 |
| `fix/*` | 버그 수정 |
| `docs/*` | 문서 수정 |
| `refactor/*` | 구조 개선 |
| `test/*` | 테스트 추가 또는 검증 코드 |

브랜치 이름은 소문자 영어와 `-`를 사용합니다.

```text
feature/client-player-movement
feature/server-position-sync
fix/renderer-camera-jitter
docs/development-environment
refactor/client-input-manager
```

규칙:

- 직접 `main`에 커밋하지 않습니다.
- 기능 개발은 `develop`에서 새 브랜치를 만들어 진행합니다.
- 작업이 끝나면 Pull Request 또는 팀 리뷰를 거쳐 `develop`에 병합합니다.
- 발표나 제출 직전에만 `develop`에서 `main`으로 병합합니다.
- 오래 걸리는 작업은 중간 진행 상황을 PR 설명 또는 회의록에 남깁니다.

## Commit Convention

커밋 메시지는 아래 형식을 사용합니다.

```text
type(scope): summary
```

예시:

```text
feat(client): 플레이어 이동 입력 추가
feat(server): 플레이어 위치 패킷 추가
fix(renderer): 카메라 뷰 행렬 수정
docs(readme): 협업 기준 추가
refactor(client): 입력 매니저 분리
build(client): Visual Studio 프로젝트 설정 갱신
```

커밋 타입:

| 타입 | 의미 |
| --- | --- |
| `feat` | 기능 추가 |
| `fix` | 버그 수정 |
| `docs` | 문서 수정 |
| `style` | 포맷팅, 주석, 네이밍 등 동작 변화 없는 수정 |
| `refactor` | 구조 개선 |
| `test` | 테스트 코드 추가 또는 수정 |
| `build` | 빌드 설정, 프로젝트 설정 수정 |
| `chore` | 기타 관리 작업 |

권장 scope:

| Scope | 대상 |
| --- | --- |
| `client` | 클라이언트 전체 |
| `server` | 서버 전체 |
| `renderer` | DirectX 렌더링 |
| `network` | 통신, 패킷, 세션 |
| `gameplay` | 게임 규칙, 전투, 이동 |
| `asset` | 모델, 텍스처, 사운드 |
| `docs` | 문서 |
| `build` | 솔루션, 프로젝트, 빌드 설정 |

커밋 규칙:

- 한 커밋에는 하나의 논리적 변경만 담습니다.
- 커밋 메시지의 `type`과 `scope`는 영어 키워드를 사용하고, `summary`는 한국어로 작성합니다.
- 설명이 필요한 변경은 커밋 본문이나 PR 설명에 이유를 적습니다.
- 임시 로그, 주석 처리된 코드, 사용하지 않는 파일은 커밋하지 않습니다.

## Pull Request Rule

PR은 아래 항목을 포함합니다. 템플릿은 `.github/pull_request_template.md`에 있습니다.

```md
## Summary
- 변경한 내용

## Test
- 빌드 확인 여부
- 실행 확인 여부
- 테스트한 시나리오

## Note
- 리뷰어가 알아야 할 내용
- 아직 남은 작업
```

리뷰 기준:

- 빌드가 되는가
- 기존 기능을 깨지 않는가
- 역할 영역을 침범하지 않는가
- 네이밍과 폴더 구조가 규칙에 맞는가
- 패킷, 빌드 설정, 에셋 변경이 문서에 반영되었는가
- 임시 코드, 디버그 코드, 하드코딩이 남아 있지 않은가

## Code Style

기본 포맷은 `.editorconfig`와 `.clang-format`을 따릅니다.

| 항목 | 기준 |
| --- | --- |
| 파일 인코딩 | UTF-8 |
| 줄바꿈 | CRLF |
| 들여쓰기 | 공백 4칸 |
| 최대 줄 길이 | 120자 권장 |
| 클래스/구조체 | `PascalCase` |
| 함수 | `PascalCase` |
| 지역 변수/매개변수 | `camelCase` |
| 멤버 변수 | `mName` |
| 상수 | `UPPER_SNAKE_CASE` |

네이밍 예시:

```cpp
class PlayerController;
struct PlayerState;

void UpdatePlayerMovement(float deltaTime);

int playerId;
float moveSpeed;
```

C++ 규칙:

- 전역 변수 사용은 최소화합니다.
- 매직 넘버는 상수로 분리합니다.
- 소유권이 있는 포인터는 스마트 포인터 사용을 우선 검토합니다.
- DirectX COM 객체는 `Microsoft::WRL::ComPtr` 사용을 우선 검토합니다.
- 헤더에는 가능한 한 필요한 선언만 포함합니다.
- 순환 참조가 생기면 구조를 다시 검토합니다.

## Current Naming Rule

This section supersedes older naming rows if they conflict.

| Item | Rule |
| --- | --- |
| Member variable | `mName` |
| Constant / constexpr | `UPPER_SNAKE_CASE` |

```cpp
class PlayerController
{
private:
    int mPlayerId;
    float mMoveSpeedMps;
};

constexpr float DEFAULT_MOVE_SPEED_MPS = 5.0F;
```

## Folder Rule

목표 폴더 구조:

```text
/
  Client/
    Engine/
    Game/
    Renderer/
    Network/
    UI/
  Server/
    Core/
    Network/
    GameLogic/
  Shared/
  Assets/
    Models/
    Textures/
    Shaders/
    Sounds/
  Docs/
    meetings/
```

규칙:

- 클라이언트 전용 코드는 `Client/`에 둡니다.
- 서버 전용 코드는 `Server/`에 둡니다.
- 공용 프로토콜이나 공용 구조체는 `Shared/` 추가 후 관리합니다.
- 모델, 텍스처, 사운드 등 리소스는 `Assets/`에 둡니다.
- 설계 문서와 문제 해결 기록은 `Docs/`에 둡니다.

## Client / Server Interface Rule

클라이언트와 서버가 동시에 작업하므로 통신 규칙은 코드보다 먼저 문서화합니다.

- 패킷 구조를 변경하면 `Docs/network-protocol.md`에 기록합니다.
- 서버 API 또는 패킷 이름은 임의로 바꾸지 않습니다.
- 클라이언트와 서버가 공유하는 데이터 타입은 한 곳에서 관리합니다.
- 임시 패킷도 이름, 방향, 필드를 문서에 남깁니다.
- 패킷 변경은 Client/Server 담당자가 함께 리뷰합니다.

패킷 문서 예시:

```md
## PlayerPosition

- Direction: Client -> Server
- Purpose: 플레이어의 현재 위치 전송

| Field | Type | Description |
| --- | --- | --- |
| playerId | int | 플레이어 ID |
| x | float | X 좌표 |
| y | float | Y 좌표 |
| z | float | Z 좌표 |
```

## Asset Rule

- 외부 에셋은 출처와 라이선스를 `Docs/asset-list.md`에 기록합니다.
- 상업적 사용이 불가능하거나 출처가 불명확한 에셋은 사용하지 않습니다.
- 파일명은 영어로 작성합니다.
- 공백 대신 `-` 또는 `_`를 사용합니다.
- 원본 파일과 엔진용 변환 파일을 구분합니다.

예시:

```text
player_soldier.fbx
terrain_height_map.png
metal_wall_albedo.png
metal_wall_normal.png
```

## Meeting Rule

- 회의에서 결정된 내용은 회의록에 남깁니다.
- 결정 사항은 `Decision`, 할 일은 `Action Item`으로 구분합니다.
- 담당자와 마감일을 함께 기록합니다.
- README에는 확정된 결과만 반영하고, 자세한 논의 과정은 `Docs/meetings/`에 남깁니다.

회의록 파일명:

```text
Docs/meetings/YYYY-MM-DD.md
```

회의록 예시:

```md
## YYYY-MM-DD Meeting

### Decision
- TODO

### Action Item
| 담당 | 작업 | 마감 |
| --- | --- | --- |
| TODO | TODO | TODO |
```
