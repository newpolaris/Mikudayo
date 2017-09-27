#pragma once

class SceneNode;
class Mesh;
class Material;

class Visitor
{
public:
    virtual bool Visit( const SceneNode& ) { return true; }
    virtual bool Visit( const Mesh& ) { return true; }
    virtual bool Visit( const Material& ) { return true; }
};
