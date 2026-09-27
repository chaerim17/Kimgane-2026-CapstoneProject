# Asset List

외부 에셋의 출처, 라이선스, 사용 위치를 기록합니다. 포트폴리오와 발표 자료에 사용하기 위해 반드시 관리합니다.

## Policy

- 출처가 불명확한 에셋은 사용하지 않습니다.
- 라이선스가 프로젝트 목적에 맞는지 확인합니다.
- 수정한 에셋은 원본과 수정본을 구분합니다.
- 에셋을 추가하면 이 문서를 함께 업데이트합니다.

## Asset Table

| Asset Name | Type | Source | License | Usage | Modified | Owner | Note |
| --- | --- | --- | --- | --- | --- | --- | --- |
| TODO | Model / Texture / Sound / Font / Shader | TODO | TODO | TODO | No | TODO | TODO |
| Paperlogy-1.001 | Font | [Freesentation Paperlogy Font](https://freesentation.blog/paperlogyfont) | SIL Open Font License (OFL) | 타이틀/설정 UI 텍스트 렌더링용 폰트 | No | ymid0610 | `Assets/Fonts/Paperlogy-1.001/`, 원본 `Assets/Fonts/Paperlogy-1.001.zip`; TTF 9 weights, v1.001(2024-10-07). 글꼴 단독 판매 및 라이선스 변경 제외 사용 가능 |
| paladin_test.txt | Model | Mixamo (Adobe) | Mixamo License (상업적 사용 가능, 원본 재배포 불가) | 플레이어 캐릭터 테스트 모델 | Yes | 김준해 | 원본 Paladin FBX를 Tools/export_model_txt.py(Blender)로 텍스트 변환 |
| TestHouse.obj | Model | 직접 제작 (Unreal, Blender) | 자체 제작 (제한 없음) | 2층집 테스트 모델, ObjModelMesh 로더로 로드 | No | 김준해 | Unreal에서 직접 모델링 후 Blender에서 Obj 변환 후 Wavefront OBJ로 내보냄 |
| T_Brick_Clay_Beveled_D.dds | Texture | Unreal Engine Starter Content (Epic Games) | Unreal Engine EULA | TestHouse 벽 텍스처, 렌더러 텍스처 매핑 파이프라인 테스트용 | Yes | 김준해 | 언리얼 Content Browser에서 PNG로 export 후 VS 이미지 편집기로 DDS(BC3_UNORM, 밉맵 12단계) 변환. 테스트 목적이라 발표/포트폴리오에 실제로 쓰려면 UE 엔진 밖(자체 DX12 엔진) 재사용이 EULA상 허용되는지 먼저 확인 필요 |
| zombiegirl.txt | Model | Mixamo (Adobe) | Mixamo License (상업적 사용 가능, 원본 재배포 불가) | NPC 전용 테스트 모델 | Yes | 김준해 | 원본 ZombieGirl FBX를 Tools/export_model_txt.py(Blender)로 텍스트 변환 |
| S07 Supply Crate (ItemBox) | Model | 직접 제작 (GPT Astra + Blender) | 자체 제작 (제한 없음) | 아이템 박스 테스트 모델(Body+Lid, LOD0~3), TestHouse 옆에 임시 위치로 렌더링 테스트 | No | 김준해 | `Assets/ItemBox/obj/` (S07_Body_LOD0~3.obj, S07_Lid_LOD0~3.obj). 서버 아이템 박스 위치 패킷 미정이라 위치는 TestSceneSettings.h에 하드코딩. 1차 Blender export 시 UV V좌표가 뒤집혀 있어 v=1-v 적용해서 재변환함 |
| S07_BaseColor.dds | Texture | 직접 제작 (GPT Astra + Blender) | 자체 제작 (제한 없음) | 아이템 박스 BaseColor 텍스처 | Yes | 김준해 | `Assets/ItemBox/textures/`. 원본 PNG를 VS 이미지 편집기로 DDS(BC3_UNORM) 변환. 같이 만든 Normal_GL/DX, ORM, Roughness, Metallic, AO, Emissive는 아직 미사용 — 셰이더가 텍스처 슬롯 1개(BaseColor)만 지원해서, 멀티 텍스처(노멀맵 등) 지원 확장 후 적용 예정 |

## License Checklist

| 확인 항목 | 상태 |
| --- | --- |
| 상업적 사용 가능 여부 확인 | TODO |
| 저작자 표시 필요 여부 확인 | TODO |
| 수정 가능 여부 확인 | TODO |
| 재배포 가능 여부 확인 | TODO |
| 발표 영상 사용 가능 여부 확인 | TODO |

## Naming Rule

파일명은 영어로 작성하고 공백을 사용하지 않습니다.

```text
player_soldier.fbx
terrain_height_map.png
metal_wall_albedo.png
metal_wall_normal.png
```
