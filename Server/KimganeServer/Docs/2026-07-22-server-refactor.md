# 26-07-22 서버 리팩토링 보고서

## 개요

기존에는 대부분의 네트워크 및 세션 로직이 Main.cpp에 집중되어 있었으나,
리팩토링을 통해 역할별로 코드를 분리하고 네트워크 처리 구조를 정리하였다.

---

## 1차 리팩토링

### 목표

- Main.cpp 의존성 감소
- 역할별 코드 분리
- 서버 구조 단순화

### 작업 내용

- Session 클래스를 Session.h / Session.cpp로 분리
- Server 클래스를 Server.h / Server.cpp로 분리
- PCH(Pch.h, Pch.cpp) 추가
- ServerConfig.h 추가
- Main.cpp가 엔트리 포인트 역할만 수행하도록 수정
- Initialize(), Run() 함수 분리
- TimerThread() 분리
- error_display() 분리
- HandleAccept() 분리
- HandleRecv() 분리
- HandleDisconnect() 분리

### 결과

기존 Main.cpp 중심 구조에서 Server와 Session 중심 구조로 개선하였다.

---

## 2차 리팩토링

### 목표

- 패킷 처리 책임 분리
- Session 캡슐화 시작

### 작업 내용

- PacketHandler 클래스 추가
- 로그인 패킷 처리 분리
- 이동 시작 패킷 처리 분리
- 이동 종료 패킷 처리 분리
- Session::ProcessPacket() 단순화
- Connect() 추가
- Disconnect() 추가
- IsConnected() 추가
- GetId() 추가
- GetSocket() 추가

### 결과

패킷 처리 로직과 Session 관리 로직을 분리하여
향후 패킷 확장에 대비할 수 있는 구조를 구축하였다.

---

## 수정 사항

### AvatarInfo 위치 동기화 문제 수정

**문제**
- AvatarInfo 패킷 수신 시 위치 업데이트 큐에 등록되지 않음
- 로컬 플레이어 초기 위치가 정상 반영되지 않음

**해결**
- AvatarInfo 수신 시 LocationUpdate 큐에 등록하도록 수정

---

## 현재 구조

```text
Server
│
├─ Core
│  ├─ Server.h
│  ├─ Server.cpp
│  ├─ Session.h
│  └─ Session.cpp
│
├─ Network
│  ├─ PacketHandler.h
│  └─ PacketHandler.cpp
│
├─ Config
│  └─ ServerConfig.h
│
├─ Pch.h
├─ Pch.cpp
└─ Main.cpp
```

---

## 향후 개선 예정 (TODO)

- extern clients 전역 변수 제거
- Session 수신 버퍼 캡슐화
  - mRecvOver
  - mPrevRecv
- PacketHandler와 Session 의존성 정리
- Server가 Session 컨테이너를 직접 소유하도록 구조 개선
- 서버 HeightMap 연동
- PlayerState 구조 분리 검토
