#pragma once

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>

#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#endif

#define MY_IID_PPV_ARGS IID_PPV_ARGS
#define MY_SHADER_ARGS(ByteCode) #ByteCode, ByteCode, sizeof(ByteCode)

#if _MSC_VER >= 1800
	// To enable PIX which are blobked by defined d3d11_2_h
	#include <d3d11_2.h>
	#include <pix.h>
#endif

#include <dxgi1_4.h>
#include <d3d11_4.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <array>
#include <map>
#include <cstdio>
#include <vector>
#include <wrl.h>
#include <exception>
#include <memory>
#include <future>
#include <wincodec.h>

#include "Utility.h"
#include "VectorMath.h"
#include "EngineTuning.h"
#include "EngineProfiling.h"
