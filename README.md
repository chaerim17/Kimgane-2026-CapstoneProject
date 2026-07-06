# Kimgane-2026-CapstoneProject

DirectX 3D 기반 3인 팀 졸업작품 프로젝트입니다.

> 이 저장소는 졸업작품 개발과 포트폴리오 정리를 함께 목표로 합니다.
> 확정되지 않은 내용은 `TODO`로 남기고, 결정된 내용만 문서에 반영합니다.

## Project Overview

| 항목 | 내용 |
| --- | --- |
| 프로젝트명 | TODO |
| 장르 | TODO |
| 플랫폼 | Windows |
| 개발 환경 | Visual Studio 2026 |
| 개발 기간 | TODO |
| 팀 규모 | 3명 |
| 핵심 기술 | C++20, DirectX 12, Client, C++ Server |
| 저장소 목적 | 졸업작품 개발 및 포트폴리오 |

## Team

| 이름 | 담당 영역 | 주요 책임 |
| --- | --- | --- |
| 김영목 | Client | DirectX 12 클라이언트, WASD 이동, 서버 위치 기반 렌더링 |
| 김준해 | Client | 3인칭 카메라, Directional Light, 클라이언트 렌더링 |
| 김채림 | Server | 멀티플레이 동기화, 플레이어 위치 API |

## Documentation

| 문서 | 목적 |
| --- | --- |
| [CONTRIBUTING.md](CONTRIBUTING.md) | 브랜치, 커밋, PR, 리뷰, 코드 스타일 등 협업 규칙 |
| [Docs/development-environment.md](Docs/development-environment.md) | Visual Studio 2026, C++/DirectX 빌드 설정, 프로젝트 속성 기준 |
| [Docs/mvp-scope.md](Docs/mvp-scope.md) | 첫 번째 개발 목표, 완료 기준, 제외 범위 |
| [Docs/technical-decisions.md](Docs/technical-decisions.md) | DirectX, 서버, 네트워크, 빌드 설정 등 초기 기술 결정 |
| [Docs/engineering-units.md](Docs/engineering-units.md) | 거리, 시간, 각도, 좌표계, 밝기, 에셋 스케일 단위 |
| [Docs/dx12-bootstrap.md](Docs/dx12-bootstrap.md) | DirectX 12 초기화 순서와 첫 렌더링 체크리스트 |
| [Docs/definition-of-mvp.md](Docs/definition-of-mvp.md) | MVP 완료 조건, 시연 기준, 탈락 조건 |
| [Docs/github-labels.md](Docs/github-labels.md) | GitHub Issue/PR 라벨 기준 |
| [Docs/task-board.md](Docs/task-board.md) | 첫 주 작업 분배와 진행 상태 |
| [Docs/architecture.md](Docs/architecture.md) | 전체 시스템 구조, 모듈 책임, 데이터 흐름 |
| [Docs/network-protocol.md](Docs/network-protocol.md) | 클라이언트/서버 패킷, 동기화 규칙, 프로토콜 변경 이력 |
| [Docs/rendering-pipeline.md](Docs/rendering-pipeline.md) | DirectX 렌더링 파이프라인, 셰이더, 리소스 관리 |
| [Docs/troubleshooting.md](Docs/troubleshooting.md) | 빌드, 실행, 그래픽스, 네트워크 문제 해결 기록 |
| [Docs/asset-list.md](Docs/asset-list.md) | 외부 에셋 출처, 라이선스, 사용 위치 |

## Tech Stack

| 영역 | 기술 |
| --- | --- |
| Client | C++20 |
| Graphics | DirectX 12 |
| Server | C++20 |
| Network | TCP/IP, Network API TODO, TCP for MVP |
| IDE | Visual Studio 2026 |
| Version Control | Git, GitHub TODO |
| Asset Tool | TODO |

## Repository Structure

초기 목표 구조입니다. 실제 폴더는 프로젝트 생성 후 점진적으로 추가합니다.

```text
/
  KimganeCapstone.sln
  Client/
    Kimgane.Client.vcxproj
    src/
      Engine/
        Core/
  Server/        # TODO
    Core/
    Network/
    GameLogic/
  Shared/        # TODO
  Assets/
    Models/
    Textures/
    Shaders/
    Sounds/
  Docs/
  .github/
  README.md
  CONTRIBUTING.md
```

## Getting Started

TODO: 솔루션 생성 후 정확한 실행 절차를 작성합니다.

```text
1. Repository clone
2. Visual Studio 2026 설치 확인
3. Windows SDK 10.0.26100.0 및 C++ Desktop workload 확인
4. `KimganeCapstone.sln` 열기
5. `Debug|x64` 또는 `Release|x64` 선택
6. `Kimgane.Client` 빌드
7. `Kimgane.Client` 실행
```

## Milestone

| 기간 | 목표 | 상태 |
| --- | --- | --- |
| TODO | 프로젝트 세팅 | TODO |
| TODO | DirectX 기본 렌더링 | TODO |
| TODO | 플레이어 이동 | TODO |
| TODO | 클라이언트-서버 위치 동기화 | TODO |
| TODO | 게임플레이 구현 | TODO |
| TODO | 최적화 및 폴리싱 | TODO |
| TODO | 발표 및 포트폴리오 정리 | TODO |

## README 작성 원칙

- 프로젝트의 현재 상태를 기준으로 작성합니다.
- 구현되지 않은 기능은 완료된 것처럼 적지 않습니다.
- 결정되지 않은 내용은 `TODO`로 표시합니다.
- 협업 규칙은 [CONTRIBUTING.md](CONTRIBUTING.md)에 정리합니다.
- 세부 기술 문서는 `Docs/` 아래에 주제별로 작성합니다.
- 포트폴리오로 사용할 수 있도록 기술 선택 이유, 문제 해결 과정, 결과 지표를 함께 기록합니다.

## Portfolio Notes

프로젝트 종료 시 아래 내용을 정리합니다.

- 핵심 기술 선택 이유
- 구현한 시스템 구조
- 가장 어려웠던 문제와 해결 과정
- 성능 측정 결과
- 팀 내 본인 기여도
- 시연 영상 및 스크린샷

## License

TODO
