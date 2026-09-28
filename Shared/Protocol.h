#pragma once

#include "World/ObjectTypes.h"
#include "Items/ItemTypes.h"

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
    C2S_PLAYER_STATE,       // 플레이어의 움직임 전송
    C2S_SHOOT,
    C2S_OPEN_ITEM_BOX,

    S2C_LOGIN_RESULT,

    S2C_AVATAR_INFO,
    S2C_ADD_OBJECT,
    S2C_REMOVE_OBJECT,
    S2C_MOVE_OBJECT,
    S2C_ROTATE,
    // 점프 애니메이션 시 구현 필요
    // S2C_JUMP
    S2C_DAMAGE,
    S2C_OPEN_ITEM_BOX_RESULT,
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

enum class OpenItemBoxStatus : std::uint8_t
{
    Success = 0,
    NotFound = 1,       // 다른 플레이어가 먼저 수령한 경우도 포함
    NotItemBox = 2,
    TooFar = 3,
    RewardFailed = 4
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

struct C2S_PlayerState
{
    unsigned char size;
    PACKET_TYPE type;

    float x;
    float y;
    float z;

    float yaw;
    bool isJumping;
};

// 패킷 전송용 벡터 데이터. 연산 후 vec3 구조체로 변환할 것
struct Vec3
{
    float x;
    float y;
    float z;
};

struct C2S_Shoot
{
    unsigned char size;
    PACKET_TYPE type;

    int playerId;

    Vec3 direction;
};

// 플레이어는 접속 세션으로 구분, 요청에는 상자 ID만 보냄
struct C2S_OpenItemBox
{
    unsigned char size;
    PACKET_TYPE type;
    int objectId;
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
    Kimgane::Shared::World::ObjectType objectType;
    char username[MAX_NAME_LEN];

    float x;
    float y;
    float z;

    float yaw;

    int maxHp; // 0이면 HP바를 사용하지 않음. (현재 타 player)
    int currentHp;
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

struct S2C_Damage
{
    unsigned char size;
    PACKET_TYPE type;

    int attackerId;
    int targetId;
    int damage;    // 이번에 실제 적용된 데미지
    int maxHp;
    int currentHp; // 데미지 적용 후 체력
};

struct S2C_OpenItemBoxResult
{
    unsigned char size;
    PACKET_TYPE type;
    int objectId;
    OpenItemBoxStatus status;
    // 실패 시 None/0/0 성공 시 보상과 지급 계산 후 서버의 총 보유 수량
    Kimgane::Shared::Items::ItemId itemId;
    std::uint32_t quantity;
    std::uint64_t totalQuantity;
};

#pragma pack(pop)
