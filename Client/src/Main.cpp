#include "Pch.h"

#include "Engine/Application/ClientApplication.h"

#include <Windows.h>

#include <exception>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int commandShow)
{
    UNREFERENCED_PARAMETER(previousInstance);
    UNREFERENCED_PARAMETER(commandLine);

    try
    {
        Kimgane::Engine::ClientApplication app;
        return app.Run(instance, commandShow);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "Kimgane.Client", MB_OK | MB_ICONERROR);
        return -1;
    }
}
