#include "stdafx.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef _DEBUG
#pragma comment(lib, "gtestd.lib")
#pragma comment(lib, "gmockd.lib")
#else
#pragma comment(lib, "gtest.lib")
#pragma comment(lib, "gmock.lib")
#endif

using namespace ::testing;

int main(int argc, char *argv[])
{
    InitGoogleTest(&argc, argv);
    InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}