#include "stdafx.h"
#include "RigidBody.h"

RigidBody::RigidBody() : m_Index(-1)
{
}

RigidBody::~RigidBody() {}

void RigidBody::SetName( const std::wstring& name )
{
    m_Name = name;
}

void RigidBody::SetNameEnglish( const std::wstring& name )
{
    m_NameEnglish = name;
}

void RigidBody::SetIndex( int32_t index )
{
    m_Index = index;
}
