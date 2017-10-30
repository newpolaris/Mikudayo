static const float3 BaseBias = float3(1,1,1)*1.1;
static const float3 ShadowBias = float3(1,0.8,0.8);
static float SP_Power = 1.0;
static float SP_Scale = 0.2;
static float3 SP_Add = float3(0.2,0.1,0.1);
static float RimPow	= 8.0;
static float3 RimLight = 1*float3(255,255,255)/255.0;
static float LightScale = 1.0;
static float MainLightParam = 0.7;
static float SubLightParam = 0.5;
static float MainHLamb = 0.25;
static float SubHLamb = 0.5;
static float ShadowLimitLength = 0;
static float SoftShadowParam = 0.5;
#define SHADOWMAP_SIZE 4096

static float3 LightPos[16] = {
    float3(0.53812504, 0.18565957, -0.43192),
    float3(0.13790712, 0.24864247, 0.44301823),
    float3(0.33715037, 0.56794053, -0.005789503),
    float3(-0.6999805, -0.04511441, -0.0019965635),
    float3(0.06896307, -0.15983082, -0.85477847),
    float3(0.056099437, 0.006954967, -0.1843352),
    float3(-0.014653638, 0.14027752, 0.0762037),
    float3(0.010019933, -0.1924225, -0.034443386),
    float3(-0.35775623, -0.5301969, -0.43581226),
    float3(-0.3169221, 0.106360726, 0.015860917),
    float3(0.010350345, -0.58698344, 0.0046293875),
    float3(-0.08972908, -0.49408212, 0.3287904),
    float3(0.7119986, -0.0154690035, -0.09183723),
    float3(-0.053382345, 0.059675813, -0.5411899),
    float3(0.035267662, -0.063188605, 0.54602677),
    float3(-0.47761092, 0.2847911, -0.0271716)
};