#include "stdafx.h"
#include "Clipping.h"
#include "Math/BoundingBox.h"
#include "Math/BoundingFrustum.h"
#include "Math/BoundingPlane.h"
#include "Camera.h"

namespace Math {

    PolyObject calcViewFrustObject(const FrustumCorner& pts)
    {
        // kNearLowerLeft, kNearUpperLeft, kNearLowerRight, kNearUpperRight,
        // kFarLowerLeft, kFarUpperLeft, kFarLowerRight, kFarUpperRight

        // viewFrustumWorldCoord[0] == near-bottom-left
        // viewFrustumWorldCoord[1] == near-bottom-right
        // viewFrustumWorldCoord[2] == near-top-right
        // viewFrustumWorldCoord[3] == near-top-left
        // viewFrustumWorldCoord[4] == far-bottom-left
        // viewFrustumWorldCoord[5] == far-bottom-right
        // viewFrustumWorldCoord[6] == far-top-right
        // viewFrustumWorldCoord[7] == far-top-left

        std::array<uint32_t, 8> ix = { 0, 2, 3, 1, 4, 6, 7, 5 };
        auto Conv = [&]( std::vector<uint32_t> poly ) {
            std::vector<Vector3> arr;
            for (uint32_t i = 0; i < poly.size(); i++)
                arr.push_back( pts[ix[poly[i]]] );
            return arr;
        };

        // calcViewFrustObject
        PolyObject obj(6);
        // near poly ccw
        obj[0] = Conv( { 0, 1, 2, 3 } );
        //far poly ccw
        obj[1] = Conv( { 7, 6, 5, 4 } );
        //left poly ccw
        obj[2] = Conv( { 0, 3, 7, 4 } );
        //right poly ccw
        obj[3] = Conv( { 1, 5, 6, 2 } );
        //bottom poly ccw
        obj[4] = Conv( { 4, 5, 1, 0 } );
        //top poly ccw
        obj[5] = Conv( { 6, 7, 3, 2 } );

        return obj;
    }

    bool intersectPlaneEdge( Vector3& Output, const BoundingPlane& Plane,
        const Vector3& A, const Vector3& B )
    {
        Vector3 U = B - A;
        Vector3 N = Plane.GetNormal();
        Scalar D = Plane.GetDistance();
        Scalar T = (-Dot(N, A) - D) / Dot( N, U );
        if (T < 0.0f || 1.0f < T)
            return false;
        Output = Vector3(XMVectorMultiplyAdd(U, T, A));
        return true;
    }

    void clipVecPointByPlane( VecPoint& polyOut, VecPoint& interPts,
        const VecPoint& poly, const BoundingPlane& A )
    {
        if (poly.size() < 3)
            return;
        polyOut.clear();
        interPts.clear();
        std::vector<bool> outside;
        outside.resize( poly.size() );

        for (size_t i = 0; i < poly.size(); i++)
            outside[i] = A.DistanceFromPoint( poly[i] ) < 0.f;

        for (size_t i = 0; i < poly.size(); i++)
        {
            size_t idNext = (i + 1) % poly.size();
            // both outside -> save none
            if (outside[i] && outside[idNext])
                continue;
            // outside to inside -> calc intersection save intersection and save i+1
            if (outside[i])
            {
                Vector3 inter;
                if (intersectPlaneEdge( inter, A, poly[i], poly[idNext] )) {
                    polyOut.push_back( inter );
                    interPts.push_back( inter );
                }
                polyOut.push_back( poly[idNext] );
                continue;
            }
            //inside to outside -> calc intersection save intersection
            if (outside[idNext]) {
                Vector3 inter;
                if (intersectPlaneEdge( inter, A, poly[i], poly[idNext] )) {
                    polyOut.push_back( inter );
                    interPts.push_back( inter );
                }
                continue;
            }
            //both inside -> save point i+1
            polyOut.push_back( poly[idNext] );
        }
    }

    int findSamePointInVecPoint( const VecPoint& poly, const Vector3& p )
    {
        int i;
        for (i = 0; i < poly.size(); i++) {
            if (DirectX::XMVector3NearEqual( poly[i], p, Scalar(0.001f) ))
                return i;
        }
        return -1;
    }

    int findSamePointInObjectAndSwapWithLast( PolyObject& inter, const Vector3& p )
    {
        int i;
        if (1 > inter.size())
            return -1;
        for (i = int(inter.size()); i > 0; i--) {
            VecPoint& poly = inter[i - 1];
            if (2 == poly.size())
            {
                const int nr = findSamePointInVecPoint( poly, p );
                if (0 <= nr) {
                    // swap with last
                    std::swap( poly, inter.back() );
                    return nr;
                }
            }
        }
        return -1;
    }

    void appendIntersectionVecPoint( PolyObject& obj, const PolyObject& interObj)
    {
        PolyObject inter = interObj;

        int size = int(obj.size());
        // you need at least 3 sides for a polygon
        if (3 > inter.size())
            return;
        // compact inter: remove poly.size != 2 from end on forward
        int i = 0;
        for (i = int(inter.size()); 0 < i; i--) {
            if (2 == inter[i - 1].size())
                break;
        }
        inter.resize( i );

        // you need at least 3 sides for a polygon
        if (3 > inter.size())
            return;
        // make place for one additional polygon in obj
        obj.resize( obj.size() + 1 );
        VecPoint& polyOut = obj.back();
        // we have line segments in each poly of inter
        // take last linesegment as first and second point
        VecPoint& polyIn = inter.back();
        polyOut.push_back( polyIn[0] );
        polyOut.push_back( polyIn[1] );
        // remove last poly from inter, because it is already saved
        inter.pop_back();

        //iterate over inter until their is no line segment left
        while (0 < inter.size())
        {
            //pointer on last point to compare
            Vector3& lastPt = polyOut.back();
            //find same point in inter to continue polygon
            const int nr = findSamePointInObjectAndSwapWithLast(inter, lastPt);
            if (0 <= nr) {
                //last line segment
                polyIn = inter.back();
                //get the other point in this polygon and save into polyOut
                polyOut.push_back( polyIn[(nr + 1) % 2] );
            }
            // remove last poly from inter, because it is already saved or degenerated
            inter.pop_back();
        }
        // last point can be deleted, because he is the same as the first (closes polygon)
        polyOut.pop_back();
    }

    PolyObject ClipObjectByPlane( const PolyObject& obj, const BoundingPlane& plane )
    {
        PolyObject inter, objOut;
        for (auto& o : obj)
        {
            VecPoint polyOut, interPts;
            clipVecPointByPlane( polyOut, interPts, o, plane );
            if (polyOut.size() > 0)
            {
                inter.push_back( interPts );
                objOut.push_back( polyOut );
            }
        }
        // add a polygon of all intersection points with plane to close the object
        appendIntersectionVecPoint( objOut, inter );

        return objOut;
    }

    PolyObject clipObjectByAABox( const PolyObject& obj, const BoundingBox& box )
    {
        FrustumPlanes plane = box.GetPlanes();
        PolyObject objOut = obj;
        for (auto& p : plane)
            objOut = ClipObjectByPlane( objOut, p );
        return objOut;
    }

    void convObject2VecPoint( VecPoint& pts, const PolyObject& obj ) {
        int i, j;
        VecPoint points;
        for (i = 0; i < obj.size(); i++) {
            const VecPoint& p = obj[i];
            for (j = 0; j < p.size(); j++) {
                points.push_back( p[j] );
            }
        }
        pts = points;
    }

    int clipTest( const float p, const float q, float * u1, float * u2 ) {
        // Return value is 'true' if line segment intersects the current test
        // plane.  Otherwise 'false' is returned in which case the line segment
        // is entirely clipped.
        if (0 == u1 || 0 == u2) {
            return 0;
        }
        if (p < 0.f) {
            float r = q / p;
            if (r > ( *u2 )) {
                return 0;
            }
            else {
                if (r > (*u1)) {
                    (*u1) = r;
                }
                return 1;
            }
        }
        else {
            if (p > 0.0) {
                float r = q / p;
                if (r < (*u1)) {
                    return 0;
                }
                else {
                    if (r < (*u2)) {
                        (*u2) = r;
                    }
                    return 1;
                }
            }
            else {
                return q >= 0.f;
            }
        }
    }

    int intersectionLineAABox( Vector3& v, const Vector3& p, const Vector3& dir, const BoundingBox& b ) {
        float t = 0;
        if (!b.Intersect( &t, p, dir ))
            return false;
        v = p + t*dir;
        return true;
    }

    void includeObjectLightVolume( VecPoint& pts, const PolyObject& obj,
        const Vector3& lightDir, const BoundingBox& sceneAABox )
    {
        int i, size;
        Vector3 ld = -lightDir;
        VecPoint points;
        convObject2VecPoint( points, obj );
        size = int(points.size());
        // for each point add the point on the ray in -lightDir
        // intersected with the sceneAABox
        for (i = 0; i < size; i++) {
            Vector3 pt;
            if (intersectionLineAABox( pt, points[i], ld, sceneAABox ))
                points.push_back( pt );
        }
        pts = points;
    }

    void calcFocusedLightVolumePoints( VecPoint& points, const Vector3& lightDir,
        const BoundingFrustum& worldFrustum, const BoundingBox& sceneAABox )
    {
        FrustumCorner pts = worldFrustum.GetFrustumCorners();
        PolyObject obj = calcViewFrustObject(pts);
        PolyObject clip = clipObjectByAABox( obj, sceneAABox );
        includeObjectLightVolume( points, clip, lightDir, sceneAABox );
    }
}