#pragma once
#include "../Pch.h"

class Session;
class Server;
class Player;

class PacketHandler
{
public:
    static void HandlePacket(Server& server, Session* session, unsigned char* packet);

private:
    static void HandleOpenItemBox(Server& server, Session* session, unsigned char* packet);

    static void HandleLogin(Server& server, Session* session, Player& player, unsigned char* packet);

    static void HandleMoveStart(Player& player, unsigned char* packet);

    static void HandleMoveStop(Player& player, unsigned char* packet);

    static void HandleRotate(Server& server, Session* session, Player& player, unsigned char* packet);

    static void HandleJump(Player& player, unsigned char* packet);

    static void HandlePlayerState(Player& player, unsigned char* packet);

    static void HandleShoot(Server& server, Session* session, Player& player, unsigned char* packet);
};
