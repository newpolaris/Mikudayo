#include "KeyFrameAnimation.h"
#include <algorithm>

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
// http://zerogram.info/?p=860
//
float Animation::Bezier( Vector4 C, float p )
{
	XMFLOAT4 coeff;
	XMStoreFloat4( &coeff, C );

	const float x1 = coeff.x, x2 = coeff.y, y1 = coeff.z, y2 = coeff.w;

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
	for (uint8_t k = kInterpX; k <= kInterpR; k++)
		zero.BezierCoeff[k] = Vector4( 0.15748f, 0.8425f, 0.1574f, 0.8425f );
	return zero;
}

void BoneMotion::InsertKeyFrame( const BoneKeyFrame& frame )
{
	m_KeyFrames.push_back( frame );
}

void BoneMotion::SortKeyFrame()
{
	std::sort( m_KeyFrames.begin(), m_KeyFrames.end(), [](const BoneKeyFrame& a, const BoneKeyFrame& b) {
		return a.Frame < b.Frame;
	});
}

void BoneMotion::Interpolate( float t )
{
	if (m_KeyFrames.size() == 0)
		return;

	auto& first = m_KeyFrames.front();
	auto& last = m_KeyFrames.back();

	if (t <= first.Frame)
	{
		m_Local = first.Local;
	}
	if (t >= last.Frame)
	{
		m_Local = last.Local;
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

		float c[kInterpMax];
		for (uint8_t k = kInterpX; k <= kInterpR; k++)
			c[k] = Bezier( a.BezierCoeff[k], p );
		
		m_Local.SetTranslation( Lerp( a.Local.GetTranslation(), b.Local.GetTranslation(), Vector3( c[kInterpX], c[kInterpY], c[kInterpZ] ) ) );
		m_Local.SetRotation( Slerp( a.Local.GetRotation(), b.Local.GetRotation(), c[kInterpR] ) );
	}
}

void FaceMotion::Interpolate( float t )
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

FaceMotion::FaceMotion() : m_Weight( 0.f ), m_WeightPre( 0.f )
{
}

void FaceMotion::InsertKeyFrame( const FaceKeyFrame& frame )
{
	m_KeyFrames.push_back( frame );
}

void FaceMotion::SortKeyFrame()
{
	std::sort( m_KeyFrames.begin(), m_KeyFrames.end(), 
		[]( auto& a, auto& b ) { return a.Frame < b.Frame; } );
}
