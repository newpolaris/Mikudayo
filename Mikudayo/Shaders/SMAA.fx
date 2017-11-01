/**
 * Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
 * Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
 * Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
 * Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
 * Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software. As clarification, there
 * is no requirement that the copyright notice and permission be included in
 * binary distributions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/**
 * This is only required for temporal modes (SMAA T2x).
 */
static const float4 subsampleIndices = float4(0,0,0,0);

/**
 * This is required for blending the results of previous subsample with the
 * output render target; it's used in SMAA S2x and 4x, for other modes just use
 * 1.0 (no blending).
 */
#define blendFactor 1.0
#define blendHalf 0.5

/**
 * This can be ignored; its purpose is to support interactive custom parameter
 * tweaking.
 */
float threshld;
float maxSearchSteps;
float maxSearchStepsDiag;
float cornerRounding;


// Use a real macro here for maximum performance!
#ifndef SMAA_RT_METRICS // This is just for compilation-time syntax checking.
#define SMAA_RT_METRICS float4(1.0 / 1920.0, 1.0 / 1080.0, 1920.0, 1080.0)
#endif

// Set the HLSL version:
#define SMAA_HLSL_4_1
#define SMAA_PRESET_HIGH

// Set preset defines:
#ifdef SMAA_PRESET_CUSTOM
#define SMAA_THRESHOLD threshld
#define SMAA_MAX_SEARCH_STEPS maxSearchSteps
#define SMAA_MAX_SEARCH_STEPS_DIAG maxSearchStepsDiag
#define SMAA_CORNER_ROUNDING cornerRounding
#endif

// And include our header!
#include "SMAA.hlsl"

// Set pixel shader version accordingly:
#ifdef SMAA_HLSL_4_1
#define PS_VERSION ps_4_1
#else
#define PS_VERSION ps_4_0
#endif


/**
 * DepthStencilState's and company
 */
DepthStencilState DisableDepthStencil {
    DepthEnable = FALSE;
    StencilEnable = FALSE;
};

DepthStencilState DisableDepthReplaceStencil {
    DepthEnable = FALSE;
    StencilEnable = TRUE;
    FrontFaceStencilPass = REPLACE;
};

DepthStencilState DisableDepthUseStencil {
    DepthEnable = FALSE;
    StencilEnable = TRUE;
    FrontFaceStencilFunc = EQUAL;
};

BlendState Blend {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;
    SrcBlend[0] = BLEND_FACTOR;
    DestBlend[0] = INV_BLEND_FACTOR;
    BlendOp[0] = ADD;
};

BlendState NoBlending {
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};


/**
 * Pre-computed area and search textures
 */
Texture2D areaTex : register(t0);
Texture2D searchTex : register(t1);
/**
 * Input textures
 */
Texture2D colorTex : register(t2);
Texture2D edgesTex : register(t3);
Texture2D blendTex : register(t4);

Texture2D colorTexPrev;
Texture2DMS<float4, 2> colorTexMS;
Texture2D depthTex;
Texture2D velocityTex;

/**
 * Function wrappers
 */
void DX10_SMAAEdgeDetectionVS(float4 position : POSITION,
                              out float4 svPosition : SV_POSITION,
                              inout float2 texcoord : TEXCOORD0,
                              out float4 offset[3] : TEXCOORD1) {
    svPosition = position;
    SMAAEdgeDetectionVS(texcoord, offset);
}

void DX10_SMAABlendingWeightCalculationVS(float4 position : POSITION,
                                          out float4 svPosition : SV_POSITION,
                                          inout float2 texcoord : TEXCOORD0,
                                          out float2 pixcoord : TEXCOORD1,
                                          out float4 offset[3] : TEXCOORD2) {
    svPosition = position;
    SMAABlendingWeightCalculationVS(texcoord, pixcoord, offset);
}

void DX10_SMAANeighborhoodBlendingVS(float4 position : POSITION,
                                     out float4 svPosition : SV_POSITION,
                                     inout float2 texcoord : TEXCOORD0,
                                     out float4 offset : TEXCOORD1) {
    svPosition = position;
    SMAANeighborhoodBlendingVS(texcoord, offset);
}

void DX10_SMAAResolveVS(float4 position : POSITION,
                        out float4 svPosition : SV_POSITION,
                        inout float2 texcoord : TEXCOORD0) {
    svPosition = position;
}

void DX10_SMAASeparateVS(float4 position : POSITION,
                         out float4 svPosition : SV_POSITION,
                         inout float2 texcoord : TEXCOORD0) {
    svPosition = position;
}

float2 DX10_SMAALumaEdgeDetectionPS(float4 position : SV_POSITION,
                                    float2 texcoord : TEXCOORD0,
                                    float4 offset[3] : TEXCOORD1) : SV_TARGET {
    #if SMAA_PREDICATION
    return SMAALumaEdgeDetectionPS(texcoord, offset, colorTex, depthTex);
    #else
    return SMAALumaEdgeDetectionPS(texcoord, offset, colorTex);
    #endif
}

float2 DX10_SMAAColorEdgeDetectionPS(float4 position : SV_POSITION,
                                     float2 texcoord : TEXCOORD0,
                                     float4 offset[3] : TEXCOORD1) : SV_TARGET {
    #if SMAA_PREDICATION
    return SMAAColorEdgeDetectionPS(texcoord, offset, colorTex, depthTex);
    #else
    return SMAAColorEdgeDetectionPS(texcoord, offset, colorTex);
    #endif
}

float2 DX10_SMAADepthEdgeDetectionPS(float4 position : SV_POSITION,
                                     float2 texcoord : TEXCOORD0,
                                     float4 offset[3] : TEXCOORD1) : SV_TARGET {
    return SMAADepthEdgeDetectionPS(texcoord, offset, depthTex);
}

float4 DX10_SMAABlendingWeightCalculationPS(float4 position : SV_POSITION,
                                            float2 texcoord : TEXCOORD0,
                                            float2 pixcoord : TEXCOORD1,
                                            float4 offset[3] : TEXCOORD2) : SV_TARGET {
    return SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edgesTex, areaTex, searchTex, subsampleIndices);
}

float4 DX10_SMAANeighborhoodBlendingPS(float4 position : SV_POSITION,
                                       float2 texcoord : TEXCOORD0,
                                       float4 offset : TEXCOORD1) : SV_TARGET {
    #if SMAA_REPROJECTION
    return SMAANeighborhoodBlendingPS(texcoord, offset, colorTex, blendTex, velocityTex);
    #else
    return SMAANeighborhoodBlendingPS(texcoord, offset, colorTex, blendTex);
    #endif
}

float4 DX10_SMAAResolvePS(float4 position : SV_POSITION,
                          float2 texcoord : TEXCOORD0) : SV_TARGET {
    #if SMAA_REPROJECTION
    return SMAAResolvePS(texcoord, colorTex, colorTexPrev, velocityTex);
    #else
    return SMAAResolvePS(texcoord, colorTex, colorTexPrev);
    #endif
}

void DX10_SMAASeparatePS(float4 position : SV_POSITION,
                         float2 texcoord : TEXCOORD0,
                         out float4 target0 : SV_TARGET0,
                         out float4 target1 : SV_TARGET1) {
    SMAASeparatePS(position, texcoord, target0, target1, colorTexMS);
}

VertexShader vs_edge = CompileShader(vs_4_0, DX10_SMAAEdgeDetectionVS());
VertexShader vs_blendWeight = CompileShader(vs_4_0, DX10_SMAABlendingWeightCalculationVS());
VertexShader vs_neighborBlend = CompileShader(vs_4_0, DX10_SMAANeighborhoodBlendingVS());
VertexShader vs_resolve = CompileShader(vs_4_0, DX10_SMAAResolveVS());
VertexShader vs_separate = CompileShader(vs_4_0, DX10_SMAASeparateVS());
PixelShader ps_edgeLuma = CompileShader(PS_VERSION, DX10_SMAALumaEdgeDetectionPS());
PixelShader ps_edgeColor = CompileShader(PS_VERSION, DX10_SMAAColorEdgeDetectionPS());
PixelShader ps_edgeDepth = CompileShader(PS_VERSION, DX10_SMAAColorEdgeDetectionPS());
PixelShader ps_blendWeight = CompileShader(PS_VERSION, DX10_SMAABlendingWeightCalculationPS());
PixelShader ps_neighborBlend = CompileShader(PS_VERSION, DX10_SMAANeighborhoodBlendingPS());
PixelShader ps_resolve = CompileShader(PS_VERSION, DX10_SMAAResolvePS());
PixelShader ps_separate = CompileShader(PS_VERSION, DX10_SMAASeparatePS());

/**
 * Edge detection techniques
 */
technique11 LumaEdgeDetection {
    pass LumaEdgeDetection {
        SetVertexShader(vs_edge);
        SetGeometryShader(NULL);
        SetPixelShader(ps_edgeLuma);

        SetDepthStencilState(DisableDepthReplaceStencil, 1);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}

technique11 ColorEdgeDetection {
    pass ColorEdgeDetection {
        SetVertexShader(vs_edge);
        SetGeometryShader(NULL);
        SetPixelShader(ps_edgeColor);

        SetDepthStencilState(DisableDepthReplaceStencil, 1);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}

technique11 DepthEdgeDetection {
    pass DepthEdgeDetection {
        SetVertexShader(vs_edge);
        SetGeometryShader(NULL);
        SetPixelShader(ps_edgeDepth);

        SetDepthStencilState(DisableDepthReplaceStencil, 1);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}

/**
 * Blending weight calculation technique
 */
technique11 BlendingWeightCalculation {
    pass BlendingWeightCalculation {
        SetVertexShader(vs_blendWeight);
        SetGeometryShader(NULL);
        SetPixelShader(ps_blendWeight);

        SetDepthStencilState(DisableDepthUseStencil, 1);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}

/**
 * Neighborhood blending technique
 */
technique11 NeighborhoodBlending {
    pass NeighborhoodBlending {
        SetVertexShader(vs_neighborBlend);
        SetGeometryShader(NULL);
        SetPixelShader(ps_neighborBlend);

        SetDepthStencilState(DisableDepthStencil, 0);
        SetBlendState(Blend, float4(blendFactor, blendFactor, blendFactor, blendFactor), 0xFFFFFFFF);
        // For SMAA 1x, just use NoBlending!
    }
}

technique11 NeighborhoodBlendingHalf {
    pass NeighborhoodBlending {
        SetVertexShader(vs_neighborBlend);
        SetGeometryShader(NULL);
        SetPixelShader(ps_neighborBlend);

        SetDepthStencilState(DisableDepthStencil, 0);
        SetBlendState(Blend, float4(blendHalf, blendHalf, blendHalf, blendHalf), 0xFFFFFFFF);
        // For SMAA 1x, just use NoBlending!
    }
}

/**
 * Temporal resolve technique
 */
technique11 Resolve {
    pass Resolve {
        SetVertexShader(vs_resolve);
        SetGeometryShader(NULL);
        SetPixelShader(ps_resolve);

        SetDepthStencilState(DisableDepthStencil, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}

/**
 * 2x multisampled buffer conversion into two regular buffers
 */
technique11 Separate {
    pass Separate {
        SetVertexShader(vs_separate);
        SetGeometryShader(NULL);
        SetPixelShader(ps_separate);

        SetDepthStencilState(DisableDepthStencil, 0);
        SetBlendState(NoBlending, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
    }
}