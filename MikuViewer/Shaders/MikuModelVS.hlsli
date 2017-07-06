// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer VSConstants : register(b0)
{
	matrix view;
	matrix projection;
};

cbuffer SkinDualQuat : register(b1)
{
	float4 boneDualQuat[256][2];
}

cbuffer Model : register(b2)
{
	matrix model;
}

// Per-vertex data used as input to the vertex shader.
struct AttributeInput
{
	float3 normal : NORMAL;
	float2 uv : TEXTURE;
	uint2 boneID : BONE_ID;
	uint boneWeight : BONE_WEIGHT;
	uint boneFloat : EDGE_FLAT;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 posH : SV_POSITION;
	float3 posV : POSITION;
	float3 normalV : NORMAL;
	float2 uv : TEXTURE;
};

float2x4 GetBoneDualQuaternion( uint boneIndex )
{
	return float2x4(
		boneDualQuat[boneIndex][0],
		boneDualQuat[boneIndex][1]
	);
}

float2x4 GetBlendedDualQuaternion( uint2 boneIndices, float weight )
{
    float2x4 dq0 = GetBoneDualQuaternion( boneIndices.x );
    float2x4 dq1 = GetBoneDualQuaternion( boneIndices.y );

    float2x4 blendedDQ = lerp(dq0, dq1, weight);
    float normDQ = length(blendedDQ[0]);
    return blendedDQ / normDQ;
}

float3 transformPositionDualQuat( float3 position, float4 realDQ, float4 dualDQ )
{
    return position +
        2 * cross( realDQ.xyz, cross(realDQ.xyz, position) + realDQ.w*position ) +
        2 * (realDQ.w * dualDQ.xyz - dualDQ.w * realDQ.xyz + 
            cross( realDQ.xyz, dualDQ.xyz));
}
 
float3 transformNormalDualQuat( float3 normal, float4 realDQ, float4 dualDQ )
{
    return normal + 2.0 * cross( realDQ.xyz, cross( realDQ.xyz, normal ) + 
                          realDQ.w * normal );
}

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(AttributeInput input, float3 position : POSITION)
{
	PixelShaderInput output;

#ifdef SKINNING_DLB
	float w0 = 1.0 - float(input.boneWeight) / 100.0f;
    float2x4 blended = GetBlendedDualQuaternion( input.boneID.xy, w0 );
    float3 pos = transformPositionDualQuat( position, blended[0], blended[1] );
    float3 normal = transformNormalDualQuat( input.normal, blended[0], blended[1] );
#elif SKINNING_LBS
	float4 pos0 = mul( boneMatrix[input.boneID.x], float4(position, 1.0) );
	float4 pos1 = mul( boneMatrix[input.boneID.y], float4(position, 1.0) );
	float3 pos = float4(lerp(pos0.xyz, pos1.xyz, w0), 1.0f);
	float3 normal0 = mul( (float3x3)boneMatrix[input.boneID.x], input.normal );
	float3 normal1 = mul( (float3x3)boneMatrix[input.boneID.y], input.normal );
	float3 normal = lerp( normal0, normal1, w0 );
#else
    float3 pos = position;
    float3 normal = input.normal;
#endif

    // Transform the vertex position into projected space.
	matrix modelview = mul( view, model );
	float4 posV = mul( modelview, float4(pos, 1.0) );
	output.posV = posV.xyz; 
	output.posH = mul( projection, posV );
	output.normalV = mul( (float3x3)modelview, normal );
	output.uv = input.uv;

	return output;
}
