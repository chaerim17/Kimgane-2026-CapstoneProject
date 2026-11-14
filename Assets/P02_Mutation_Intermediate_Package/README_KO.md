# P02 중간 변이 DirectX 12 에셋

왼쪽 다리가 변이된 플레이어 모델의 대기 자세를 기존 졸업작품용 정적 TXT 형식으로 내보낸 패키지입니다.

| 단계 | 파일 | 삼각형 | 내보낸 정점 |
| --- | --- | ---: | ---: |
| LOD0 | `txt_dx/P02_Mutation_Idle_LOD0.txt` | 29,976 | 20,135 |
| LOD1 | `txt_dx/P02_Mutation_Idle_LOD1.txt` | 14,988 | 11,416 |
| LOD2 | `txt_dx/P02_Mutation_Idle_LOD2.txt` | 7,484 | 6,481 |
| LOD3 | `txt_dx/P02_Mutation_Idle_LOD3.txt` | 3,466 | 3,637 |

네 단계 모두 `textures/P02_Mutation_BaseColor.dds`를 사용합니다. 기존 장갑 색과 변이 조직의 색을 4096×4096 Base Color 아틀라스에 합쳤습니다. DDS는 BC1(DXT1) 형식이며 1×1까지 총 13개 밉 레벨을 포함합니다.

## 프로젝트에서 불러오기

이 폴더를 프로젝트의 `Assets` 아래에 복사한 경우, 현재 `TestSceneSettings.h`와 같은 확장자 생략 규칙으로 아래 경로를 사용합니다.

```cpp
L"Assets/P02_Mutation_Intermediate_Package/txt_dx/P02_Mutation_Idle_LOD0"
L"Assets/P02_Mutation_Intermediate_Package/textures/P02_Mutation_BaseColor"
```

모델은 `FbxModelMesh::Load`, 텍스처는 `Texture::Load`로 읽습니다. TXT의 좌표는 미터 단위의 왼손 좌표계이며 +Y가 위, +Z가 정면입니다. 현재 P02와 같은 발밑 원점을 기준으로 좌표, 삼각형 순서, UV를 변환했습니다.

LOD 파일은 네 개의 정적 메시입니다. 변이 상태에 따른 모델 선택과 거리에 따른 LOD 선택은 게임에서 연결하는 작업입니다.

## 검증과 출처

TXT의 정점·인덱스, 노멀, UV, 높이와 DDS 형식을 검사했습니다. 원형과 텍스처의 출처는 프로젝트의 P02 및 M06 패키지에 있는 `LICENSE_NOTES_KO.md`에 기록되어 있습니다.
