#pragma once

constexpr short PORT = 3500;

constexpr int MAX_PLAYERS = 50;
constexpr int MAX_NAME_LEN = 20;

constexpr int NPC_COUNT = 10;

constexpr int MAX_OBJECTS = MAX_PLAYERS + NPC_COUNT;

constexpr float PLAYER_MOVE_SPEED = 5.0f;

constexpr float JUMP_POWER = 8.0f;
constexpr float GRAVITY = 20.0f;

enum PACKET_TYPE
{
    C2S_LOGIN,
    C2S_MOVE,
    C2S_MOVE_START,
    C2S_MOVE_STOP,
    C2S_ROTATE,
    C2S_JUMP,

    S2C_LOGIN_RESULT,

    S2C_AVATAR_INFO,
    S2C_ADD_OBJECT,
    S2C_REMOVE_OBJECT,
    S2C_MOVE_OBJECT,
    S2C_ROTATE,
    // 점프 애니메이션 시 구현 필요
    // S2C_JUMP
};

enum DIRECTION
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Todo : 몬스터 타입 정의 필요
enum MONSTER_TYPE
{
};

#pragma pack(push, 1)

struct C2S_Login
{
    unsigned char size;
    PACKET_TYPE type;

    char username[MAX_NAME_LEN];
};

struct C2S_Move
{
    unsigned char size;
    PACKET_TYPE type;

    DIRECTION direction;

    float yaw;
};

struct C2S_Rotate
{
    unsigned char size;
    PACKET_TYPE type;

    int playerId;
    float yaw;
};

struct C2S_Jump
{
    unsigned char size;
    PACKET_TYPE type;
};

struct S2C_LoginResult
{
    unsigned char size;
    PACKET_TYPE type;
    bool success;
    char message[50];
};

struct S2C_AvatarInfo
{
    unsigned char size;
    PACKET_TYPE type;

    int playerId;

    float x;
    float y;
    float z;

    float yaw;
};

struct S2C_AddObject
{
    unsigned char size;
    PACKET_TYPE type;

    int objectId;
    char username[MAX_NAME_LEN];

    float x;
    float y;
    float z;

    float yaw;
};

struct S2C_RemoveObject
{
    unsigned char size;
    PACKET_TYPE type;

    int objectId;
};

struct S2C_MoveObject
{
    unsigned char size;
    PACKET_TYPE type;

    int objectId;

    float x;
    float y;
    float z;
    
    float yaw;
};

struct S2C_Rotate
{
    unsigned char size;
    PACKET_TYPE type;

    int objectId;
    float yaw;
};

#pragma pack(pop)
