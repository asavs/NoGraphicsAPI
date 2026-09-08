#pragma once

#include <NoGraphicsAPIUtility/shader_types.h>

struct CryptRootArguments
{
    float2 candle_positions[4];
    float2 player_pos;
    float2 resolution;
    float time;
    uint32 lit_mask;
    int32 active_candle;
};
