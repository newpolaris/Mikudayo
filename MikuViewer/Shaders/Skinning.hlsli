#define SKINNING_LBS 1

struct SkinData
{
#ifdef SKINNING_DLB
	float4 boneDualQuat[256][2];
#elif SKINNING_LBS
    matrix boneMatrix[256];
#endif
};

struct SkinInput
{
    float3 position;
    float3 normal;
	float boneWeight;
    uint2  boneID;
};

float2x4 GetBoneDualQuaternion( SkinData data, uint boneIndex )
{
#ifdef SKINNING_DLB
	return float2x4(
		data.boneDualQuat[boneIndex][0],
		data.boneDualQuat[boneIndex][1]
	);
#endif
}

float2x4 GetBlendedDualQuaternion( SkinData skin, uint2 boneIndices, float weight )
{
    float2x4 dq0 = GetBoneDualQuaternion( skin, boneIndices.x );
    float2x4 dq1 = GetBoneDualQuaternion( skin, boneIndices.y );

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

void Skinning( SkinInput input, SkinData skin, out float3 pos, out float3 normal )
{
#ifdef SKINNING_DLB
	float w0 = 1.0 - float(input.boneWeight) / 100.0f;
    float2x4 blended = GetBlendedDualQuaternion( skin, input.boneID.xy, w0 );
    pos = transformPositionDualQuat( input.position, blended[0], blended[1] );
    normal = transformNormalDualQuat( input.normal, blended[0], blended[1] );
#elif SKINNING_LBS
	float w0 = 1.0 - float(input.boneWeight) / 100.0f;
	float4 pos0 = mul( skin.boneMatrix[input.boneID.x], float4(input.position, 1.0) );
	float4 pos1 = mul( skin.boneMatrix[input.boneID.y], float4(input.position, 1.0) );
	pos = float4(lerp(pos0.xyz, pos1.xyz, w0), 1.0f).xyz;
	float3 normal0 = mul( (float3x3)skin.boneMatrix[input.boneID.x], input.normal );
	float3 normal1 = mul( (float3x3)skin.boneMatrix[input.boneID.y], input.normal );
	normal = lerp( normal0, normal1, w0 );
#else
    pos = input.position;
    normal = input.normal;
#endif
}

