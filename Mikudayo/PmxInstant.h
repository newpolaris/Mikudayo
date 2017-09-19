#pragma once

class Model;
class PmxInstant
{
public:

    PmxInstant( const Model& model );

    void DrawColor( GraphicsContext& Context );
    bool LoadModel();
    bool LoadMotion( const std::wstring& motionPath );
    void Update( float deltaT );

protected:

    struct Context;
    std::shared_ptr<Context> m_Context;
};
