#include "Session.h"
#include "Server.h"
#include "../Network/PacketHandler.h"

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
void Session::SendMoveObject(int moverId)
{
    if (!clients[moverId] || !clients[moverId]->mIsConnected)
        return;

    S2C_MoveObject moveObjectPacket;
    moveObjectPacket.size = sizeof(moveObjectPacket);
    moveObjectPacket.type = S2C_MOVE_OBJECT;
    moveObjectPacket.objectId = moverId;
    moveObjectPacket.x = clients[moverId]->mX;
    moveObjectPacket.y = clients[moverId]->mY;
    moveObjectPacket.z = clients[moverId]->mZ;
    moveObjectPacket.yaw = clients[moverId]->mYaw;
    DoSend(moveObjectPacket.size, reinterpret_cast<char*>(&moveObjectPacket));
}
void Session::SendAddObject(int objectId)
{
    S2C_AddObject addObjectPacket;
    addObjectPacket.size = sizeof(addObjectPacket);
    addObjectPacket.type = S2C_ADD_OBJECT;
    addObjectPacket.objectId = objectId;
    addObjectPacket.x = clients[objectId]->mX;
    addObjectPacket.y = clients[objectId]->mY;
    addObjectPacket.z = clients[objectId]->mZ;
    addObjectPacket.yaw = clients[objectId]->mYaw;

    DoSend(addObjectPacket.size, reinterpret_cast<char*>(&addObjectPacket));
}
void Session::SendRemoveObject(int objectId)
{
    S2C_RemoveObject removeObjectPacket;
    removeObjectPacket.size = sizeof(S2C_RemoveObject);
    removeObjectPacket.type = S2C_REMOVE_OBJECT;
    removeObjectPacket.objectId = objectId;
    DoSend(removeObjectPacket.size, reinterpret_cast<char*>(&removeObjectPacket));
}
