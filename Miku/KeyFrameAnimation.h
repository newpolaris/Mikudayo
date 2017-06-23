#pragma once

#include <DirectXMath.h>
#include <vector>
#include "VectorMath.h"

namespace Animation
{
	using namespace DirectX;
	using namespace Math;

	enum EInterpolation : uint8_t {
		kInterpX = 0, kInterpY, kInterpZ, kInterpR, kInterpD, kInterpA 
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
		Vector4 BezierCoeff[kInterpR+1];
	};

	class BoneMotion
	{
	public:
		std::wstring m_Name; 

		bool bLimitXAngle;
		uint32_t m_ParentIndex;
		std::vector<BoneKeyFrame> m_KeyFrames;

		void InsertKeyFrame( const BoneKeyFrame& frame );
		void SortKeyFrame();

		OrthogonalTransform m_Local; 
		void Interpolate( float t );
	};

	struct MorphKeyFrame
	{
		int32_t Frame;
		float Weight; 
	};

	class MorphMotion
	{
	public:
		std::wstring m_Name; 
		std::vector<MorphKeyFrame> m_KeyFrames;
		std::vector<std::pair<uint32_t, XMFLOAT3>> m_MorphVertices;

		MorphMotion();
		void InsertKeyFrame( const MorphKeyFrame& frame );
		void SortKeyFrame();

		float m_Weight, m_WeightPre;
		void Interpolate( float t );
	};

	struct CameraKeyFrame
	{
		int Frame;
		Vector3 Target;
		Quaternion Rotation;
		float Distance;
		float FovY;
		bool bPerspective;
		Vector4 BezierCoeff[kInterpA+1];
	};

	class CameraMotion
	{
	public:
		std::vector<CameraKeyFrame> m_KeyFrames;

		void InsertKeyFrame( const CameraKeyFrame& frame );
		void SortKeyFrame();

		bool m_bPerspective;
		float m_FovY;
		OrthogonalTransform m_CameraToWorld; 
		void Interpolate( float t );
	};
}