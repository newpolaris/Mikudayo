#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 12
#define FXAA_GREEN_AS_LUMA 1

#include "FXAAPS.hlsli"

cbuffer CB0 : register(b0)
{
    float2 RcpTextureSize;
    float ContrastThreshold;	// default = 0.166, lower is more expensive
    float ContrastThresholdMin;	// default = 0.0833, lower is more expensive
    float SubpixelRemoval;		// default = 0.75, lower blurs less
};

struct FxaaVS_Output {
    float4 Pos : SV_Position;
    float2 Tex : TexCoords0;
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
        RcpTextureSize.xy,						// FxaaFloat2 fxaaQualityRcpFrame,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt2,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsole360RcpFrameOpt2,
        SubpixelRemoval,						// FxaaFloat fxaaQualitySubpix,
        ContrastThreshold,						// FxaaFloat fxaaQualityEdgeThreshold,
        ContrastThresholdMin,					// FxaaFloat fxaaQualityEdgeThresholdMin,
        0.0f,									// FxaaFloat fxaaConsoleEdgeSharpness,
        0.0f,									// FxaaFloat fxaaConsoleEdgeThreshold,
        0.0f,									// FxaaFloat fxaaConsoleEdgeThresholdMin,
        FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)		// FxaaFloat fxaaConsole360ConstDir,
    );
}