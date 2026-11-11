# NR01 네레아 정화기

네레아의 문명 붕괴 이후에도 작동하는 자동 정화 설비를 상정한 배치용 에셋입니다. 사용자 스케치의 전면 경사로와 원형 발판을 기준으로 설계했습니다. 오염 정화량과 비용은 통합기획서에서 조정 항목으로 남겨둔 상태이므로 모델에 수치를 부여하지 않았습니다.

## 모델과 텍스처

| 단계 | 파일 | 삼각형 |
| --- | --- | ---: |
| LOD0 | `txt_dx/NR01_Purifier_LOD0.txt` | 10,960 |
| LOD1 | `txt_dx/NR01_Purifier_LOD1.txt` | 7,192 |
| LOD2 | `txt_dx/NR01_Purifier_LOD2.txt` | 3,276 |
| LOD3 | `txt_dx/NR01_Purifier_LOD3.txt` | 2,732 |

모든 TXT는 `textures/NR01_Purifier_BaseColor.dds`를 공유합니다. 2048×2048 BC1 텍스처와 전체 밉 체인을 포함합니다. 게임용 DDS는 표시등 색이 들어 있는 Base Color입니다.

## 배치와 상호작용

모델의 외곽은 약 3.13m × 5.22m, 최고 높이는 2.16m입니다. 발판 보행면은 약 0.64m이고 접근 경사로는 폭 2.16m, 길이 2.44m, 기울기 약 14.1도입니다.

TXT는 미터 단위 DirectX 왼손 좌표계로 변환했습니다. +Y가 위이고 +Z가 경사로 입구 방향입니다. 원점은 설비 기초 높이의 지면입니다. 폴더를 프로젝트의 `Assets` 아래에 복사하면 기존 `FbxModelMesh::Load`와 `Texture::Load`에서 아래 경로를 사용할 수 있습니다.

```cpp
L"Assets/NR01_Nerea_Purifier_Package/txt_dx/NR01_Purifier_LOD0"
L"Assets/NR01_Nerea_Purifier_Package/textures/NR01_Purifier_BaseColor"
```

- `collision/NR01_Purifier_collision.txt`에 발판 기초와 후면 설비의 AABB를 저장했습니다.
- 경사로는 `collision/NR01_Purifier_placement.json`의 치수로 클라이언트와 서버의 기존 Ramp 충돌 타입에 등록합니다. 로컬 중심은 `(0, 0.3325, 2.35)`, 전체 크기는 `(2.16, 0.615, 2.44)`, 오르막 방향은 `NegativeZ`입니다. 기존 박스 TXT 로더는 AABB만 읽으므로 이 램프 값은 별도로 사용합니다.
- F 상호작용의 기준 위치는 발판 중심 `(0, 0.642, 0.035)`입니다. 발 위치 기준 반경 0.73m와 높이 허용 범위 ±0.14m는 배치용 제안값입니다.

네 LOD의 TXT를 다시 읽어 정점·인덱스·노멀·UV를 확인하고, DDS를 불러왔습니다. 경사로 중앙과 좌우 보행면을 각 LOD에서 확인했습니다. 맵 등록, F 입력 처리와 실제 정화 동작은 게임에서 연결할 부분입니다.
