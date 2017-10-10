#pragma once

class SceneNode;
class Mesh;
class Material;

class Visitor
{
public:
    virtual bool Visit( SceneNode& );
    virtual bool Visit( Mesh& );
    virtual bool Visit( Material& );
};

inline bool Visitor::Visit( SceneNode& ) { return true; }
inline bool Visitor::Visit( Mesh& ) { return true; }
inline bool Visitor::Visit( Material& ) { return true; }
