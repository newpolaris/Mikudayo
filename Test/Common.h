#pragma once

#pragma warning(disable: 4324)

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
    return (Root/Path).generic_wstring();
}

MATCHER_P(MatcherNearFast, Precision, "")
{
    (result_listener);
    return Near( get<0>( arg ), get<1>( arg ), Math::Scalar( Precision ) );
}

MATCHER_P2(MatcherNearFast, Precision, Ref, "")
{
    (result_listener);
    return Near( arg, Ref, Math::Scalar( Precision ) );
}

MATCHER_P(MatcherNearRelative, Precision, "")
{
    (result_listener);
    auto A = get<0>( arg ), B = get<1>( arg );
    return NearRelative( A, B, Math::Scalar(Precision) );
}

MATCHER_P2(MatcherNearRelative, Precision, Ref, "")
{
    (result_listener);
    return NearRelative( arg, Ref, Math::Scalar(Precision) );
}
