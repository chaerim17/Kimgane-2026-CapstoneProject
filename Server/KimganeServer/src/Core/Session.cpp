#include "Session.h"
#include "../World/Player.h"

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

void Session::SendOpenItemBoxResult(int objectId, OpenItemBoxStatus status,
    Kimgane::Shared::Items::ItemReward reward, std::uint64_t totalQuantity)
{
    S2C_OpenItemBoxResult packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_OPEN_ITEM_BOX_RESULT;
    packet.objectId = objectId;
    packet.status = status;
    packet.itemId = reward.itemId;
    packet.quantity = reward.quantity;
    packet.totalQuantity = totalQuantity;
    std::cout << "[OPEN_BOX RESULT SEND] playerId=" << GetId() << " objectId=" << objectId
              << " status=" << static_cast<int>(status)
              << " itemId=" << static_cast<int>(reward.itemId)
              << " quantity=" << reward.quantity << " total=" << totalQuantity << '\n';
    DoSend(sizeof(packet), reinterpret_cast<char*>(&packet));
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
void Session::SendAvatarInfo(const Player& player)
{
    const auto& position = player.GetPositionM();
    S2C_AvatarInfo packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_AVATAR_INFO;
    packet.playerId = player.GetId();
    packet.x = position.x;
    packet.y = position.y;
    packet.z = position.z;
    packet.yaw = player.GetYawRad();
    DoSend(sizeof(packet), reinterpret_cast<char*>(&packet));
}

void Session::SendMoveObject(const GameObject& object)
{
    const auto& position = object.GetPositionM();
    S2C_MoveObject packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_MOVE_OBJECT;
    packet.objectId = object.GetId();
    packet.x = position.x;
    packet.y = position.y;
    packet.z = position.z;
    packet.yaw = object.GetYawRad();
    DoSend(sizeof(packet), reinterpret_cast<char*>(&packet));
}

void Session::SendAddObject(const GameObject& object, int maxHp, int currentHp)
{
    const auto& position = object.GetPositionM();
    S2C_AddObject packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_ADD_OBJECT;
    packet.objectId = object.GetId();
    packet.objectType = object.GetType();
    packet.x = position.x;
    packet.y = position.y;
    packet.z = position.z;
    packet.yaw = object.GetYawRad();
    packet.maxHp = maxHp;
    packet.currentHp = currentHp;
    // 디버깅: 오브젝트 정보를 제대로 보내는지 확인
    //std::cout << "[ADD_OBJECT SEND] recipientId=" << GetId()
    //          << " size=" << static_cast<int>(packet.size)
    //          << " packetType=" << static_cast<int>(packet.type)
    //          << " objectId=" << packet.objectId
    //          << " objectType=" << static_cast<int>(packet.objectType)
    //          << " pos=(" << packet.x << ", " << packet.y << ", " << packet.z << ')'
    //          << " yaw=" << packet.yaw
    //          << " maxHp=" << packet.maxHp << " currentHp=" << packet.currentHp << '\n';
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

void Session::SendRotateObject(const GameObject& object)
{
    S2C_Rotate packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_ROTATE;
    packet.objectId = object.GetId();
    packet.yaw = object.GetYawRad();
    DoSend(sizeof(packet), reinterpret_cast<char*>(&packet));
}

void Session::SendDamage(int attackerId, int targetId, int damage, int maxHp, int currentHp)
{
    S2C_Damage packet{};
    packet.size = sizeof(packet);
    packet.type = S2C_DAMAGE;
    packet.attackerId = attackerId;
    packet.targetId = targetId;
    packet.damage = damage;
    packet.maxHp = maxHp;
    packet.currentHp = currentHp;

    DoSend(packet.size, reinterpret_cast<char*>(&packet));

     std::cout << "[DAMAGE SEND] recipientId=" << GetId()
               << " attackerId=" << packet.attackerId << " targetId=" << packet.targetId
               << " damage=" << packet.damage << " maxHp=" << packet.maxHp
               << " currentHp=" << packet.currentHp << '\n';
}
