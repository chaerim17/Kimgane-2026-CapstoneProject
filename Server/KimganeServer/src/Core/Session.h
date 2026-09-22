#pragma once

#include "../Pch.h"
#include "../Config/ServerConfig.h"
#include "../../../../Shared/Physics/RigidbodyTypes.h"

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
    char mUserName[MAX_NAME_LEN];

    float mX{}, mY{}, mZ{};
    float mYaw{};      // 정면 표시(브로드캐스트)용
    float mMoveYaw{};  // 이동 계산 전용

    bool mMoveUp = false;
    bool mMoveDown = false;
    bool mMoveLeft = false;
    bool mMoveRight = false;

    // 입력과 강체 상태 접근은 기존 Server::mWorldMutex 아래에서 처리합니다.
    bool mJumpRequested = false;
    Kimgane::Shared::Physics::RigidbodyState mMovementState;

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
    void SendAvatarInfo();
    void SendMoveObject(int moverId);
    void SendAddObject(int objectId);
    void SendRemoveObject(int objectId);
    void SendRotateObject(int objectId);
    void SendDamage(int attackerId, int targetId, int damage, int maxHp, int currentHp);

private:
    SOCKET mClient;
    int mId;
    bool mIsConnected;
};
