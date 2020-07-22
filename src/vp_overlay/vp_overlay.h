#include <d3d11.h>
#include <windows.h>
#include <dcomp.h>
#include <wrl/client.h>

class DxVideoInfoChecker {
public:
	const UINT DXGI_FORMAT_MAX = 133;
	const UINT DXGI_COLOR_SPACE_TYPE_MAX = DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020;

	void CheckOverlayCapability();
	void CheckVideoProcessorCapability();
};
