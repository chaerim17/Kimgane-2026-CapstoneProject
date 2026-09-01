# Development Environment

이 문서는 팀원이 같은 Visual Studio 설정으로 빌드하고 디버깅하기 위한 기준입니다. 프로젝트 속성은 솔루션 생성 후 실제 값에 맞춰 계속 업데이트합니다.

## Required Environment

| 항목 | 기준 |
| --- | --- |
| OS | Windows 10/11 |
| IDE | Visual Studio 2026 |
| Workload | Desktop development with C++ |
| Language | C++20 |
| Graphics API | DirectX 12 |
| Windows SDK | 10.0.26100.0 |
| Platform Toolset | v145 |
| Build Platform | x64 |
| Configuration | Debug, Release |
| Version Control | Git |

## Visual Studio Installation Checklist

Visual Studio Installer에서 아래 항목을 확인합니다.

- `Desktop development with C++`
- Latest installed Windows SDK
- MSVC vNext 또는 프로젝트에서 사용하는 MSVC toolset
- C++ CMake tools for Windows, 사용하는 경우
- Git for Windows, 필요한 경우
- Graphics Tools / DirectX 관련 디버깅 도구

## Solution Rule

| 항목 | 기준 |
| --- | --- |
| Solution 이름 | KimganeCapstone |
| Client 프로젝트 | Kimgane.Client |
| Server 프로젝트 | TODO |
| Shared 프로젝트 | TODO |
| 시작 프로젝트 | Kimgane.Client |
| 기본 플랫폼 | `x64` |
| 기본 구성 | `Debug` |

규칙:

- 팀원 모두 `x64` 기준으로 빌드합니다.
- `Win32` 구성은 필요할 때만 유지합니다.
- 개인별 `.user` 설정 파일은 Git에 올리지 않습니다.
- 프로젝트 파일을 수정한 경우 PR에 변경 이유를 적습니다.
- 빌드 설정 변경은 이 문서에 기록합니다.

## Project Property Baseline

Visual Studio 프로젝트 속성에서 기준값을 아래처럼 관리합니다.

### General

| 설정 | Debug | Release | 비고 |
| --- | --- | --- | --- |
| Configuration Type | Application | Application | Client 기준 |
| Windows SDK Version | 10.0.26100.0 | 10.0.26100.0 | 팀 공통 버전 |
| Platform Toolset | v145 | v145 | Visual Studio 2026 |
| C++ Language Standard | ISO C++20 | ISO C++20 | `/std:c++20` |
| Character Set | Unicode | Unicode | Windows API 사용 기준 |

### C/C++ - General

| 설정 | Debug | Release | 비고 |
| --- | --- | --- | --- |
| Additional Include Directories | `$(ProjectDir)src` | `$(ProjectDir)src` | PCH 및 엔진 하위 헤더 include 기준 |
| Warning Level | Level4 | Level4 | `/W4` 권장 |
| SDL Checks | Yes | Yes | `/sdl` 사용 여부. 필요 시 사유 기록 |
| Treat Warnings As Errors | No | No | MVP 이후 모듈별 Yes 검토 |
| Multi-processor Compilation | Yes | Yes | `/MP` |
| Debug Information Format | Program Database | Program Database | `/Zi` 또는 `/ZI` |
| Precompiled Header | `Pch.h` | `Pch.h` | Windows/DirectX/STL 안정 헤더만 포함 |

### C/C++ - Language

| 설정 | Debug | Release | 비고 |
| --- | --- | --- | --- |
| Conformance Mode | Yes | Yes | `/permissive-` |
| C++ Exceptions | Yes | Yes | MVP에서는 기본값 유지 |
| Runtime Type Information | Yes | Yes | MVP에서는 기본값 유지 |

### C/C++ - Preprocessor

| 설정 | Debug | Release | 비고 |
| --- | --- | --- | --- |
| Preprocessor Definitions | `WIN32;_DEBUG;_WINDOWS` | `WIN32;NDEBUG;_WINDOWS` | Client 기준 |

### C/C++ - Code Generation

| 설정 | Debug | Release | 비고 |
| --- | --- | --- | --- |
| Runtime Library | Multi-threaded Debug DLL | Multi-threaded DLL | `/MDd`, `/MD` |
| Enable Enhanced Instruction Set | TODO | TODO | 필요 시 결정 |
| Floating Point Model | Precise | Precise | DirectX 수학 연산 기준 |

### Linker - General

| 설정 | Debug | Release | 비고 |
| --- | --- | --- | --- |
| Additional Library Directories | TODO | TODO | 외부 라이브러리 추가 시 기록 |
| Output File | `bin/x64/Debug/Kimgane.Client.exe` | `bin/x64/Release/Kimgane.Client.exe` | 빌드 산출물 위치 통일 |
| Enable Incremental Linking | Yes | No | Debug 빠른 링크, Release 최적화 |

### Linker - Input

| 라이브러리 | 사용 여부 | 비고 |
| --- | --- | --- |
| `d3d11.lib` | No | DirectX 11 호환 코드가 필요할 때만 검토 |
| `d3d12.lib` | Yes | DirectX 12 |
| `dxgi.lib` | Yes | Swap Chain, Adapter |
| `d3dcompiler.lib` | Yes | 초기 단색 셰이더 런타임 컴파일용. DXC 전환 시 재검토 |
| `dxcompiler.lib` | TODO | DXC 기반 HLSL 컴파일 사용 시 |
| `DirectXTex.lib` | TODO | 텍스처 처리 사용 시 |
| `Ws2_32.lib` | TODO | Winsock2 사용 시 필요 |

### Debugging

| 설정 | 값 | 비고 |
| --- | --- | --- |
| Working Directory | TODO | Visual Studio 디버깅 설정에서 확정 |
| Command Arguments | TODO | 서버 IP, 포트 등 |
| Environment | TODO | 로컬 환경변수 |

## SDL Checks Policy

현재 기준:

| 프로젝트 | Debug | Release | 결정 |
| --- | --- | --- | --- |
| Client | Yes | Yes | Active |
| Server | Yes | Yes | Active |
| Shared | Yes | Yes | Active |

원칙:

- 기본값은 `Yes`로 둡니다.
- 특정 경고나 성능 문제 때문에 끄는 경우 PR에 사유를 적습니다.
- 설정을 바꾸면 이 문서의 변경 이력에 남깁니다.

## Build Verification

작업 완료 전 최소 확인:

```text
1. Clean Build
2. Debug x64 build
3. Release x64 build
4. Client 단독 실행
5. Server 단독 실행
6. Client/Server 연결 확인, 관련 작업인 경우
```

## Runtime Configuration

실행 설정은 코드에 하드코딩하지 않고 설정 파일 또는 실행 인자로 분리하는 것을 목표로 합니다.

| 항목 | 기본값 | 비고 |
| --- | --- | --- |
| Server IP | `127.0.0.1` | 로컬 테스트 |
| Server Port | `27015` | MVP 기본 포트, 충돌 시 변경 |
| Window Width | `1280` | Client 초기값 |
| Window Height | `720` | Client 초기값 |
| Fullscreen | `false` | MVP에서는 창 모드 |
| Asset Root | TODO | 실행 기준 경로와 맞추기 |

## External Dependencies

| 라이브러리 | 버전 | 관리 방식 | 사용 위치 | 비고 |
| --- | --- | --- | --- | --- |
| TODO | TODO | TODO | TODO | TODO |

관리 원칙:

- 외부 라이브러리는 버전을 고정합니다.
- 설치 방법을 README 또는 이 문서에 남깁니다.
- 팀원 PC마다 다른 경로를 직접 참조하지 않습니다.
- 가능하면 상대 경로, 환경변수, 패키지 매니저 사용을 검토합니다.

## Environment Change Log

| 날짜 | 변경자 | 변경 내용 | 이유 |
| --- | --- | --- | --- |
| 2026-07-06 | Codex | C++20, DirectX 12, TCP/IP, Network API TODO, SDL Checks Yes 기준 작성 | 개발 시작 전 환경 통일 |
| 2026-07-06 | Codex | KimganeCapstone 솔루션과 Kimgane.Client 프로젝트 생성, Windows SDK 10.0.26100.0, toolset v145 반영 | Client 프로젝트 생성 |
| 2026-07-06 | Codex | `d3dcompiler.lib` 링크 추가 | 초기 HLSL 셰이더 컴파일 |
| 2026-07-06 | Codex | `Pch.h/Pch.cpp`와 `$(ProjectDir)src` include 경로 추가 | DirectX/Windows 헤더 빌드 비용 완화 |
| 2026-09-01 | Codex | Client/Server 프로젝트가 `Shared/Physics` 공용 물리 소스를 빌드하도록 구성 | 서버 권위 물리 계산과 클라 예측 이동이 같은 코어를 재사용 |
