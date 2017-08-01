//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

#include "VectorMath.h"
#include <vector>

namespace Math
{
	class BoundingSphere
	{
	public:
		BoundingSphere() {}
		BoundingSphere( Vector3 center, Scalar radius );
		explicit BoundingSphere( Vector4 sphere );

		Vector3 GetCenter( void ) const;
		Scalar GetRadius( void ) const;

		friend BoundingSphere operator* ( const OrthogonalTransform& xform, const BoundingSphere& sphere );	// Fast
		friend BoundingSphere operator* ( const AffineTransform& xform, const BoundingSphere& sphere );		// Slow
		friend BoundingSphere operator* ( const Matrix4& xform, const BoundingSphere& sphere );				// Slowest (and most general)

	private:

		Vector4 m_repr;
	};

	//=======================================================================================================
	// Inline implementations
	//

	inline BoundingSphere::BoundingSphere( Vector3 center, Scalar radius )
	{
		m_repr = Vector4(center);
		m_repr.SetW(radius);
	}

	inline BoundingSphere::BoundingSphere( Vector4 sphere )
		: m_repr(sphere)
	{
	}

	inline Vector3 BoundingSphere::GetCenter( void ) const
	{
		return Vector3(m_repr);
	}

	inline Scalar BoundingSphere::GetRadius( void ) const
	{
		return m_repr.GetW();
	}

	inline BoundingSphere operator* ( const OrthogonalTransform& mtx, const BoundingSphere& sphere )
    {
        Vector3 Center = mtx * sphere.GetCenter();
        return BoundingSphere( Center, sphere.GetRadius() );
    }

	inline BoundingSphere operator* ( const AffineTransform& mtx, const BoundingSphere& sphere )
    {
        Vector3 Center = mtx * sphere.GetCenter();
        Vector3 Radius = mtx.GetBasis() * sphere.GetRadius();
        Scalar r = Max(Radius.GetX(), Max(Radius.GetY(), Radius.GetZ()));
        return BoundingSphere( Center, r );
    }

	inline BoundingSphere operator* ( const Matrix4& mtx, const BoundingSphere& sphere )
	{
        Vector3 Center = mtx.Transform( sphere.GetCenter() );
        Vector3 Radius = mtx.Get3x3() * Vector3(sphere.GetRadius());
        Scalar r = Max(Radius.GetX(), Max(Radius.GetY(), Radius.GetZ()));
        return BoundingSphere( Center, r );
    }

    BoundingSphere ComputeBoundingSphereFromVertices( const std::vector<XMFLOAT3>& vertices, const std::vector<uint16_t>& indices, uint32_t numPoints, uint32_t offset );
    BoundingSphere ComputeBoundingSphereFromVertices( const std::vector<XMFLOAT3>& vertices, const std::vector<uint32_t>& indices, uint32_t numPoints, uint32_t offset );

} // namespace Math
