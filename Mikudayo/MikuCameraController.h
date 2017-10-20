#pragma once

#include "GameCore.h"
#include "VectorMath.h"

namespace Math
{
	class MikuCamera;
}

namespace Graphics
{
	class Motion;
}

namespace GameCore
{
	using namespace Math;

	enum ECameraMove { kCameraMoveMMD, kCameraMove3D, kCameraMoveMotion };
	class MikuCameraController
	{
	public:
		MikuCameraController( MikuCamera& camera, Vector3 worldUp );

		void Update( float dt );
		void SlowRotation( bool enable ) { m_FineRotation = enable; }
		void EnableMomentum( bool enable ) { m_Momentum = enable; }

	private:
		void ApplyMomentum( float& oldValue, float& newValue, float deltaTime );
		void UpdateFromInput( ECameraMove kCameraMode, float deltaTime );

		Vector3 m_WorldUp;
		Vector3 m_WorldNorth;
		Vector3 m_WorldEast;
		MikuCamera& m_TargetCamera;
		float m_HorizontalLookSensitivity;
		float m_VerticalLookSensitivity;
		float m_MoveSpeed;
		float m_StrafeSpeed;
		float m_MouseSensitivityX;
		float m_MouseSensitivityY;
		float m_MouseWheelSensivity;

		float m_CurrentHeading;
		float m_CurrentPitch;

		bool m_FineMovement;
		bool m_FineRotation;
		bool m_Momentum;

		float m_LastYaw;
		float m_LastPitch;
		float m_LastForward;
		float m_LastStrafe;
		float m_LastAscent;
	};
}
