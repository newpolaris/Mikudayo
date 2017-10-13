#pragma once

#include <string>
#include "BaseJoint.h"

using JointPtr = std::shared_ptr<class Joint>;

class Joint : public BaseJoint
{
public:

    Joint();

    void SetIndex( int32_t index );
    void SetName( const std::wstring& name );
    void SetNameEnglish( const std::wstring& name );

    int32_t m_Index;
    std::wstring m_Name;
    std::wstring m_NameEnglish;
};
