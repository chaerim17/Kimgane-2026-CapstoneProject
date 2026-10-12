# 현재 프로젝트용 정적 렌더링 TXT

`FbxModelMesh.cpp`의 `<Frame>:` / `<Mesh>:` / `<Positions>:` / `<Normals>:` / `<Indices>:` 구조에 맞춰 변환했다. OBJ가 아닌 자체 변환 메시 형식이다. 4단계 LOD의 A 포즈와 팔을 내린 Idle 정지 자세 1개를 제공한다.

| 파일 | 자세 | 삼각형 |
|---|---|---:|
| P02_Player_Idle_LOD0.txt | 팔을 내린 정지 자세, 첫 적용 권장 | 28,000 |
| P02_Player_LOD0.txt | A 포즈 | 28,000 |
| P02_Player_LOD1.txt | A 포즈 | 14,000 |
| P02_Player_LOD2.txt | A 포즈 | 7,000 |
| P02_Player_LOD3.txt | A 포즈 | 3,500 |

## 적용 순서

1. `P02_Player_Idle_LOD0.txt`를 사용자가 프로젝트의 `Assets/Models/`에 복사한다.
2. `Client/src/Engine/Scene/TestSceneSettings.h`의 모델 경로를 다음처럼 지정한다.

```cpp
inline constexpr wchar_t PLAYER_MODEL_PATH[] = L"Assets/Models/P02_Player_Idle_LOD0.txt";
```

3. 기존 `FbxModelMesh::Load()` 경로로 형상을 표시한다. 파일은 이미 미터 단위, 왼손 좌표계, +Y 위/+Z 정면이다. 높이 약 1.86m, 원점은 발바닥 기준이며 변환 행렬은 단위 행렬이다. OBJ에 적용하던 Z 반전·정점 순서 반전·V 반전을 이 TXT에 다시 적용하지 않는다.

LOD 파일은 엔진에서 선택하는 기능을 추가해야 전환된다. Idle 파일은 단일 정지 자세이며 애니메이션 클립이 아니다. 정지 자세의 최하단은 약간 변할 수 있다.

## 텍스처 적용 시 필요한 보강

각 파일에 `<TextureCoords0>: 정점수`와 UV 쌍을 보존했다. V는 이미 `1-v`로 바뀐 값이다. 다만 현재 프로젝트의 `FbxModelMesh.cpp`는 이 항목을 건너뛰고 정점의 UV를 `(0,0)`으로 채운다. 텍스처를 연결하기 전, 로더에서 이 UV 배열을 읽어 각 `Mesh::Vertex::uv`에 넣어야 한다. TXT 파일만 복사한 현재 경로는 형상 확인용이다.

텍스처는 `../textures/P02_BaseColor.png`를 DDS로 변환해 별도로 연결한다. 이 TXT에는 텍스처 경로나 재질 자동 연결 기능이 없다. 현재 로더는 프레임 이름으로 정점 색을 만들어서, 정확한 텍스처 색상을 확인할 때에는 로더의 정점 색을 흰색으로 설정해야 한다. 노멀·거칠기·금속성 맵에는 별도의 셰이더 지원이 필요하다.

## 이후 애니메이션

이 TXT에는 뼈·가중치·애니메이션이 포함되지 않는다. `../fbx/`의 FBX와 `../rig/`, `../skinned_dx/`의 데이터를 보관하고 스키닝을 구현할 때 사용한다. 특히 Idle TXT는 이미 자세가 변형된 정적 메시이므로 스키닝의 기준 메시로 사용하지 않는다.

원본 OBJ 대비 좌표·노멀·UV·삼각형 순서, 인덱스 범위와 퇴화 면을 검사했다. 실제 게임 엔진 실행 검증은 아직 하지 않았다. 게임 프로젝트는 수정하지 않았으며, 생성물의 출처 및 이용 조건은 상위 폴더의 `LICENSE_NOTES_KO.md`를 따른다.
