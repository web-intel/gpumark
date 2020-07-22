///////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
///////////////////////////////////////////////////////////////////////////////

#include <sdkddkver.h>
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h> // must be after windows.h
#include <ShellScalingApi.h>
#include <interactioncontext.h>
#include <mmsystem.h>
#include <map>
#include <vector>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <memory>

//#include "asteroids_d3d11.h"
#include "asteroids_d3d12.h"
#include "camera.h"
//#include "profile.h"
#include "gui.h"

using namespace DirectX;

namespace {

// Global demo state
Settings gSettings;
OrbitCamera gCamera;

IDXGIFactory2* gDXGIFactory = nullptr;

//AsteroidsD3D11::Asteroids* gWorkloadD3D11 = nullptr;
AsteroidsD3D12::Asteroids* gWorkloadD3D12 = nullptr;

GUI gGUI;
GUISprite* gD3D11Control;
GUISprite* gD3D12Control;
GUIText* gFPSControl;
GUIText* gVRSControl;

//int gScreenshotIndex = 1;

enum
{
    basicStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE,
    windowedStyle = basicStyle | WS_OVERLAPPEDWINDOW,
    fullscreenStyle = basicStyle
};

/*
bool CheckDll(char const* dllName)
{
    auto hModule = LoadLibrary(dllName);
    if (hModule == NULL) {
        return false;
    }
    FreeLibrary(hModule);
    return true;
}
*/

UINT SetupDPI()
{
    // Just do system DPI awareness for now for simplicity... scale the 3D content
    SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);

    UINT dpiX = 0, dpiY;
    POINT pt = {1, 1};
    auto hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        return dpiX;
    } else {
        return 96; // default
    }
}


void ResetCameraView()
{
    auto center    = XMVectorSet(0.0f, -0.4f*SIM_DISC_RADIUS, 0.0f, 0.0f);
    auto radius    = SIM_ORBIT_RADIUS + SIM_DISC_RADIUS + 10.f;
    auto minRadius = SIM_ORBIT_RADIUS - 3.0f * SIM_DISC_RADIUS;
    auto maxRadius = SIM_ORBIT_RADIUS + 3.0f * SIM_DISC_RADIUS;
    auto longAngle = 4.50f;
    auto latAngle  = 1.45f;
    gCamera.View(center, radius, minRadius, maxRadius, longAngle, latAngle);
}

void ToggleFullscreen(HWND hWnd)
{
    static WINDOWPLACEMENT prevPlacement = { sizeof(prevPlacement) };
    DWORD dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    if ((dwStyle & windowedStyle) == windowedStyle)
    {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(hWnd, &prevPlacement) &&
            GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
        {
            SetWindowLong(hWnd, GWL_STYLE, fullscreenStyle);
            SetWindowPos(hWnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(hWnd, GWL_STYLE, windowedStyle);
        SetWindowPlacement(hWnd, &prevPlacement);
        SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

const char* kCameraDataFileName = "Camera.txt";

void SaveCameraToFile() {
    static int index = 0;
    std::wstring cameraDataFile = L"Camera";
    if (index != 0) {
        cameraDataFile += std::to_wstring(index);
    }
    index++;
    cameraDataFile += L".txt";
    std::wofstream stream(L"");
    stream << cameraDataFile;
    float cameraData[11];
    gCamera.GetAllData(cameraData);
    for (int i = 0; i < 11; ++i) {
        stream << cameraData[i] << " ";
    }
    stream.close();
    //printf("Camera has been logged into %s.\n", cameraDataFile.str().c_str());
}

void LoadCameraFromFile() {
    std::ifstream stream;
    stream.open(kCameraDataFileName);
    if (!stream) {
        printf("Failed to open %s\n", kCameraDataFileName);
        exit(1);
    }
    float cameraData[11];
    for (int i = 0; i < 11; ++i) {
        stream >> cameraData[i];
    }
    stream.close();
    gCamera.LoadData(cameraData);
    printf("Camera has been initialized from %s.\n", kCameraDataFileName);
}

} // namespace

LRESULT CALLBACK WindowProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            UINT ww = LOWORD(lParam);
            UINT wh = HIWORD(lParam);

            // Ignore resizing to minimized
            if (ww == 0 || wh == 0) return 0;

            gSettings.windowWidth = (int)ww;
            gSettings.windowHeight = (int)wh;
            gSettings.renderWidth = (UINT)(double(gSettings.windowWidth)  * gSettings.renderScale);
            gSettings.renderHeight = (UINT)(double(gSettings.windowHeight) * gSettings.renderScale);

            // Update camera projection
            float aspect = (float)gSettings.renderWidth / (float)gSettings.renderHeight;
            gCamera.Projection(XM_PIDIV2 * 0.8f * 3 / 2, aspect);

            // Resize currently active swap chain
            //if (gSettings.d3d12)
                gWorkloadD3D12->ResizeSwapChain(gDXGIFactory, hWnd, gSettings.renderWidth, gSettings.renderHeight);
            //else
            //    gWorkloadD3D11->ResizeSwapChain(gDXGIFactory, hWnd, gSettings.renderWidth, gSettings.renderHeight);

            return 0;
        }

        case WM_KEYDOWN:
            if (lParam & (1 << 30)) {
                // Ignore repeats
                return 0;
            }
            switch (wParam) {
            case VK_SPACE:
                if (!gSettings.neverAnimate) {
                    gSettings.animate = !gSettings.animate;
                    if (gSettings.animate) {
                        std::cout << "Animate" << std::endl;
                    }
                    else {
                        std::cout << "Stop" << std::endl;
                        gSettings.takeScreenshot = true;
                        SaveCameraToFile();
                    }
                }
                else {
                    gSettings.takeScreenshot = true;
                    std::cout << "Save screenshot to file " << gWorkloadD3D12->GetScreenShotIndex() << ".bmp: " << std::endl;
                    SaveCameraToFile();
                }

                return 0;

                /* Disabled for demo setup */
            case 'V':
                gSettings.vsync = !gSettings.vsync;
                std::cout << "Vsync: " << gSettings.vsync << std::endl;
                return 0;
            case 'M':
                gSettings.multithreadedRendering = !gSettings.multithreadedRendering;
                std::cout << "Multithreaded Rendering: " << gSettings.multithreadedRendering << std::endl;
                return 0;
            case 'I':
                gSettings.executeIndirect = !gSettings.executeIndirect;
                std::cout << "ExecuteIndirect Rendering: " << gSettings.executeIndirect << std::endl;
                return 0;
            case 'S':
                gSettings.submitRendering = !gSettings.submitRendering;
                std::cout << "Submit Rendering: " << gSettings.submitRendering << std::endl;
                return 0;

            case '0':
                gSettings.useVRS = false;
                std::cout << "Disable VRS" << std::endl;
                return 0;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                if (gWorkloadD3D12 && !gWorkloadD3D12->SupportVRS()) {
                    std::cout << "System cannot enable VRS." << std::endl;
                }
                else {
                    gSettings.useVRS = true;
                    gSettings.shadingRate = kD3D12ShadingRates[char(wParam) - '1'];
                    std::cout << "Enable VRS (" << D3D12ShadingRateToStr(gSettings.shadingRate) << ")" << std::endl;
                }
                return 0;

            case VK_ESCAPE:
                SendMessage(hWnd, WM_CLOSE, 0, 0);
                return 0;
            } // Switch on key code
            return 0;

        case WM_SYSKEYDOWN:
            if (lParam & (1 << 30)) {
                // Ignore repeats
                return 0;
            }
            switch (wParam) {
            case VK_RETURN:
                ToggleFullscreen(hWnd);
                break;
            }
            return 0;

        case WM_MOUSEWHEEL: {
            auto delta = GET_WHEEL_DELTA_WPARAM(wParam);
            gCamera.ZoomRadius(-0.07f * delta);
            return 0;
        }

        case WM_POINTERDOWN:
        case WM_POINTERUPDATE:
        case WM_POINTERUP: {
            auto pointerId = GET_POINTERID_WPARAM(wParam);
            POINTER_INFO pointerInfo;
            if (GetPointerInfo(pointerId, &pointerInfo)) {
                if (message == WM_POINTERDOWN) {
                    // Compute pointer position in render units
                    POINT p = pointerInfo.ptPixelLocation;
                    ScreenToClient(hWnd, &p);
                    RECT clientRect;
                    GetClientRect(hWnd, &clientRect);
                    p.x = p.x * gSettings.renderWidth / (clientRect.right - clientRect.left);
                    p.y = p.y * gSettings.renderHeight / (clientRect.bottom - clientRect.top);

                    auto guiControl = gGUI.HitTest(p.x, p.y);
                    if (guiControl == gFPSControl) {
                        gSettings.lockFrameRate = !gSettings.lockFrameRate;
                    } else if (guiControl == gD3D11Control) { // Switch to D3D12
                        gSettings.d3d12 = (gWorkloadD3D12 != nullptr);
                    } else if (guiControl == gD3D12Control) { // Switch to D3D11
                        //gSettings.d3d12 = (gWorkloadD3D11 == nullptr);
                        gSettings.d3d12 = true;
                    } else { // Camera manipulation
                        gCamera.AddPointer(pointerId);
                    }
                }

                // Otherwise send it to the camera controls
                gCamera.ProcessPointerFrames(pointerId, &pointerInfo);
                if (message == WM_POINTERUP) gCamera.RemovePointer(pointerId);
            }
            return 0;
        }

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
}


int main(int argc, char** argv)
{
	//bool d3d11Available = CheckDll("d3d11.dll");
    //bool d3d12Available = CheckDll("d3d12.dll");
    bool d3d12Available = true;

    // Must be done before any windowing-system-like things or else virtualization will kick in
    auto dpi = SetupDPI();
    // By default render at the lower resolution and scale up based on system settings
    gSettings.renderScale = 96.0 / double(dpi);

    // Scale default window size based on dpi
    gSettings.windowWidth *= dpi / 96;
    gSettings.windowHeight *= dpi / 96;

    for (int a = 1; a < argc; ++a) {
        if (_stricmp(argv[a], "--close-after") == 0 && a + 1 < argc) {
            gSettings.closeAfterSeconds = atof(argv[++a]);
        } else if (_stricmp(argv[a], "--warp") == 0) {
            gSettings.warp = true;
        } else if (_stricmp(argv[a], "--nod3d12") == 0) {
            d3d12Available = false;
        } else if (_stricmp(argv[a], "--indirect") == 0) {
            gSettings.executeIndirect = true;
        } else if (_stricmp(argv[a], "--fullscreen") == 0) {
            gSettings.windowed = false;
        } else if (_stricmp(argv[a], "--window") == 0 && a + 2 < argc) {
            gSettings.windowWidth = atoi(argv[++a]);
            gSettings.windowHeight = atoi(argv[++a]);
        } else if (_stricmp(argv[a], "--rendersscale") == 0 && a + 1 < argc) {
            gSettings.renderScale = atof(argv[++a]);
        } else if (_stricmp(argv[a], "--locked-fps") == 0 && a + 1 < argc) {
            gSettings.lockedFrameRate = atoi(argv[++a]);
        } else if (_stricmp(argv[a], "--enable-log-fps") == 0) {
            gSettings.enableLogFPS = true;
        } else if (_stricmp(argv[a], "--enable-non-vi-stereo-mode") == 0) {
            gSettings.enableStereoMode = true;
            gSettings.enableViewportInstancing = false;
        } else if (_stricmp(argv[a], "--enable-vi") == 0) {
            gSettings.enableStereoMode = true;
            gSettings.enableViewportInstancing = true;
		}
		else if (_stricmp(argv[a], "--use-view-instance-mask") == 0) {
			gSettings.useViewInstanceMask = true;
		}
		else if (_stricmp(argv[a], "--show-left") == 0) {
			gSettings.showLeft = true;
		}
		else if (_stricmp(argv[a], "--show-right") == 0) {
			gSettings.showRight = true;
		}
		else if (_stricmp(argv[a], "--shading-rate") == 0 && a + 1 < argc) {
			gSettings.useVRS = true;
            ++a;
            if (_stricmp(argv[a], "1x1") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_1X1;
            }
            else if (_stricmp(argv[a], "1x2") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_1X2;
            }
            else if (_stricmp(argv[a], "2x1") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_2X1;
            }
            else if (_stricmp(argv[a], "2x2") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_2X2;
            }
            else if (_stricmp(argv[a], "2x4") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_2X4;
            }
            else if (_stricmp(argv[a], "4x2") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_4X2;
            }
            else if (_stricmp(argv[a], "4x4") == 0) {
                gSettings.shadingRate = D3D12_SHADING_RATE_4X4;
            }
            else {
                fprintf(stderr, "error: incorrect shading rate '%s'\n", argv[a]);
                fprintf(stderr, "valid shading rates: 1x1 1x2 2x1 2x2 2x4 4x2 4x4\n");
                return -1;
            }
        } else if (_stricmp(argv[a], "--num-asteroids") == 0) {
            gSettings.numAsteroids = atoi(argv[++a]);
        } else if (_stricmp(argv[a], "--initialize-camera-data") == 0) {
            gSettings.initializeCamera = true;
        } else if (_stricmp(argv[a], "--never-animation") == 0) {
            gSettings.neverAnimate = true;
            gSettings.animate = false;
        } else {
            if (_stricmp(argv[a], "-h") == 0) {
                fprintf(stderr, "usage: asteroids_d3d12 [options]\n");
                fprintf(stderr, "options:\n");
                fprintf(stderr, "  --close-after [seconds]\n");
                //fprintf(stderr, "  -nod3d11\n");
                //fprintf(stderr, "  -nod3d12\n");
                //fprintf(stderr, "  -fullscreen\n");
                //fprintf(stderr, "  -window [width] [height]\n");
                //fprintf(stderr, "  -render_scale [scale]\n");
                //fprintf(stderr, "  -locked_fps [fps]\n");
               // fprintf(stderr, "  -warp\n");
                fprintf(stderr, "  --enable-non-vi-stereo-mode\n");
                fprintf(stderr, "  --enable-vi\n");
                //fprintf(stderr, "  -use-view-instance-mask\n");
                //fprintf(stderr, "  -show-left\n");
               // fprintf(stderr, "  -show-right\n");
                fprintf(stderr, "  --shading-rate [1x1, 1x2, 2x1, 2x2, 2x4, 4x2, 4x4]\n");
                fprintf(stderr, "  --num-asteroids [number]\n");
                fprintf(stderr, "  --enable-log-fps\n");
                fprintf(stderr, "  --initialize-camera-data (initialize gCamera from the txt file Camera.txt (the file name is fixed!))\n");
                fprintf(stderr, "  --never-animation\n");
                fprintf(stderr, "Press SPACE to stop animation, take the screenshot into a BMP file and save the gCamera data into Camera.txt.\n");
                fprintf(stderr, "Press SPACE again to resume animation if \"-never-animation\" is not specified.\n");
            }
            else {
                fprintf(stderr, "error: unrecognized argument '%s'. Press '-h' for all available arguments.\n", argv[a]);
            }
            return -1;
        }
    }

    //d3d11Available = false;

    if (!d3d12Available) {
        fprintf(stderr, "error: neither D3D11 nor D3D12 available.\n");
        return -1;
    }

	{
		fprintf(stderr, "Configurations:\n");

        fprintf(stderr, "Num asteroids: %d\n", gSettings.numAsteroids);

		if (gSettings.enableStereoMode) {
			if (gSettings.enableViewportInstancing) {
				fprintf(stderr, "- Enable VI\n");
				if (gSettings.useViewInstanceMask) {
					fprintf(stderr, "- Use view instance mask\n");
					if (gSettings.showLeft) {
						fprintf(stderr, "- Set bit for the left viewport in the view instance mask\n");
					}
					if (gSettings.showRight) {
						fprintf(stderr, "- Set bit for the right viewport in the view instance mask\n");
					}
				}
			}
			else {
				fprintf(stderr, "- Enable non-VI stereo mode\n");
			}
		}

        if (gSettings.useVRS) {
            const char* shadingRateStr = D3D12ShadingRateToStr(gSettings.shadingRate);
            fprintf(stderr, "- Enable VRS. Initial shading rate: %s\n", shadingRateStr);
        }

        if (gSettings.neverAnimate) {
            fprintf(stderr, "- Never animation\n");
        }
	}

    // DXGI Factory
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&gDXGIFactory)));

    // Setup GUI
    gD3D12Control = gGUI.AddSprite(5, 10, 140, 50, L"asteroid/directx12.dds");
    //gD3D11Control = gGUI.AddSprite(5, 10, 140, 50, L"directx11.dds");
    gFPSControl = gGUI.AddText(150, 10);
    gVRSControl = gGUI.AddText(340, 10);

    if (gSettings.initializeCamera) {
        LoadCameraFromFile();
    } else {
        ResetCameraView();
    }

    // Camera projection set up in WM_SIZE

    AsteroidsSimulation asteroids(1337, gSettings.numAsteroids, NUM_UNIQUE_MESHES, MESH_MAX_SUBDIV_LEVELS, NUM_UNIQUE_TEXTURES);

    // Create workloads
    //if (d3d11Available) {
    //    gWorkloadD3D11 = new AsteroidsD3D11::Asteroids(&asteroids, &gGUI, gSettings.warp);
    //}

    IDXGIAdapter1* adapter = nullptr;
    if (d3d12Available) {
        // If requested, enumerate the warp adapter
		IDXGIFactory4* DXGIFactory4 = nullptr;
		if (FAILED(gDXGIFactory->QueryInterface(&DXGIFactory4))) {
			fprintf(stderr, "error: WARP requires IDXGIFactory4 interface which is not present on this system!\n");
			return -1;
		}

		std::vector<IDXGIAdapter1*> adapters;
		for (int adapterIndex = 0;; ++adapterIndex) {
			IDXGIAdapter1* currentAdapter = nullptr;
			auto hr = DXGIFactory4->EnumAdapters1(adapterIndex, &currentAdapter);
			if (hr == DXGI_ERROR_NOT_FOUND) {
				break;
			} else if (FAILED(hr)) {
				fprintf(stderr, "error: WARP adapter not present on this system!\n");
				return -1;
			}

			adapters.push_back(currentAdapter);
		}

        if (gSettings.warp) {
			auto hr = DXGIFactory4->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		    if (FAILED(hr)) {
				fprintf(stderr, "error: WARP adapter not present on this system!\n");
				return -1;
			}
        }

		DXGIFactory4->Release();

		for (IDXGIAdapter1* curAdapter : adapters) {
			DXGI_ADAPTER_DESC1 adapterDesc;
			curAdapter->GetDesc1(&adapterDesc);
			if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
				adapter = curAdapter;
				break;
			}
		}

        gWorkloadD3D12 = new AsteroidsD3D12::Asteroids(&asteroids, &gGUI, NUM_SUBSETS, adapter, gSettings);
    }
    gSettings.d3d12 = (gWorkloadD3D12 != nullptr);

    // init window class
    WNDCLASSEX windowClass;
    ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"AsteroidsD3D12WindowClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, gSettings.windowWidth, gSettings.windowHeight };
    AdjustWindowRect(&windowRect, windowedStyle, FALSE);

    // create the window and store a handle to it
    auto hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"AsteroidsD3D12WindowClass",
        L"AsteroidsD3D12",
        windowedStyle,
        0, // CW_USE_DEFAULT
        0, // CW_USE_DEFAULT
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL,
        NULL,
        windowClass.hInstance,
        NULL);

    if (!gSettings.windowed) {
        ToggleFullscreen(hWnd);
    }

    SetForegroundWindow(hWnd);

    // Initialize performance counters
    UINT64 perfCounterFreq = 0;
    UINT64 lastPerfCount = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&perfCounterFreq);
    QueryPerformanceCounter((LARGE_INTEGER*)&lastPerfCount);

    // main loop
    double elapsedTime = 0.0;
    double frameTime = 0.0;
    //int lastMouseX = 0;
    //int lastMouseY = 0;
    //POINTER_INFO pointerInfo = {};

    timeBeginPeriod(1);
    EnableMouseInPointer(TRUE);

    if (gWorkloadD3D12->SupportVRS()) {
        fprintf(stderr, "System supports VRS. Press '0' to '7' to disable/change the VRS shading rate:\n");
        fprintf(stderr, "0: disable VRS (don't call RSSetShadingRate()\n");
        for (int i = 0; i < int(ARRAYSIZE(kD3D12ShadingRates)); ++i) {
            fprintf(stderr, "%d: %s\n", (i + 1), D3D12ShadingRateToStr(kD3D12ShadingRates[i]));
        }
    }
    /*
    else {
        fprintf(stderr, "System does not support VRS.\n");
    }
    */

    std::unique_ptr<std::wofstream> logFilePtr;
    if (gSettings.enableLogFPS) {
        logFilePtr.reset(new std::wofstream("fpslog.txt"));

        *logFilePtr << L"Num asteroids: " << gSettings.numAsteroids << std::endl;
        if (gSettings.enableStereoMode) {
            if (gSettings.enableViewportInstancing) {
                *logFilePtr << L"Enable VI" << std::endl;
            }
            else {
                *logFilePtr << L"Enable non-VI stereo mode" << std::endl;
            }
        }
        else {
            *logFilePtr << L"Disable stereo mode" << std::endl;
        }

        if (gSettings.neverAnimate) {
            *logFilePtr << L"Never Animate" << std::endl;
        }
        else {
            *logFilePtr << L"Disable never animate" << std::endl;
        }

        if (adapter != nullptr) {
            DXGI_ADAPTER_DESC adapterDesc = {};
            adapter->GetDesc(&adapterDesc);
            *logFilePtr << adapterDesc.Description << L"(0x" << std::hex << std::setw(4) << adapterDesc.DeviceId << L") Vendor ID: 0x" << adapterDesc.VendorId << std::endl;
            *logFilePtr << L"ViewportInstancing Tier: " << static_cast<int>(gWorkloadD3D12->GetViewInstancingTier()) << std::endl;

            *logFilePtr << L"VRS support: ";
            if (gWorkloadD3D12->SupportVRS()) {
                *logFilePtr << L"true" << std::endl;
                if (gSettings.useVRS) {
                    *logFilePtr << L"Enable VRS. Initial shading rate: " << D3D12ShadingRateToStr(gSettings.shadingRate) << std::endl;
                }
            }
            else {
                *logFilePtr << L"false" << std::endl;
            }
        }
    }

    for (;;)
    {
        bool d3d12LastFrame = gSettings.d3d12;

        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                // Cleanup
                if (logFilePtr) {
                    logFilePtr->close();
                }
                //delete gWorkloadD3D11;
                delete gWorkloadD3D12;
                SafeRelease(&gDXGIFactory);
                timeEndPeriod(1);
                EnableMouseInPointer(FALSE);
                return (int)msg.wParam;
            };

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If we swap to a new API we need to recreate swap chains
        if (d3d12LastFrame != gSettings.d3d12) {
            if (gSettings.d3d12) {
                //gWorkloadD3D11->ReleaseSwapChain();
                gWorkloadD3D12->ResizeSwapChain(gDXGIFactory, hWnd, gSettings.renderWidth, gSettings.renderHeight);
            } else {
                gWorkloadD3D12->ReleaseSwapChain();
                //gWorkloadD3D11->ResizeSwapChain(gDXGIFactory, hWnd, gSettings.renderWidth, gSettings.renderHeight);
            }
        }

        // Still need to process inertia even when no interaction is happening
        gCamera.ProcessInertia();

        // In D3D12 we'll wait on the GPU before taking the timestamp (more consistent)
        if (gSettings.d3d12) {
            gWorkloadD3D12->WaitForReadyToRender();
        }

        // Get time delta
        UINT64 count;
        QueryPerformanceCounter((LARGE_INTEGER*)&count);
        auto rawFrameTime = (double)(count - lastPerfCount) / perfCounterFreq;
        elapsedTime += rawFrameTime;
        lastPerfCount = count;

        // Maintaining absolute time sync is not important in this demo so we can err on the "smoother" side
        double alpha = 0.2f;
        frameTime = alpha * rawFrameTime + (1.0f - alpha) * frameTime;

        // Update GUI
        {
            char buffer[256];
            sprintf(buffer, "Asteroids D3D1%c - %4.1f ms", gSettings.d3d12 ? '2' : '1', 1000.f * frameTime);
            //SetWindowText(hWnd, buffer);

            if (gSettings.lockFrameRate) {
                sprintf(buffer, "(Locked)");
            } else {
                sprintf(buffer, "%.0f fps", 1.0f / frameTime);
            }
            gFPSControl->Text(buffer);

            if (gSettings.enableLogFPS) {
                *logFilePtr << buffer;
            }

            char bufferVRSAndVI[80];
            memset(bufferVRSAndVI, 0, sizeof(bufferVRSAndVI));
            size_t offset = 0;
            if (gSettings.enableStereoMode) {
                if (gSettings.enableViewportInstancing) {
                    const char* enableVI = "VI Enabled";
                    sprintf(bufferVRSAndVI, enableVI);
                    offset += strlen(enableVI);
                }
                else {
                    const char* enableNonVIStereoMode = "VI Disabled";
                    sprintf(bufferVRSAndVI, enableNonVIStereoMode);
                    offset += strlen(enableNonVIStereoMode);
                }
            }
            if (gSettings.useVRS) {
                if (gSettings.enableStereoMode) {
                    sprintf(bufferVRSAndVI + offset, ", ");
                    offset += 2u;
                }
                sprintf(bufferVRSAndVI + offset, "VRS Enabled (%s)", D3D12ShadingRateToStr(gSettings.shadingRate));
            }

            gVRSControl->Text(bufferVRSAndVI);

            gD3D12Control->Visible(gSettings.d3d12);
            //gD3D11Control->Visible(!gSettings.d3d12);
        }

        //if (gSettings.d3d12) {
            gWorkloadD3D12->Render((float)frameTime, gCamera, gSettings);
        //} else {
            //gWorkloadD3D11->Render((float)frameTime, gCamera, gSettings);
        //}

        if (gSettings.enableLogFPS) {
            *logFilePtr << " Total indices count in this frame:" << std::dec << gWorkloadD3D12->GetCurrentFrameTotalIndexCount() << "\n";
        }

        if (gSettings.lockFrameRate) {
            //ProfileBeginFrameLockWait();

            UINT64 afterRenderCount;
            QueryPerformanceCounter((LARGE_INTEGER*)&afterRenderCount);
            double renderTime = (double)(afterRenderCount - count) / perfCounterFreq;

            double targetRenderTime = 1.0 / double(gSettings.lockedFrameRate);
            double deltaMs = (targetRenderTime - renderTime) * 1000.0;
            if (deltaMs > 1.0) {
                Sleep((DWORD)deltaMs);
            }

            //ProfileEndFrameLockWait();
        }

        if (gSettings.takeScreenshot) {
            gSettings.takeScreenshot = false;
        }

        // All done?
        if (gSettings.closeAfterSeconds > 0.0 && elapsedTime > gSettings.closeAfterSeconds) {
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            if (logFilePtr) {
                logFilePtr->close();
            }
            printf("[RESULT] FPS:%.0f\n", 1.0f / frameTime);
            break;
        }
    }

    // Shouldn't get here
    return 0;
}
