#pragma once

#include <vector>

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Bullet/LinearMath.h"
#include "PmxModel.h"

using namespace Math;

class Bone
{
public:

    Bone();

    void Load( uint32_t index, std::vector<Bone>& BoneList, const std::vector<PmxModel::Bone>& Bones );
    void Update();
    void UpdateInherentTransform();

    const btTransform GetTransform() const;
    void SetTransform( const btTransform& transform );

    int32_t m_Index;

    Bone* m_Parent; // parent bone
    Bone* m_Child; 
    Bone* m_InherentRotation;
    Bone* m_InherentTranslation;
    float m_InherentBoneCoefficent;

    Quaternion m_LocalInherentOrientation;
    Vector3 m_LocalInherentTranslation;
    OrthogonalTransform m_LocalPose;
    OrthogonalTransform m_LocalPoseDefault; // offset matrix
    OrthogonalTransform m_Pose;
    OrthogonalTransform m_Skinning; // final skinning transform
    OrthogonalTransform m_ToRoot; // inverse initial pose
};
