#include "pch.h"
#include "Color.h"

Color::Color( uint16_t r, uint16_t g, uint16_t b, uint16_t a, uint16_t bitDepth )
{
    float depth = static_cast<float>((1 << bitDepth) - 1);
    m_value = XMVectorSet(
        static_cast<float>(r) / depth,
        static_cast<float>(g) / depth,
        static_cast<float>(b) / depth,
        static_cast<float>(a) / depth );
}

Color::Color( BYTE r, BYTE g, BYTE b )
{
    m_value = XMVectorSet(
        static_cast<float>(r) / 255,
        static_cast<float>(g) / 255,
        static_cast<float>(b) / 255,
        1 );
}
