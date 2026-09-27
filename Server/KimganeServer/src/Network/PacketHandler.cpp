#include "PacketHandler.h"

#include <cmath>

#include "../Core/Session.h"
#include "../Core/Server.h"
#include "../Npc/Npc.h"

void PacketHandler::HandlePacket(Server& server, Session* session, unsigned char* packet)
{
    auto* player = server.GetWorld().FindPlayer(session->GetId());
    if (!player)
        return;

    PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&packet[1]);

    switch (type)
    {
    case C2S_LOGIN:
        HandleLogin(server, session, *player, packet);
        break;

    case C2S_MOVE_START:
        HandleMoveStart(*player, packet);
        break;

    case C2S_MOVE_STOP:
        HandleMoveStop(*player, packet);
        break;

    case C2S_ROTATE:
        HandleRotate(server, session, *player, packet);
        break;

    case C2S_JUMP:
        HandleJump(*player, packet);
        break;

    case C2S_PLAYER_STATE:
        HandlePlayerState(*player, packet);
        break;

    case C2S_SHOOT:
        HandleShoot(server, session, *player, packet);
        break;
    }
}


void PacketHandler::HandleLogin(Server& server, Session* session, Player& player, unsigned char* packet)
{
    (void)packet;
    std::cout << "Client[" << session->GetId() << "] Login: " << player.mUserName << std::endl;

    session->SendAvatarInfo(player);
    for (const auto& [id, object] : server.GetWorld().GetObjects())
    {
        if (object->GetType() != GameObject::ObjectType::Npc)
            continue;
        const auto& npc = static_cast<const Npc&>(*object);
        session->SendAddObject(npc, npc.mMaxHp, npc.mCurrentHp);
    }

    session->SendLoginSuccess();
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (server.GetSessions()[i] && server.GetSessions()[i]->IsConnected() && i != session->GetId())
        {
            const auto* otherPlayer = server.GetWorld().FindPlayer(i);
            if (!otherPlayer)
                continue;
            session->SendAddObject(*otherPlayer);
            server.GetSessions()[i]->SendAddObject(player);
        }
    }

    // 디버그용 초기 회전값 설정
   /* player.mYaw = 90.0f;
    std::cout << "[MOVE SEND] objectId=" << session->GetId() << " yaw=" << player.mYaw << '\n';*/

    //플레이어 초기 스폰 위치
    std::cout << "[Spawn] Player " << session->GetId() << " Pos(" << player.GetPositionM().x << ", " << player.GetPositionM().y
              << ", " << player.GetPositionM().z << ")\n";


    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (server.GetSessions()[i] && server.GetSessions()[i]->IsConnected())
        {
            server.GetSessions()[i]->SendRotateObject(player);
        }
    }
}

void PacketHandler::HandleMoveStart(Player& player, unsigned char* packet)
{
    auto* movePacket = reinterpret_cast<C2S_Move*>(packet);
    player.mMoveYaw = movePacket->yaw;
    switch (movePacket->direction)
    {
    case UP:
        player.mMoveUp = true;
        break;
    case DOWN:
        player.mMoveDown = true;
        break;
    case LEFT:
        player.mMoveLeft = true;
        break;
    case RIGHT:
        player.mMoveRight = true;
        break;
    }
    //std::cout << "[START] Player " << player.GetId() << '\n';
    //std::cout << "[MOVE START] yaw=" << movePacket->yaw << '\n';
}

void PacketHandler::HandleMoveStop(Player& player, unsigned char* packet)
{
    auto* movePacket = reinterpret_cast<C2S_Move*>(packet);
    player.mMoveYaw = movePacket->yaw;
    switch (movePacket->direction)
    {
    case UP:
        player.mMoveUp = false;
        break;
    case DOWN:
        player.mMoveDown = false;
        break;
    case LEFT:
        player.mMoveLeft = false;
        break;
    case RIGHT:
        player.mMoveRight = false;
        break;
    }
    //std::cout << "[STOP] Player " << player.GetId() << '\n';
}

void PacketHandler::HandleRotate(Server& server, Session* session, Player& player, unsigned char* packet)
{
    auto* rotatePacket = reinterpret_cast<C2S_Rotate*>(packet);

    player.mYaw = rotatePacket->yaw;

    // 브로드캐스트 (이후 sector 기반으로 최적화할 것)
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (server.GetSessions()[i] && server.GetSessions()[i]->IsConnected() && i != session->GetId())
        {
            server.GetSessions()[i]->SendRotateObject(player);
        }
    }
}

void PacketHandler::HandleJump(Player& player, unsigned char* packet)
{
    (void)packet;
    player.mJumpRequested = true;
}

// 클라와 서버 위치 오차 측정
// TODO: 이후 보간 및 보정 로직 구현 필요
void PacketHandler::HandlePlayerState(Player& player, unsigned char* packet)
{
    auto* p = reinterpret_cast<C2S_PlayerState*>(packet);

    /*std::cout << "Server(" << player.GetPositionM().x << ", " << player.GetPositionM().y << ", " << player.GetPositionM().z << ") "
              << "Client("  << p->x << ", "  << p->y << ", "  << p->z << ")\n";*/

    float dx = player.GetPositionM().x - p->x;
    float dy = player.GetPositionM().y - p->y;
    float dz = player.GetPositionM().z - p->z;

    float error = sqrtf(dx * dx + dy * dy + dz * dz);

    if (error > 10.0f)
    {
        /*std::cout << "Server(" << player.GetPositionM().x << ", " << player.GetPositionM().y << ", " << player.GetPositionM().z << ") "
                  << "Client(" << p->x << ", " << p->y << ", " << p->z << ") "
                  << "Error=" << error << '\n';*/
    }
}

void PacketHandler::HandleShoot(Server& server, Session* session, Player& player, unsigned char* packet)
{
    if (packet[0] != sizeof(C2S_Shoot) || !session->IsConnected())
        return;

    auto* shootPacket = reinterpret_cast<const C2S_Shoot*>(packet);
    server.HandleShoot(player, shootPacket->direction);
}
