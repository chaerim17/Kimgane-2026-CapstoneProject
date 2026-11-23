#include "Pch.h"

#include "App/ClientApplication.h"

#include <Windows.h>
#include <objbase.h>

#include <exception>
#include <string>

namespace
{
// /utf-8 빌드라 예외 메시지는 UTF-8. MessageBoxA는 시스템 코드페이지(CP949)로 읽어서 한국어가 깨지므로 변환해서 띄움.
void ShowErrorMessage(const char* utf8Message)
{
    const int length = MultiByteToWideChar(CP_UTF8, 0, utf8Message, -1, nullptr, 0);
    std::wstring message(length > 0 ? static_cast<size_t>(length) : 1U, L'\0');
    if (length > 0)
    {
        MultiByteToWideChar(CP_UTF8, 0, utf8Message, -1, message.data(), length);
    }
    MessageBoxW(nullptr, message.c_str(), L"Kimgane.Client", MB_OK | MB_ICONERROR);
}
} // namespace

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
        Kimgane::Client::ClientApplication app;
        exitCode = app.Run(instance, commandShow);
    }
    catch (const std::exception& exception)
    {
        ShowErrorMessage(exception.what());
    }

    if (SUCCEEDED(comResult))
    {
        CoUninitialize();
    }
    return exitCode;
}
