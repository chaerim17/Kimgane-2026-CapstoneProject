#pragma once

constexpr short PORT = 3500;

constexpr int MAX_PLAYERS = 50;
constexpr int MAX_NAME_LEN = 20;

constexpr int NPC_COUNT = 10;

constexpr int MAX_OBJECTS = MAX_PLAYERS + NPC_COUNT;

constexpr float PLAYER_MOVE_SPEED = 5.0f;

enum PACKET_TYPE
{
    C2S_LOGIN,
    C2S_MOVE,
    C2S_MOVE_START,
    C2S_MOVE_STOP,
    C2S_ROTATE,

    S2C_LOGIN_RESULT,

    S2C_AVATAR_INFO,
    S2C_ADD_OBJECT,
    S2C_REMOVE_OBJECT,
    S2C_MOVE_OBJECT,
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
    float yaw;
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

#pragma pack(pop)
