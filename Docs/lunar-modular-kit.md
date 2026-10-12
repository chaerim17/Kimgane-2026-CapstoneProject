# L02 달 기지 모듈 키트

`feature/lunar-modular-environment`, 기준 `origin/develop` `cb0b682` (2026-10-12 확인).
기존 작업과 충돌하지 않도록 Codex에 연결된 별도 worktree에서 제작했습니다.

## 기획 반영

기존 S07 상자의 흰색 장갑판, 짙은 금속 몸체, 노란 체결부, 청록 표시등을 건축 파츠에도 사용합니다.
로컬 기획서 `잔해항로_게임기획서_v1.docx`의 달 화물기지·잔해 수색·여러 접근로·정비 시설과,
첨부한 네레이아 변이/정화·PK/포식자 유도 이미지를 시설 외형과 엄폐 배치의 참고로 사용했습니다.
정화 캡슐, 경광등, 통신탑은 환경 모델입니다. 정화 효과·포식자 유도·탈출 기능을 구현한 오브젝트는 아닙니다.
플레이어가 건물을 건설하는 기능은 기획 범위에 추가하지 않았습니다. 이 키트는 레벨 제작용입니다.

## 제공 파일

| 위치 | 용도 |
| --- | --- |
| `Assets/LunarKit/source/L02_LunarKit_Layout.blend` | 전체 달 지형, 배치 예시, Asset Browser 파츠/프리팹, 파츠/LOD 검토 장면 |
| `Assets/LunarKit/catalog.json` | 39종 파츠, 7종 프리팹, LOD 경로/삼각형 수, 지지 관계, 원형 콜라이더 |
| `Assets/LunarKit/prefabs/recipes.json` | 수정 가능한 조립 레시피, Blender 좌표계의 로컬 위치/회전 |
| `Assets/LunarKit/obj/parts/` | 파츠마다 LOD0~3, 총 156개 OBJ |
| `Assets/LunarKit/obj/prefabs/` | 프리팹마다 LOD0~3, 총 28개 OBJ |
| `Assets/LunarKit/layout.txt` | 엔진에서 읽는 프리팹/파츠 인스턴스 배치 |
| `Assets/LunarKit/layout.json` | 배치 + 파츠별 충돌 파일 인덱스/지지 관계 |
| `Shared/Geometry/L02_LunarKit_collision.txt` | 기존 `Physics::Box`로 읽는 충돌체 371개 |
| `Assets/LunarKit/textures/` | BaseColor PNG/DDS, NormalGL PNG/NormalDX DDS, ORM PNG/DDS, Roughness/Metallic/Emissive PNG |
| `Assets/LunarKit/previews/` | 화물기지, 정화시설 단면, 배치, 39종 파츠 목록, 4 LOD 비교 렌더 |

현재 엔진은 Unity 프로젝트가 아닌 C++/DirectX12 프로젝트입니다. 프리팹은 **Blender Collection Asset + JSON 레시피 + 엔진용 OBJ/배치 데이터**입니다.
Unity 전용 `.prefab` 파일은 생성하지 않았습니다.

## 조립 규격과 파츠

1 Blender unit = 1m. Blender는 Z 위/Y 깊이, 엔진은 Y 위/Z 깊이입니다.
바닥과 지붕은 4m, 작은 바닥은 2×4m, 벽은 폭 2m/높이 4m, 기둥 높이는 4m입니다.
벽·기둥은 바닥 끝에, 지붕·보는 Z=4에 놓습니다. 바닥 파츠의 원점은 보행면이고, 기둥/설비는 바닥면 원점입니다.
간판은 중앙 원점입니다. 난간과 보의 길이는 4m입니다.

| 분류 | 파츠 |
| --- | --- |
| 구조 | 바닥 2종, 지붕 2종, 막힌 벽/창 벽/환기 벽/낮은 벽, 파손 벽, 출입구, 정상/파손 기둥, 보 2종, 난간 |
| 시설 | 산소 랙, 발전기, 공조기, 콘솔, 작업대, 정화 캡슐, 통신탑, 태양광 패널, 직선/ㄱ자 배관 |
| 엄폐·표식 | 방벽, 화물 컨테이너, 일반/경고 비콘, 화물/정화/통신 간판 |
| 파괴 준비 | 파손 벽 조각, 바닥 잔해판, 잔해 보, 폐쇄된 벽 파편 4종 |

모든 파츠는 두께가 있는 폐쇄 메시로 만들었습니다. 파손 벽의 내부 절단면도 포함합니다.
출입구는 문턱 없이 폭 2.8m, 높이 3.1m가 비어 있습니다. 창에는 유리/투명 충돌체가 없습니다.
주 화물기지에는 정면/후면 출입구와 정면/측면 파손 통로가 있어 한 방향으로만 갇히지 않습니다.
바닥은 달 지형 중앙 평탄부에서 약 5mm 높여 보행면이 보이게 했습니다. 큰 경사에서는 바닥 높이와 진입 경사를 직접 조정해야 합니다.
원격의 정상 창고는 북서 크레이터 바닥의 조합 예시입니다. 주 플레이 검증은 중앙 기지의 통로를 대상으로 했습니다.

## 조합 프리팹과 LOD

아래 수는 **삼각형 기준**이며, 개별 파츠를 모두 합친 값입니다. 참조 지형과 기존 S07 상자는 제외합니다.

| 프리팹 | 파츠 수 | LOD0 | LOD1 | LOD2 | LOD3 |
| --- | ---: | ---: | ---: | ---: | ---: |
| CargoDepot_Ruined | 53 | 25,988 | 12,556 | 3,336 | 1,512 |
| CargoDepot_Intact | 44 | 22,480 | 11,280 | 2,772 | 1,224 |
| PurificationStation | 44 | 22,516 | 11,284 | 2,820 | 1,284 |
| ServiceCorridor | 18 | 8,920 | 4,456 | 1,056 | 432 |
| RelayPlatform | 12 | 6,220 | 3,004 | 1,180 | 552 |
| CoverCheckpoint | 5 | 3,420 | 1,940 | 988 | 416 |
| WreckWorkshop | 13 | 5,824 | 2,568 | 916 | 400 |

LOD0의 체결부와 세부 환기구부터 순서대로 제거합니다. LOD1은 모서리 가공, LOD2는 주요 색상 부품,
LOD3는 외곽과 출입구/파손 구멍을 유지합니다. 콜라이더는 LOD와 독립적입니다.
엔진은 인스턴스별로 4개 메시를 로드하고 카메라 거리에서 외곽 반경을 뺀 값을 기준으로 22/55/110m에서 전환합니다.
경계에서 2m 히스테리시스를 적용합니다. 같은 프리팹을 여러 번 배치하면 GPU 메시를 재사용합니다.
현재 배치는 7개 프리팹 + 태양광/배관 3개 인스턴스입니다.

## Blender에서 배치하기

1. `L02_LunarKit_Layout.blend`를 열고 기본 배치 장면을 사용합니다. 별도로 달 지형을 가져올 필요가 없습니다.
2. `L02_LAYOUT_Export_Instances` 컬렉션의 오브젝트를 선택합니다. `Shift+D`로 복제하고 `G`로 이동합니다.
3. `R`, `Z`, `90`처럼 **Z축 90도 단위**로 회전합니다. 스케일은 1을 유지합니다.
4. 새 파츠/프리팹은 Asset Browser의 `Current File`에서 꺼내 같은 배치 컬렉션에 넣습니다.
   `Make Instances Real`로 분해한 오브젝트는 전체 배치 exporter의 입력이 아닙니다.
5. Text Editor에서 내장된 `export_lunar_layout.py`를 실행합니다.
   `layout.txt`, `layout.json`, 공유 충돌 파일을 함께 갱신합니다. 엔진을 재시작하면 적용됩니다.
6. `.blend`도 저장합니다. 배치 파일을 저장한 뒤 재생성하면 위치/회전을 유지합니다.

`L02_COLLIDERS_Reference_Toggle_Viewport` 컬렉션의 모니터 아이콘을 켜면 제작 당시 콜라이더를 볼 수 있습니다.
이 참조 컬렉션은 자동으로 실시간 이동하지 않습니다. 배치 수정 후 exporter 출력이 실제 충돌 기준이며 재생성하면 참조도 갱신됩니다.
원본 RAW, BaseColor 3종을 사용한 달 표면, 기존 상자는 참조 오브젝트로 내보내기에서 제외합니다.
모든 사용 이미지가 `.blend`에 내장되어 있습니다. 엔진과 Blender의 조명/재질 셰이더는 달라 화면이 완전히 같지는 않습니다.

작업 폴더를 옮긴 경우 Scene의 Custom Property `lunar_project_root`를 새 저장소 절대경로로 바꿉니다.
내보내기는 회전/스케일/중복 식별자를 먼저 검사합니다. 지원하지 않는 변환이 있으면 출력 파일을 수정하기 전에 오류로 중단합니다.
내부 충돌체는 90도 회전 시 축을 바꾸고 월드 위치로 변환하므로 문이나 파손 구멍을 큰 박스로 덮지 않습니다.

## 새 프리팹 만들기

Asset Browser의 파츠들을 별도 공간에 조립한 뒤 3D Cursor를 새 프리팹의 원점으로 옮깁니다.
파츠 인스턴스만 선택하고 Scene Custom Property `lunar_new_prefab_name`에 `CustomHabitat` 같은 고유 영문 이름을 넣습니다.
내장 `save_lunar_prefab.py`를 실행하면 선택한 파츠와 원점 기준 위치를 `recipes.json`에 추가합니다.
기존 이름은 덮어쓰지 않습니다. 레시피를 텍스트로 복사/수정하는 방법도 가능합니다.

먼저 현재 배치를 exporter로 저장하고 Blender 파일도 저장한 다음, **새 백그라운드 프로세스**에서 아래 빌더를 실행합니다.
현재 열린 Blender 세션에서 빌더를 실행하면 장면을 지우므로 실행하지 마세요.
빌더는 저장된 `layout.json`의 배치를 유지하고 모든 프리팹의 4 LOD와 새 `.blend`를 다시 생성합니다.
`--reset-layout` 옵션은 예시 배치로 돌아가려는 경우에만 사용합니다.

```powershell
python Tools/generate_lunar_kit_atlas.py # numpy, Pillow 필요. 재질을 수정한 경우에만 필요
& 'D:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/build_lunar_kit.py -- --render
```

새 원본을 연 뒤 새 프리팹을 Asset Browser에서 배치 컬렉션으로 끌어다 놓고 다시 exporter를 실행합니다.
임의 조합도 기존 Box 충돌체와 LOD를 같은 파츠 정의에서 생성하므로 엔진 코드를 조합마다 수정할 필요가 없습니다.

## 텍스처와 파괴 연동 범위

BaseColor는 2048² BC1 sRGB/12 mip, NormalDX는 1024² BC5/11 mip, ORM은 1024² BC1 linear/11 mip입니다.
ORM 채널은 R=AO, G=Roughness, B=Metallic입니다. Blender는 PNG NormalGL과 개별 재질 맵을 사용합니다.
**현재 일반 건물 셰이더에는 BaseColor DDS와 고정 금속도/거칠기를 적용했습니다.**
Normal/ORM/Emissive의 픽셀 단위 효과는 Blender에 적용되어 있으며, 엔진 일반 메시 셰이더의 후속 확장용으로 제공합니다.
기존 달 지형의 전용 PBR 셰이더는 변경하지 않았습니다.

파괴는 제작 준비 단계입니다. 현재 런타임에서는 프리팹 하나를 합친 정적 메시로 렌더링합니다.
체력·네트워크 동기화·파편 Rigidbody·구조 붕괴를 구현하지 않았습니다.
후속 구현에서는 프리팹 노드별 파츠 OBJ를 사용하고, `layout.json`의 `colliderFileIndices`에
`FIRST_HOUSE_COLLIDER_ID`를 더해 해당 노드의 기존 충돌체를 제거할 수 있습니다.
`supportNodeIds`는 바닥 → 기둥 → 보 → 지붕의 비순환 지지 그래프입니다.
막힌 벽에는 추가로 4개 벽 파편 메시가 연결되어 있습니다. 파편 물리용 질량/수명/충돌체는 후속으로 설정합니다.

## 검증

- Client/Server Debug 및 Release x64 빌드.
- 네이티브 `LunarKitTests`: 184 OBJ의 UV/법선/와인딩/퇴화 삼각형 검사, 생산 DDS 로더로 3개 DDS와 전체 mip 로딩,
  LOD 경계, 공유 월드의 4개 통로 양방향 이동, 벽 이동/레이캐스트 차단.
- `validate_lunar_kit.py`: 39×4 폐쇄/양의 체적 메시, 삼각형 수, 지지 그래프, 내장 텍스처, RAW 높이 정합,
  복제/Asset Browser/90도 회전/지원하지 않는 스케일·회전 내보내기.
- 기존 `TerrainTests`: 64개 지형 청크 × 4 LOD, 경계 정합. 테스트 프로젝트의 누락된 CollisionResolver 링크 입력도 보완했습니다.
- Blender Cycles 렌더로 파츠, LOD0~3 외곽과 구멍, 상자와 재질 조합 확인.

```powershell
msbuild Client/Tests/LunarKitTests.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
& build/LunarKitTests/LunarKitTests.exe
& 'D:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background Assets/LunarKit/source/L02_LunarKit_Layout.blend --python-exit-code 1 --python Tools/validate_lunar_kit.py
```

키트의 모델·아틀라스는 프로젝트를 위해 절차적으로 새로 제작했습니다.
기존 달 지형·S07 상자와 그 참조 이미지는 저장소 원본의 기존 이용 조건을 따릅니다.
