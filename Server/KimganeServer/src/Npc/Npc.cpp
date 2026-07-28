#include <random>
#include "Npc.h"

void Npc::RandomMove()
{
    static std::mt19937 rng(std::random_device{}());

    std::uniform_real_distribution<float> moveDist(-mMoveSpeed, mMoveSpeed);

    mX += moveDist(rng);
    mZ += moveDist(rng);
}
