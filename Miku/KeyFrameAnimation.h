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

	struct BoneKeyFrame
	{
		int Frame;
		OrthogonalTransform Local; 
		Vector4 BezierCoeff[kInterpMax];
	};

	class BoneMotion
	{
	public:
		std::wstring m_Name; 
		OrthogonalTransform m_Local; 

		uint32_t m_ParentIndex;

		// IK rotation limit
		XMVECTOR m_MinIK;
		XMVECTOR m_MaxIK;
		std::vector<BoneKeyFrame> m_KeyFrames;

		void InsertKeyFrame( const BoneKeyFrame& frame );
		void SortKeyFrame();
		void Interpolate( float t );
	};

	struct FaceKeyFrame
	{
		int32_t Frame;
		float Weight; 
	};

	class FaceMotion
	{
	public:
		std::wstring m_Name; 
		std::vector<FaceKeyFrame> m_KeyFrames;
		std::vector<std::pair<uint32_t, XMFLOAT3>> m_MorphVertices;
		float m_Weight, m_WeightPre;

		FaceMotion();
		void InsertKeyFrame( const FaceKeyFrame& frame );
		void SortKeyFrame();
		void Interpolate( float t );
	};
}