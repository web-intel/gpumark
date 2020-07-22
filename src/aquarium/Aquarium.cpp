//
// Copyright (c) 2019 The Aquarium Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Aquarium.cpp: Create context for specific graphics API.
// Data preparation, load vertex and index buffer, images and shaders.
// Implements logic of rendering background, fishes, seaweeds and
// other models. Calculate fish count for each type of fish.
// Update uniforms for each frame.

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#endif

#include "Aquarium.h"
#include "ContextFactory.h"
#include "FishModel.h"
#include "Matrix.h"
#include "Program.h"
#include "SeaweedModel.h"
#include "Texture.h"

#include "AQUARIUM_ASSERT.h"
#include "CmdArgsHelper.h"
#include "Context.h"

#include "rapidjson/document.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

Aquarium::Aquarium()
    : mModelEnumMap(),
      mTextureMap(),
      mProgramMap(),
      mAquariumModels(),
      mContext(nullptr),
      mFpsTimer(),
      mCurFishCount(30000),
      mPreFishCount(0),
      mTestTime(INT_MAX),
      mBackendType(BACKENDTYPE::BACKENDTYPED3D12),
      mFactory(nullptr)
{
    g.then          = 0.0;
    g.mclock        = 0.0;
    g.eyeClock      = 0.0;
    g.alpha         = "1";

    lightUniforms.lightColor[0] = 1.0f;
    lightUniforms.lightColor[1] = 1.0f;
    lightUniforms.lightColor[2] = 1.0f;
    lightUniforms.lightColor[3] = 1.0f;

    lightUniforms.specular[0] = 1.0f;
    lightUniforms.specular[1] = 1.0f;
    lightUniforms.specular[2] = 1.0f;
    lightUniforms.specular[3] = 1.0f;

    fogUniforms.fogColor[0] = g_fogRed;
    fogUniforms.fogColor[1] = g_fogGreen;
    fogUniforms.fogColor[2] = g_fogBlue;
    fogUniforms.fogColor[3] = 1.0f;

    fogUniforms.fogPower  = g_fogPower;
    fogUniforms.fogMult   = g_fogMult;
    fogUniforms.fogOffset = g_fogOffset;

    lightUniforms.ambient[0] = g_ambientRed;
    lightUniforms.ambient[1] = g_ambientGreen;
    lightUniforms.ambient[2] = g_ambientBlue;
    lightUniforms.ambient[3] = 0.0f;

    memset(fishCounts, 0, 5);
}

Aquarium::~Aquarium()
{
    for (auto &tex : mTextureMap)
    {
        if (tex.second != nullptr)
        {
            delete tex.second;
            tex.second = nullptr;
        }
    }

    for (auto &program : mProgramMap)
    {
        if (program.second != nullptr)
        {
            delete program.second;
            program.second = nullptr;
        }
    }

    for (int i = 0; i < MODELNAME::MODELMAX; ++i)
    {
        delete mAquariumModels[i];
    }

    if (toggleBitset.test(static_cast<size_t>(TOGGLE::SIMULATINGFISHCOMEANDGO)))
    {
        while (!mFishBehavior.empty())
        {
            Behavior *behave = mFishBehavior.front();
            mFishBehavior.pop();
            delete behave;
        }
    }

    delete mFactory;
}

BACKENDTYPE Aquarium::getBackendType(const std::string &backendPath)
{
    if (backendPath == "opengl")
    {
        return BACKENDTYPE::BACKENDTYPEOPENGL;
    }
    else if (backendPath == "dawn_d3d12")
    {
#if defined(WIN32) || defined(_WIN32)
        return BACKENDTYPE::BACKENDTYPEDAWND3D12;
#endif
    }
    else if (backendPath == "dawn_metal")
    {
#if defined(__APPLE__)
        return BACKENDTYPE::BACKENDTYPEDAWNMETAL;
#endif
    }
    else if (backendPath == "dawn_vulkan")
    {
#if defined(WIN32) || defined(_WIN32) || defined(__linux__)
        return BACKENDTYPE::BACKENDTYPEDAWNVULKAN;
#endif
    }
    else if (backendPath == "angle")
    {
#if defined(WIN32) || defined(_WIN32)
        return BACKENDTYPE::BACKENDTYPEANGLE;
#endif
    }
    else if (backendPath == "d3d12")
    {
#if defined(WIN32) || defined(_WIN32)
        return BACKENDTYPED3D12;
#endif
    }

    return BACKENDTYPELAST;
}

bool Aquarium::init(int argc, char **argv)
{
    mFactory = new ContextFactory();

    // Create context of different backends through the cmd args.
    // "--backend" {backend}: create different backends. currently opengl is supported.
    // "--fish-count" {fishcount}: imply rendering fish count.
    // "--msaa-count" {msaacount}: MSAA sample counts.
    // "--enable-instanced-draws": use instanced draw. By default, it's individual draw.
    char *pNext;
    for (int i = 1; i < argc; ++i)
    {
        std::string cmd(argv[i]);
        if (cmd == "-h" || cmd == "--h")
        {
            std::cout << cmdArgsStrAquarium << std::endl;

            return false;
        }
        if (cmd == "--backend")
        {
            std::string backend = argv[i++ + 1];
            mBackendType        = getBackendType(backend);
        }
    }

    mContext = mFactory->createContext(mBackendType);

    if (mContext == nullptr)
    {
        std::cout << "Failed to create context." << std::endl;
        return false;
    }

    std::bitset<static_cast<size_t>(TOGGLE::TOGGLEMAX)> availableToggleBitset =
        mContext->getAvailableToggleBitset();
    if (availableToggleBitset.test(static_cast<size_t>(TOGGLE::UPATEANDDRAWFOREACHMODEL)))
    {
        toggleBitset.set(static_cast<size_t>(TOGGLE::UPATEANDDRAWFOREACHMODEL));
    }
    if (availableToggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEDYNAMICBUFFEROFFSET)))
    {
        toggleBitset.set(static_cast<size_t>(TOGGLE::ENABLEDYNAMICBUFFEROFFSET));
    }
    toggleBitset.set(static_cast<size_t>(TOGGLE::ENABLEALPHABLENDING));

    int windowWidth  = 0;
    int windowHeight = 0;
    int MSAACount = 4;

    for (int i = 1; i < argc; ++i)
    {
        std::string cmd(argv[i]);
        if (cmd == "--fish-count")
        {
            mCurFishCount = strtol(argv[i++ + 1], &pNext, 10);
            if (mCurFishCount < 0)
            {
                std::cerr << "Fish count should larger or equal to 0." << std::endl;
                return false;
            }
        }
        else if (cmd == "--msaa-count")
        {
            MSAACount = strtol(argv[i++ + 1], &pNext, 10);
        }
        else if (cmd == "--enable-instanced-draws")
        {
            /*if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS)))
            {
                std::cerr << "Instanced draw path isn't implemented for the backend." << std::endl;
                return false;
            }
            toggleBitset.set(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS));
            // Disable map write aync for instanced draw mode
            toggleBitset.reset(static_cast<size_t>(TOGGLE::BUFFERMAPPINGASYNC));*/
            std::cerr << "Instanced draw path is deprecated." << std::endl;
            return false;
        }
        else if (cmd == "--disable-dynamic-buffer-offset")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEDYNAMICBUFFEROFFSET)))
            {
                std::cerr << "Dynamic buffer offset is only implemented for Dawn Vulkan, Dawn "
                             "Metal and D3D12 backend."
                          << std::endl;
                return false;
            }
            toggleBitset.set(static_cast<size_t>(TOGGLE::ENABLEDYNAMICBUFFEROFFSET), false);
        }
        else if (cmd == "--integrated-gpu")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::INTEGRATEDGPU)) &&
                !availableToggleBitset.test(static_cast<size_t>(TOGGLE::DISCRETEGPU)))
            {
                std::cerr << "Dynamically choose gpu isn't supported for the backend." << std::endl;
                return false;
            }

            if (toggleBitset.test(static_cast<size_t>(TOGGLE::DISCRETEGPU)))
            {
                std::cerr << "Integrated and Discrete gpu cannot be used simultaneosly.";
            }
            toggleBitset.set(static_cast<size_t>(TOGGLE::INTEGRATEDGPU));
        }
        else if (cmd == "--discrete-gpu")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::INTEGRATEDGPU)) &&
                !availableToggleBitset.test(static_cast<size_t>(TOGGLE::DISCRETEGPU)))
            {
                std::cerr << "Dynamically choose gpu isn't supported for the backend." << std::endl;
                return false;
            }

            if (toggleBitset.test(static_cast<size_t>(TOGGLE::INTEGRATEDGPU)))
            {
                std::cerr << "Integrated and Discrete gpu cannot be used simultaneosly.";
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::DISCRETEGPU));
        }
        else if (cmd == "--print-log")
        {
            toggleBitset.set(static_cast<size_t>(TOGGLE::PRINTLOG));
        }
        else if (cmd == "--buffer-mapping-async")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::BUFFERMAPPINGASYNC)))
            {
                std::cerr << "Buffer mapping async isn't supported for the backend." << std::endl;
                return false;
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::BUFFERMAPPINGASYNC));
        }
        else if (cmd == "--turn-off-vsync")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::TURNOFFVSYNC)))
            {
                std::cerr << "Turn off vsync isn't supported for the backend." << std::endl;
                return false;
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::TURNOFFVSYNC));
        }
        else if (cmd == "--test-time")
        {

            mTestTime = strtol(argv[i++ + 1], &pNext, 10);
            toggleBitset.set(static_cast<size_t>(TOGGLE::AUTOSTOP));
        }
        else if (cmd.find("--window-size") != std::string::npos)
        {
            size_t pos1  = cmd.find("=");
            size_t pos2  = cmd.find(",");
            windowWidth  = strtol(cmd.substr(pos1 + 1, pos2 - pos1).c_str(), &pNext, 10);
            windowHeight = strtol(cmd.substr(pos2 + 1).c_str(), &pNext, 10);
            if (windowWidth == 0 || windowHeight == 0)
            {
                std::cerr << "Please input window size with the format: "
                             "'--window-size=[width],[height].' ";
            }
        }
        else if (cmd == "--disable-d3d12-render-pass")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::DISABLED3D12RENDERPASS)))
            {
                std::cerr << "Render pass is only supported for dawn_d3d12 backend. This feature "
                             "is only supported on Intel gen 10 or more advanced platforms. "
                             "Windows 1809 or later version is also required."
                          << std::endl;
                return false;
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::DISABLED3D12RENDERPASS));
        }
        else if (cmd == "--disable-dawn-validation")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::DISABLEDAWNVALIDATION)))
            {
                std::cerr << "Disable validation for Dawn backend." << std::endl;
                return false;
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::DISABLEDAWNVALIDATION));
        }
        else if (cmd.find("--enable-alpha-blending") != std::string::npos)
        {
            size_t pos = cmd.find("=");
            if (pos != std::string::npos)
            {
                g.alpha = cmd.substr(pos + 1).c_str();
                if (g.alpha == "false")
                {
                    toggleBitset.reset(static_cast<size_t>(TOGGLE::ENABLEALPHABLENDING));
                }
            }
        }
        else if (cmd == "--simulating-fish-come-and-go")
        {
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::SIMULATINGFISHCOMEANDGO)))
            {
                std::cerr << "Simulating fish come and go is only implemented for Dawn backend."
                          << std::endl;
                return false;
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::SIMULATINGFISHCOMEANDGO));
        }
        else if (cmd == "--disable-control-panel")
        {
            toggleBitset.set(static_cast<size_t>(TOGGLE::DISABLECONTROLPANEL));
        }

        //else if (cmd == "--enable-full-screen-mode")
        //{
            if (!availableToggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEFULLSCREENMODE)))
            {
                std::cerr << "Full screen mode isn't supported for the backend." << std::endl;
                return false;
            }

            toggleBitset.set(static_cast<size_t>(TOGGLE::ENABLEFULLSCREENMODE));
        //}

        mContext->mMSAACount = MSAACount;
    }

    if (!mContext->initialize(mBackendType, toggleBitset, windowWidth, windowHeight))
    {
        return false;
    }

    calculateFishCount();

    //std::cout << "Init resources ..." << std::endl;
    getElapsedTime();

    const ResourceHelper *resourceHelper = mContext->getResourceHelper();
    std::vector<std::string> skyUrls;
    resourceHelper->getSkyBoxUrls(&skyUrls);
    mTextureMap["skybox"] = mContext->createTexture("skybox", skyUrls);

    // Init general buffer and binding groups for dawn backend.
    mContext->initGeneralResources(this);
    // Avoid resource allocation in the first render loop
    mPreFishCount = mCurFishCount;

    setupModelEnumMap();
    loadReource();
    mContext->Flush();

    //std::cout << "End loading.\nCost " << getElapsedTime() << "s totally." << std::endl;
    mContext->showWindow();

    resetFpsTime();

    return true;
}

void Aquarium::resetFpsTime()
{
#ifdef _WIN32
    g.start = GetTickCount64() / 1000.0;
#else
    g.start    = clock() / 1000000.0;
#endif
    g.then          = g.start;
}

void Aquarium::display()
{
    while (!mContext->ShouldQuit())
    {
        mContext->KeyBoardQuit();
        render();

        mContext->DoFlush(toggleBitset);

        if (toggleBitset.test(static_cast<size_t>(TOGGLE::AUTOSTOP)) &&
            (g.then - g.start) > mTestTime)
        {
            break;
        }
    }

    mContext->Terminate();

    int avg = mFpsTimer.variance();
    printf("[RESULT] RENDERPASS:%s,MSAA:%d,FPS:%d\n", mContext->mDisableD3D12RenderPass ? "False" : "True", mContext->mMSAACount, avg);
}

void Aquarium::loadReource()
{
    loadModels();
    loadPlacement();
    if (toggleBitset.test(static_cast<size_t>(TOGGLE::SIMULATINGFISHCOMEANDGO)))
    {
        loadFishScenario();
    }
}

void Aquarium::setupModelEnumMap()
{
    for (auto &info : g_sceneInfo)
    {
        mModelEnumMap[info.namestr] = info.name;
    }
}

// Load world matrices of models from json file.
void Aquarium::loadPlacement()
{
    const ResourceHelper *resourceHelper = mContext->getResourceHelper();
    std::string proppath                 = resourceHelper->getPropPlacementPath();
    std::ifstream PlacementStream(proppath, std::ios::in);
    rapidjson::IStreamWrapper isPlacement(PlacementStream);
    rapidjson::Document document;
    document.ParseStream(isPlacement);

    ASSERT(document.IsObject());

    ASSERT(document.HasMember("objects"));
    const rapidjson::Value &objects = document["objects"];
    ASSERT(objects.IsArray());

    for (rapidjson::SizeType i = 0; i < objects.Size(); ++i)
    {
        const rapidjson::Value &name        = objects[i]["name"];
        const rapidjson::Value &worldMatrix = objects[i]["worldMatrix"];
        ASSERT(worldMatrix.IsArray() && worldMatrix.Size() == 16);

        std::vector<float> matrix;
        for (rapidjson::SizeType j = 0; j < worldMatrix.Size(); ++j)
        {
            matrix.push_back(worldMatrix[j].GetFloat());
        }

        MODELNAME modelname = mModelEnumMap[name.GetString()];
        // MODELFIRST means the model is not found in the Map
        if (modelname != MODELNAME::MODELFIRST)
        {
            mAquariumModels[modelname]->worldmatrices.push_back(matrix);
        }
    }
}

void Aquarium::loadModels()
{
    bool enableInstanceddraw = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS));
    for (const auto &info : g_sceneInfo)
    {
        if ((enableInstanceddraw && info.type == MODELGROUP::FISH) ||
            ((!enableInstanceddraw) && info.type == MODELGROUP::FISHINSTANCEDDRAW))
        {
            continue;
        }
        loadModel(info);
    }
}

void Aquarium::loadFishScenario()
{
    const ResourceHelper *resourceHelper = mContext->getResourceHelper();
    std::string fishBehaviorPath         = resourceHelper->getFishBehaviorPath();

    std::ifstream FishStream(fishBehaviorPath, std::ios::in);
    rapidjson::IStreamWrapper is(FishStream);
    rapidjson::Document document;
    document.ParseStream(is);
    ASSERT(document.IsObject());
    const rapidjson::Value &behaviors = document["behaviors"];
    ASSERT(behaviors.IsArray());

    for (rapidjson::SizeType i = 0; i < behaviors.Size(); ++i)
    {
        int frame      = behaviors[i]["frame"].GetInt();
        std::string op = behaviors[i]["op"].GetString();
        int count      = behaviors[i]["count"].GetInt();

        Behavior *behave = new Behavior(frame, op, count);
        mFishBehavior.push(behave);
    }
}

// Load vertex and index buffers, textures and program for each model.
void Aquarium::loadModel(const G_sceneInfo &info)
{
    const ResourceHelper *resourceHelper = mContext->getResourceHelper();
    std::string imagePath                = resourceHelper->getImagePath();
    std::string programPath              = resourceHelper->getProgramPath();
    std::string modelPath                = resourceHelper->getModelPath(std::string(info.namestr));

    std::ifstream ModelStream(modelPath, std::ios::in);
    rapidjson::IStreamWrapper is(ModelStream);
    rapidjson::Document document;
    document.ParseStream(is);
    ASSERT(document.IsObject());
    const rapidjson::Value &models = document["models"];
    ASSERT(models.IsArray());

    Model *model;
    if (toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEALPHABLENDING)) &&
        info.type != MODELGROUP::INNER && info.type != MODELGROUP::OUTSIDE)
    {
        model = mContext->createModel(this, info.type, info.name, true);
    }
    else
    {
        model = mContext->createModel(this, info.type, info.name, info.blend);
    }
    mAquariumModels[info.name] = model;

    auto &value = models.GetArray()[models.GetArray().Size() - 1];
    {
        // set up textures
        const rapidjson::Value &textures = value["textures"];
        for (rapidjson::Value::ConstMemberIterator itr = textures.MemberBegin();
             itr != textures.MemberEnd(); ++itr)
        {
            std::string name  = itr->name.GetString();
            std::string image = itr->value.GetString();

            if (mTextureMap.find(image) == mTextureMap.end())
            {
                mTextureMap[image] = mContext->createTexture(name, imagePath + image);
            }

            model->textureMap[name] = mTextureMap[image];
        }

        // set up vertices
        const rapidjson::Value &arrays = value["fields"];
        for (rapidjson::Value::ConstMemberIterator itr = arrays.MemberBegin();
             itr != arrays.MemberEnd(); ++itr)
        {
            std::string name  = itr->name.GetString();
            int numComponents = itr->value["numComponents"].GetInt();
            std::string type  = itr->value["type"].GetString();
            Buffer *buffer;
            if (name == "indices")
            {
                std::vector<unsigned short> vec;
                for (auto &data : itr->value["data"].GetArray())
                {
                    vec.push_back(data.GetInt());
                }
                buffer = mContext->createBuffer(numComponents, &vec, true);
            }
            else
            {
                std::vector<float> vec;
                for (auto &data : itr->value["data"].GetArray())
                {
                    vec.push_back(data.GetFloat());
                }
                buffer = mContext->createBuffer(numComponents, &vec, false);
            }

            model->bufferMap[name] = buffer;
        }

        // setup program
        // There are 3 programs
        // DM
        // DM+NM
        // DM+NM+RM
        std::string vsId;
        std::string fsId;

        vsId = info.program[0];
        fsId = info.program[1];

        if (vsId != "" && fsId != "")
        {
            model->textureMap["skybox"] = mTextureMap["skybox"];
        }
        else if (model->textureMap["reflection"] != nullptr)
        {
            vsId = "reflectionMapVertexShader";
            fsId = "reflectionMapFragmentShader";

            model->textureMap["skybox"] = mTextureMap["skybox"];
        }
        else if (model->textureMap["normalMap"] != nullptr)
        {
            vsId = "normalMapVertexShader";
            fsId = "normalMapFragmentShader";
        }
        else
        {
            vsId = "diffuseVertexShader";
            fsId = "diffuseFragmentShader";
        }

        Program *program;
        if (mProgramMap.find(vsId + fsId) != mProgramMap.end())
        {
            program = mProgramMap[vsId + fsId];
        }
        else
        {
            program = mContext->createProgram(programPath + vsId, programPath + fsId);
            if (toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEALPHABLENDING)) &&
                info.type != MODELGROUP::INNER && info.type != MODELGROUP::OUTSIDE)
            {
                program->compileProgram(true, g.alpha);
            }
            else
            {
                program->compileProgram(false, g.alpha);
            }
            mProgramMap[vsId + fsId] = program;
        }

        model->setProgram(program);
        model->init();
    }
}

void Aquarium::calculateFishCount()
{
    // Calculate fish count for each type of fish
    int numLeft = mCurFishCount;
    for (int i = 0; i < FISHENUM::MAX; ++i)
    {
        for (auto &fishInfo : fishTable)
        {
            if (fishInfo.type != i)
            {
                continue;
            }
            int numfloat = numLeft;
            if (i == FISHENUM::BIG)
            {
                int temp = mCurFishCount < g_smallFishCount ? 1 : 2;
                numfloat = std::min(numLeft, temp);
            }
            else if (i == FISHENUM::MEDIUM)
            {
                if (mCurFishCount < g_mediumFishCount)
                {
                    numfloat = std::min(numLeft, mCurFishCount / 10);
                }
                else if (mCurFishCount < g_bigFishCount)
                {
                    numfloat = std::min(numLeft, g_leftSmallFishCount);
                }
                else
                {
                    numfloat = std::min(numLeft, g_leftBigFishCount);
                }
            }
            numLeft                                                    = numLeft - numfloat;
            fishCounts[fishInfo.modelName - MODELNAME::MODELSMALLFISHA] = numfloat;
        }
    }
}

double Aquarium::getElapsedTime()
{
    // Update our time
#ifdef _WIN32
    double now = GetTickCount64() / 1000.0;
#else
    double now = clock() / 1000000.0;
#endif
    double elapsedTime = 0.0;
    if (g.then == 0.0)
    {
        elapsedTime = 0.0;
    }
    else
    {
        elapsedTime = now - g.then;
    }
    g.then = now;

    return elapsedTime;
}

void Aquarium::printAvgFps()
{
    int avg = mFpsTimer.variance();

    std::cout << "result: {\"FPS\":" << avg << "}" << std::endl;
    if (avg == 0)
    {
        std::cout << "Invalid value. The fps is unstable." << std::endl;
    }
}

void Aquarium::updateGlobalUniforms()
{
    double elapsedTime   = getElapsedTime();
    double renderingTime = g.then - g.start;

    mFpsTimer.update(elapsedTime, renderingTime, mTestTime);
    g.mclock += elapsedTime * g_speed;
    g.eyeClock += elapsedTime * g_eyeSpeed;

    g.eyePosition[0] = sin(g.eyeClock) * g_eyeRadius;
    g.eyePosition[1] = g_eyeHeight;
    g.eyePosition[2] = cos(g.eyeClock) * g_eyeRadius;
    g.target[0]      = static_cast<float>(sin(g.eyeClock + M_PI)) * g_targetRadius;
    g.target[1]      = g_targetHeight;
    g.target[2]      = static_cast<float>(cos(g.eyeClock + M_PI)) * g_targetRadius;

    float nearPlane = 1;
    float farPlane  = 25000.0f;
    float aspect    = static_cast<float>(mContext->getClientWidth()) /
                   static_cast<float>(mContext->getclientHeight());
    float top    = tan(matrix::degToRad(g_fieldOfView * g_fovFudge) * 0.5f) * nearPlane;
    float bottom = -top;
    float left   = aspect * bottom;
    float right  = aspect * top;
    float width  = abs(right - left);
    float height = abs(top - bottom);
    float xOff   = width * g_net_offset[0] * g_net_offsetMult;
    float yOff   = height * g_net_offset[1] * g_net_offsetMult;

    // set frustm and camera look at
    matrix::frustum(g.projection, left + xOff, right + xOff, bottom + yOff, top + yOff, nearPlane,
                    farPlane);
    matrix::cameraLookAt(lightWorldPositionUniform.viewInverse, g.eyePosition, g.target, g.up);
    matrix::inverse4(g.view, lightWorldPositionUniform.viewInverse);
    matrix::mulMatrixMatrix4(lightWorldPositionUniform.viewProjection, g.view, g.projection);
    matrix::inverse4(g.viewProjectionInverse, lightWorldPositionUniform.viewProjection);

    memcpy(g.skyView, g.view, 16 * sizeof(float));
    g.skyView[12] = 0.0;
    g.skyView[13] = 0.0;
    g.skyView[14] = 0.0;
    matrix::mulMatrixMatrix4(g.skyViewProjection, g.skyView, g.projection);
    matrix::inverse4(g.skyViewProjectionInverse, g.skyViewProjection);

    matrix::getAxis(g.v3t0, lightWorldPositionUniform.viewInverse, 0);
    matrix::getAxis(g.v3t1, lightWorldPositionUniform.viewInverse, 1);
    matrix::mulScalarVector(20.0f, g.v3t0, 3);
    matrix::mulScalarVector(30.0f, g.v3t1, 3);
    matrix::addVector(lightWorldPositionUniform.lightWorldPos, g.eyePosition, g.v3t0, 3);
    matrix::addVector(lightWorldPositionUniform.lightWorldPos,
                      lightWorldPositionUniform.lightWorldPos, g.v3t1, 3);

    // update world uniforms for dawn backend
    mContext->updateWorldlUniforms(this);
}

void Aquarium::render()
{
    matrix::resetPseudoRandom();

    mContext->preFrame();

    // Global Uniforms should update after command reallocation.
    updateGlobalUniforms();

    if (toggleBitset.test(static_cast<size_t>(TOGGLE::SIMULATINGFISHCOMEANDGO)))
    {
        if (!mFishBehavior.empty())
        {
            Behavior *behave = mFishBehavior.front();
            int frame        = behave->getFrame();
            if (frame == 0)
            {
                mFishBehavior.pop();
                if (behave->getOp() == "+")
                {
                    mCurFishCount += behave->getCount();
                }
                else
                {
                    mCurFishCount -= behave->getCount();
                }
                std::cout << "Fish count" << mCurFishCount << std::endl;
            }
            else
            {
                behave->setFrame(--frame);
            }
        }
    }

    // TODO(yizhou): Functionality of reallocate fish count during rendering
    // isn't supported for instanced draw.
    // To try this functionality now, use composition of "--backend dawn_xxx", or
    // "--backend dawn_xxx --disable-dyanmic-buffer-offset"
    if (!toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS)))
        if (mCurFishCount != mPreFishCount)
        {
            calculateFishCount();
            bool enableDynamicBufferOffset =
                toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEDYNAMICBUFFEROFFSET));
            mContext->reallocResource(mPreFishCount, mCurFishCount, enableDynamicBufferOffset);
            mPreFishCount = mCurFishCount;

            resetFpsTime();
        }

    bool updateAndDrawForEachFish =
        toggleBitset.test(static_cast<size_t>(TOGGLE::UPATEANDDRAWFOREACHMODEL));

    if (updateAndDrawForEachFish)
    {
        updateAndDrawBackground();
        updateAndDrawFishes();
        mContext->updateFPS(mFpsTimer, &mCurFishCount, &toggleBitset);
    }
    else
    {
        updateBackground();
        updateFishes();
        mContext->updateFPS(mFpsTimer, &mCurFishCount, &toggleBitset);

        // Begin render pass
        mContext->beginRenderPass();

        drawBackground();
        drawFishes();
        mContext->showFPS();

        // End renderpass
    }
}

void Aquarium::updateAndDrawBackground()
{
    Model *model = mAquariumModels[MODELNAME::MODELRUINCOlOMN];
    for (int i = MODELNAME::MODELRUINCOlOMN; i <= MODELNAME::MODELSEAWEEDB; ++i)
    {
        model = mAquariumModels[i];
        updateWorldMatrixAndDraw(model);
    }
}

void Aquarium::drawBackground()
{
    Model *model = mAquariumModels[MODELNAME::MODELRUINCOlOMN];
    for (int i = MODELNAME::MODELRUINCOlOMN; i <= MODELNAME::MODELSEAWEEDB; ++i)
    {
        model = mAquariumModels[i];
        model->draw();
    }
}

void Aquarium::updateAndDrawFishes()
{
    int begin = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS))
                    ? MODELNAME::MODELSMALLFISHAINSTANCEDDRAWS
                    : MODELNAME::MODELSMALLFISHA;
    int end = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS))
                  ? MODELNAME::MODELBIGFISHBINSTANCEDDRAWS
                  : MODELNAME::MODELBIGFISHB;

    for (int i = begin; i <= end; ++i)
    {
        FishModel *model = static_cast<FishModel *>(mAquariumModels[i]);

        const Fish &fishInfo = fishTable[i - begin];
        int fishCount          = fishCounts[i - begin];

        model->prepareForDraw();

        float fishBaseClock   = g.mclock * g_fishSpeed;
        float fishRadius      = fishInfo.radius;
        float fishRadiusRange = fishInfo.radiusRange;
        float fishSpeed       = fishInfo.speed;
        float fishSpeedRange  = fishInfo.speedRange;
        float fishTailSpeed   = fishInfo.tailSpeed * g_fishTailSpeed;
        float fishOffset      = g_fishOffset;
        // float fishClockSpeed  = g_fishSpeed;
        float fishHeight      = g_fishHeight + fishInfo.heightOffset;
        float fishHeightRange = g_fishHeightRange * fishInfo.heightRange;
        float fishXClock      = g_fishXClock;
        float fishYClock      = g_fishYClock;
        float fishZClock      = g_fishZClock;

        for (int ii = 0; ii < fishCount; ++ii)
        {
            float fishClock = fishBaseClock + ii * fishOffset;
            float speed = fishSpeed + static_cast<float>(matrix::pseudoRandom()) * fishSpeedRange;
            float scale = 1.0f + static_cast<float>(matrix::pseudoRandom()) * 1;
            float xRadius =
                fishRadius + static_cast<float>(matrix::pseudoRandom()) * fishRadiusRange;
            float yRadius = 2.0f + static_cast<float>(matrix::pseudoRandom()) * fishHeightRange;
            float zRadius =
                fishRadius + static_cast<float>(matrix::pseudoRandom()) * fishRadiusRange;
            float fishSpeedClock = fishClock * speed;
            float xClock         = fishSpeedClock * fishXClock;
            float yClock         = fishSpeedClock * fishYClock;
            float zClock         = fishSpeedClock * fishZClock;

            model->updateFishPerUniforms(
                sin(xClock) * xRadius, sin(yClock) * yRadius + fishHeight, cos(zClock) * zRadius,
                sin(xClock - 0.04f) * xRadius, sin(yClock - 0.01f) * yRadius + fishHeight,
                cos(zClock - 0.04f) * zRadius, scale,
                fmod((g.mclock + ii * g_tailOffsetMult) * fishTailSpeed * speed,
                     static_cast<float>(M_PI) * 2),
                ii);

            model->updatePerInstanceUniforms(worldUniforms);
            model->draw();
        }
    }
}

void Aquarium::updateBackground()
{
    Model *model = mAquariumModels[MODELNAME::MODELRUINCOlOMN];
    for (int i = MODELNAME::MODELRUINCOlOMN; i <= MODELNAME::MODELSEAWEEDB; ++i)
    {
        model = mAquariumModels[i];
        updateWorldMatrix(model);
    }
}

void Aquarium::updateFishes()
{
    int begin = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS))
                    ? MODELNAME::MODELSMALLFISHAINSTANCEDDRAWS
                    : MODELNAME::MODELSMALLFISHA;
    int end = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS))
                  ? MODELNAME::MODELBIGFISHBINSTANCEDDRAWS
                  : MODELNAME::MODELBIGFISHB;

    for (int i = begin; i <= end; ++i)
    {
        FishModel *model = static_cast<FishModel *>(mAquariumModels[i]);

        const Fish &fishInfo = fishTable[i - begin];
        int fishCount          = fishCounts[i - begin];

        model->prepareForDraw();

        float fishBaseClock   = g.mclock * g_fishSpeed;
        float fishRadius      = fishInfo.radius;
        float fishRadiusRange = fishInfo.radiusRange;
        float fishSpeed       = fishInfo.speed;
        float fishSpeedRange  = fishInfo.speedRange;
        float fishTailSpeed   = fishInfo.tailSpeed * g_fishTailSpeed;
        float fishOffset      = g_fishOffset;
        // float fishClockSpeed  = g_fishSpeed;
        float fishHeight      = g_fishHeight + fishInfo.heightOffset;
        float fishHeightRange = g_fishHeightRange * fishInfo.heightRange;
        float fishXClock      = g_fishXClock;
        float fishYClock      = g_fishYClock;
        float fishZClock      = g_fishZClock;

        for (int ii = 0; ii < fishCount; ++ii)
        {
            float fishClock = fishBaseClock + ii * fishOffset;
            float speed = fishSpeed + static_cast<float>(matrix::pseudoRandom()) * fishSpeedRange;
            float scale = 1.0f + static_cast<float>(matrix::pseudoRandom()) * 1;
            float xRadius =
                fishRadius + static_cast<float>(matrix::pseudoRandom()) * fishRadiusRange;
            float yRadius = 2.0f + static_cast<float>(matrix::pseudoRandom()) * fishHeightRange;
            float zRadius =
                fishRadius + static_cast<float>(matrix::pseudoRandom()) * fishRadiusRange;
            float fishSpeedClock = fishClock * speed;
            float xClock         = fishSpeedClock * fishXClock;
            float yClock         = fishSpeedClock * fishYClock;
            float zClock         = fishSpeedClock * fishZClock;

            model->updateFishPerUniforms(
                sin(xClock) * xRadius, sin(yClock) * yRadius + fishHeight, cos(zClock) * zRadius,
                sin(xClock - 0.04f) * xRadius, sin(yClock - 0.01f) * yRadius + fishHeight,
                cos(zClock - 0.04f) * zRadius, scale,
                fmod((g.mclock + ii * g_tailOffsetMult) * fishTailSpeed * speed,
                     static_cast<float>(M_PI) * 2),
                ii);
        }
    }

    mContext->updateAllFishData();
}

void Aquarium::drawFishes()
{
    int begin = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS))
                    ? MODELNAME::MODELSMALLFISHAINSTANCEDDRAWS
                    : MODELNAME::MODELSMALLFISHA;
    int end = toggleBitset.test(static_cast<size_t>(TOGGLE::ENABLEINSTANCEDDRAWS))
                  ? MODELNAME::MODELBIGFISHBINSTANCEDDRAWS
                  : MODELNAME::MODELBIGFISHB;

    for (int i = begin; i <= end; ++i)
    {
        FishModel *model = static_cast<FishModel *>(mAquariumModels[i]);
        model->draw();
    }
}

void Aquarium::updateWorldProjections(const std::vector<float> &w)
{
    ASSERT(w.size() == 16);
    memcpy(worldUniforms.world, w.data(), 16 * sizeof(float));
    matrix::mulMatrixMatrix4(worldUniforms.worldViewProjection, worldUniforms.world,
                             lightWorldPositionUniform.viewProjection);
    matrix::inverse4(g.worldInverse, worldUniforms.world);
    matrix::transpose4(worldUniforms.worldInverseTranspose, g.worldInverse);
}

void Aquarium::updateWorldMatrixAndDraw(Model *model)
{
    if (model->worldmatrices.size())
    {
        for (auto &world : model->worldmatrices)
        {
            updateWorldProjections(world);
            model->prepareForDraw();
            model->updatePerInstanceUniforms(worldUniforms);
            model->draw();
        }
    }
}

void Aquarium::updateWorldMatrix(Model *model)
{
    if (model->worldmatrices.size())
    {
        for (auto &world : model->worldmatrices)
        {
            updateWorldProjections(world);
            model->updatePerInstanceUniforms(worldUniforms);
        }
    }

    model->prepareForDraw();
}
