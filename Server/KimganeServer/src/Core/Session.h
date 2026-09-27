#pragma once

#include "../Pch.h"
#include "../Config/ServerConfig.h"

class GameObject;
class Player;

enum IOType
{
    IO_SEND,
    IO_RECV,
    IO_ACCEPT
};

class ExpOver
{
public:
    WSAOVERLAPPED mOver;
    IOType mIoType;
    WSABUF mWsa;
    char mBuffer[BUF_SIZE];

    ExpOver()
    {
        ZeroMemory(&mOver, sizeof(mOver));
        mWsa.buf = mBuffer;
        mWsa.len = BUF_SIZE;
    }

    ExpOver(IOType ioType) : mIoType(ioType)
    {
        ZeroMemory(&mOver, sizeof(mOver));
        mWsa.buf = mBuffer;
        mWsa.len = BUF_SIZE;
    }
};

class Session
{
public:
    ExpOver mRecvOver;
    int mPrevRecv{};

public:
    Session();
    ~Session();

    SOCKET GetSocket() const;
    int GetId() const;
    bool IsConnected() const;

    void Connect(SOCKET socket, int id);
    void Disconnect();

    void DoRecv();
    void DoSend(int size, char* buffer);

    void SendLoginSuccess();
    void SendAvatarInfo(const Player& player);
    void SendMoveObject(const GameObject& object);
    void SendAddObject(const GameObject& object, int maxHp = 0, int currentHp = 0);
    void SendRemoveObject(int objectId);
    void SendRotateObject(const GameObject& object);
    void SendDamage(int attackerId, int targetId, int damage, int maxHp, int currentHp);

private:
    SOCKET mClient;
    int mId;
    bool mIsConnected;
};
