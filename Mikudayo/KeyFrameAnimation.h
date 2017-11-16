#pragma once

#include <vector>
#include "VectorMath.h"

namespace Animation
{
	using namespace Math;

	enum EInterpolation : uint8_t {
		kInterpX = 0, kInterpY, kInterpZ, kInterpR, kInterpD, kInterpA
	};

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
		std::vector<BoneKeyFrame> m_KeyFrames;

		void InsertKeyFrame( const BoneKeyFrame& frame );
		void SortKeyFrame();
		void Interpolate( float t, OrthogonalTransform& local );
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
        std::wstring m_NameEnglish;
		std::vector<MorphKeyFrame> m_KeyFrames;
		std::vector<uint32_t> m_MorphIndices;
		std::vector<Vector3> m_MorphVertices;

		MorphMotion();
		void InsertKeyFrame( const MorphKeyFrame& frame );
		void SortKeyFrame();

		float m_Weight, m_WeightPre;
		void Interpolate( float t );
	};

	struct CameraFrame
	{
		static CameraFrame Default();

		bool bPerspective;
		float FovY;
		float Distance;
		Vector3 Position;
		Quaternion Rotation;
	};

	struct CameraKeyFrame
	{
		int Frame;
		CameraFrame Data;
		Vector4 BezierCoeff[kInterpA+1];
	};

	class CameraMotion
	{
	public:
		std::vector<CameraKeyFrame> m_KeyFrames;

		void InsertKeyFrame( const CameraKeyFrame& frame );
		void SortKeyFrame();
		CameraFrame Interpolate( float t );
	};
}