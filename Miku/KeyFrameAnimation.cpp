#include "KeyFrameAnimation.h"

#include <algorithm>
#include "Utility.h"

using namespace Animation;
using namespace Math;

template <typename T>
int32_t FindPreviousFrameIndex( const std::vector<T>& frames, const float t )
{
	if (frames.size() <= 0)
		return -1;

	int32_t low = 0, hi = (int32_t)frames.size();
	while (low < hi)
	{
		auto mid = low + (hi - low + 1) / 2;
		if (frames[mid].Frame >= t)
			hi = mid - 1;
		else
			low = mid;
	}

	if (frames[low].Frame >= t)
		return -1;
	return low;
}

//
// http://d.hatena.ne.jp/edvakf/20111016/1318716097
//
// TODO: apply partial cache, optimize (https://github.com/gre/bezier-easing/blob/master/src/index.js)
//
float Animation::Bezier( Vector4 C, float p )
{
	XMFLOAT4 coeff;
	XMStoreFloat4( &coeff, C );

	const float x1 = coeff.x, y1 = coeff.y, x2 = coeff.z, y2 = coeff.w;

	auto ft = [=](float t) {
		float s = 1.0f - t;
		return 3*s*s*t*x1 + 3*s*t*t*x2 + t*t*t;
	};
	float low = 0.0f, high = 1.0f;
	for (int i = 0; i < 15; i++) {
		float mid = (low + high) / 2;
		if (ft(mid) - p < 0)
			low = mid;
		else
			high = mid;
	}
	float t = (low + high) / 2, s = 1.0f - t;	
	return 3 * s*s*t*y1 + 3 * s*t*t*y2 + t*t*t;
}

const BoneKeyFrame& ZeroFrame()
{
	static BoneKeyFrame zero;
	zero.Frame = 0;
	zero.Local = OrthogonalTransform( kIdentity );

	// linear
	for (uint8_t k = 0; k < 4; k++)
		zero.BezierCoeff[k] = Vector4( 0.15748f, 0.1574f, 0.8425f, 0.8425f );
	return zero;
}

void BoneMotion::InsertKeyFrame( const BoneKeyFrame& frame )
{
	m_KeyFrames.push_back( frame );
}

void BoneMotion::SortKeyFrame()
{
	std::sort( m_KeyFrames.begin(), m_KeyFrames.end(), [](const auto& a, const auto& b) {
		return a.Frame < b.Frame;
	});
}

void BoneMotion::Interpolate( float t, OrthogonalTransform & local )
{
	if (m_KeyFrames.size() == 0)
		return;

	auto& first = m_KeyFrames.front();
	auto& last = m_KeyFrames.back();

	if (t <= first.Frame)
	{
		local = first.Local;
	}
	else if (t >= last.Frame)
	{
		local = last.Local;
	}
	else
	{
		// VMD provide 0 frame. So, below zero check code is not nesseary.
		int32_t prev = FindPreviousFrameIndex( m_KeyFrames, t );
		auto& a = (prev < 0) ? ZeroFrame() : m_KeyFrames[prev];
		auto& b = m_KeyFrames[prev + 1];

		float p = 1.f;
		if (b.Frame - a.Frame > 0)
			p = (t - a.Frame) / (b.Frame - a.Frame);

		float c[kInterpR+1];
		for (uint8_t k = kInterpX; k <= kInterpR; k++)
			c[k] = Bezier( a.BezierCoeff[k], p );
		
		local.SetTranslation( Lerp( a.Local.GetTranslation(), b.Local.GetTranslation(), Vector3( c[kInterpX], c[kInterpY], c[kInterpZ] ) ) );
		local.SetRotation( Slerp( a.Local.GetRotation(), b.Local.GetRotation(), c[kInterpR] ) );
	}
}

void MorphMotion::Interpolate( float t )
{
	if (m_KeyFrames.size() == 0)
		return;

	m_WeightPre = m_Weight;

	auto& first = m_KeyFrames.front();
	auto& last = m_KeyFrames.back();

	if (t <= first.Frame)
		m_Weight = first.Weight;
	else if (t >= last.Frame)
		m_Weight = last.Weight;
	else
	{
		auto prev = FindPreviousFrameIndex( m_KeyFrames, t );
		auto &a = m_KeyFrames[prev], &b = m_KeyFrames[prev+1];
		float p = 1.0f;
		if (b.Frame - a.Frame > 0)
			p = (t - a.Frame) / (b.Frame - a.Frame);
		m_Weight = a.Weight * (1.0f - p) + b.Weight * p;
	}
}

MorphMotion::MorphMotion() : m_Weight( 0.f ), m_WeightPre( 0.f )
{
}

void MorphMotion::InsertKeyFrame( const MorphKeyFrame& frame )
{
	m_KeyFrames.push_back( frame );
}

void MorphMotion::SortKeyFrame()
{
	std::sort( m_KeyFrames.begin(), m_KeyFrames.end(), 
		[]( auto& a, auto& b ) { return a.Frame < b.Frame; } );
}

void CameraMotion::InsertKeyFrame( const CameraKeyFrame& frame )
{
	m_KeyFrames.push_back( frame );
}

void CameraMotion::SortKeyFrame()
{
	std::sort( m_KeyFrames.begin(), m_KeyFrames.end(), [](const auto& a, const auto& b) {
		return a.Frame < b.Frame;
	});
}

CameraFrame CameraMotion::Interpolate( float t )
{
	if (m_KeyFrames.size() <= 0)
		return CameraFrame::Default();

	auto& first = m_KeyFrames.front();
	auto& last = m_KeyFrames.back();

	CameraFrame Data;
	if (t <= first.Frame)
	{
		Data = first.Data;
	}
	else if (t >= last.Frame)
	{
		Data = last.Data;
	}
	else
	{
		// VMD provide 0 frame. So, below zero check code is not nesseary.
		int32_t prev = FindPreviousFrameIndex( m_KeyFrames, t );
		ASSERT( prev >= 0 );
		auto& a = m_KeyFrames[prev];
		auto& b = m_KeyFrames[prev + 1];

		Data.bPerspective = a.Data.bPerspective;

		float p = 1.f;
		if (b.Frame - a.Frame > 0)
			p = (t - a.Frame) / (b.Frame - a.Frame);

		float c[kInterpA+1];
		for (uint8_t k = kInterpX; k <= kInterpA; k++)
			c[k] = Bezier( a.BezierCoeff[k], p );
		
		Data.Position = Lerp( a.Data.Position, b.Data.Position, Vector3( c[kInterpX], c[kInterpY], c[kInterpZ] ) );
		Data.Rotation = Slerp( a.Data.Rotation, b.Data.Rotation, c[kInterpR] );
		Data.Distance = Lerp( a.Data.Distance, b.Data.Distance, c[kInterpD] );
		Data.FovY = Lerp( a.Data.FovY, b.Data.FovY, c[kInterpA] );
	}
	return Data;
}

CameraFrame CameraFrame::Default()
{
	static CameraFrame frame;
	frame.bPerspective = true;
	frame.Distance = -45.f;
	frame.FovY = 30.f;
	frame.Position = Vector3( 0.f, 10.f, 0.f );
	return frame;
}
