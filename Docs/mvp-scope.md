# MVP Scope

이 문서는 첫 번째 개발 목표를 작게 고정하기 위한 기준입니다. MVP는 완성 게임이 아니라, 팀의 Client/Server/DirectX 작업이 실제로 연결되는 최소 단위입니다.

## MVP Goal

20x20m 테스트 공간에서 플레이어가 WASD로 이동하고, 서버가 플레이어 위치를 관리하며, 클라이언트가 서버에서 받은 위치를 기준으로 플레이어를 렌더링합니다.

## Core Scenario

```text
1. Server 실행
2. Client 1 실행 후 서버 접속
3. Client 2 실행 후 서버 접속
4. Client에서 WASD 입력
5. Client가 입력 또는 위치 정보를 Server에 전송
6. Server가 플레이어 상태를 갱신
7. Server가 각 Client에 플레이어 위치를 전달
8. 각 Client가 자기 자신과 다른 플레이어 위치를 렌더링
```

## MVP Features

| 영역 | 기능 | 완료 기준 |
| --- | --- | --- |
| Client | Windows 창 생성 | 실행 시 창이 정상 생성되고 종료 가능 |
| Client | DirectX 초기화 | Clear Color 출력 및 Present 성공 |
| Client | 3인칭 카메라 | 플레이어를 기준으로 화면 구성 |
| Client | WASD 입력 | 입력에 따라 이동 의도 생성 |
| Client | 플레이어 렌더링 | 최소 박스, 캡슐, 임시 모델 중 하나로 표시 |
| Client | 위치 수신 반영 | 서버에서 받은 위치로 플레이어 표시 |
| Renderer | 20x20m 테스트 공간 | Height Map 또는 임시 평면 표시 |
| Renderer | Directional Light | 기본 조명 적용 |
| Server | 클라이언트 접속 | 2개 이상의 로컬 클라이언트 접속 가능 |
| Server | 플레이어 ID 관리 | 접속한 클라이언트에 고유 ID 부여 |
| Server | 위치 상태 관리 | 플레이어별 위치 저장 |
| Server | 위치 브로드캐스트 | 각 클라이언트에 플레이어 위치 전달 |
| Shared | 패킷 정의 | Client/Server가 같은 구조 기준으로 통신 |

## Non-Goals

MVP 단계에서는 아래 기능을 구현하지 않습니다.

- 로그인, 회원가입, DB 저장
- 매칭, 로비, 방 시스템
- 전투, 총기, 스킬, 아이템
- 인벤토리
- 몬스터 AI
- 완성 모델, 완성 애니메이션
- 복잡한 물리 시뮬레이션
- 지형 편집 툴
- 사운드 시스템
- 최종 UI

## Success Criteria

MVP 완료 기준:

- `Debug x64` 빌드 성공
- Server 1개, Client 2개를 로컬에서 실행 가능
- 한 클라이언트에서 이동하면 다른 클라이언트 화면에도 위치 변화가 보임
- 최소 3분 동안 크래시 없이 실행
- 패킷 구조가 `Docs/network-protocol.md`에 기록됨
- 구현 중 발생한 주요 문제는 `Docs/troubleshooting.md`에 기록됨

세부 완료 기준은 `Docs/definition-of-mvp.md`에서 관리합니다.

## Demo Target

MVP 시연에서 보여줄 화면:

```text
Server Console:
  client connected
  player position updated

Client:
  20x20m test field
  local player
  remote player
  WASD movement
  third-person camera
```

## MVP Milestones

| 단계 | 목표 | 담당 | 상태 |
| --- | --- | --- | --- |
| MVP-01 | Visual Studio 솔루션 및 프로젝트 생성 | TODO | TODO |
| MVP-02 | DirectX 창, 디바이스, 렌더 루프 구성 | TODO | TODO |
| MVP-03 | 테스트 공간, 카메라, 플레이어 표시 | TODO | TODO |
| MVP-04 | Server 접속 및 세션 관리 | TODO | TODO |
| MVP-05 | PlayerPosition 패킷 송수신 | TODO | TODO |
| MVP-06 | 두 클라이언트 위치 동기화 | TODO | TODO |
| MVP-07 | MVP 시연 영상 및 README 업데이트 | TODO | TODO |

## Change Log

| 날짜 | 변경 내용 | 이유 |
| --- | --- | --- |
| 2026-07-06 | MVP 기준선 작성 | 개발 시작 전 팀 목표 통일 |
