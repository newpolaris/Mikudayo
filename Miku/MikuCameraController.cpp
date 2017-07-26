#include "MikuCameraController.h"
#include "MikuCamera.h"
#include "GameInput.h"
#include "Motion.h"

using namespace GameCore;

namespace GameCore
{
    const char* CameraLabels[] = { "CameraMMD", "Camera3D", "CameraMotion" };
    EnumVar CameraMode("Application/Camera/Camera Move", kCameraMoveMMD, kCameraMoveMotion, CameraLabels);

    Matrix3 GetBasis( Vector3 forward, Vector3 up )
    {
        // Given, but ensure normalization
        Scalar forwardLenSq = LengthSquare(forward);
        forward = Select(forward * RecipSqrt(forwardLenSq), -Vector3(kZUnitVector), forwardLenSq < Scalar(0.000001f));

        // Deduce a valid, orthogonal right vector

        Vector3 right = Cross(forward, up); // forward = -look
        Scalar rightLenSq = LengthSquare(right);
        right = Select(right * RecipSqrt(rightLenSq), Quaternion(Vector3(kYUnitVector), -XM_PIDIV2) * forward, rightLenSq < Scalar(0.000001f));

        // Compute actual up vector
        up = Cross(right, forward); // forward = -look

        // Finish constructing basis
        return Matrix3(right, up, -forward); // -forward = look
    }
}

MikuCameraController::MikuCameraController( MikuCamera& camera, Vector3 worldUp ) :
    m_MainCamera( camera ), m_pSceondCamera( nullptr ), m_pMotion( nullptr )
{
	m_WorldUp = Normalize(worldUp);
	m_WorldNorth = Normalize(Cross(m_WorldUp, Vector3(kXUnitVector)));
	m_WorldEast = Cross(m_WorldNorth, m_WorldUp);

	m_HorizontalLookSensitivity = 2.0f;
	m_VerticalLookSensitivity = 2.0f;
	m_MoveSpeed = 100.0f;
	m_StrafeSpeed = 100.0f;
	m_MouseSensitivityX = 1.0f;
	m_MouseSensitivityY = 1.0f;
	m_MouseWheelSensivity = 3.0f;

	m_CurrentPitch = Sin(Dot(camera.GetForwardVec(), m_WorldUp));

	Vector3 forward = Normalize(Cross(m_WorldUp, camera.GetRightVec()));
	m_CurrentHeading = ATan2(-Dot(forward, m_WorldEast), Dot(forward, m_WorldNorth));

	m_FineMovement = false;
	m_FineRotation = false;
	m_Momentum = true;

	m_LastYaw = 0.0f;
	m_LastPitch = 0.0f;
	m_LastForward = 0.0f;
	m_LastStrafe = 0.0f;
	m_LastAscent = 0.0f;
}

namespace Graphics
{
	extern EnumVar DebugZoom;
}

void MikuCameraController::Update( float deltaTime )
{
    auto mode = ECameraMove((int)CameraMode);
	if (mode == kCameraMoveMotion)
	{
		if (m_pMotion)
			m_pMotion->Animate( m_MainCamera );
	}

	if (m_pSceondCamera)
        UpdateFromInput( m_pSceondCamera, kCameraMove3D, deltaTime );
    else if (mode != kCameraMoveMotion)
        UpdateFromInput( &m_MainCamera, mode, deltaTime );

	m_MainCamera.UpdateViewMatrix();
	m_MainCamera.Update();
    if (m_pSceondCamera)
    {
        m_pSceondCamera->UpdateViewMatrix();
        m_pSceondCamera->Update();
    }
}

void MikuCameraController::UpdateFromInput( MikuCamera* TargetCamera, ECameraMove kCameraMode, float deltaTime )
{
	(deltaTime);

	float timeScale = Graphics::DebugZoom == 0 ? 1.0f : Graphics::DebugZoom == 1 ? 0.5f : 0.25f;

	if (GameInput::IsFirstPressed(GameInput::kLThumbClick) || GameInput::IsFirstPressed(GameInput::kKey_lshift))
		m_FineMovement = !m_FineMovement;

	if (GameInput::IsFirstPressed(GameInput::kRThumbClick))
		m_FineRotation = !m_FineRotation;

	float speedScale = (m_FineMovement ? 0.1f : 1.0f) * timeScale;
	float panScale = (m_FineRotation ? 0.5f : 1.0f) * timeScale;

	float yaw = GameInput::GetTimeCorrectedAnalogInput( GameInput::kAnalogRightStickX ) * m_HorizontalLookSensitivity * panScale;
	float pitch = GameInput::GetTimeCorrectedAnalogInput( GameInput::kAnalogRightStickY ) * m_VerticalLookSensitivity * panScale;
	float forward =	m_MoveSpeed * speedScale * (
		GameInput::GetTimeCorrectedAnalogInput( GameInput::kAnalogLeftStickY ) +
		(GameInput::IsPressed( GameInput::kKey_w ) ? deltaTime : 0.0f) +
		(GameInput::IsPressed( GameInput::kKey_s ) ? -deltaTime : 0.0f)
		);
	float strafe = m_StrafeSpeed * speedScale * (
		GameInput::GetTimeCorrectedAnalogInput( GameInput::kAnalogLeftStickX  ) +
		(GameInput::IsPressed( GameInput::kKey_d ) ? deltaTime : 0.0f) +
		(GameInput::IsPressed( GameInput::kKey_a ) ? -deltaTime : 0.0f)
		);
	float ascent = m_StrafeSpeed * speedScale * (
		GameInput::GetTimeCorrectedAnalogInput( GameInput::kAnalogRightTrigger ) -
		GameInput::GetTimeCorrectedAnalogInput( GameInput::kAnalogLeftTrigger ) +
		(GameInput::IsPressed( GameInput::kKey_e ) ? deltaTime : 0.0f) +
		(GameInput::IsPressed( GameInput::kKey_q ) ? -deltaTime : 0.0f)
		);

	if (m_Momentum)
	{
		ApplyMomentum(m_LastYaw, yaw, deltaTime);
		ApplyMomentum(m_LastPitch, pitch, deltaTime);
		ApplyMomentum(m_LastForward, forward, deltaTime);
		ApplyMomentum(m_LastStrafe, strafe, deltaTime);
		ApplyMomentum(m_LastAscent, ascent, deltaTime);
	}

	if (kCameraMode == kCameraMove3D ||
		kCameraMode == kCameraMoveMMD && GameInput::IsPressed( GameInput::kMouse0 ))
	{
		// don't apply momentum to mouse inputs
		yaw += GameInput::GetAnalogInput(GameInput::kAnalogMouseX) * m_MouseSensitivityX;
		pitch += GameInput::GetAnalogInput(GameInput::kAnalogMouseY) * m_MouseSensitivityY;
	}

	float distanceDelta = GameInput::GetAnalogInput( GameInput::kAnalogMouseScroll ) * m_MouseWheelSensivity;

	m_CurrentPitch += pitch;
	m_CurrentPitch = XMMin( XM_PIDIV2, m_CurrentPitch);
	m_CurrentPitch = XMMax(-XM_PIDIV2, m_CurrentPitch);

	m_CurrentHeading -= yaw;
	if (m_CurrentHeading > XM_PI)
		m_CurrentHeading -= XM_2PI;
	else if (m_CurrentHeading <= -XM_PI)
		m_CurrentHeading += XM_2PI; 

	if (kCameraMode == kCameraMoveMMD)
	{
		Matrix3 orientation = Matrix3(m_WorldEast, m_WorldUp, -m_WorldNorth) * Matrix3::MakeYRotation( m_CurrentHeading ) * Matrix3::MakeXRotation( m_CurrentPitch );
		TargetCamera->SetDistance(TargetCamera->GetDistance() + distanceDelta);
		TargetCamera->SetRotationUI( Quaternion(orientation) );
	}
	else if (kCameraMode == kCameraMove3D)
	{
		Matrix3 orientation = Matrix3( m_WorldEast, m_WorldUp, -m_WorldNorth ) * Matrix3::MakeYRotation( m_CurrentHeading ) * Matrix3::MakeXRotation( m_CurrentPitch );
		Vector3 position = orientation * Vector3( strafe, ascent, -forward ) + TargetCamera->GetPosition();
		Quaternion basis = Quaternion(GetBasis( -orientation.GetZ(), orientation.GetY() ));
		Vector3 CameraPos = position - basis * TargetCamera->GetDistanceVector();
		TargetCamera->SetRotationUI( basis );
		TargetCamera->SetPositionUI( CameraPos );
	}
}

void MikuCameraController::ApplyMomentum( float& oldValue,  float& newValue, float deltaTime )
{
	float blendedValue;
	if (Abs(newValue) > Abs(oldValue))
		blendedValue = Lerp(newValue, oldValue, Pow(0.6f, deltaTime * 60.0f));
	else
		blendedValue = Lerp(newValue, oldValue, Pow(0.8f, deltaTime * 60.0f));
	oldValue = blendedValue;
	newValue = blendedValue;
}
