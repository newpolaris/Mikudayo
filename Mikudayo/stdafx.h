#pragma once

#include <iostream>
#include <algorithm>
#include <regex>
#include <map>
#include <vector>
#include <codecvt>
#include <locale>
#include <sstream>
#include <fstream>
#include "GameCore.h"
#include "GameInput.h"
#include "GraphicsCore.h"
#include "PipelineState.h"
#include "CommandContext.h"
#include "GpuBuffer.h"
#include "InputLayout.h"
#include "ColorBuffer.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CameraController.h"
#include "SamplerManager.h"
#include "GameInput.h"
#include "TextureManager.h"
#include "FileUtility.h"
#include "Utility.h"
#include "Math/BoundingBox.h"
#include "GeometryGenerator.h"
#include "boost/filesystem.hpp"

#pragma warning(push)
#pragma warning(disable: 4819)
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#pragma warning(pop)

#define FREEIMAGE_LIB
#include "FreeImage.h"