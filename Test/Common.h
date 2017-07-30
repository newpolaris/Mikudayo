#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <boost/filesystem.hpp>

#include <stdint.h> // For int32_t, etc.
#include "VectorMath.h"

using namespace ::testing;
namespace fs = boost::filesystem;

inline std::wstring ResourcePath( fs::path Path )
{
    auto Root = L"C:/Users/newpolaris/Projects/MikuViewer/Test";
    return (Root / Path).generic_wstring();
}

MATCHER_P(MatcherNearFast, Precision, "")
{
    return Near( get<0>( arg ), get<1>( arg ), Scalar( Precision ) );
}

MATCHER_P2(MatcherNearFast, Precision, Ref, "")
{
    return Near( arg, Ref, Scalar( Precision ) );
}

MATCHER_P(MatcherNearRelative, Precision, "")
{
    auto A = get<0>( arg ), B = get<1>( arg );
    return NearRelative( A, B, Scalar(Precision) );
}

MATCHER_P2(MatcherNearRelative, Precision, Ref, "")
{
    return NearRelative( arg, Ref, Scalar(Precision) );
}
