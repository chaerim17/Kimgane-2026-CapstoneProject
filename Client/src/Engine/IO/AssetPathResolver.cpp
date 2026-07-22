#include "Pch.h"

#include "AssetPathResolver.h"

#include <vector>

namespace Kimgane::Engine
{
namespace
{
std::filesystem::path GetModuleDirectory()
{
    wchar_t modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, _countof(modulePath));
    if (length == 0U || length == _countof(modulePath))
    {
        return {};
    }

    return std::filesystem::path(modulePath).parent_path();
}

std::vector<std::filesystem::path> BuildAssetSearchRoots()
{
    std::vector<std::filesystem::path> roots;
    roots.push_back(std::filesystem::current_path());

    std::filesystem::path moduleDirectory = GetModuleDirectory();
    if (!moduleDirectory.empty())
    {
        roots.push_back(moduleDirectory);
        std::filesystem::path current = moduleDirectory;
        for (int depth = 0; depth < 5 && current.has_parent_path(); ++depth)
        {
            current = current.parent_path();
            roots.push_back(current);
        }
    }

    return roots;
}
} // namespace

std::filesystem::path ResolveAssetPath(const std::filesystem::path& filePath)
{
    if (std::filesystem::exists(filePath))
    {
        return std::filesystem::absolute(filePath);
    }

    for (const std::filesystem::path& root : BuildAssetSearchRoots())
    {
        const std::filesystem::path candidate = root / filePath;
        if (std::filesystem::exists(candidate))
        {
            return std::filesystem::absolute(candidate);
        }
    }

    return {};
}
} // namespace Kimgane::Engine
