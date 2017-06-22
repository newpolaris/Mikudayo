
//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Camera1.h"
#include <cmath>

Camera1::Camera1()
	: mPosition(0.0f, 0.0f, 0.0f), 
	  mRight(1.0f, 0.0f, 0.0f),
	  mUp(0.0f, 1.0f, 0.0f),
	  mLook(0.0f, 0.0f, 1.0f)
{
	SetLens( 0.25f*3.14f, 1.f / (9.0f / 16.0f), 1.0f, 1000.0f );
}

Camera1::~Camera1()
{
}

XMVECTOR Camera1::GetPositionXM()const
{
	return XMLoadFloat3(&mPosition);
}

XMFLOAT3 Camera1::GetPosition()const
{
	return mPosition;
}

void Camera1::SetPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
}

void Camera1::SetPosition(const XMFLOAT3& v)
{
	mPosition = v;
}

XMVECTOR Camera1::GetRightXM()const
{
	return XMLoadFloat3(&mRight);
}

XMFLOAT3 Camera1::GetRight()const
{
	return mRight;
}

XMVECTOR Camera1::GetUpXM()const
{
	return XMLoadFloat3(&mUp);
}

XMFLOAT3 Camera1::GetUp()const
{
	return mUp;
}

XMVECTOR Camera1::GetLookXM()const
{
	return XMLoadFloat3(&mLook);
}

XMFLOAT3 Camera1::GetLook()const
{
	return mLook;
}

float Camera1::GetNearZ()const
{
	return mNearZ;
}

float Camera1::GetFarZ()const
{
	return mFarZ;
}

float Camera1::GetAspect()const
{
	return mAspect;
}

float Camera1::GetFovY()const
{
	return mFovY;
}

float Camera1::GetFovX()const
{
	float halfWidth = 0.5f*GetNearWindowWidth();
	return 2.0f * std::atan(halfWidth / mNearZ);
}

float Camera1::GetNearWindowWidth()const
{
	return mAspect * mNearWindowHeight;
}

float Camera1::GetNearWindowHeight()const
{
	return mNearWindowHeight;
}

float Camera1::GetFarWindowWidth()const
{
	return mAspect * mFarWindowHeight;
}

float Camera1::GetFarWindowHeight()const
{
	return mFarWindowHeight;
}

void Camera1::SetLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf( 0.5f*mFovY );
	mFarWindowHeight  = 2.0f * mFarZ * tanf( 0.5f*mFovY );

	XMMATRIX P = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	XMStoreFloat4x4(&mProj, P);
}

void Camera1::Rate(float f) {
	mRate += 0.1f * f;
}

void Camera1::Zoom(float f) {
	mFactor = f;
}

void Camera1::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
}

void Camera1::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);
}

XMMATRIX Camera1::View()const
{
	return XMLoadFloat4x4(&mView);
}

XMMATRIX Camera1::Proj()const
{
	return XMLoadFloat4x4(&mProj);
}

XMMATRIX Camera1::ViewProj()const
{
	return XMMatrixMultiply(View(), Proj());
}

void Camera1::Strafe(float d)
{
	d *= (mFactor*mRate);
	// mPosition += d*mRight
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, r, p));
}

void Camera1::Walk(float d)
{
	d *= mFactor;
	// mPosition += d*mLook
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, l, p));
}

void Camera1::Pitch(float angle)
{
	// Rotate up and look vector about the right vector.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMStoreFloat3(&mUp,   XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));
}

void Camera1::RotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis.

	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight,   XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));
}

void Camera1::UpdateViewMatrix()
{
	XMVECTOR R = XMLoadFloat3(&mRight);
	XMVECTOR U = XMLoadFloat3(&mUp);
	XMVECTOR L = XMLoadFloat3(&mLook);
	XMVECTOR P = XMLoadFloat3(&mPosition);

	// Keep camera's axes orthogonal to each other and of unit length.
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = XMVector3Cross(U, L); 

	// Fill in the view matrix entries.
	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);
	XMStoreFloat3(&mLook, L);

	mView(0,0) = mRight.x; 
	mView(1,0) = mRight.y; 
	mView(2,0) = mRight.z; 
	mView(3,0) = x;   

	mView(0,1) = mUp.x;
	mView(1,1) = mUp.y;
	mView(2,1) = mUp.z;
	mView(3,1) = y;  

	mView(0,2) = mLook.x; 
	mView(1,2) = mLook.y; 
	mView(2,2) = mLook.z; 
	mView(3,2) = z;   

	mView(0,3) = 0.0f;
	mView(1,3) = 0.0f;
	mView(2,3) = 0.0f;
	mView(3,3) = 1.0f;
}

