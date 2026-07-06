# Task Board

GitHub Projects를 사용하기 전까지 이 문서를 임시 태스크보드로 사용합니다. GitHub Issues를 사용하기 시작하면 이 문서는 전체 흐름 요약용으로 유지합니다.

## Board Rule

- 작업은 `Todo`, `In Progress`, `Review`, `Done` 중 하나의 상태를 가집니다.
- 한 사람이 동시에 진행하는 작업은 1~2개로 제한합니다.
- 작업이 막히면 `Blocked`를 표시하고 이유를 남깁니다.
- 완료된 작업은 관련 PR, 커밋, 문서를 연결합니다.

## Status

| 상태 | 의미 |
| --- | --- |
| Todo | 아직 시작하지 않음 |
| In Progress | 담당자가 작업 중 |
| Review | 리뷰 또는 통합 확인 필요 |
| Done | 완료 |
| Blocked | 외부 결정이나 다른 작업 대기 |

## First Week Tasks

| ID | 작업 | 담당 | 상태 | 완료 기준 |
| --- | --- | --- | --- | --- |
| SETUP-01 | Visual Studio 솔루션 구조 생성 | TODO | Todo | Client, Server, Shared 프로젝트 기준 생성 |
| SETUP-02 | Debug/Release x64 설정 확인 | TODO | Todo | Visual Studio 2026에서 Debug x64 빌드 가능 |
| CLIENT-01 | Win32 창 생성 | TODO | Todo | 창 생성, 메시지 루프, 종료 처리 |
| CLIENT-02 | DirectX 12 디바이스/스왑체인 초기화 | TODO | Todo | Clear Color 출력 및 Present 성공 |
| CLIENT-03 | 게임 루프와 deltaTime 계산 | TODO | Todo | Update/Render 흐름 분리 |
| CLIENT-04 | WASD 입력 처리 | TODO | Todo | 이동 입력 상태를 매 프레임 갱신 |
| CLIENT-05 | 3인칭 카메라 초안 | TODO | Todo | 플레이어 기준 카메라 위치 계산 |
| RENDER-01 | 20x20m 테스트 공간 표시 | TODO | Todo | 임시 평면 또는 Height Map 렌더링 |
| RENDER-02 | Directional Light 기본 적용 | TODO | Todo | 조명 방향에 따라 기본 음영 확인 |
| SERVER-01 | TCP/IP 서버 부팅 | TODO | Todo | 로컬 포트에서 listen 가능 |
| SERVER-02 | 클라이언트 접속/해제 처리 | TODO | Todo | 2개 클라이언트 접속 로그 확인 |
| SERVER-03 | 플레이어 ID와 위치 상태 저장 | TODO | Todo | 접속별 playerId와 position 관리 |
| NETWORK-01 | 공용 패킷 헤더 정의 | TODO | Todo | `Docs/network-protocol.md`와 코드 기준 일치 |
| NETWORK-02 | PlayerPosition 송수신 | TODO | Todo | Client -> Server 위치 전달 |
| NETWORK-03 | PlayerSnapshot 브로드캐스트 | TODO | Todo | Server -> Clients 위치 전달 |
| DOCS-01 | MVP 진행 결과 기록 | TODO | Todo | 구현 후 README와 Docs 업데이트 |

## Suggested Ownership

| 담당 영역 | 추천 첫 작업 |
| --- | --- |
| 김영목 | `CLIENT-01`, `CLIENT-02`, `CLIENT-03` |
| 김준해 | `CLIENT-04`, `CLIENT-05`, `RENDER-01`, `RENDER-02` |
| 김채림 | `SERVER-01`, `SERVER-02`, `SERVER-03`, `NETWORK-01` |
| Together | `NETWORK-02`, `NETWORK-03`, MVP 통합 테스트 |

## Review Queue

| ID | PR/Branch | 확인할 내용 | 상태 |
| --- | --- | --- | --- |
| TODO | TODO | TODO | TODO |

## Done

| ID | 완료 내용 | 관련 문서/PR |
| --- | --- | --- |
| DOCS-SETUP-01 | README, 협업 규칙, 개발환경, MVP/기술결정/태스크보드 문서 추가 | TODO |
