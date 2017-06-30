struct FxaaVS_Output {
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

FxaaVS_Output main(uint id : SV_VertexID) {
    FxaaVS_Output Output;
    Output.Tex = float2((id << 1) & 2, id & 2);
    Output.Pos = float4(Output.Tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return Output;
}