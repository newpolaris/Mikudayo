#pragma once

#include "Camera.h"
#include "VectorMath.h"

namespace Math
{
	class MikuCamera : public BaseCamera
	{
	public:
		MikuCamera();

		void SetPerspective( float verticalFovRadians, float aspectHeightOverWidth, float nearZClip, float farZClip );
		void SetFOV( float verticalFovInRadians ) { m_VerticalFOV = verticalFovInRadians; }
		void SetAspectRatio( float heightOverWidth ) { m_AspectRatio = heightOverWidth; }
		void SetZRange( float nearZ, float farZ) { m_NearClip = nearZ; m_FarClip = farZ; }
		void SetDistance( float distance ) { m_Distance = distance; }
		void SetPositionUI( Vector3 position ) { m_Position = position; }
		void SetRotationUI( Quaternion rotation ) { m_Rotation = rotation; }
		void ReverseZ( bool enable ) { m_ReverseZ = enable; }
		void Perspective( bool enable ) { m_bPerspective = enable; }

		float GetFOV() const { return m_VerticalFOV; }
		float GetAspectRatio() const { return m_AspectRatio; }
		float GetDistance() const { return m_Distance; }
		Vector3 GetDistanceVector() const;
		float GetNearClip() const { return m_NearClip; }
		float GetFarClip() const { return m_FarClip; }
		float GetClearDepth() const { return m_ReverseZ ? 0.0f : 1.0f; }
		Quaternion GetRotationUI() const { return m_Rotation; }
		Vector3 GetPositionUI() const { return m_Position; }
		bool GetReverseZ() const { return m_ReverseZ; }

		void UpdateViewMatrix();
		void UpdateProjMatrix();

	protected:
		void UpdateOrthogonalMatrix();
		void UpdatePerspectiveMatrix();

		float m_VerticalFOV;			// Field of view angle in radians
		float m_AspectRatio;
		float m_NearClip;
		float m_FarClip;
		bool m_ReverseZ;				// Invert near and far clip distances so that Z=0 is the far plane

		// MMD camera parameter
		bool m_bPerspective;
		float m_Distance;
		Quaternion m_Rotation;
		Vector3 m_Position;
	};
}
