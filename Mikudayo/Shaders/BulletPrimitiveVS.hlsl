/*
    Program source code are licensed under the zlib license, except source code of external library.

    Zerogram Sample Program
    http://zerogram.info/

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it freely,
    subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

struct VS_INPUT
{
	float4 Pos : POSITION;
	float3 Nor : NORMAL;
	float2 Tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 Pos : SV_POSITION;
	float3 Nor : NORMAL;
	float2 Tex : TEXCOORD;
	float4 WPos : WORLD_POSITION;
};

cbuffer CB0 : register(b0)
{
	matrix ViewToClip;
};

cbuffer CB1 : register(b1)
{
	matrix World[1024];
};

PS_INPUT main( VS_INPUT input, uint instanceID : SV_InstanceID )
{
	PS_INPUT output = (PS_INPUT)0;
	float4 pos = mul( World[instanceID], input.Pos );
	output.WPos = pos;
	output.Pos = mul( ViewToClip, pos );
	output.Nor = mul( (float3x3)World[instanceID], input.Nor );
	output.Tex = input.Tex;
	return output;
}