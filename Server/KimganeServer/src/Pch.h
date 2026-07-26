#pragma once

#include <iostream>
#include <array>
#include <memory>
#include <thread>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include "../../../Shared/Protocol.h"

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")
