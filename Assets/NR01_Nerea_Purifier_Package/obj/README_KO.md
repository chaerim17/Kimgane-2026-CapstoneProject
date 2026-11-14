# NR01 정화기 DirectX 12용 OBJ

S07 아이템 박스와 같은 규칙으로 정화기 LOD0~3을 변환했습니다. 파일은 미터 단위, Y-up OBJ이며 UV의 V는 DirectX용 값입니다. 프로젝트의 `ObjLoader`가 Z와 삼각형 순서를 변환하고 UV 값은 그대로 읽습니다.

| 파일 | 삼각형 |
| --- | ---: |
| `NR01_Purifier_LOD0.obj` | 10,960 |
| `NR01_Purifier_LOD1.obj` | 7,192 |
| `NR01_Purifier_LOD2.obj` | 3,276 |
| `NR01_Purifier_LOD3.obj` | 2,732 |

## 프로젝트에서 불러오기

모델 경로를 `Assets/NR01_Nerea_Purifier_Package/obj/NR01_Purifier_LOD0`으로 지정하고 아이템 박스처럼 `ObjModelMesh::Load`로 불러옵니다. 텍스처는 `Assets/NR01_Nerea_Purifier_Package/textures/NR01_Purifier_BaseColor`를 `Texture::Load`에 넘깁니다.

`NR01_Shared.mtl`은 편집 도구용으로 `../textures/NR01_Purifier_BaseColor.png`를 참조하지만, 저장소에는 PNG를 포함하지 않습니다. 게임에서는 아이템 박스와 같은 방식으로 공용 DDS를 별도로 불러옵니다. UV 방향을 편집할 때는 원본 Blender 파일을 사용하면 됩니다.

프로젝트의 실제 `ObjLoader`로 네 파일을 다시 읽어 기존 TXT와 삼각형 순서, 위치, 법선 및 UV가 일치하는 것을 확인했습니다.
