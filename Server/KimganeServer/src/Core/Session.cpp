#include "Session.h"
#include "Server.h"
#include "../Network/PacketHandler.h"
#include "../Npc/NpcSetting.h"
#include "../Npc/Npc.h"

Session::Session()
{
    mClient = INVALID_SOCKET;
    mId = -1;
    mIsConnected = false;
}
Session::~Session()
{
    if (mIsConnected)
        closesocket(mClient);
}

SOCKET Session::GetSocket() const
{
    return mClient;
}
int Session::GetId() const
{
    return mId;
}
bool Session::IsConnected() const
{
    return mIsConnected;
}

void Session::Connect(SOCKET socket, int id)
{
    mClient = socket;
    mId = id;
    mIsConnected = true;

    // 초기 위치 설정
    mX = -5.0f;
    mZ = 0.0f;
    mYaw = 0.0f;
}
void Session::Disconnect()
{
    mIsConnected = false;

    if (mClient != INVALID_SOCKET)
    {
        closesocket(mClient);
        mClient = INVALID_SOCKET;
    }
}

void Session::DoRecv()
{
    DWORD recv_flag = 0;
    ZeroMemory(&mRecvOver.mOver, sizeof(mRecvOver.mOver));
    mRecvOver.mIoType = IO_RECV;
    // 패킷 재조립
    mRecvOver.mWsa.len = BUF_SIZE - mPrevRecv;
    mRecvOver.mWsa.buf = mRecvOver.mBuffer + mPrevRecv;
    WSARecv(mClient, &mRecvOver.mWsa, 1, 0, &recv_flag, &mRecvOver.mOver, nullptr);
}
void Session::DoSend(int size, char* buffer)
{
    ExpOver* o = new ExpOver(IO_SEND);
    o->mWsa.len = size;
    memcpy(o->mBuffer, buffer, size);
    WSASend(mClient, &o->mWsa, 1, 0, 0, &o->mOver, nullptr);
}

void Session::ProcessPacket(unsigned char* packet)
{
    PacketHandler::HandlePacket(this, packet);
}

void Session::SendLoginSuccess()
{
    S2C_LoginResult loginResultPacket;
    loginResultPacket.size = sizeof(S2C_LoginResult);
    loginResultPacket.type = S2C_LOGIN_RESULT;
    loginResultPacket.success = true;
    strncpy_s(loginResultPacket.message, "Login successful.", sizeof(loginResultPacket.message));
    DoSend(loginResultPacket.size, reinterpret_cast<char*>(&loginResultPacket));
}
void Session::SendAvatarInfo()
{
    S2C_AvatarInfo avatarPacket;
    avatarPacket.size = sizeof(S2C_AvatarInfo);
    avatarPacket.type = S2C_AVATAR_INFO;
    avatarPacket.playerId = mId;
    avatarPacket.x = mX;
    avatarPacket.y = mY;
    avatarPacket.z = mZ;
    avatarPacket.yaw = mYaw;
    DoSend(avatarPacket.size, reinterpret_cast<char*>(&avatarPacket));
}
void Session::SendMoveObject(int objectId)
{
    S2C_MoveObject packet{};

    packet.size = sizeof(packet);
    packet.type = S2C_MOVE_OBJECT;
    packet.objectId = objectId;

    if (NpcSetting::IsNpc(objectId))
    {
        auto it = std::find_if(
            NpcSetting::gNpcs.begin(),
            NpcSetting::gNpcs.end(),
            [objectId](const std::unique_ptr<Npc>& npc) {
            return npc->mId == objectId;
            });

        if (it == NpcSetting::gNpcs.end())
            return;

        Npc* npc = it->get();

        packet.x = npc->mX;
        packet.y = npc->mY;
        packet.z = npc->mZ;
        packet.yaw = npc->mYaw;
    }
    else
    {
        packet.x = clients[objectId]->mX;
        packet.y = clients[objectId]->mY;
        packet.z = clients[objectId]->mZ;
        packet.yaw = clients[objectId]->mYaw;
    }

    // 디버그용
    //std::cout << "[SERVER YAW] " << clients[objectId]->mYaw << '\n';

    DoSend(sizeof(packet), reinterpret_cast<char*>(&packet));
}

void Session::SendAddObject(int objectId)
{
    S2C_AddObject packet{};

    packet.size = sizeof(packet);
    packet.type = S2C_ADD_OBJECT;
    packet.objectId = objectId;

    if (NpcSetting::IsNpc(objectId))
    {
        auto it = std::find_if(
            NpcSetting::gNpcs.begin(),
            NpcSetting::gNpcs.end(),
            [objectId](const std::unique_ptr<Npc>& npc) {
                return npc->mId == objectId;
            });

        if (it == NpcSetting::gNpcs.end())
            return;

        Npc* npc = it->get();

        packet.x = npc->mX;
        packet.y = npc->mY;
        packet.z = npc->mZ;
        packet.yaw = npc->mYaw;
    }
    else
    {
        packet.x = clients[objectId]->mX;
        packet.y = clients[objectId]->mY;
        packet.z = clients[objectId]->mZ;
        packet.yaw = clients[objectId]->mYaw;
    }

    DoSend(sizeof(packet), reinterpret_cast<char*>(&packet));
}

void Session::SendRemoveObject(int objectId)
{
    S2C_RemoveObject removeObjectPacket;
    removeObjectPacket.size = sizeof(S2C_RemoveObject);
    removeObjectPacket.type = S2C_REMOVE_OBJECT;
    removeObjectPacket.objectId = objectId;
    DoSend(removeObjectPacket.size, reinterpret_cast<char*>(&removeObjectPacket));
}

void Session::SendRotateObject(int objectId)
{
    S2C_Rotate rotatePacket{};
    rotatePacket.size = sizeof(S2C_Rotate);
    rotatePacket.type = S2C_ROTATE;
    rotatePacket.objectId = objectId;

    if (NpcSetting::IsNpc(objectId))
    {
        auto it = std::find_if(
            NpcSetting::gNpcs.begin(),
            NpcSetting::gNpcs.end(),
            [objectId](const std::unique_ptr<Npc>& npc){
            return npc->mId == objectId;
            });

        if (it == NpcSetting::gNpcs.end())
            return;

        rotatePacket.yaw = (*it)->mYaw;
    }
    else
    {
        if (objectId < 0 || objectId >= MAX_PLAYERS)
            return;

        if (!clients[objectId] || !clients[objectId]->IsConnected())
            return;

        rotatePacket.yaw = clients[objectId]->mYaw;
    }

    DoSend(sizeof(rotatePacket), reinterpret_cast<char*>(&rotatePacket));
}
