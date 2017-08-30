#pragma once

#include <string>

namespace Rendering
{
    class Model;
}

bool LoadPmxFromFile( const std::wstring& FilePath, Rendering::Model& Model );