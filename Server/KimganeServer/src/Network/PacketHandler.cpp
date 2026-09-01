#include "PacketHandler.h"

#include "../Core/Session.h"
#include "../Core/Server.h"
#include "../Npc/NpcSetting.h"
#include "../Npc/Npc.h"

void PacketHandler::HandlePacket(Session* session, unsigned char* packet)
{
    PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&packet[1]);

    switch (type)
    {
    case C2S_LOGIN:
        HandleLogin(session, packet);
        break;

    case C2S_MOVE_START:
        HandleMoveStart(session, packet);
        break;

    case C2S_MOVE_STOP:
        HandleMoveStop(session, packet);
        break;

    case C2S_ROTATE:
        HandleRotate(session, packet);
        break;

    case C2S_JUMP:
        HandleJump(session, packet);
        break;

    case C2S_PLAYER_STATE:
        HandlePlayerState(session, packet);
        break;
    }
}


void PacketHandler::HandleLogin(Session* session, unsigned char* packet)
{
    std::cout << "Client[" << session->GetId() << "] Login: " << session->mUserName << std::endl;

    //점프 초기화 시 강제 Test
    //session->mIsJumping = true;
    //session->mVelocityY = JUMP_POWER;.

    session->SendAvatarInfo();
    for (auto& npc : NpcSetting::gNpcs)
    {
        session->SendAddObject(npc->mId);
    }

    // 디버그용 출력
    std::cout << "[NPC] Sent " << NpcSetting::gNpcs.size() << " NPCs to Client[" << session->GetId() << "]\n";

    session->SendLoginSuccess();
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (clients[i] && clients[i]->IsConnected() && i != session->GetId())
        {
            session->SendAddObject(i);
            clients[i]->SendAddObject(session->GetId());
        }
    }

    // 디버그용 초기 회전값 설정
   /* session->mYaw = 90.0f;
    std::cout << "[MOVE SEND] objectId=" << session->GetId() << " yaw=" << session->mYaw << '\n';*/

    //플레이어 초기 스폰 위치
    std::cout << "[Spawn] Player " << session->GetId() << " Pos(" << session->mX << ", " << session->mY
              << ", " << session->mZ << ")\n";


    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (clients[i] && clients[i]->IsConnected())
        {
            clients[i]->SendRotateObject(session->GetId());
        }
    }
}

void PacketHandler::HandleMoveStart(Session* session, unsigned char* packet)
{
    auto* movePacket = reinterpret_cast<C2S_Move*>(packet);
    session->mYaw = movePacket->yaw;
    switch (movePacket->direction)
    {
    case UP:
        session->mMoveUp = true;
        break;
    case DOWN:
        session->mMoveDown = true;
        break;
    case LEFT:
        session->mMoveLeft = true;
        break;
    case RIGHT:
        session->mMoveRight = true;
        break;
    }
    //std::cout << "[START] Player " << session->GetId() << '\n';
    //std::cout << "[MOVE START] yaw=" << movePacket->yaw << '\n';
}

void PacketHandler::HandleMoveStop(Session* session, unsigned char* packet)
{
    auto* movePacket = reinterpret_cast<C2S_Move*>(packet);
    switch (movePacket->direction)
    {
    case UP:
        session->mMoveUp = false;
        break;
    case DOWN:
        session->mMoveDown = false;
        break;
    case LEFT:
        session->mMoveLeft = false;
        break;
    case RIGHT:
        session->mMoveRight = false;
        break;
    }
    //std::cout << "[STOP] Player " << session->GetId() << '\n';
}

void PacketHandler::HandleRotate(Session* session, unsigned char* packet)
{
    auto* rotatePacket = reinterpret_cast<C2S_Rotate*>(packet);

    session->mYaw = rotatePacket->yaw;

    // 브로드캐스트 (이후 sector 기반으로 최적화할 것)
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        if (clients[i] && clients[i]->IsConnected() && i != session->GetId())
        {
            clients[i]->SendRotateObject(session->GetId());
        }
    }
}

void PacketHandler::HandleJump(Session* session, unsigned char* packet)
{  
    if (session->mIsJumping)
        return;

    session->mIsJumping = true;
    session->mVelocityY = JUMP_POWER;

    //std::cout << "Jumping=" << session->mIsJumping << " Vel=" << session->mVelocityY << '\n';
}

// 클라와 서버 위치 오차 측정
// TODO: 이후 보간 및 보정 로직 구현 필요
void PacketHandler::HandlePlayerState(Session* session, unsigned char* packet)
{
    auto* p = reinterpret_cast<C2S_PlayerState*>(packet);

    /*std::cout << "Server(" << session->mX << ", " << session->mY << ", " << session->mZ << ") "
              << "Client("  << p->x << ", "  << p->y << ", "  << p->z << ")\n";*/

    float dx = session->mX - p->x;
    float dy = session->mY - p->y;
    float dz = session->mZ - p->z;

    float error = sqrtf(dx * dx + dy * dy + dz * dz);

    if (error > 10.0f)
    {
        /*std::cout << "Server(" << session->mX << ", " << session->mY << ", " << session->mZ << ") "
                  << "Client(" << p->x << ", " << p->y << ", " << p->z << ") "
                  << "Error=" << error << '\n';*/
    }
}
