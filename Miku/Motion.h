#pragma once

#include <string>
#include <memory>

#include "KeyFrameAnimation.h"

namespace Math
{
	class MikuCamera;
}

namespace Graphics
{
	class Motion
	{
	public:
		Motion( bool bRightHand = true );
		void LoadMotion(const std::wstring& path);
		void Update( float kFrameTime );
		void Animate( Math::MikuCamera& camera );

		bool m_bRightHand;
		Animation::CameraFrame m_CameraFrame;
		Animation::CameraMotion m_CameraMotion;
	};
}