#include "pch.h"
#include "BoundingSphere.h"

namespace Math
{
    //
    // Uses code from "https://github.com/TheRealMJP/Shadows" by MJP
    //
    // Finds the approximate smallest enclosing bounding sphere for a set of points.
    // The sphere calculated is about 5%~20% bigger than the ideal minimum-radius sphere.
    // Based on "An Efficient Bounding Sphere", by Jack Ritter.
    //
    template <typename VertCont>
    BoundingSphere ComputeBoundingSphereFromVerticesBase(
        const VertCont& vertices, uint32_t numPoints )
    {
        using namespace DirectX;
        ASSERT( numPoints > 0 );
        ASSERT( vertices.size() > 0 );

        auto points = [&]( uint32_t t ) -> Vector3 {
            return vertices[t];
        };

        // Find the points with minimum and maximum x, y, and z
        Vector3 MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
        MinX = MaxX = MinY = MaxY = MinZ = MaxZ = points( 0 );

        for (uint32_t i = 1; i < numPoints; i++)
        {
            Vector3 Point = points( i );

            float px = Point.GetX();
            float py = Point.GetY();
            float pz = Point.GetZ();

            if (px < MinX.GetX())
                MinX = Point;

            if (px > MaxX.GetX())
                MaxX = Point;

            if (py < MinY.GetY())
                MinY = Point;

            if (py > MaxY.GetY())
                MaxY = Point;

            if (pz < MinZ.GetZ())
                MinZ = Point;

            if (pz > MaxZ.GetZ())
                MaxZ = Point;
        }

        // Use the min/max pair that are farthest apart to form the initial sphere.
        Vector3 DeltaX = MaxX - MinX;
        Vector3 DistX = Length( DeltaX );

        Vector3 DeltaY = MaxY - MinY;
        Vector3 DistY = Length( DeltaY );

        Vector3 DeltaZ = MaxZ - MinZ;
        Vector3 DistZ = Length( DeltaZ );

        Vector3 Center;
        Vector3 Radius;

        if (XMVector3Greater( DistX, DistY ))
        {
            if (XMVector3Greater( DistX, DistZ ))
            {
                // Use min/max x.
                Center = (MaxX + MinX) * 0.5f;
                Radius = DistX * Scalar(0.5f);
            }
            else
            {
                // Use min/max z.
                Center = (MaxZ + MinZ) * 0.5f;
                Radius = DistZ * 0.5f;
            }
        }
        else // Y >= X
        {
            if (XMVector3Greater( DistY, DistZ ))
            {
                // Use min/max y.
                Center = (MaxY + MinY) * 0.5f;
                Radius = DistY * 0.5f;
            }
            else
            {
                // Use min/max z.
                Center = (MaxZ + MinZ) * 0.5f;
                Radius = DistZ * 0.5f;
            }
        }

        // Add any points not inside the sphere.
        for (uint32_t i = 0; i < numPoints; i++)
        {
            Vector3 Point = points( i );
            Vector3 Delta = Point - Center;
            Vector3 Dist = Length( Delta );

            if (XMVector3Greater( Dist, Radius ))
            {
                // Adjust sphere to include the new point.
                Radius = (Radius + Dist) * Scalar(0.5f);
                Center += (Vector3( 1.0f ) - Radius * Recip( Dist )) * Delta;
            }
        }
        return BoundingSphere( Vector3( Center ), Scalar( Radius ) );
    }

    template <typename VertCont, typename IdxCont>
    BoundingSphere ComputeBoundingSphereFromVerticesBase(
        const VertCont& vertices,
        const IdxCont& indices,
        uint32_t numPoints, uint32_t offset )
    {
        using namespace DirectX;
        ASSERT( numPoints > 0 );
        ASSERT( vertices.size() > 0 );
        ASSERT( indices.size() > 0 );

        auto points = [&]( uint32_t t ) -> Vector3 {
            return vertices[indices[offset + t]];
        };

        // Find the points with minimum and maximum x, y, and z
        Vector3 MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
        MinX = MaxX = MinY = MaxY = MinZ = MaxZ = points( 0 );

        for (uint32_t i = 1; i < numPoints; i++)
        {
            Vector3 Point = points( i );

            float px = Point.GetX();
            float py = Point.GetY();
            float pz = Point.GetZ();

            if (px < MinX.GetX())
                MinX = Point;

            if (px > MaxX.GetX())
                MaxX = Point;

            if (py < MinY.GetY())
                MinY = Point;

            if (py > MaxY.GetY())
                MaxY = Point;

            if (pz < MinZ.GetZ())
                MinZ = Point;

            if (pz > MaxZ.GetZ())
                MaxZ = Point;
        }

        // Use the min/max pair that are farthest apart to form the initial sphere.
        Vector3 DeltaX = MaxX - MinX;
        Vector3 DistX = Length( DeltaX );

        Vector3 DeltaY = MaxY - MinY;
        Vector3 DistY = Length( DeltaY );

        Vector3 DeltaZ = MaxZ - MinZ;
        Vector3 DistZ = Length( DeltaZ );

        Vector3 Center;
        Vector3 Radius;

        if (XMVector3Greater( DistX, DistY ))
        {
            if (XMVector3Greater( DistX, DistZ ))
            {
                // Use min/max x.
                Center = (MaxX + MinX) * 0.5f;
                Radius = DistX * Scalar(0.5f);
            }
            else
            {
                // Use min/max z.
                Center = (MaxZ + MinZ) * 0.5f;
                Radius = DistZ * 0.5f;
            }
        }
        else // Y >= X
        {
            if (XMVector3Greater( DistY, DistZ ))
            {
                // Use min/max y.
                Center = (MaxY + MinY) * 0.5f;
                Radius = DistY * 0.5f;
            }
            else
            {
                // Use min/max z.
                Center = (MaxZ + MinZ) * 0.5f;
                Radius = DistZ * 0.5f;
            }
        }

        // Add any points not inside the sphere.
        for (uint32_t i = 0; i < numPoints; i++)
        {
            Vector3 Point = points( i );
            Vector3 Delta = Point - Center;
            Vector3 Dist = Length( Delta );

            if (XMVector3Greater( Dist, Radius ))
            {
                // Adjust sphere to include the new point.
                Radius = (Radius + Dist) * Scalar(0.5f);
                Center += (Vector3( 1.0f ) - Radius * Recip( Dist )) * Delta;
            }
        }
        return BoundingSphere( Vector3( Center ), Scalar( Radius ) );
    }

    BoundingSphere ComputeBoundingSphereFromVertices( const std::vector<XMFLOAT3>& vertices )
    {
        return ComputeBoundingSphereFromVerticesBase( vertices, (uint32_t)vertices.size() );
    }

    BoundingSphere ComputeBoundingSphereFromVertices(
        const std::vector<XMFLOAT3>& vertices,
        const std::vector<uint32_t>& indices,
        uint32_t numPoints,
        uint32_t offset )
    {
        return ComputeBoundingSphereFromVerticesBase( vertices, indices, numPoints, offset );
    }

    BoundingSphere ComputeBoundingSphereFromVertices(
        const std::vector<XMFLOAT3>& vertices,
        const std::vector<uint16_t>& indices,
        uint32_t numPoints, uint32_t offset )
    {
        return ComputeBoundingSphereFromVerticesBase( vertices, indices, numPoints, offset );
    }
}
