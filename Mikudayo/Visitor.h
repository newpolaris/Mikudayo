#pragma once

class SceneNode;
class IMesh;
class IMaterial;

class Visitor
{
public:
    virtual bool Visit( SceneNode& );
    virtual bool Visit( IMaterial& );
    virtual bool Visit( IMesh& );
    virtual bool Visit( IMesh& mesh, SceneNode& node );
};

inline bool Visitor::Visit( SceneNode& ) { return true; }
inline bool Visitor::Visit( IMesh& ) { return true; }
inline bool Visitor::Visit( IMesh&, SceneNode& ) { return true; }
inline bool Visitor::Visit( IMaterial& ) { return true; }
