# Technical Decisions

이 문서는 개발 시작 전에 맞춰야 하는 기술 기준을 기록합니다. 변경은 가능하지만, 변경 시 이유와 영향을 남깁니다.

## Current Baseline

| 항목 | 결정 | 이유 |
| --- | --- | --- |
| IDE | Visual Studio 2026 | 팀 공통 개발환경 |
| OS | Windows 10/11 | DirectX 및 Visual Studio 기준 |
| Client Language | C++20 | DirectX 개발과 현대 C++ 기능 사용 |
| Server Language | C++20 | Client/Server 공용 타입 관리와 학습 비용 절감 |
| Graphics API | DirectX 12 | 실제 개발환경과 포트폴리오 기술 깊이 반영 |
| Network Environment | TCP/IP | 클라이언트/서버 통신 기준 |
| Network API | TODO | Winsock2, ASIO, IOCP 등 후보 검토 필요 |
| MVP Transport | TCP | 초기 디버깅과 패킷 검증 단순화 |
| Post-MVP Network Review | UDP 검토 | 실시간 위치 동기화 품질 개선 가능성 |
| Build Platform | x64 | 64비트 Windows 기준 |
| Build Configurations | Debug, Release | 개발/시연 빌드 분리 |
| Branch Strategy | main / develop / feature | 안정 버전과 개발 버전 분리 |
| Member Variable Naming | `mName` | C++ 클래스 코드 스타일 통일 |
| Constant Naming | `UPPER_SNAKE_CASE` | 상수와 일반 변수 구분 |

## Visual Studio Project Settings

| 설정 | 기준 | 비고 |
| --- | --- | --- |
| C++ Language Standard | ISO C++20 | `/std:c++20` |
| Warning Level | Level4 | `/W4` |
| SDL Checks | Yes | `/sdl` |
| Conformance Mode | Yes | `/permissive-` |
| Multi-processor Compilation | Yes | `/MP` |
| Character Set | Unicode | Windows API 기준 |
| Runtime Library Debug | Multi-threaded Debug DLL | `/MDd` |
| Runtime Library Release | Multi-threaded DLL | `/MD` |
| Treat Warnings As Errors | No for MVP | MVP 이후 모듈별 Yes 검토 |

## Architecture Decisions

| 결정 | 내용 | 이유 |
| --- | --- | --- |
| Server Authority | 서버가 플레이어 위치의 기준 상태를 관리 | 멀티플레이 동기화 기준 확보 |
| Shared Types | 패킷과 공용 타입은 `Shared/`에서 관리 예정 | Client/Server 구조 불일치 방지 |
| MVP Rendering | 임시 지형과 임시 플레이어 모델 허용 | 네트워크 연동 우선 |
| Asset Policy | 외부 에셋은 출처와 라이선스 기록 | 발표/포트폴리오 리스크 방지 |
| Documentation | 패킷, 빌드 설정, 에셋 변경은 문서 동시 수정 | 팀 통일성 유지 |
| Framework Import Policy | 기존 프레임워크는 구조를 참고하되 현재 프로젝트 기준으로 재설계 | 불필요한 의존성 유입 방지 |
| PCH Policy | `Pch.h`에는 Windows, DirectX, STL 등 안정적인 외부 헤더만 포함 | 프로젝트 내부 의존성 은닉 방지 |
| Settings Header Policy | 도메인별 `*Settings.h`에 기본값과 상수를 분리 | 매직 넘버 축소와 설정 위치 통일 |

## DirectX Decision

초기 그래픽스 API는 DirectX 12로 진행합니다.

선택 이유:

- 실제 개발환경이 DirectX 12 기준입니다.
- 커맨드 큐, 커맨드 리스트, 디스크립터 힙, 리소스 배리어 등 저수준 그래픽스 구조를 포트폴리오에서 명확히 보여줄 수 있습니다.
- DirectX 11보다 초기 구현 부담은 크지만, 렌더링 파이프라인 이해도를 증명하기 좋습니다.

주의할 점:

- MVP 범위를 작게 유지해서 DirectX 12 초기화와 네트워크 통합을 동시에 과도하게 키우지 않습니다.
- Debug Layer를 켜고 경고를 `Docs/troubleshooting.md`에 기록합니다.
- Command Allocator, Command List, Fence, Frame Resource 관리 규칙을 `Docs/rendering-pipeline.md`에 계속 업데이트합니다.

## Network Decision

MVP는 TCP/IP 환경에서 TCP로 진행합니다.

선택 이유:

- 접속, 패킷 경계, 디버깅 흐름을 먼저 안정화하기 쉽습니다.
- 로컬 2클라이언트 위치 동기화 시연에는 충분합니다.
- 이후 UDP 전환 여부를 실제 측정 후 판단할 수 있습니다.

Post-MVP 검토:

- 이동 동기화는 UDP snapshot 방식 검토
- 중요 이벤트는 TCP 또는 reliable layer 검토
- 지연 보정, 보간, 예측은 MVP 이후 설계

## Decision Log

| 날짜 | 결정 | 상태 | 영향 |
| --- | --- | --- | --- |
| 2026-07-06 | MVP는 DirectX 12, C++20, C++ Server, TCP/IP 기준으로 시작 | Active | 첫 솔루션/프로젝트 설정 기준 |
| 2026-07-06 | 멤버 변수는 `mName`, 상수는 `UPPER_SNAKE_CASE`로 통일 | Active | 신규 C++ 클래스 네이밍 기준 |
| 2026-07-06 | NexonGameJam 프레임워크는 복사보다 모듈 단위 재설계 방식으로 이식 | Active | Mesh, Collider, Camera, Scene을 현재 구조에 맞춰 분리 |
| 2026-07-06 | PCH와 도메인별 Settings 헤더를 사용 | Active | 빌드 속도와 설정값 관리 개선 |

## Engine Decisions

| Date | Decision | Status | Impact |
| --- | --- | --- | --- |
| 2026-07-06 | `Mesh` uses one vertex/index geometry path for generated terrain and imported models. | Active | Prevent duplicate render paths before asset import grows. |
| 2026-07-06 | `GameClock` is the client frame delta source. | Active | Physics and network tick policy can be layered on top later. |
| 2026-07-06 | Terrain data is shared by render mesh and collider components. | Active | RAW height maps can be tested without duplicating sampling rules. |
| 2026-07-06 | Physics keeps movement integration in `RigidbodyComponent` and collision queries in `CollisionManager`. | Active | Components stay small enough to extend with server authority later. |
| 2026-07-06 | FBX MVP import uses converted text beside the `.fbx` source. | Active | Avoids locking the project to FBX SDK until the team agrees on external dependencies. |
| 2026-07-06 | Bootstrap lighting uses one directional light and root constants. | Active | Keeps DX12 setup simple until descriptor heaps and material buffers are introduced. |
| 2026-07-06 | New C++ naming policy uses member variables as `mName` and constants as `UPPER_SNAKE_CASE`. | Active | New code follows this rule; legacy code should be migrated in a separate refactor branch. |
| 2026-07-06 | Runtime lighting should enter scenes through `DirectionalLightComponent`. | Active | Keeps rendering behavior aligned with the component architecture. |

## Open Questions

| 항목 | 선택지 | 결정 필요 시점 |
| --- | --- | --- |
| ECS/DOD 적용 범위 | MVP 이후 적용 / 초기부터 적용 | MVP 렌더링 구조 완성 전 |
| 외부 라이브러리 관리 | 직접 포함 / vcpkg / submodule | 외부 라이브러리 도입 전 |
