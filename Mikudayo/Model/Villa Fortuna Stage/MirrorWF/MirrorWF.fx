////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MirrorWF.fx ver0.0.4  任意平面への鏡像描画
//  作成: 針金P( 舞力介入P氏のMirror.fx, full.fx,改変 )
//
////////////////////////////////////////////////////////////////////////////////////////////////
// アクセに組み込む場合はここを適宜変更してください．
float3 MirrorColor = float3(1.0, 1.0, 1.0); // 鏡面の乗算色(RGB)
float3 MirrorAlpha = 1.0; // 鏡面の初期透過値

///////////////////////////////////////////////////////////////////////////////////////////////

// 座標変換行列
float4x4 WorldMatrix     : WORLD;
float4x4 ViewMatrix      : VIEW;
float4x4 ProjMatrix      : PROJECTION;
float4x4 ViewProjMatrix  : VIEWPROJECTION;

//カメラ位置
float3 CameraPosition : POSITION  < string Object = "Camera"; >;

// マテリアル色
float4 MaterialDiffuse : DIFFUSE  < string Object = "Geometry"; >;
static float AcsAlpha = MaterialDiffuse.a;

// スクリーンサイズ
float2 ViewportSize : VIEWPORTPIXELSIZE;
static float2 ViewportOffset = (float2(0.5f, 0.5f)/ViewportSize);

//MMM対応
#ifndef MIKUMIKUMOVING
    #define OFFSCREEN_FX_HIDE    "hide"
    #define OFFSCREEN_FX_OBJECT  "MWF_Object.fxsub"     // オフスクリーンモデル描画エフェクト1
    #define GET_VPMAT(p) (ViewProjMatrix)
#else
    #define OFFSCREEN_FX_HIDE    "Hide.fxsub"
    #define OFFSCREEN_FX_OBJECT  "MWF_Object_MMM.fxsub" // オフスクリーンモデル描画エフェクト1
    #define GET_VPMAT(p) (MMM_IsDinamicProjection ? mul(ViewMatrix, MMM_DynamicFov(ProjMatrix, length(CameraPosition-p.xyz))) : ViewProjMatrix)
#endif

// 鏡像描画のオフスクリーンバッファ
texture MirrorWFRT : OFFSCREENRENDERTARGET <
    string Description = "OffScreen RenderTarget for MirrorWF.fx";
    float2 ViewPortRatio = {1.0,1.0};
    float4 ClearColor = { 1, 1, 1, 1 };
    float ClearDepth = 1.0;
    bool AntiAlias = true;
    string DefaultEffect = 
        "self = " OFFSCREEN_FX_HIDE ";"
        "* = " OFFSCREEN_FX_OBJECT ";" ;
>;
sampler MirrorWFView = sampler_state {
    texture = <MirrorWFRT>;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    MipFilter = NONE;
    AddressU  = CLAMP;
    AddressV = CLAMP;
};


////////////////////////////////////////////////////////////////////////////////////////////////
// 鏡像描画

struct VS_OUTPUT {
    float4 Pos  : POSITION;
    float4 VPos : TEXCOORD1;
};

VS_OUTPUT VS_Mirror(float4 Pos : POSITION)
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    // ワールド座標変換
    Pos = mul( Pos, WorldMatrix );

    // カメラ視点のビュー射影変換
    Out.Pos = mul( Pos, GET_VPMAT(Pos) );
    Out.VPos = Out.Pos;

    return Out;
}

float4 PS_Mirror(VS_OUTPUT IN) : COLOR
{
    // 鏡像のスクリーンの座標(左右反転しているので元に戻す)
    float2 texCoord = float2( 1.0f - ( IN.VPos.x/IN.VPos.w + 1.0f ) * 0.5f,
                              1.0f - ( IN.VPos.y/IN.VPos.w + 1.0f ) * 0.5f ) + ViewportOffset;

    // 鏡像の色
    float4 Color = tex2D(MirrorWFView, texCoord);
    Color.xyz *= MirrorColor;
    Color.a *= AcsAlpha * MirrorAlpha;

    return Color;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//テクニック

technique MainTec{
    pass DrawObject{
        CullMode = NONE;
        VertexShader = compile vs_2_0 VS_Mirror();
        PixelShader  = compile ps_2_0 PS_Mirror();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////



