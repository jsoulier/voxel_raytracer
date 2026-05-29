#pragma once

#include <array>
#include <cstdint>

#include "buffer.hpp"

#define BLOCKS \
    X(Air) \
    X(Grass) \
    X(BirchGrass) \
    X(JungleGrass) \
    X(CherryGrass) \
    X(AutumnGrass) \
    X(BlueGrass) \
    X(Dirt) \
    X(Sand) \
    X(Water) \
    X(Glass) \
    X(Stone) \
    X(OakWood) \
    X(OakLeaves) \
    X(Snow) \
    X(PineWood) \
    X(PineLeaves) \
    X(BirchWood) \
    X(BirchLeaves) \
    X(JungleWood) \
    X(JungleLeaves) \
    X(CherryWood) \
    X(CherryLeaves) \
    X(MapleWood) \
    X(MapleLeaves) \
    X(BlueWood) \
    X(BlueLeaves) \
    X(Clay) \
    X(Steel) \
    X(RedLight) \
    X(GreenLight) \
    X(BlueLight) \
    X(CyanLight) \
    X(MagentaLight) \
    X(YellowLight) \
    X(WhiteLight) \

enum Block : uint8_t
{
#define X(block) Block##block,
    BLOCKS
#undef X
    BlockCount,
    BlockFirst = BlockAir + 1,
    BlockLast = BlockCount - 1,
};

struct BlockProperty
{
    uint32_t Color;
    float Light;
    float Roughness;
    float IOR;
};

using BlockState = std::array<BlockProperty, BlockCount>;

BlockState BlockGetState();
const char* BlockToString(Block block);
const char** BlockGetStrings();
