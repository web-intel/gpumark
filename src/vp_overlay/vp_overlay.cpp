#include <iostream>
#include <iomanip>
#include <string>
#include "vp_overlay.h"

std::string WStringToString(const std::wstring& wstr)
{
	std::string str(wstr.length(), ' ');
	std::copy(wstr.begin(), wstr.end(), str.begin());
	return str;
}

static std::string get_dxgi_format_name(DXGI_FORMAT format)
{
    if (format == DXGI_FORMAT_UNKNOWN)                                 return "UNKNOWN";
    if (format == DXGI_FORMAT_R32G32B32A32_TYPELESS)                   return "R32G32B32A32_TYPELESS";
    if (format == DXGI_FORMAT_R32G32B32A32_FLOAT)                      return "R32G32B32A32_FLOAT";
    if (format == DXGI_FORMAT_R32G32B32A32_UINT)                       return "R32G32B32A32_UINT";
    if (format == DXGI_FORMAT_R32G32B32A32_SINT)                       return "R32G32B32A32_SINT";
    if (format == DXGI_FORMAT_R32G32B32_TYPELESS)                      return "R32G32B32_TYPELESS";
    if (format == DXGI_FORMAT_R32G32B32_FLOAT)                         return "R32G32B32_FLOAT";
    if (format == DXGI_FORMAT_R32G32B32_UINT)                          return "R32G32B32_UINT";
    if (format == DXGI_FORMAT_R32G32B32_SINT)                          return "R32G32B32_SINT";
    if (format == DXGI_FORMAT_R16G16B16A16_TYPELESS)                   return "R16G16B16A16_TYPELESS";
    if (format == DXGI_FORMAT_R16G16B16A16_FLOAT)                      return "R16G16B16A16_FLOAT";
    if (format == DXGI_FORMAT_R16G16B16A16_UNORM)                      return "R16G16B16A16_UNORM";
    if (format == DXGI_FORMAT_R16G16B16A16_UINT)                       return "R16G16B16A16_UINT";
    if (format == DXGI_FORMAT_R16G16B16A16_SNORM)                      return "R16G16B16A16_SNORM";
    if (format == DXGI_FORMAT_R16G16B16A16_SINT)                       return "R16G16B16A16_SINT";
    if (format == DXGI_FORMAT_R32G32_TYPELESS)                         return "R32G32_TYPELESS";
    if (format == DXGI_FORMAT_R32G32_FLOAT)                            return "R32G32_FLOAT";
    if (format == DXGI_FORMAT_R32G32_UINT)                             return "R32G32_UINT";
    if (format == DXGI_FORMAT_R32G32_SINT)                             return "R32G32_SINT";
    if (format == DXGI_FORMAT_R32G8X24_TYPELESS)                       return "R32G8X24_TYPELESS";
    if (format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)                    return "D32_FLOAT_S8X24_UINT";
    if (format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS)                return "R32_FLOAT_X8X24_TYPELESS";
    if (format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT)                 return "X32_TYPELESS_G8X24_UINT";
    if (format == DXGI_FORMAT_R10G10B10A2_TYPELESS)                    return "R10G10B10A2_TYPELESS";
    if (format == DXGI_FORMAT_R10G10B10A2_UNORM)                       return "R10G10B10A2_UNORM";
    if (format == DXGI_FORMAT_R10G10B10A2_UINT)                        return "R10G10B10A2_UINT";
    if (format == DXGI_FORMAT_R11G11B10_FLOAT)                         return "R11G11B10_FLOAT";
    if (format == DXGI_FORMAT_R8G8B8A8_TYPELESS)                       return "R8G8B8A8_TYPELESS";
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM)                          return "R8G8B8A8_UNORM";
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)                     return "R8G8B8A8_UNORM_SRGB";
    if (format == DXGI_FORMAT_R8G8B8A8_UINT)                           return "R8G8B8A8_UINT";
    if (format == DXGI_FORMAT_R8G8B8A8_SNORM)                          return "R8G8B8A8_SNORM";
    if (format == DXGI_FORMAT_R8G8B8A8_SINT)                           return "R8G8B8A8_SINT";
    if (format == DXGI_FORMAT_R16G16_TYPELESS)                         return "R16G16_TYPELESS";
    if (format == DXGI_FORMAT_R16G16_FLOAT)                            return "R16G16_FLOAT";
    if (format == DXGI_FORMAT_R16G16_UNORM)                            return "R16G16_UNORM";
    if (format == DXGI_FORMAT_R16G16_UINT)                             return "R16G16_UINT";
    if (format == DXGI_FORMAT_R16G16_SNORM)                            return "R16G16_SNORM";
    if (format == DXGI_FORMAT_R16G16_SINT)                             return "R16G16_SINT";
    if (format == DXGI_FORMAT_R32_TYPELESS)                            return "R32_TYPELESS";
    if (format == DXGI_FORMAT_D32_FLOAT)                               return "D32_FLOAT";
    if (format == DXGI_FORMAT_R32_FLOAT)                               return "R32_FLOAT";
    if (format == DXGI_FORMAT_R32_UINT)                                return "R32_UINT";
    if (format == DXGI_FORMAT_R32_SINT)                                return "R32_SINT";
    if (format == DXGI_FORMAT_R24G8_TYPELESS)                          return "R24G8_TYPELESS";
    if (format == DXGI_FORMAT_D24_UNORM_S8_UINT)                       return "D24_UNORM_S8_UINT";
    if (format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS)                   return "R24_UNORM_X8_TYPELESS";
    if (format == DXGI_FORMAT_X24_TYPELESS_G8_UINT)                    return "X24_TYPELESS_G8_UINT";
    if (format == DXGI_FORMAT_R8G8_TYPELESS)                           return "R8G8_TYPELESS";
    if (format == DXGI_FORMAT_R8G8_UNORM)                              return "R8G8_UNORM";
    if (format == DXGI_FORMAT_R8G8_UINT)                               return "R8G8_UINT";
    if (format == DXGI_FORMAT_R8G8_SNORM)                              return "R8G8_SNORM";
    if (format == DXGI_FORMAT_R8G8_SINT)                               return "R8G8_SINT";
    if (format == DXGI_FORMAT_R16_TYPELESS)                            return "R16_TYPELESS";
    if (format == DXGI_FORMAT_R16_FLOAT)                               return "R16_FLOAT";
    if (format == DXGI_FORMAT_D16_UNORM)                               return "D16_UNORM";
    if (format == DXGI_FORMAT_R16_UNORM)                               return "R16_UNORM";
    if (format == DXGI_FORMAT_R16_UINT)                                return "R16_UINT";
    if (format == DXGI_FORMAT_R16_SNORM)                               return "R16_SNORM";
    if (format == DXGI_FORMAT_R16_SINT)                                return "R16_SINT";
    if (format == DXGI_FORMAT_R8_TYPELESS)                             return "R8_TYPELESS";
    if (format == DXGI_FORMAT_R8_UNORM)                                return "R8_UNORM";
    if (format == DXGI_FORMAT_R8_UINT)                                 return "R8_UINT";
    if (format == DXGI_FORMAT_R8_SNORM)                                return "R8_SNORM";
    if (format == DXGI_FORMAT_R8_SINT)                                 return "R8_SINT";
    if (format == DXGI_FORMAT_A8_UNORM)                                return "A8_UNORM";
    if (format == DXGI_FORMAT_R1_UNORM)                                return "R1_UNORM";
    if (format == DXGI_FORMAT_R9G9B9E5_SHAREDEXP)                      return "R9G9B9E5_SHAREDEXP";
    if (format == DXGI_FORMAT_R8G8_B8G8_UNORM)                         return "R8G8_B8G8_UNORM";
    if (format == DXGI_FORMAT_G8R8_G8B8_UNORM)                         return "G8R8_G8B8_UNORM";
    if (format == DXGI_FORMAT_BC1_TYPELESS)                            return "BC1_TYPELESS";
    if (format == DXGI_FORMAT_BC1_UNORM)                               return "BC1_UNORM";
    if (format == DXGI_FORMAT_BC1_UNORM_SRGB)                          return "BC1_UNORM_SRGB";
    if (format == DXGI_FORMAT_BC2_TYPELESS)                            return "BC2_TYPELESS";
    if (format == DXGI_FORMAT_BC2_UNORM)                               return "BC2_UNORM";
    if (format == DXGI_FORMAT_BC2_UNORM_SRGB)                          return "BC2_UNORM_SRGB";
    if (format == DXGI_FORMAT_BC3_TYPELESS)                            return "BC3_TYPELESS";
    if (format == DXGI_FORMAT_BC3_UNORM)                               return "BC3_UNORM";
    if (format == DXGI_FORMAT_BC3_UNORM_SRGB)                          return "BC3_UNORM_SRGB";
    if (format == DXGI_FORMAT_BC4_TYPELESS)                            return "BC4_TYPELESS";
    if (format == DXGI_FORMAT_BC4_UNORM)                               return "BC4_UNORM";
    if (format == DXGI_FORMAT_BC4_SNORM)                               return "FORMAT_BC4_SNORM";
    if (format == DXGI_FORMAT_BC5_TYPELESS)                            return "BC5_TYPELESS";
    if (format == DXGI_FORMAT_BC5_UNORM)                               return "BC5_UNORM";
    if (format == DXGI_FORMAT_BC5_SNORM)                               return "BC5_SNORM";
    if (format == DXGI_FORMAT_B5G6R5_UNORM)                            return "B5G6R5_UNORM";
    if (format == DXGI_FORMAT_B5G5R5A1_UNORM)                          return "B5G5R5A1_UNORM";
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM)                          return "B8G8R8A8_UNORM";
    if (format == DXGI_FORMAT_B8G8R8X8_UNORM)                          return "B8G8R8X8_UNORM";
    if (format == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)              return "R10G10B10_XR_BIAS_A2_UNORM";
    if (format == DXGI_FORMAT_B8G8R8A8_TYPELESS)                       return "B8G8R8A8_TYPELESS";
    if (format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)                     return "B8G8R8A8_UNORM_SRGB";
    if (format == DXGI_FORMAT_B8G8R8X8_TYPELESS)                       return "B8G8R8X8_TYPELESS";
    if (format == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB)                     return "B8G8R8X8_UNORM_SRGB";
    if (format == DXGI_FORMAT_BC6H_TYPELESS)                           return "BC6H_TYPELESS";
    if (format == DXGI_FORMAT_BC6H_UF16)                               return "BC6H_UF16";
    if (format == DXGI_FORMAT_BC6H_SF16)                               return "BC6H_SF16";
    if (format == DXGI_FORMAT_BC7_TYPELESS)                            return "BC7_TYPELESS";
    if (format == DXGI_FORMAT_BC7_UNORM)                               return "BC7_UNORM";
    if (format == DXGI_FORMAT_BC7_UNORM_SRGB)                          return "BC7_UNORM_SRGB";
    if (format == DXGI_FORMAT_AYUV)                                    return "AYUV";
    if (format == DXGI_FORMAT_Y410)                                    return "Y410";
    if (format == DXGI_FORMAT_Y416)                                    return "Y416";
    if (format == DXGI_FORMAT_NV12)                                    return "NV12";
    if (format == DXGI_FORMAT_P010)                                    return "P010";
    if (format == DXGI_FORMAT_P016)                                    return "P016";
    if (format == DXGI_FORMAT_420_OPAQUE)                              return "420_OPAQUE";
    if (format == DXGI_FORMAT_YUY2)                                    return "YUY2";
    if (format == DXGI_FORMAT_Y210)                                    return "Y210";
    if (format == DXGI_FORMAT_Y216)                                    return "Y216";
    if (format == DXGI_FORMAT_NV11)                                    return "NV11";
    if (format == DXGI_FORMAT_AI44)                                    return "AI44";
    if (format == DXGI_FORMAT_IA44)                                    return "IA44";
    if (format == DXGI_FORMAT_P8)                                      return "P8";
    if (format == DXGI_FORMAT_A8P8)                                    return "A8P8";
    if (format == DXGI_FORMAT_B4G4R4A4_UNORM)                          return "B4G4R4A4_UNORM";
    if (format == DXGI_FORMAT_P208)                                    return "P208";
    if (format == DXGI_FORMAT_V208)                                    return "V208";
    if (format == DXGI_FORMAT_V408)                                    return "V408";
    if (format == DXGI_FORMAT_FORCE_UINT)                              return "FORCE_UINT";
    if (format == DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE)         return "SAMPLER_FEEDBACK_MIN_MIP_OPAQUE";
    if (format == DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE) return "SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE";

    return "<unrecognized format>";
}

static std::string get_dxgi_color_space_type_name(DXGI_COLOR_SPACE_TYPE type)
{
    if (type == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709)           return "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709";
    if (type == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709)           return "DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709";
    if (type == DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709)         return "DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709";
    if (type == DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020)        return "DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020";
    if (type == DXGI_COLOR_SPACE_RESERVED)                         return "DXGI_COLOR_SPACE_RESERVED";
    if (type == DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601)    return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601)       return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601";
    if (type == DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601)         return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709)       return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709";
    if (type == DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709)         return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020)      return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020)        return "DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020";
    if (type == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)        return "DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020)    return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020";
    if (type == DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020)      return "DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020)   return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020) return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020";
    if (type == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020)          return "DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020)  return "DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020)    return "DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020";
    if (type == DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709)         return "DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709";
    if (type == DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020)        return "DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709)       return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020)      return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020";
    if (type == DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020)   return "DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020";

    return "<unrecognized type>";
}

void DxVideoInfoChecker::CheckVideoProcessorCapability() {
	std::cout << "[VP]" << std::endl;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
	if(FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_factory))))
		std::cout << "Create DXGI factory error." << std::endl;

	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter;
	if(FAILED(dxgi_factory->EnumAdapters(0,&dxgi_adapter)))
		std::cout << "Enum adapters error." << std::endl;

	UINT flags = 0;
	D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
	D3D_FEATURE_LEVEL feature_level_out = D3D_FEATURE_LEVEL_11_1;
	Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context;
	HRESULT hr = D3D11CreateDevice(dxgi_adapter.Get(),   //IDXGIAdapter
		D3D_DRIVER_TYPE_UNKNOWN,    //DriverType
		0,					    	//Software
		flags,						//Flags
		feature_levels,			    //*pFeatureLevels
		1,							//FeatureLevels
		D3D11_SDK_VERSION,			//SDKVersion
		&d3d11_device,				//**ppDevice
		&feature_level_out,			//*pFeatureLevel
		&d3d11_device_context       //**ppImmediateContext
	);
	if (FAILED(hr)) {
		std::cout << "Create D3D11 device failed. Code:" << std::hex << hr << std::endl;
		return;
	}

	Microsoft::WRL::ComPtr<ID3D11VideoDevice> v_device;
	if (!SUCCEEDED(d3d11_device.As(&v_device))) {
		std::cout << "Create d3d video device failed." << std::endl;
		return;
	}

	// The values here should have _no_ effect on supported profiles, but they
	// are needed anyway for initialization.
	D3D11_VIDEO_PROCESSOR_CONTENT_DESC desc;
	desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
	desc.InputFrameRate.Numerator = 60;
	desc.InputFrameRate.Denominator = 1;
	desc.InputWidth = 1920;
	desc.InputHeight = 1080;
	desc.OutputFrameRate.Numerator = 60;
	desc.OutputFrameRate.Denominator = 1;
	desc.OutputWidth = 1920;
	desc.OutputHeight = 1080;
	desc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

	Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> enumerator;
	if (!SUCCEEDED(v_device->CreateVideoProcessorEnumerator(&desc, &enumerator))) {
		std::cout << "Create video processor failed." << std::endl;
		return;
	}

	UINT support_flag = 0;
	for (UINT i = 0; i <= DXGI_FORMAT_MAX; i++) {
		if (FAILED(enumerator->CheckVideoProcessorFormat(static_cast<DXGI_FORMAT>(i), &support_flag))) {
			std::cout << "Check video processor format Failed."<<std::endl;
			continue;
		}
		if (support_flag & D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT)
			std::cout << get_dxgi_format_name(DXGI_FORMAT(i))<<std::endl;
	}
}

void DxVideoInfoChecker::CheckOverlayCapability() {
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter;
	if(FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_factory))))
		std::cout << "Create DXGI factory error." << std::endl;

	if(FAILED(dxgi_factory->EnumAdapters(0,&dxgi_adapter)))
		std::cout << "Enum adapters error." << std::endl;

	UINT flags = 0;
	D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
	D3D_FEATURE_LEVEL feature_level_out = D3D_FEATURE_LEVEL_11_1;
	Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context;
	HRESULT hr = D3D11CreateDevice(dxgi_adapter.Get(),   //IDXGIAdapter
		D3D_DRIVER_TYPE_UNKNOWN,    //DriverType
		0,					    	//Software
		flags,						//Flags
		feature_levels,			    //*pFeatureLevels
		1,							//FeatureLevels
		D3D11_SDK_VERSION,			//SDKVersion
		&d3d11_device,				//**ppDevice
		&feature_level_out,			//*pFeatureLevel
		&d3d11_device_context       //**ppImmediateContext
	);
	if (FAILED(hr)) {
		std::cout<<"Create D3D11 device failed. Code:"<<std::hex<<hr<<std::endl;
		return;
	}

	UINT i = 0;
	while (true) {
        int nv12[2] = {0};
        int yuy2[2] = {0};
        int rgba8[2] = {0};
        int rgb10a2 = 0;

		Microsoft::WRL::ComPtr<IDXGIOutput> output;
		if (FAILED(dxgi_adapter->EnumOutputs(i++, &output)))
			break;

		DXGI_OUTPUT_DESC desc;
		if (SUCCEEDED(output->GetDesc(&desc))) {
			std::wstring wdevice_name(desc.DeviceName);
			//UINT width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
			//UINT height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
			//std::cout<<"DeviceName:"<<WStringToString(wdevice_name)<<std::endl;
			//std::cout << "Resulotion:" << width << "x" << height<<std::endl;
		}

		Microsoft::WRL::ComPtr<IDXGIOutput3> output3;
		if (FAILED(output.As(&output3)))
			continue;

		std::cout << "[Overlay IDXGIOutput3::CheckOverlaySupport]" << std::endl;
		UINT overlay_support_flags;

		for (UINT i = 0; i <= DXGI_FORMAT_MAX; i++) {
			output3->CheckOverlaySupport(static_cast<DXGI_FORMAT>(i), d3d11_device.Get(),
				&overlay_support_flags);
			if (overlay_support_flags) {
                if (i == DXGI_FORMAT_NV12) {
                    if (overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_DIRECT)
                        nv12[0] = 1;
                    if (overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_SCALING)
                        nv12[1] = 1;
                }
                if (i == DXGI_FORMAT_YUY2) {
                    if (overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_DIRECT)
                        yuy2[0] = 1;
                    if (overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_SCALING)
                        yuy2[1] = 1;
                }
                if (i == DXGI_FORMAT_R8G8B8A8_UNORM) {
                    if (overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_DIRECT)
                        rgba8[0] = 1;
                    if (overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_SCALING)
                        rgba8[1] = 1;
                }
				std::cout << get_dxgi_format_name(DXGI_FORMAT(i))
					<< ",DIRECT:" << bool(overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_DIRECT)
					<< ",SCALING:" << bool(overlay_support_flags & DXGI_OVERLAY_SUPPORT_FLAG_SCALING)
					<< std::endl;
			}
		}

		Microsoft::WRL::ComPtr<IDXGIOutput4> output4;
		if (FAILED(output.As(&output4)))
			continue;
		std::cout << "[Overlay IDXGIOutput4::CheckOverlayColorSpaceSupport]" << std::endl;
		UINT colorspace_support_flag;
		for (UINT format_idx = 0; format_idx <= DXGI_FORMAT_MAX; format_idx++) {
			for (UINT colorspace_idx = 0; colorspace_idx <= DXGI_COLOR_SPACE_TYPE_MAX; colorspace_idx++) {
				output4->CheckOverlayColorSpaceSupport(static_cast<DXGI_FORMAT>(format_idx),
					static_cast<DXGI_COLOR_SPACE_TYPE>(colorspace_idx),
					d3d11_device.Get(),
					&colorspace_support_flag);

				if (colorspace_support_flag) {
					std::cout << get_dxgi_format_name(DXGI_FORMAT(format_idx)) << "," << get_dxgi_color_space_type_name(DXGI_COLOR_SPACE_TYPE(colorspace_idx)) << std::endl;
                    if (format_idx == DXGI_FORMAT_R10G10B10A2_UNORM && colorspace_idx == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
                        rgb10a2 = 1;
                    }
				}
			}
		}

        printf("[RESULT] NV12:{direct:%d,scaling:%d},YUY2:{direct:%d,scaling:%d},RGBA8:{direct:%d,scaling:%d},RGBA10A2:%d\n", nv12[0], nv12[1], yuy2[0], yuy2[1], rgba8[0], rgba8[1], rgb10a2);

	}
}

int main() {
	DxVideoInfoChecker video_info_checker;
	video_info_checker.CheckVideoProcessorCapability();
	video_info_checker.CheckOverlayCapability();
	return 0;
}