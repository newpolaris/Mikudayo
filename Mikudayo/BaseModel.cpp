#include "stdafx.h"
#include "BaseModel.h"

BaseModel::BaseModel() : m_DefaultShader(L"Default")
{
}

void BaseModel::Clear()
{
}

bool BaseModel::Load( const ModelInfo& info )
{
    m_Type = info.Type;
    m_Name = info.Name;
    m_File = info.File;
    m_DefaultShader = info.DefaultShader;

    return true;
}