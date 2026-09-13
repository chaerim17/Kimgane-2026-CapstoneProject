#pragma once
#include "../Pch.h"

class Session;
class Server;

class PacketHandler
{
public:
    static void HandlePacket(Server& server, Session* session, unsigned char* packet);

private:
    static void HandleLogin(Session* session, unsigned char* packet);

    static void HandleMoveStart(Session* session, unsigned char* packet);

    static void HandleMoveStop(Session* session, unsigned char* packet);

    static void HandleRotate(Session* session, unsigned char* packet);

    static void HandleJump(Session* session, unsigned char* packet);

    static void HandlePlayerState(Session* session, unsigned char* packet);

    static void HandleShoot(Server& server, Session* session, unsigned char* packet);
};
