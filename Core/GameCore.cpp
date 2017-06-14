// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "pch.h"
#include "GameCore.h"
#include "GraphicsCore.h"
#include "GameInput.h"
#include "SystemTime.h"


#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

extern void OnMouseDown( WPARAM btnState, int x, int y );
extern void OnMouseUp( WPARAM btnState, int x, int y );
extern void OnMouseMove( WPARAM btnState, int x, int y );

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	#pragma comment(lib, "runtimeobject.lib")
#else
	#include <agile.h>
	using namespace Windows::ApplicationModel;
	using namespace Windows::UI::Core;
	using namespace Windows::UI::ViewManagement;
	using Windows::ApplicationModel::Core::CoreApplication;
	using Windows::ApplicationModel::Core::CoreApplicationView;
	using Windows::ApplicationModel::Activation::IActivatedEventArgs;
	using Windows::Foundation::TypedEventHandler;
#endif

namespace GameCore
{
	using namespace Graphics;

	void InitializeApplication( IGameApp& game )
	{
		Graphics::Initialize();
		SystemTime::Initialize();
		GameInput::Initialize();
		EngineTuning::Initialize();

		game.Startup();

	}

	void TerminateApplication( IGameApp& game )
	{
		game.Cleanup();

		GameInput::Shutdown();
	}

	bool UpdateApplication( IGameApp& game )
	{
		float DeltaTime = Graphics::GetFrameTime();

#if 1
		GameInput::Update( DeltaTime );
#endif
		EngineTuning::Update( DeltaTime );
		
		game.Update( DeltaTime );
		game.RenderScene();

		Graphics::Present();

		return !game.IsDone();
	}

	// Default implementation to be overridden by the application
	bool IGameApp::IsDone( void )
	{
		return GameInput::IsFirstPressed(GameInput::kKey_escape);
	}

#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#else // Win32

	HWND g_hWnd = nullptr;

	void InitWindow( const wchar_t* className );
	LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

	void RunApplication( IGameApp& app, const wchar_t* className )	
	{
		Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
		ASSERT_SUCCEEDED(InitializeWinRT);

		HINSTANCE hInst = GetModuleHandle(0);

		// Register class
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInst;
		wcex.hIcon = LoadIcon(hInst, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = className;
		wcex.hIconSm = LoadIcon(hInst, IDI_APPLICATION);
		ASSERT(0 != RegisterClassEx(&wcex), "Unable to register a window");

		// Create window
		RECT rc = { 0, 0, (LONG)g_DisplayWidth, (LONG)g_DisplayHeight };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

		g_hWnd = CreateWindow(className, className, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInst, nullptr);

		ASSERT(g_hWnd != 0);

		InitializeApplication(app);

		ShowWindow( g_hWnd, SW_SHOWDEFAULT );

		do
		{
			MSG msg = {};
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (msg.message == WM_QUIT)
				break;
		}
		while (UpdateApplication(app));	// Returns false to quit loop

		// Graphics::Terminate();
		TerminateApplication(app);
		Graphics::Shutdown();
	}

	//--------------------------------------------------------------------------------------
	// Called every time the application receives a message
	//--------------------------------------------------------------------------------------
	LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
	{
		switch( message )
		{
			case WM_SIZE:
				if (wParam != SIZE_MINIMIZED)
					Graphics::Resize( (UINT)(UINT64)lParam & 0xFFFF, (UINT)(UINT64)lParam >> 16 );
				break;

			case WM_DESTROY:
				PostQuitMessage(0);
				break;

			case WM_LBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_RBUTTONDOWN:
				OnMouseDown( wParam, GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
				return 0;
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				OnMouseUp( wParam, GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
				return 0;
			case WM_MOUSEMOVE:
				OnMouseMove( wParam, GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) );
				return 0;

			default:
				return DefWindowProc( hWnd, message, wParam, lParam );
		}

		return 0;
	}

#endif
}