#pragma once

#include "../Pch.h"
#include "../Config/ServerConfig.h"

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
    SOCKET mClient;
    int mId;
    bool mIsConnected;

    ExpOver mRecvOver;
    int mPrevRecv{};
    char mUserName[MAX_NAME_LEN];

    float mX{}, mY{}, mZ{};
    float mYaw{};

    bool mMoveUp = false;
    bool mMoveDown = false;
    bool mMoveLeft = false;
    bool mMoveRight = false;

    Session();
    ~Session();

    void DoRecv();
    void DoSend(int size, char* buffer);

    void ProcessPacket(unsigned char* packet);

    void SendLoginSuccess();
    void SendAvatarInfo();
    void SendMovePlayer(int moverId);
    void SendAddPlayer(int playerId);
    void SendRemovePlayer(int playerId);
};
