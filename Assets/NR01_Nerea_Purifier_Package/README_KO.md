# NR01 네레아 정화기

네레아의 문명 붕괴 이후에도 작동하는 자동 정화 설비를 상정한 배치용 에셋입니다. 사용자 스케치의 전면 경사로와 원형 발판을 기준으로 설계했습니다. 오염 정화량과 비용은 통합기획서에서 조정 항목으로 남겨둔 상태이므로 모델에 수치를 부여하지 않았습니다.

## 모델과 텍스처

| 단계 | 파일 | 삼각형 |
| --- | --- | ---: |
| LOD0 | `obj/NR01_Purifier_LOD0.obj` | 10,960 |
| LOD1 | `obj/NR01_Purifier_LOD1.obj` | 7,192 |
| LOD2 | `obj/NR01_Purifier_LOD2.obj` | 3,276 |
| LOD3 | `obj/NR01_Purifier_LOD3.obj` | 2,732 |

모든 OBJ는 `textures/NR01_Purifier_BaseColor.dds`를 공유합니다. 2048×2048 BC3(DXT5) 텍스처와 전체 밉 체인을 포함하며, 표시등 색이 들어 있는 Base Color입니다. OBJ 형식과 변환 규칙은 `obj/README_KO.md`를 참고합니다.

## 프로젝트에서 불러오기

S07 아이템 박스와 같은 방식으로 모델은 `ObjModelMesh::Load`, 텍스처는 `Texture::Load`에 확장자를 뺀 경로를 넘깁니다. 현재 `TestSceneSettings.h`의 `PURIFIER_MODEL_PATH`, `PURIFIER_TEXTURE_PATH`가 이 경로를 사용합니다.

```cpp
L"Assets/NR01_Nerea_Purifier_Package/obj/NR01_Purifier_LOD0"
L"Assets/NR01_Nerea_Purifier_Package/textures/NR01_Purifier_BaseColor"
```

## 배치와 상호작용

모델의 외곽은 약 3.13m × 5.22m, 최고 높이는 2.16m입니다. 발판 보행면은 약 0.64m이고 접근 경사로는 폭 2.16m, 길이 2.44m, 기울기 약 14.1도입니다.

아래 좌표는 모두 `ObjLoader`로 읽은 뒤의 엔진 좌표(미터, DirectX 왼손 좌표계)입니다. +Y가 위이고 +Z가 경사로 입구 방향이며, 원점은 설비 기초 높이의 지면입니다.

- `collision/NR01_Purifier_collision.txt`에 발판 기초와 후면 설비의 AABB 4개를 저장했습니다. 형식은 기존 박스 TXT(`box 이름 cx cy cz hx hy hz rx ry rz`, 중심과 반크기)와 같습니다.
- 경사로는 `collision/NR01_Purifier_placement.json`의 치수로 기존 Ramp 충돌 타입에 등록합니다. 로컬 중심은 `(0, 0.3325, 2.35)`, 전체 크기는 `(2.16, 0.615, 2.44)`, 오르막 방향은 `NegativeZ`입니다. 박스 TXT 로더는 AABB만 읽으므로 이 램프 값은 별도로 사용합니다.
- F 상호작용의 기준 위치는 발판 중심 `(0, 0.642, 0.035)`입니다. 발 위치 기준 반경 0.73m와 높이 허용 범위 ±0.14m는 배치용 제안값입니다.

현재 클라이언트는 `GameScene::CreatePurifier`에서 정화기를 배치하고, 박스와 경사로 충돌은 로컬 모드에서만 등록합니다. 서버 맵 등록, F 입력 처리와 실제 정화 동작은 이후 연결할 부분입니다.
