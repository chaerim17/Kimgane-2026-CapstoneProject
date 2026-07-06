# Definition Of MVP

이 문서는 MVP가 정말 완료되었는지 판단하기 위한 기준입니다. `Docs/mvp-scope.md`가 범위를 설명한다면, 이 문서는 완료 판정을 위한 체크리스트입니다.

## MVP Statement

MVP는 아래 문장으로 설명할 수 있어야 합니다.

```text
두 개의 로컬 클라이언트가 TCP/IP 환경의 서버에 접속하고,
20x20m 테스트 공간에서 플레이어 위치가 서버 기준으로 동기화되며,
클라이언트는 DirectX 12로 자기 자신과 다른 플레이어를 렌더링한다.
```

## Required Demo

MVP 시연은 아래 순서로 진행할 수 있어야 합니다.

```text
1. Server 실행
2. Client A 실행
3. Client B 실행
4. Client A에서 WASD 이동
5. Client B 화면에서 Client A 위치 변화 확인
6. Client B에서 WASD 이동
7. Client A 화면에서 Client B 위치 변화 확인
8. 3분 동안 크래시 없이 유지
```

## Must Have

| 영역 | 완료 조건 |
| --- | --- |
| Build | Debug x64 빌드 성공 |
| Client | Win32 창 생성 및 정상 종료 |
| Renderer | DirectX 12 Clear Color, Present 성공 |
| Renderer | 20x20m 테스트 공간 표시 |
| Renderer | 로컬/원격 플레이어 구분 가능 |
| Input | WASD 입력으로 이동 의도 생성 |
| Camera | 3인칭 시점으로 플레이어 확인 가능 |
| Network | TCP/IP 환경에서 로컬 서버 접속 |
| Server | 플레이어별 ID와 위치 상태 관리 |
| Sync | 다른 클라이언트의 위치 변화 확인 가능 |
| Docs | 패킷, 단위, 주요 문제 해결 기록 업데이트 |

## Should Have

| 영역 | 완료 조건 |
| --- | --- |
| Renderer | Directional Light 기본 적용 |
| Renderer | Depth Buffer 적용 |
| Debug | FPS 또는 frame time 표시 |
| Network | 접속/해제 로그 |
| Docs | MVP 시연 절차와 실행 방법 README 반영 |

## Not Required

MVP 완료 판정에 아래 항목은 필요하지 않습니다.

- 최종 캐릭터 모델
- 최종 애니메이션
- 전투 시스템
- 아이템, 인벤토리
- 로그인, DB
- 매칭, 로비
- 사운드
- 최종 UI
- 배포 빌드 자동화

## Fail Conditions

아래 중 하나라도 해당하면 MVP 완료로 보지 않습니다.

- Debug x64 빌드 실패
- 클라이언트가 1개만 실행 가능
- 서버 없이만 동작함
- 위치 동기화가 시연에서 확인되지 않음
- 1분 이내 반복 크래시 발생
- 패킷 구조나 단위 기준이 문서와 다름
- DirectX 12가 아니라 다른 렌더링 경로로 시연함

## MVP Review Checklist

- [ ] Debug x64 빌드 성공
- [ ] Server 실행 성공
- [ ] Client 2개 실행 성공
- [ ] Client 2개 모두 서버 접속 성공
- [ ] 로컬 플레이어 이동 가능
- [ ] 원격 플레이어 위치 표시 가능
- [ ] 20x20m 테스트 공간 표시 가능
- [ ] 3분 이상 크래시 없음
- [ ] `Docs/network-protocol.md` 업데이트
- [ ] `Docs/engineering-units.md` 기준 준수
- [ ] `Docs/troubleshooting.md` 업데이트
- [ ] README 실행 방법 업데이트
- [ ] 시연 영상 또는 GIF 준비

## Change Log

| 날짜 | 변경 내용 | 이유 |
| --- | --- | --- |
| 2026-07-06 | MVP 완료 정의 작성 | 완료 판정 기준 통일 |
