#pragma once

class BoolVar;
class NumVar;

class GraphicsContext;

namespace Graphics {
namespace PrimitiveUtility {
    enum PrimtiveMeshType {
        kBoneMesh,
        kSphereMesh,
        kFarClipMesh,
        kBatchMax
    };

    extern InputDesc Desc[4];
    extern BoolVar s_bEnableDrawBone;
    extern BoolVar s_bEnableDrawBoundingSphere;
    extern BoolVar s_bExcludeSkyBox;
    extern NumVar s_ExcludeRange;

    void Initialize();
    void Shutdown();
    void Append( PrimtiveMeshType Type, const class Math::Matrix4& Transform );
    void Flush( GraphicsContext& Context );
    void Render( GraphicsContext& Context, PrimtiveMeshType Type );
}
}