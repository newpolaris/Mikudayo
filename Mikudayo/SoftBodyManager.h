#pragma once

#include <memory>

struct SoftBodyGeometry
{
    std::vector<XMFLOAT3> Positions;
    std::vector<uint32_t> Indices;
};

class SoftBodyManager final
{
public:
    SoftBodyManager();
    ~SoftBodyManager();

    btSoftBody* LoadFromFile( const std::string& Filename );
    btSoftBody* LoadFromGeometry( const SoftBodyGeometry& Geometry );

protected:
    bool LoadGeometryFromFile( const std::string& Filename );
    btSoftBody* CreateSoftBody( const SoftBodyGeometry& Geometry );

    std::map<std::string, SoftBodyGeometry> m_Geometry;
};
