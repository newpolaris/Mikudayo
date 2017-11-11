#pragma once

#include "pch.h"
#include <vector>

struct StreamOutDesc
{
	enum { kLength = 64 };
    UINT Stream;
	char SemanticName[kLength];
    UINT SemanticIndex;
    BYTE StartComponent;
    BYTE ComponentCount;
    BYTE OutputSlot;
};

std::size_t hash_value( const StreamOutDesc& Desc );
using StreamOutEntries = std::vector<StreamOutDesc>;