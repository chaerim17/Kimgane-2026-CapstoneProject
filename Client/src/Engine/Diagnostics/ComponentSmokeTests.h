#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>

namespace Kimgane::Engine::Diagnostics
{
void RunClientComponentSmokeTests(ID3D12Device& device);
} // namespace Kimgane::Engine::Diagnostics
