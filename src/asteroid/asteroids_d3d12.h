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

#pragma once

#include <d3d11.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <directxmath.h>
#include <deque>
#include <random>
#include <map>
#include <mutex>

#include "camera.h"
#include "settings.h"
#include "simulation.h"
#include "subset_d3d12.h"
#include "descriptor.h"
#include "upload_heap.h"
#include "util.h"
#include "gui.h"
#include "../include/util.h"

namespace AsteroidsD3D12 {

CBUFFER_ALIGN struct DrawConstantBuffer {
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT4X4 mViewProjection;
    DirectX::XMFLOAT3 mSurfaceColor;
    float unused0;
    DirectX::XMFLOAT3 mDeepColor;
    float unused1;
    UINT mTextureIndex;
};

CBUFFER_ALIGN struct SkyboxConstantBuffer {
    DirectX::XMFLOAT4X4 mViewProjection;
};

struct ExecuteIndirectArgs {
    D3D12_GPU_VIRTUAL_ADDRESS mConstantBuffer;
    D3D12_DRAW_INDEXED_ARGUMENTS mDrawIndexed;
};

struct DynamicUploadHeap {
    DrawConstantBuffer mDrawConstantBuffers[NUM_ASTEROIDS];
    SkyboxConstantBuffer mSkyboxConstants;
    ExecuteIndirectArgs mIndirectArgs[NUM_ASTEROIDS];
    SpriteVertex mSpriteVertices[MAX_SPRITE_VERTICES_PER_FRAME];
};

class Asteroids {
public:
    Asteroids(AsteroidsSimulation* asteroids, GUI *gui, UINT minCmdLsts, IDXGIAdapter* adapter, Settings settings);
    ~Asteroids();

    void WaitForReadyToRender();
    void Render(float frameTime, const OrbitCamera& camera, const Settings& settings);

    void ReleaseSwapChain();
    void ResizeSwapChain(IDXGIFactory2* dxgiFactory, HWND outputWindow, unsigned int width, unsigned int height);

    int GetScreenShotIndex() const { return mScreenShotIndex; }

    bool SupportVRS() const {
        return mSupportVRS;
    }

    D3D12_VIEW_INSTANCING_TIER GetViewInstancingTier() const {
        return mVITier;
    }

    size_t GetCurrentFrameTotalIndexCount() const {
        return mTotalIndexCountPerFrame;
    }

private:
    void WaitForAll();

    void RenderSubset(
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView,
        size_t frameIndex, float frameTime,
        SubsetD3D12* subset, UINT subsetIdx,
        DirectX::XMVECTOR cameraEye, DirectX::XMMATRIX viewProjection,
        const Settings& settings);

    void CreatePSOs();

    void CreateSubsets(UINT numHeapsPerFrame);
    void ReleaseSubsets();

    void CreateMeshes();
    void CreateGUIResources();

    void UpdateTotalIndexCount(size_t indexCountInOneSubset);

    struct Frame {
        std::vector<SubsetD3D12*>   mSubsets;
        ID3D12CommandAllocator*     mCmdAlloc = nullptr;

        UploadHeap* mDynamicUpload = nullptr;
        D3D12_VERTEX_BUFFER_VIEW    mSpriteVertexBufferView;
        D3D12_GPU_VIRTUAL_ADDRESS   mDrawConstantBuffersGPUVA;

        // Descriptor heap and associated GPU handles
        SRVDescriptorList*          mSRVDescs = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS   mSkyboxConstants;
        D3D12_GPU_DESCRIPTOR_HANDLE mSkyboxTexture;
        UINT                        mSRVDescsDynamicStart = 0;

        UINT64                      mFrameCompleteFence = 0;
    } mFrame[NUM_FRAMES_TO_BUFFER];

    // Swap chain resources
    struct SwapChainBuffer {
        ID3D12Resource*             mRenderTarget = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE mRenderTargetView;
    } mSwapChainBuffer[NUM_SWAP_CHAIN_BUFFERS];

    // Swap chain stuff
    IDXGISwapChain3*            mSwapChain = nullptr;
    DXGI_FORMAT                 mRTVFormat;
    CD3DX12_VIEWPORT              mViewPorts[2];
    CD3DX12_RECT                  mScissorRects[2];

    // Fences and synchronization
    size_t                      mCurrentFrameIndex = 0;
    ID3D12Fence*                mFence = nullptr;
    HANDLE                      mFenceEventHandle = NULL;
    UINT64                      mCurrentFence = 0;

    // Device
    ID3D12Device2*               mDevice = nullptr;
    ID3D12CommandQueue*         mCommandQueue = nullptr;

    // Root signature, descriptors and binding
    ID3D12RootSignature*        mGenericRootSignature = nullptr;
    ID3D12RootSignature*        mAsteroidsRootSignature = nullptr;
    ID3D12CommandSignature*     mCommandSignature = nullptr;
    RTVDescriptorList*          mRTVDescs = nullptr;
    DSVDescriptorList*          mDSVDescs = nullptr;
    SRVDescriptorList*          mSRVDescs = nullptr;
    SMPDescriptorList*          mSMPDescs = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mDepthStencilView;
    D3D12_GPU_DESCRIPTOR_HANDLE mSampler;
    D3D12_VERTEX_BUFFER_VIEW    mSkyboxVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW     mAsteroidIndexBufferView;
    D3D12_VERTEX_BUFFER_VIEW    mAsteroidVertexBufferView;

    // Command lists
    ID3D12GraphicsCommandList2*  mPreCmdLst = nullptr;
    ID3D12GraphicsCommandList2*  mPostCmdLst = nullptr;

    DXGI_FORMAT                 mDSVFormat;
    ID3D12Resource*             mDepthStencil = nullptr;

    AsteroidsSimulation*        mAsteroids = nullptr;
    ID3D12Resource*             mAsteroidTextures[NUM_UNIQUE_TEXTURES];

    // Mesh
    UploadHeap*                 mMeshUpload = nullptr;
    UINT                        mIndexOffsets[MESH_MAX_SUBDIV_LEVELS + 2]; // inclusive
    //UINT                        mNumVerticesPerMesh = 0;

    ID3D12PipelineState*        mAsteroidPSO = nullptr;

    ID3D12PipelineState*        mSkyboxPSO = nullptr;
    ID3D12Resource*             mSkybox = nullptr;

    ID3D12PipelineState*        mFontPSO = nullptr;
    ID3D12Resource*             mFontTexture = nullptr;

    ID3D12PipelineState*        mSpritePSO = nullptr;
    ID3D12PipelineState*        mSpriteIconPSO = nullptr;
    std::map<std::wstring, ID3D12Resource*> mSpriteTextures;

    GUI*                        mGUI = nullptr;

    // Transient, just here to avoid allocations each frame
    std::vector<ID3D12GraphicsCommandList*> mCmdListsToSubmit;

    UINT                        mSubsetCount = 0;
    UINT                        mDrawsPerSubset = 0;

    Settings                    mSettings;

    bool                        mSupportVRS;

    int mScreenShotIndex = 1;

    D3D12_VIEW_INSTANCING_TIER mVITier = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;

    std::mutex mMutex;
    size_t mTotalIndexCountPerFrame = 0;
};

} // namespace AsteroidsD3D12

constexpr D3D12_SHADING_RATE kD3D12ShadingRates[] = {
    D3D12_SHADING_RATE_1X1, D3D12_SHADING_RATE_1X2, D3D12_SHADING_RATE_2X1,
    D3D12_SHADING_RATE_2X2, D3D12_SHADING_RATE_2X4, D3D12_SHADING_RATE_4X2,
    D3D12_SHADING_RATE_4X4 };

const char* D3D12ShadingRateToStr(D3D12_SHADING_RATE shadingRate);