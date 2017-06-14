#include "KeyFrameAnimation.h"
#include <algorithm>

using namespace Animation;
using namespace Math;

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

void MeshBone::Interpolate( float t )
{
	if (m_KeyFrames.size() == 0)
		return;

	auto& first = m_KeyFrames.front();
	auto& last = m_KeyFrames.back();

	if (t <= first.Frame)
	{
		m_LocalPosition = first.Translation;
		m_Rotation = first.Rotation;
	}
	else if (t >= last.Frame)
	{
		m_LocalPosition = last.Translation;
		m_Rotation = last.Rotation;
	}
	else
	{
		for (int32_t i = 0; i < (int)m_KeyFrames.size() - 1; i++)
		{
			if (t >= m_KeyFrames[i].Frame && t <= m_KeyFrames[i + 1].Frame)
			{
				auto &a = m_KeyFrames[i], &b = m_KeyFrames[i + 1];
				float p = (t - a.Frame) / (b.Frame - a.Frame);

				float c[kInterpMax];
				for (uint8_t k = kInterpX; k <= kInterpR; k++)
					c[k] = Bezier( a.BezierCoeff[k], p );
				
				m_LocalPosition = Lerp( a.Translation, b.Translation, Vector3( c[kInterpX], c[kInterpY], c[kInterpZ] ) );
				m_Rotation = Quaternion( DirectX::XMQuaternionSlerp( a.Rotation, b.Rotation, c[kInterpR] ) );
				break;
			}
		}
	}
}

void MeshBone::InsertKeyFrame( const KeyFrame& frame )
{
	m_KeyFrames.push_back( frame );
}

void MeshBone::SortKeyFrame()
{
	std::stable_sort( m_KeyFrames.begin(), m_KeyFrames.end(), [](const KeyFrame& a, const KeyFrame& b) {
		return a.Frame < b.Frame;
	});
}

