#include "pch.h"
#include "Functions.h"

//
// Uses code from "https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/"
//

namespace
{
    // See
    // https://randomascii.wordpress.com/2012/01/11/tricks-with-the-floating-point-format/
    // the potential portability problems with the union and bit-fields below.
    //
    union Float_t
    {
        Float_t( float num = 0.0f ) : f( num ) {}
        // Portable extraction of components.
        bool Negative() const { return i < 0; }
        int32_t RawMantissa() const { return i & ((1 << 23) - 1); }
        int32_t RawExponent() const { return (i >> 23) & 0xFF; }

        int32_t i;
        float f;
#ifdef _DEBUG
        struct
        { // Bitfields for exploration. Do not use in production code.
            uint32_t mantissa : 23;
            uint32_t exponent : 8;
            uint32_t sign : 1;
        } parts;
#endif
    };

}

namespace Math
{
    bool AlmostEqualUlps( float A, float B, int maxUlpsDiff );
    bool AlmostEqualRelative( float A, float B, float maxRelDiff = FLT_EPSILON );

    bool AlmostEqualUlps( float A, float B, int maxUlpsDiff )
    {
        Float_t uA( A );
        Float_t uB( B );

        // Different signs means they do not match.
        if (uA.Negative() != uB.Negative())
        {
            // Check for equality to make sure +0==-0
            if (A == B)
                return true;
            return false;
        }

        // Find the difference in ULPs.
        int ulpsDiff = abs( uA.i - uB.i );
        if (ulpsDiff <= maxUlpsDiff)
            return true;

        return false;
    }

    bool AlmostEqualRelative( float A, float B, float maxRelDiff )
    {
        // Calculate the difference.
        float diff = fabs( A - B );
        A = fabs( A );
        B = fabs( B );
        // Find the largest
        float largest = (B > A) ? B : A;

        if (diff <= largest * maxRelDiff)
            return true;
        return false;
    }

    bool NearRelative( const Vector3& v1, const Vector3& v2, const Vector3& eps )
    {
        return AlmostEqualRelative( v1.GetX(), v2.GetX(), eps.GetX() ) &&
            AlmostEqualRelative( v1.GetY(), v2.GetY(), eps.GetY() ) &&
            AlmostEqualRelative( v1.GetZ(), v2.GetZ(), eps.GetZ() );
    }

    bool NearRelative( const Vector4& v1, const Vector4& v2, const Vector4& eps )
    {
        return AlmostEqualRelative( v1.GetX(), v2.GetX(), eps.GetX() ) &&
            AlmostEqualRelative( v1.GetY(), v2.GetY(), eps.GetY() ) &&
            AlmostEqualRelative( v1.GetZ(), v2.GetZ(), eps.GetZ() ) &&
            AlmostEqualRelative( v1.GetW(), v2.GetW(), eps.GetW() );
    }

    bool NearRelative( const Matrix3& m1, const Matrix3& m2, const Vector3& eps )
    {
        return NearRelative( m1.GetX(), m2.GetX(), eps ) &&
            NearRelative( m1.GetY(), m2.GetY(), eps ) &&
            NearRelative( m1.GetZ(), m2.GetZ(), eps );
    }

    bool NearRelative( const Matrix4& m1, const Matrix4& m2, const Vector4& eps )
    {
        return NearRelative( m1.GetX(), m2.GetX(), eps ) &&
            NearRelative( m1.GetY(), m2.GetY(), eps ) &&
            NearRelative( m1.GetZ(), m2.GetZ(), eps ) &&
            NearRelative( m1.GetW(), m2.GetW(), eps );
    }

    Matrix4 PerspectiveMatrix( float VerticalFOV, float AspectRatio, float NearClip, float FarClip, bool bReverseZ )
    {
        float Y = 1.0f / std::tanf( VerticalFOV * 0.5f );
        float X = Y * AspectRatio;

        float Q1, Q2;

        //
        // ReverseZ puts far plane at Z=0 and near plane at Z=1.  This is never a bad idea, and it's
        // actually a great idea with F32 depth buffers to redistribute precision more evenly across
        // the entire range.  It requires clearing Z to 0.0f and using a GREATER variant depth test.
        // Some care must also be done to properly reconstruct linear W in a pixel shader from hyperbolic Z.
        //
        if (bReverseZ)
        {
            Q1 = NearClip / (FarClip - NearClip);
            Q2 = Q1 * FarClip;
        }
        else
        {
            Q1 = FarClip / (NearClip - FarClip);
            Q2 = Q1 * NearClip;
        }

        return Matrix4(
            Vector4( X, 0.0f, 0.0f, 0.0f ),
            Vector4( 0.0f, Y, 0.0f, 0.0f ),
            Vector4( 0.0f, 0.0f, Q1, -1.0f ),
            Vector4( 0.0f, 0.0f, Q2, 0.0f )
        );
    }

    Matrix4 OrthographicMatrix( float W, float H, float NearClip, float FarClip, bool bReverseZ )
    {
        auto X = 2 / W;
        auto Y = 2 / H;

        float Q1, Q2;
        if (bReverseZ)
        {
            Q1 = 1 / (FarClip - NearClip);
            Q2 = Q1 * FarClip;
        }
        else
        {
            Q1 = 1 / (NearClip - FarClip);
            Q2 = Q1 * NearClip;
        }

        return Matrix4(
            Vector4( X, 0.f, 0.f, 0.f ),
            Vector4( 0.f, Y, 0.f, 0.f ),
            Vector4( 0.f, 0.f, Q1, 0.f ),
            Vector4( 0.f, 0.f, Q2, 1.f )
        );
    }

    Matrix4 OrthographicMatrix( float L, float R, float B, float T, float NearClip, float FarClip, bool bReverseZ )
    {
        auto RcpWidth = 1.f / (R - L);
        auto RcpHeight = 1.f / (T - B);
        auto X = RcpWidth * 2, Y = RcpHeight * 2,
            XT = -(L + R) * RcpWidth, YT = -(T + B) * RcpHeight;

        float Q1, Q2;
        if (bReverseZ)
        {
            Q1 = 1 / (FarClip - NearClip);
            Q2 = Q1 * FarClip;
        }
        else
        {
            Q1 = 1 / (NearClip - FarClip);
            Q2 = Q1 * NearClip;
        }

        return Matrix4(
            Vector4( X, 0.f, 0.f, 0.f ),
            Vector4( 0.f, Y, 0.f, 0.f ),
            Vector4( 0.f, 0.f, Q1, 0.f ),
            Vector4( XT, YT, Q2, 1.f )
        );
    }
}
