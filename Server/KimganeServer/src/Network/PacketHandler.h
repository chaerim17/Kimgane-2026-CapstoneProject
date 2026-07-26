#pragma once
#include "../Pch.h"

class Session;

class PacketHandler
{
public:
    static void HandlePacket(Session* session, unsigned char* packet);

private:
    static void HandleLogin(Session* session, unsigned char* packet);

    static void HandleMoveStart(Session* session, unsigned char* packet);

    static void HandleMoveStop(Session* session, unsigned char* packet);
};
