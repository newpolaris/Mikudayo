ByteAddressBuffer SkinUnit : register(t0);

cbuffer SkinningConstants : register(b1)
{
    float4 boneOrtho[1024][2];
}

cbuffer SkinningConstants2 : register(b3)
{
    matrix boneMatrix[1024];
}

static const uint Bdef1 = 0;
static const uint Bdef2 = 1;
static const uint Bdef4 = 2;
static const uint Sdef = 3;
static const uint Qdef = 4;
static const uint elementSize = 52;

float4 MulQuaternions( float4 q1, float4 q2 )
{
	float4 res;
	res.w = q1.w * q2.w - dot( q1.xyz, q2.xyz );
	res.xyz = q1.w * q2.xyz + q2.w * q1.xyz + cross( q1.xyz, q2.xyz );
	return res;
}

float3 MulQuaternionVector( in float4 q, in float3 v )
{
	float3 t = 2.0 * cross( q.xyz, v );
	return v + q.w * t + cross( q.xyz, t );
}

float3 TransformPosition( float4 quat, float4 translate, float3 vec )
{
    return MulQuaternionVector( quat, vec ) + translate.xyz;
}

float3 MultiplyBone( float3 vec, uint boneIndex )
{
    return TransformPosition( boneOrtho[boneIndex][0], boneOrtho[boneIndex][1], vec );
}

float3 Slerp( float3 n0, float3 n1, float t )
{
    float3 N0 = normalize( n0 );
    float3 N1 = normalize( n1 );
    float phi = acos( dot( N0, N1 ) );
    float sinPhi = sin( phi );
    float3 ret = (sin( (1.0f - t)*phi ) / sinPhi)*N0 + (sin( t*phi ) / sinPhi)*N1;
    return phi;
}
#if 0
vec3 slerp( vec3 p1, vec3 p2, float delta ) {
    float omega = acos( dot( p1, p2 ) );
    float sinOmega = sin( omega );
    vec3 n1 = sin( (1.0 - delta) * omega ) / sinOmega * p1;
    vec3 n2 = sin( delta * omega ) / sinOmega * p2;
    return n1 + n2;
}
#endif

float4 q_slerp( float4 a, float4 b, float t ) 
{
    // if either input is zero, return the other.
    if (length(a) == 0.0) {
        if (length(b) == 0.0) {
            return float4(0, 0, 0, 1);
        }
        return b;
    } else if (length(b) == 0.0) {
        return a;
    }
    float cosHalfAngle = a.w * b.w + dot(a.xyz, b.xyz);

    if (cosHalfAngle >= 1.0 || cosHalfAngle <= -1.0) {
        return a;
    } else if (cosHalfAngle < 0.0) {
        b.xyz = -b.xyz;
        b.w = -b.w;
        cosHalfAngle = -cosHalfAngle;
    }

    float blendA;
    float blendB;
    if (cosHalfAngle < 0.99) {
        // do proper slerp for big angles
        float halfAngle = acos(cosHalfAngle);
        float sinHalfAngle = sin(halfAngle);
        float oneOverSinHalfAngle = 1.0 / sinHalfAngle;
        blendA = sin(halfAngle * (1.0 - t)) * oneOverSinHalfAngle;
        blendB = sin(halfAngle * t) * oneOverSinHalfAngle;
    } else {
        // do lerp if angle is really small.
        blendA = 1.0 - t;
        blendB = t;
    }

    float4 result = float4(blendA * a.xyz + blendB * b.xyz, blendA * a.w + blendB * b.w);
    if (length(result) > 0.0) {
        return normalize(result);
    }
    return float4(0, 0, 0, 1);
}

void CalcSdefWeight( out float _rWeight0, out float _rWeight1, in float3 _rSdefR0, in float3 _rSdefR1 )
{
	float l0 = length( _rSdefR0 );
	float l1 = length( _rSdefR1 );
    if (abs( l0 - l1 ) < 0.0001f)
	{
        _rWeight1 = 0.5f;
	}
	else
	{
        _rWeight1 = saturate( l0 / (l0 + l1) );
	}
    _rWeight0 = 1.0f - _rWeight1;
}

float3 Skinning( in float3 position, in uint id )
{
    const uint baseOffset = id * elementSize;
    const uint type = SkinUnit.Load( baseOffset );
    float3 pos = float3(0, 0, 0);
    if (type == Bdef1)
    {
        const uint boneID = SkinUnit.Load( baseOffset + 4 );
        pos = MultiplyBone( position, boneID );
    }
    else if (type == Bdef2)
    {
        const uint2 boneID = SkinUnit.Load2( baseOffset + 4 );
        const float weight = asfloat(SkinUnit.Load( baseOffset + 12 ));
	    pos += weight * MultiplyBone( position, boneID.x );
	    pos += (1-weight) * MultiplyBone( position, boneID.y );
    }
    else if (type == Bdef4)
    {
        const uint4 boneID = SkinUnit.Load4( baseOffset + 4 );
        const float4 weight = asfloat(SkinUnit.Load4( baseOffset + 20 ));
        for (int i = 0; i < 4; i++)
        {
            pos += weight[i] * MultiplyBone( position, boneID[i] );
        }
    }
    else if (type == Sdef)
    {
        const uint2 boneID = SkinUnit.Load2( baseOffset + 4 );
        const float weight = asfloat(SkinUnit.Load( baseOffset + 12 ));
        float3 sdefC = asfloat( SkinUnit.Load3( baseOffset + 16 ) );
        float3 sdefR0 = asfloat( SkinUnit.Load3( baseOffset + 28 ) );
        float3 sdefR1 = asfloat( SkinUnit.Load3( baseOffset + 40 ) );
        float w0 = weight;
        float w1 = 1 - w0;
        matrix m0 = boneMatrix[boneID.x];
        matrix m1 = boneMatrix[boneID.y];
        float w2, w3;
        CalcSdefWeight( w2, w3, sdefR0 + sdefC, sdefR1 + sdefC );

        float4 r0 = float4(sdefR0 + sdefC, 1);
        float4 r1 = float4(sdefR1 + sdefC, 1);
        matrix mrc = m0 * w0 + m1 * w1;
        float3 prc = mul( float4(sdefC, 1), mrc ).xyz;
        {
            matrix m2 = m0 * w0;
            matrix m3 = m1 * w1;
            matrix m = m2 + m3;
            float3 v0 = mul( r0, m2 + m * w1 ).xyz - mul( r0, m ).xyz;
            float3 v1 = mul( r1, m * w0 + m3 ).xyz - mul( r1, m ).xyz;
            prc += v0 * w2 + v1 * w3;
        }
        float4 q0 = boneOrtho[boneID.x][0] * w0;
        float4 q1 = boneOrtho[boneID.y][0] * w1;
        float4 qc = q_slerp( q0, q1, w3 );
        pos = prc + MulQuaternionVector( qc, position.xyz - sdefC );
    }
    return pos;
}
