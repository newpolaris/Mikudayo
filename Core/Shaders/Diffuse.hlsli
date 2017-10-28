#define MIX_TYPE  1
#define BCOMP_TYPE  0
#define SAMP_NUM  7

cbuffer Constants : register(b0)
{
    float2 Extent;
};

static float Scaling = 0.1;
static float Strength = 0.7;
static const float2 SampStep = Extent * Scaling;

