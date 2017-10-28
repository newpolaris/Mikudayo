#pragma once

class BoolVar;
class NumVar;
class ComputeContext;

namespace Diffuse 
{
    extern BoolVar Enable;

    void Initialize( void );
    void Shutdown( void );
    void Render( ComputeContext& Context );
} // namespace FXAA
