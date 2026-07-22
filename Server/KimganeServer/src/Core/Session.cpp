#include "Session.h"
#include "Server.h"

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
    PACKET_TYPE type = *reinterpret_cast<PACKET_TYPE*>(&packet[1]);
    switch (type)
    {
    case C2S_LOGIN:
    {
        /* C2S_Login* loginPacket = reinterpret_cast<C2S_Login*>(packet);
         std::cout << "username = " << loginPacket->username << '\n';
         strncpy_s(mUserName, loginPacket->username, MAX_NAME_LEN);
         */
        std::cout << "Client[" << mId << "] Login: " << mUserName << std::endl;

        SendAvatarInfo();
        SendLoginSuccess();

        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (clients[i] && clients[i]->mIsConnected && i != mId)
            {
                SendAddPlayer(i);
                clients[i]->SendAddPlayer(mId);
            }
        }

        break;
    }

    case C2S_MOVE_START:
    {
        auto* movePacket = reinterpret_cast<C2S_Move*>(packet);
        mYaw = movePacket->yaw;

        switch (movePacket->direction)
        {
        case UP:
            mMoveUp = true;
            break;
        case DOWN:
            mMoveDown = true;
            break;
        case LEFT:
            mMoveLeft = true;
            break;
        case RIGHT:
            mMoveRight = true;
            break;
        }

        std::cout << "[START] Player " << mId << '\n';

        break;
    }

    case C2S_MOVE_STOP:
    {
        auto* movePacket = reinterpret_cast<C2S_Move*>(packet);

        switch (movePacket->direction)
        {
        case UP:
            mMoveUp = false;
            break;
        case DOWN:
            mMoveDown = false;
            break;
        case LEFT:
            mMoveLeft = false;
            break;
        case RIGHT:
            mMoveRight = false;
            break;
        }

        std::cout << "[STOP] Player " << mId << '\n';

        break;
    }
    }
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
void Session::SendMovePlayer(int moverId)
{
    if (!clients[moverId] || !clients[moverId]->mIsConnected)
        return;

    S2C_MovePlayer movePlayerPacket;
    movePlayerPacket.size = sizeof(movePlayerPacket);
    movePlayerPacket.type = S2C_MOVE_PLAYER;
    movePlayerPacket.playerId = moverId;
    movePlayerPacket.x = clients[moverId]->mX;
    movePlayerPacket.y = clients[moverId]->mY;
    movePlayerPacket.z = clients[moverId]->mZ;
    movePlayerPacket.yaw = clients[moverId]->mYaw;
    DoSend(movePlayerPacket.size, reinterpret_cast<char*>(&movePlayerPacket));
}
void Session::SendAddPlayer(int playerId)
{
    S2C_AddPlayer addPlayerPacket;
    addPlayerPacket.size = sizeof(addPlayerPacket);
    addPlayerPacket.type = S2C_ADD_PLAYER;
    addPlayerPacket.playerId = playerId;
    addPlayerPacket.x = clients[playerId]->mX;
    addPlayerPacket.y = clients[playerId]->mY;
    addPlayerPacket.z = clients[playerId]->mZ;
    addPlayerPacket.yaw = clients[playerId]->mYaw;

    DoSend(addPlayerPacket.size, reinterpret_cast<char*>(&addPlayerPacket));
}
void Session::SendRemovePlayer(int playerId)
{
    S2C_RemovePlayer removePlayerPacket;
    removePlayerPacket.size = sizeof(S2C_RemovePlayer);
    removePlayerPacket.type = S2C_REMOVE_PLAYER;
    removePlayerPacket.playerId = playerId;
    DoSend(removePlayerPacket.size, reinterpret_cast<char*>(&removePlayerPacket));
}
