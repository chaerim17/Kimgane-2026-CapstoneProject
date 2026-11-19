#pragma once

// 아이템 상자 공용 설정값. 이름은 서버 ServerConfig.h와 같게 맞춤.
// TODO: 서버도 이 파일을 쓰도록 바꿀지 서버 팀과 상의 (현재 서버는 ServerConfig.h에 같은 값을 따로 가짐)
namespace Kimgane::Shared::Items
{
// F 키로 상자를 열 수 있는 최대 거리
inline constexpr float ITEM_BOX_OPEN_DISTANCE_M = 3.0F;

namespace ItemBoxAssetSettings
{
inline constexpr wchar_t BODY_MODEL_PATH[] = L"Assets/ItemBox/obj/S07_Body_LOD0";
inline constexpr wchar_t LID_MODEL_PATH[] = L"Assets/ItemBox/obj/S07_Lid_LOD0";
// 본체 기준 로컬 좌표, 본체+닫힌 뚜껑을 감싸는 박스
inline constexpr wchar_t COLLISION_PATH[] = L"Shared/Geometry/S07_Supply_Crate_collision.txt";
} // namespace ItemBoxAssetSettings
} // namespace Kimgane::Shared::Items
