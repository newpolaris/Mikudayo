#pragma once

#include "Mapping.h"

class IColorBuffer
{
public:

    virtual bool IsTransparent() const = 0;
    virtual const D3D11_SRV_HANDLE GetSRV() const = 0;
};