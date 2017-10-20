#include "stdafx.h"
#include "Motion.h"
#include "Vmd.h"
#include "FileUtility.h"
#include "MikuCamera.h"

using namespace Graphics;
using namespace Math;

Motion::Motion( bool bRightHand ) : m_bRightHand( bRightHand )
{
}

void Motion::LoadMotion( const std::wstring& path )
{
	using namespace std;
	using namespace Animation;
	
	Utility::ByteArray ba = Utility::ReadFileSync( path );
	Utility::ByteStream bs(ba);

	Vmd::VMD vmd;
	vmd.Fill( bs, m_bRightHand );

	for (auto& frame : vmd.CameraFrames)
	{
		CameraKeyFrame key;
		key.Frame = frame.Frame;
		key.Data.bPerspective = frame.TurnOffPerspective == 0;
		key.Data.Distance = frame.Distance;
		key.Data.FovY = static_cast<float>(frame.ViewAngle) * XM_PI / 180.f;
		key.Data.Rotation = Quaternion( frame.Rotation.y, frame.Rotation.x, frame.Rotation.z );
		key.Data.Position = frame.Position;

		//
		// http://harigane.at.webry.info/201103/article_1.html
		// 
		auto interp = reinterpret_cast<const char*>(&frame.Interpolation[0]);
		float scale = 1.0f / 127.0f;

		for (auto i = 0; i < 6; i++)
			key.BezierCoeff[i] = Vector4( interp[i], interp[i+2], interp[i+1], interp[i+3] ) * scale;

		m_CameraMotion.InsertKeyFrame( key );
	}
}

void Motion::Update( float kFrameTime )
{
	m_CameraFrame = m_CameraMotion.Interpolate( kFrameTime );
}

void Motion::Animate( Math::MikuCamera& camera )
{
	camera.Perspective( m_CameraFrame.bPerspective );
	camera.SetFOV( m_CameraFrame.FovY );
	camera.SetDistance( m_CameraFrame.Distance );
	camera.SetPositionUI( m_CameraFrame.Position );
	camera.SetRotationUI( m_CameraFrame.Rotation );

	camera.UpdateProjMatrix();
}