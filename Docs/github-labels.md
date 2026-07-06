# GitHub Labels

이 문서는 GitHub Issue와 Pull Request에 사용할 라벨 기준입니다. 실제 GitHub 라벨은 `.github/labels.yml`을 기준으로 만들거나 수동으로 생성합니다.

## Label Rule

- 라벨은 작업 영역, 작업 종류, 우선순위, 상태를 구분합니다.
- 하나의 이슈에는 영역 라벨 1개 이상과 작업 종류 라벨 1개를 붙이는 것을 권장합니다.
- 우선순위 라벨은 급한 작업에만 사용합니다.
- 상태 라벨은 보드 운영 방식이 정해진 뒤 사용할 수 있습니다.

## Area Labels

| Label | Purpose |
| --- | --- |
| `client` | 클라이언트 앱, 입력, 게임 루프 |
| `renderer` | DirectX 12, 셰이더, 리소스, 카메라 |
| `server` | 서버 구현 |
| `network` | TCP/IP, 패킷, 동기화 |
| `shared` | 공용 타입, 공용 헤더 |
| `asset` | 모델, 텍스처, 사운드, 폰트 |
| `docs` | 문서 |
| `build` | 솔루션, 프로젝트, 빌드 설정 |

## Type Labels

| Label | Purpose |
| --- | --- |
| `feature` | 신규 기능 |
| `bug` | 버그 |
| `refactor` | 구조 개선 |
| `test` | 테스트 |
| `chore` | 기타 관리 작업 |

## Priority Labels

| Label | Purpose |
| --- | --- |
| `priority:high` | MVP나 발표 일정에 직접 영향 |
| `priority:medium` | 중요하지만 즉시 막지는 않음 |
| `priority:low` | 여유 있을 때 처리 |

## Status Labels

| Label | Purpose |
| --- | --- |
| `blocked` | 외부 결정 또는 다른 작업 대기 |
| `needs-review` | 리뷰 필요 |
| `needs-docs` | 문서 업데이트 필요 |
| `discussion` | 팀 논의 필요 |

## Examples

```text
client, renderer, feature
server, network, feature
docs, needs-review
build, priority:high
network, discussion
```

## Change Log

| 날짜 | 변경 내용 | 이유 |
| --- | --- | --- |
| 2026-07-06 | GitHub 라벨 기준 작성 | 이슈/PR 분류 통일 |
