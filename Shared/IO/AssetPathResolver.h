#pragma once

#include <filesystem>

namespace Kimgane::Shared::IO
{
// CWD, 실행 파일 위치, 그 상위 폴더들을 순서대로 뒤져서 filePath를 찾습니다.
// 못 찾으면 빈 경로를 반환합니다.
[[nodiscard]] std::filesystem::path ResolveAssetPath(const std::filesystem::path& filePath);
} // namespace Kimgane::Shared::IO
