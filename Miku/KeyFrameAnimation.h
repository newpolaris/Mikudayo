#pragma once

#include <DirectXMath.h>
#include <vector>
#include "VectorMath.h"

namespace Animation
{
	using namespace DirectX;
	using namespace Math;

	enum EInterpolation : uint8_t {
		kInterpX = 0, kInterpY, kInterpZ, kInterpR, kInterpMax
	};

	//
	// http://d.hatena.ne.jp/edvakf/20111016/1318716097
	// http://zerogram.info/?p=860
	//
	float Bezier( Vector4 C, float p );

	struct KeyFrame
	{
		int Frame;
		Vector3 Translation;
		Quaternion Rotation;
		Vector4 BezierCoeff[kInterpMax];
	};

	class MeshBone
	{
	public:
		std::wstring m_Name; 
		Vector3 m_Scale;
		Vector3 m_LocalPosition;
		Quaternion m_Rotation;
		uint32_t m_ParentIndex;

		// IK rotation limit
		XMVECTOR m_MinIK;
		XMVECTOR m_MaxIK;

		std::vector<KeyFrame> m_KeyFrames;

		void InsertKeyFrame( const KeyFrame& frame );
		void SortKeyFrame();
		void Interpolate( float t );
	};
}