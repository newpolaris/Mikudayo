#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 12

#include "Fxaa3_11.h"

cbuffer cbFxaa : register(b0) {
    float2 RCPFrame;
};

struct FxaaVS_Output {
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

SamplerState InputSampler : register(s0);
Texture2D    InputTexture : register(t0);

float4 main(FxaaVS_Output Input) : SV_TARGET {
    FxaaTex InputFXAATex = { InputSampler, InputTexture };
    return FxaaPixelShader(
        Input.Tex.xy,							// FxaaFloat2 pos,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsolePosPos,
        InputFXAATex,							// FxaaTex tex,
        InputFXAATex,							// FxaaTex fxaaConsole360TexExpBiasNegOne,
        InputFXAATex,							// FxaaTex fxaaConsole360TexExpBiasNegTwo,
        RCPFrame.xy,							// FxaaFloat2 fxaaQualityRcpFrame,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt2,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsole360RcpFrameOpt2,
        0.75f,									// FxaaFloat fxaaQualitySubpix,
        0.166f,									// FxaaFloat fxaaQualityEdgeThreshold,
        0.0833f,								// FxaaFloat fxaaQualityEdgeThresholdMin,
        0.0f,									// FxaaFloat fxaaConsoleEdgeSharpness,
        0.0f,									// FxaaFloat fxaaConsoleEdgeThreshold,
        0.0f,									// FxaaFloat fxaaConsoleEdgeThresholdMin,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)		// FxaaFloat fxaaConsole360ConstDir,
    );
}