#include "Pch.h"

#include "Core/Session.h"
#include "Core/Server.h"

int main()
{
    Server server;

    if (!server.Initialize())
        return 1;

    server.Run();
}
