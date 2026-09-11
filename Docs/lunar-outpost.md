# SELENE 07 달 정거장 맵

DirectX 12 프로젝트의 기존 렌더러와 충돌 시스템에 맞춘 플레이 가능한 외부 맵입니다. 구형 행성이 아니라 XZ 평면의 240 × 200m 플레이 구역이며, 정거장·다리·조명탑·화물 적재장·채굴 장비·착륙장·운석 충돌 지형을 배치했습니다.

## 실행

1. 솔루션에서 클라이언트와 서버를 x64 Debug로 빌드합니다.
2. `bin/x64/Debug/Kimgane.Client.exe`를 실행하고 타이틀의 로컬 게임을 선택합니다.
3. 네트워크 플레이는 동일한 최신 소스와 맵 데이터로 빌드한 서버를 사용합니다.

`Shared/Maps/LunarOutpost/LunarMapSettings.h`의 `ENABLED = true`가 달 맵을 선택합니다. `false`로 바꾸고 양쪽을 다시 빌드하면 기존 테스트 맵으로 돌아갑니다. 시작 위치는 `(0, 17.650660, 88)`이고 카메라 원거리 클립은 500m입니다.

## 파일 구조

| 경로 | 내용 |
| --- | --- |
| `Assets/Source/LunarOutpost/SELENE_07_Kimgane.blend` | 편집 가능한 Blender 원본 |
| `Assets/Models/LunarOutpost/*.txt` | 기존 FbxModelMesh 텍스트 로더용 13개 재질 묶음, 구조물 107,024 삼각형 |
| `Shared/Maps/LunarOutpost/lunar_height.raw` | 클라이언트·서버 공용 RAW16 지형 |
| `Shared/Maps/LunarOutpost/lunar_collision.txt` | 공용 정적 충돌: 박스 253개, 경사로 6개 |
| `Shared/Maps/LunarOutpost/LunarCollision.h` | 공용 충돌 파일 검사·로더 |
| `Shared/Maps/LunarOutpost/LunarMovement.h` | 서버의 지형·발판·경사로 바닥 판정과 이동 |
| `Shared/Maps/LunarOutpost/LunarMeshBatches.h` | 재질별 메시 경로와 표면·발광 값 |
| `Tools/export_lunar_outpost.py` | Blender에서 프로젝트 형식으로 재출력 |
| `Tools/Test-LunarMap.ps1` | 실제 맵 데이터를 읽는 C++ 검증 실행 |

## 좌표와 충돌

단위는 미터입니다. Blender `(x,y,z)`는 엔진 `(-x,z+16,-y)`로 변환합니다. 지형은 281 × 281개의 little-endian unsigned 16bit 샘플이며, 셀 간격 1m, 높이 배율 64m입니다. 지형 샘플 좌표는 월드 XZ에 각각 140을 더합니다. 외곽 지형은 플레이 영역보다 넓은 280 × 280m이며 4개 경계 박스가 이동 범위를 제한합니다.

`box 이름 cx cy cz ex ey ez`의 e는 반쪽 크기입니다. `ramp 이름 cx cy cz sx sy sz 방향`의 s는 전체 크기입니다. 방향은 PositiveX, NegativeX, PositiveZ, NegativeZ 중 하나이며 높은 쪽을 뜻합니다. 회전된 OBB나 삼각형 충돌이 아닌 기존 AABB·Ramp 시스템에 맞춘 단순 충돌입니다. 건물·큰 바위·화물·기둥·다리는 막고, 작은 장식은 충돌에서 제외합니다.

클라이언트는 기존 ColliderComponent와 로컬 플레이어 충돌 해석을 사용합니다. 서버는 같은 충돌 파일을 CollisionWorld에 등록하며, 0.30m 이내의 올라갈 수 있는 표면만 바닥으로 선택합니다. 다리 아래에서 다리 위로 순간 이동하지 않고 발판에서 벗어나면 낙하를 시작합니다. 수직 이동 결과를 확정한 뒤 위치를 전송합니다.

## 재출력

Blender 4.2에서 다음 명령을 실행합니다. 경로는 자신의 설치 위치에 맞춥니다.

```powershell
& 'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe' --background `
  '.\Assets\Source\LunarOutpost\SELENE_07_Kimgane.blend' `
  --python '.\Tools\export_lunar_outpost.py' -- --output-root $PWD.Path
```

출력은 해당 프로젝트의 달 맵 메시, RAW, 충돌, 재질 표, export_report.json을 덮어씁니다. 실행 중인 클라이언트·서버를 다시 시작해야 변경된 데이터를 읽습니다. 원본 blend를 저장하려면 `--save-blend`에 별도 경로를 지정합니다. 오브젝트 이름과 컬렉션 이름은 내보내기 필터에 사용되므로 유지합니다. 지형 크기·간격·높이 배율은 스크립트의 현재 규격을 유지하고, 시작 지형을 수정했으면 export_report의 spawn 값에 맞춰 LunarMapSettings.h도 갱신합니다.

## 검증

저장소 루트에서 `powershell -ExecutionPolicy Bypass -File .\Tools\Test-LunarMap.ps1`을 실행합니다. Visual Studio C++ 도구가 필요합니다. 클라이언트와 서버 RAW16 전체 샘플 및 보간 높이 일치, 시작 지점, 다리 위·아래 바닥, 경사로 6개, 시작점에서 다리까지 이동, 다리 이탈 낙하, 외곽 차단, 잘못된 충돌 파일 거부를 확인합니다.

x64 Debug 클라이언트·서버 빌드와 실제 클라이언트 맵 로드를 확인했습니다. 자동 검증은 서버 이동과 공유 충돌 수준이며, 다중 클라이언트 네트워크 플레이와 100인 부하 테스트를 대신하지 않습니다.

## 현재 표현과 범위

현재 엔진의 POSITION/NORMAL/COLOR 렌더링 경로에 맞춰 재질의 기본색을 정점 색으로 출력했습니다. FbxModelMesh가 `<Colors>`를 실제로 읽도록 수정했으며 색이 없는 기존 모델은 기존 프레임 색을 유지합니다. 금속도·거칠기·발광 값도 적용합니다. 블렌더 렌더의 텍스처/노멀맵, 조명탑의 실제 국소 광원, 그림자·블룸·우주 배경 효과는 현재 게임 렌더러에 구현되어 있지 않아 원본 렌더와 화면이 다릅니다.

정거장은 외관과 엄폐물 용도이며 건물 내부, 문 개폐, 아이템 상호작용, 포탈, 우주선 수리 기능은 포함하지 않습니다. NPC는 기존 지형 기반 이동을 유지하므로 구조물 회피 AI가 추가로 필요합니다. 충돌은 박스 근사이므로 둥근 외형 주변에서 여유가 있고, 발광 기둥 아래의 실제 조명은 후속 렌더러 작업이 필요합니다. 이번 변경은 맵 연결과 기본 이동 검증까지이며 대규모 인원용 공간 분할·LOD 최적화는 별도입니다.
