#include "Pch.h"

#include "Engine/Application/ClientApplication.h"

#include <Windows.h>
#include <objbase.h>

#include <exception>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int commandShow)
{
    UNREFERENCED_PARAMETER(previousInstance);
    UNREFERENCED_PARAMETER(commandLine);

    // COM(WIC 이미지 로더 등)은 앱보다 먼저 켜고 앱이 완전히 소멸한 뒤에 끔.
    // 창을 가진 UI 스레드라 STA로 초기화함.
    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    int exitCode = -1;
    try
    {
        Kimgane::Engine::ClientApplication app;
        exitCode = app.Run(instance, commandShow);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "Kimgane.Client", MB_OK | MB_ICONERROR);
    }

    if (SUCCEEDED(comResult))
    {
        CoUninitialize();
    }
    return exitCode;
}
