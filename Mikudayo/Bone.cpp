#include "stdafx.h"
#include "Bone.h"

Bone::Bone() :
    m_Index( -1 ),
    m_Parent( nullptr ),
    m_Child( nullptr ),
    m_InherentRotation( nullptr ),
    m_InherentTranslation( nullptr ),
    m_InherentBoneCoefficent( 0.f ),
    m_LocalInherentTranslation( kZero )
{
}

void Bone::Load( uint32_t index, std::vector<Bone>& BoneList, const std::vector<PmxModel::Bone>& Bones )
{
    m_Index = index;

    if (Bones[index].Parent >= 0)
        m_Parent = &BoneList[Bones[index].Parent];

    m_LocalPoseDefault.SetTranslation( Bones[index].Translate );
    m_LocalPose = m_LocalPoseDefault;
    m_Pose.SetTranslation( Bones[index].Position );
    m_ToRoot.SetTranslation( -Bones[index].Position );

    if (Bones[index].bInherentRotation)
        m_InherentRotation = &BoneList[Bones[index].ParentInherentBoneIndex];
    if (Bones[index].bInherentTranslation)
        m_InherentTranslation = &BoneList[Bones[index].ParentInherentBoneIndex];
    m_InherentBoneCoefficent = Bones[index].ParentInherentBoneCoefficent;
}

void Bone::Update()
{
    m_Pose = m_LocalPose;
    if (m_Parent != nullptr)
        m_Pose = m_Parent->m_Pose * m_Pose;
}

const btTransform Bone::GetTransform() const
{
    return Convert( m_Pose );
}

void Bone::SetTransform( const btTransform& transform )
{
    // btTransform class supports rigid transforms with only translation and rotation and no scaling/shear
    AffineTransform affine = Convert( transform );
    m_Pose = OrthogonalTransform( affine.GetBasis(), affine.GetTranslation() );
}

//
// Use code from 'MMDAI'
// Copyright (c) 2010-2014  hkrn
//
void Bone::UpdateInherentTransform()
{
    Quaternion orientation( kIdentity );
    if (m_InherentRotation) {
        // If parent also Inherenet, then it has updated value. So, use cached one
        if (m_InherentRotation->m_InherentRotation) {
            orientation *= m_InherentRotation->m_LocalInherentOrientation;
        }
        else {
            orientation *= m_InherentRotation->m_LocalPose.GetRotation();
        }
        if (!Near( m_InherentBoneCoefficent, 1.f, FLT_EPSILON )) {
            orientation = Slerp( Quaternion( kIdentity ), orientation, m_InherentBoneCoefficent );
        }
        m_LocalInherentOrientation = Normalize(orientation * m_LocalPose.GetRotation());
    }
    orientation *= m_LocalPose.GetRotation();
    orientation = Normalize( orientation );

    Vector3 translation( kZero );
    if (m_InherentTranslation) 
    {
        if (m_InherentTranslation->m_InherentTranslation) {
            translation += m_InherentTranslation->m_LocalInherentTranslation;
        }
        else {
            translation += m_InherentTranslation->m_LocalPose.GetTranslation();
        }
        if (!Near( m_InherentBoneCoefficent, 1.f, FLT_EPSILON )) {
            translation *= Scalar(m_InherentBoneCoefficent);
        }
        m_LocalInherentTranslation = translation;
    }
    translation += m_LocalPose.GetTranslation();
    m_LocalPose.SetRotation( orientation );
    m_LocalPose.SetTranslation( translation );
}
