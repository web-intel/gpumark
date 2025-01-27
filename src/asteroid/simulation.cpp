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

#include "simulation.h"
#include "settings.h"
#include "texture.h"
#include "util.h"
#include "../include/util.h"

#include <random>
#include <limits>
#include <algorithm>
#include <iostream>
//#include <ppl.h>

using namespace DirectX;

static int const COLOR_SCHEMES[] = {
    156, 139, 113,  55,  49,  40,
    156, 139, 113,  58,  38,  14,
    156, 139, 113,  98, 101, 104,
    156, 139, 113, 205, 197, 178,
    153, 146, 136,  88,  88,  88,
    189, 181, 164, 148, 108, 102,
};

static int const NUM_COLOR_SCHEMES = (int) (sizeof(COLOR_SCHEMES) / (6 * sizeof(int)));

static XMVECTOR RandomPointOnSphere(std::mt19937& rng)
{
    std::normal_distribution<float> dist;

    XMVECTOR r;
    for (;;) {
        r = XMVectorSet(dist(rng), dist(rng), dist(rng), 0.0f);
        auto d2 = XMVectorGetX(XMVector3LengthSq(r));
        if (d2 > std::numeric_limits<float>::min()) {
            return XMVector3Normalize(r);
        }
    }
    // Unreachable
}

// From http://guihaire.com/code/?p=1135
static inline float VeryApproxLog2f(float x)
{
    union { float f; uint32_t i; } ux;
    ux.f = x;
    return (float)ux.i * 1.1920928955078125e-7f - 126.94269504f;
}


AsteroidsSimulation::AsteroidsSimulation(unsigned int rngSeed, unsigned int asteroidCount,
                                         unsigned int meshInstanceCount, unsigned int subdivCount,
                                         unsigned int textureCount)
    : mAsteroidStatic(asteroidCount)
    , mAsteroidDynamic(asteroidCount)
    , mIndexOffsets(subdivCount + 2) // Mesh subdivs are inclusive on both ends and need forward differencing for count
    , mSubdivCount(subdivCount)
{
    std::mt19937 rng(rngSeed);

    // Create meshes
    //std::cout
    //    << "Creating " << meshInstanceCount << " meshes, each with "
    //    << subdivCount << " subdivision levels..." << std::endl;

    CreateAsteroidsFromGeospheres(&mMeshes, mSubdivCount, meshInstanceCount,
                                  rng(), mIndexOffsets.data(), &mVertexCountPerMesh);
    std::cout << "VertexCountPerMesh: " << mVertexCountPerMesh << std::endl
              << "Indices count: ";
    for (unsigned int indexCount : mIndexOffsets) {
        std::cout << indexCount << " ";
    }
    std::cout << std::endl;

    CreateTextures(textureCount, rng());

    // Constants
    std::normal_distribution<float> orbitRadiusDist(SIM_ORBIT_RADIUS, 0.8f * SIM_DISC_RADIUS);
    std::normal_distribution<float> heightDist(0.0f, 0.4f);
    std::uniform_real_distribution<float> angleDist(-XM_PI, XM_PI);
    std::uniform_real_distribution<float> radialVelocityDist(5.0f, 15.0f);
    std::uniform_real_distribution<float> spinVelocityDist(-2.0f, 2.0f);
    std::normal_distribution<float> scaleDist(1.4f, 0.9f);
    std::normal_distribution<float> colorSchemeDist(0, NUM_COLOR_SCHEMES - 1);
    std::uniform_int_distribution<unsigned int> textureIndexDist(0, textureCount-1);

    auto instancesPerMesh = std::max(1U, asteroidCount / meshInstanceCount);

    // Approximate SRGB->Linear for colors
    float linearColorSchemes[NUM_COLOR_SCHEMES * 6];
    for (int i = 0; i < int(ARRAYSIZE(linearColorSchemes)); ++i) {
        linearColorSchemes[i] = std::powf((float)COLOR_SCHEMES[i] / 255.0f, 2.2f);
    }

    // Create a torus of asteroids that spin around the ring
    for (unsigned int i = 0; i < asteroidCount; ++i) {
        auto scale = scaleDist(rng);
#if SIM_USE_GAMMA_DIST_SCALE
        scale = scale * 0.3f;
#endif
        scale = std::max(scale, SIM_MIN_SCALE);
        auto scaleMatrix = XMMatrixScaling(scale, scale, scale);

        auto orbitRadius = std::max(0.01f, orbitRadiusDist(rng));
        auto discPosY = float(SIM_DISC_RADIUS) * heightDist(rng);

        auto disc = XMMatrixTranslation(orbitRadius, discPosY, 0.0f);

        auto positionAngle = angleDist(rng);
        auto orbit = XMMatrixRotationY(positionAngle);

        auto meshInstance = (unsigned int)(i / instancesPerMesh); // Vcache friendly ordering

        // Static data
        mAsteroidStatic[i].spinVelocity = spinVelocityDist(rng) / scale; // Smaller asteroids spin faster
        mAsteroidStatic[i].orbitVelocity = radialVelocityDist(rng) / (scale * orbitRadius); // Smaller asteroids go faster, and use arc length
        mAsteroidStatic[i].vertexStart = mVertexCountPerMesh * meshInstance;
        mAsteroidStatic[i].spinAxis = XMVector3Normalize(RandomPointOnSphere(rng));
        mAsteroidStatic[i].scale = scale;
        mAsteroidStatic[i].textureIndex = textureIndexDist(rng);

        auto colorScheme = ((int)abs(colorSchemeDist(rng))) % NUM_COLOR_SCHEMES;
        auto c = linearColorSchemes + 6 * colorScheme;
        mAsteroidStatic[i].surfaceColor = XMFLOAT3(c[0], c[1], c[2]);
        mAsteroidStatic[i].deepColor    = XMFLOAT3(c[3], c[4], c[5]);

        // Initialize dynamic data
        mAsteroidDynamic[i].world = scaleMatrix * disc * orbit;

        assert(mAsteroidStatic[i].scale > 0.0f);
        assert(mAsteroidStatic[i].orbitVelocity > 0.0f);
    }
}


size_t AsteroidsSimulation::Update(float frameTime, DirectX::XMVECTOR cameraEye, const Settings& settings,
                                 size_t startIndex, size_t count)
{
    bool animate = settings.animate;

    // TODO: This constant should really depend on resolution and/or be configurable...
    static const float minSubdivSizeLog2 = std::log2f(0.0019f);

    size_t last = count ? startIndex + count : mAsteroidDynamic.size();

    size_t totalIndicesCount = 0;
    for (size_t i = startIndex; i < last; ++i) {
        const AsteroidStatic& staticData = mAsteroidStatic[i];
        AsteroidDynamic& dynamicData = mAsteroidDynamic[i];

        if (animate) {
            auto orbit = XMMatrixRotationY(staticData.orbitVelocity * frameTime);
            auto spin = XMMatrixRotationNormal(staticData.spinAxis, staticData.spinVelocity * frameTime);
            dynamicData.world = spin * dynamicData.world * orbit;
        }

        // Pick LOD based on approx screen area - can be very approximate
        auto position = dynamicData.world.r[3];
        auto distanceToEyeRcp = XMVectorGetX(XMVector3ReciprocalLengthEst(XMVectorSubtract(cameraEye, position)));
        // Add one subdiv for each factor of 2 past min
        auto relativeScreenSizeLog2 = VeryApproxLog2f(staticData.scale * distanceToEyeRcp);
        float subdivFloat = std::max(0.0f, relativeScreenSizeLog2 - minSubdivSizeLog2);

        auto subdiv = std::min(mSubdivCount - 1, static_cast<unsigned int>(subdivFloat));
        // TODO: Ignore/cull/force lowest subdiv if offscreen?

        dynamicData.indexStart = mIndexOffsets[subdiv];
        dynamicData.indexCount = mIndexOffsets[subdiv+1] - dynamicData.indexStart;
        totalIndicesCount += dynamicData.indexCount;
    }
    return totalIndicesCount;
}


void AsteroidsSimulation::CreateTextures(unsigned int textureCount, unsigned int rngSeed)
{
    mTextureDim = TEXTURE_DIM;
    mTextureCount = textureCount;
    mTextureArraySize = 3;
    {
        DWORD msbIndex = 0;
        auto result = _BitScanReverse(&msbIndex, mTextureDim);
        ASSERT(result); // Should only fail if mTextureDim = 0
        mTextureMipLevels = msbIndex + 1;
    }

    assert((mTextureDim & (mTextureDim-1)) == 0); // Must be pow2 currently; we don't handle wacky mip chains

    //std::cout
    //    << "Creating " << textureCount << " "
    //    << mTextureDim << "x" << mTextureDim << " textures..." << std::endl;

    // Allocate space
    UINT texelSizeInBytes = 4; // RGBA8
    UINT extraSpaceForMips = 2;
    UINT totalTextureSizeInBytes = texelSizeInBytes * mTextureDim * mTextureDim * mTextureArraySize * extraSpaceForMips;
    totalTextureSizeInBytes = Align(totalTextureSizeInBytes, 64U); // Avoid false sharing

    mTextureDataBuffer.resize(totalTextureSizeInBytes * textureCount);
    mTextureSubresources.resize(mTextureArraySize * mTextureMipLevels * textureCount);

    // Parallel over textures
    std::vector<unsigned int> rngSeeds(textureCount);
    {
        std::mt19937 seeds;
        for (auto &i : rngSeeds) i = seeds();
    }

    //concurrency::parallel_for(UINT(0), textureCount, [&](UINT t) {
    for (unsigned int t = 0; t < textureCount; ++t) {
        std::mt19937 rng(rngSeeds[t]);
        auto randomNoise = std::uniform_real_distribution<float>(0.0f, 10000.0f);
        auto randomNoiseScale = std::uniform_real_distribution<float>(100, 150);
        auto randomPersistence = std::normal_distribution<float>(0.9f, 0.2f);

        BYTE* data = mTextureDataBuffer.data() + t * totalTextureSizeInBytes;
        for (UINT a = 0; a < mTextureArraySize; ++a) {
            for (UINT m = 0; m < mTextureMipLevels; ++m) {
                auto width  = mTextureDim >> m;
                auto height = mTextureDim >> m;

                D3D11_SUBRESOURCE_DATA initialData = {};
                initialData.pSysMem = data;
                initialData.SysMemPitch = width * texelSizeInBytes;
                mTextureSubresources[SubresourceIndex(t, a, m)] = initialData;

                data += initialData.SysMemPitch * height;
            }
        }

        // Use same parameters for each of the tri-planar projection planes/cube map faces/etc.
        float noiseScale = randomNoiseScale(rng) / float(mTextureDim);
        float persistence = randomPersistence(rng);
        float strength = 1.5f;

        for (UINT a = 0; a < mTextureArraySize; ++a) {
            float redScale   = 255.0f;
            float greenScale = 255.0f;
            float blueScale  = 255.0f;

            // DEBUG colors
#if 0
            redScale   = t & 1 ? 255.0f : 0.0f;
            greenScale = t & 2 ? 255.0f : 0.0f;
            blueScale  = t & 4 ? 255.0f : 0.0f;
#endif

            FillNoise2D_RGBA8(&mTextureSubresources[SubresourceIndex(t, a)], mTextureDim, mTextureDim, mTextureMipLevels,
                              randomNoise(rng), persistence, noiseScale, strength,
                              redScale, greenScale, blueScale);
        }
    }
    //); // parallel_for
}
