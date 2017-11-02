#pragma once

class ColorBuffer;
class BoolVar;
class ComputeContext;

namespace SMAA
{
    extern BoolVar Enable;

    void Initialize( void );
    void Shutdown( void );
    void Render( ComputeContext& Context );
}
