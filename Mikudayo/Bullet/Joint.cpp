#include "stdafx.h"
#include "Joint.h"
#include "BaseJoint.h"

Joint::Joint() : m_Index( -1 )
{
}

void Joint::SetIndex( int32_t index )
{
    m_Index = index;
}

void Joint::SetName( const std::wstring& name )
{
    m_Name = name;
}

void Joint::SetNameEnglish( const std::wstring& name )
{
    m_NameEnglish = name;
}