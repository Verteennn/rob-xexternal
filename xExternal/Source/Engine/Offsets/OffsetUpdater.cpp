#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include "OffsetUpdater.h"
#include "../../Engine/Offsets/Offsets.h"

#include <string>
#include <sstream>
#include <unordered_map>
#include <cstdio>
#include <cstring>

//  remote source
static const wchar_t* REMOTE_HOST = L"offsets.ntgetwritewatch.workers.dev";
static const wchar_t* REMOTE_PATH = L"/offsets.hpp";

//  mapping table: remote name -> pointer to local variable
static std::unordered_map<std::string, uintptr_t*> BuildMappingTable()
{
    std::unordered_map<std::string, uintptr_t*> mapping;
    
    // FakeDataModel
    mapping["FakeDataModelPointer"] = &Offsets::FakeDataModel::Pointer;
    mapping["FakeDataModelToDataModel"] = &Offsets::FakeDataModel::RealDataModel;

    // VisualEngine
    mapping["VisualEnginePointer"] = &Offsets::VisualEngine::Pointer;
    mapping["Dimensions"] = &Offsets::VisualEngine::Dimensions;
    mapping["VisualEngineToDataModel1"] = &Offsets::VisualEngine::FakeDataModel;
    mapping["viewmatrix"] = &Offsets::VisualEngine::ViewMatrix;

    // TaskScheduler
    mapping["TaskSchedulerPointer"] = &Offsets::TaskScheduler::Pointer;
    mapping["TaskSchedulerMaxFPS"] = &Offsets::TaskScheduler::MaxFPS;
    mapping["JobStart"] = &Offsets::TaskScheduler::JobStart;
    mapping["JobEnd"] = &Offsets::TaskScheduler::JobEnd;
    mapping["Job_Name"] = &Offsets::TaskScheduler::JobName;

    // MouseService
    mapping["MouseSensitivity"] = &Offsets::MouseService::SensitivityPointer;
    mapping["MousePosition"] = &Offsets::MouseService::MousePosition;
    mapping["InputObject"] = &Offsets::MouseService::InputObject;

    // PlayerConfigurer
    mapping["PlayerConfigurerPointer"] = &Offsets::PlayerConfigurer::Pointer;

    // Instance
    mapping["Children"] = &Offsets::Instance::ChildrenStart;
    mapping["ChildrenEnd"] = &Offsets::Instance::ChildrenEnd;
    mapping["Name"] = &Offsets::Instance::Name;
    mapping["Parent"] = &Offsets::Instance::Parent;
    mapping["ClassDescriptor"] = &Offsets::Instance::ClassDescriptor;
    mapping["ClassDescriptorToClassName"] = &Offsets::Instance::ClassName;
    mapping["InstanceAttributePointer1"] = &Offsets::Instance::AttributeContainer;
    mapping["InstanceAttributePointer2"] = &Offsets::Instance::AttributeList;
    mapping["AttributeToNext"] = &Offsets::Instance::AttributeToNext;
    mapping["AttributeToValue"] = &Offsets::Instance::AttributeToValue;
    mapping["InstanceCapabilities"] = &Offsets::Instance::Capabilities;
    mapping["OnDemandInstance"] = &Offsets::Instance::OnDemand;

    // DataModel
    mapping["CreatorId"] = &Offsets::DataModel::CreatorId;
    mapping["GameId"] = &Offsets::DataModel::GameId;
    mapping["PlaceId"] = &Offsets::DataModel::PlaceId;
    mapping["JobId"] = &Offsets::DataModel::JobId;
    mapping["Workspace"] = &Offsets::DataModel::Workspace;
    mapping["GameLoaded"] = &Offsets::DataModel::GameLoaded;
    mapping["ScriptContext"] = &Offsets::DataModel::ScriptContext;
    mapping["DataModelPrimitiveCount"] = &Offsets::DataModel::PrimitiveCount;
    mapping["DataModelDeleterPointer"] = &Offsets::DataModel::DeleterPointer;
    mapping["DataModelToRenderView1"] = &Offsets::DataModel::ToRenderView1;
    mapping["DataModelToRenderView2"] = &Offsets::DataModel::ToRenderView2;
    mapping["DataModelToRenderView3"] = &Offsets::DataModel::ToRenderView3;

    // FFlag
    mapping["FFlagList"] = &Offsets::FFlag::List;
    mapping["FFlagToValueGetSet"] = &Offsets::FFlag::ToValueGetSet;

    // ValueGetSet
    mapping["ValueGetSetToValue"] = &Offsets::ValueGetSet::ToValue;

    // Camera
    mapping["Camera"] = &Offsets::Workspace::CurrentCamera;
    mapping["CameraPos"] = &Offsets::Camera::Position;
    mapping["CameraRotation"] = &Offsets::Camera::Rotation;
    mapping["CameraSubject"] = &Offsets::Camera::CameraSubject;
    mapping["CameraType"] = &Offsets::Camera::CameraType;
    mapping["FOV"] = &Offsets::Camera::FieldOfView;
    mapping["ViewportSize"] = &Offsets::Camera::ViewportSize;

    // Humanoid
    mapping["Health"] = &Offsets::Humanoid::Health;
    mapping["MaxHealth"] = &Offsets::Humanoid::MaxHealth;
    mapping["WalkSpeed"] = &Offsets::Humanoid::Walkspeed;
    mapping["WalkSpeedCheck"] = &Offsets::Humanoid::WalkspeedCheck;
    mapping["JumpPower"] = &Offsets::Humanoid::JumpPower;
    mapping["HipHeight"] = &Offsets::Humanoid::HipHeight;
    mapping["RigType"] = &Offsets::Humanoid::RigType;
    mapping["Sit"] = &Offsets::Humanoid::Sit;
    mapping["MoveDirection"] = &Offsets::Humanoid::MoveDirection;
    mapping["HumanoidState"] = &Offsets::Humanoid::HumanoidState;
    mapping["HumanoidStateId"] = &Offsets::Humanoid::HumanoidStateID;
    mapping["HumanoidDisplayName"] = &Offsets::Humanoid::DisplayName;
    mapping["AutoJumpEnabled"] = &Offsets::Humanoid::AutoJumpEnabled;
    mapping["EvaluateStateMachine"] = &Offsets::Humanoid::EvaluateStateMachine;
    mapping["HealthDisplayDistance"] = &Offsets::Humanoid::HealthDisplayDistance;
    mapping["NameDisplayDistance"] = &Offsets::Humanoid::NameDisplayDistance;
    mapping["RootPartR15"] = &Offsets::Humanoid::HumanoidRootPart;
    mapping["RootPartR6"] = &Offsets::Humanoid::RootPartR6;

    // Primitive / BasePart
    mapping["Position"] = &Offsets::Primitive::Position;
    mapping["CFrame"] = &Offsets::Primitive::Rotation;
    mapping["Velocity"] = &Offsets::Primitive::AssemblyLinearVelocity;
    mapping["PartSize"] = &Offsets::Primitive::Size;
    mapping["Primitive"] = &Offsets::BasePart::Primitive;
    mapping["Transparency"] = &Offsets::BasePart::Transparency;
    mapping["Anchored"] = &Offsets::Primitive::Flags;
    mapping["MaterialType"] = &Offsets::Primitive::Material;

    // Player
    mapping["LocalPlayer"] = &Offsets::Player::LocalPlayer;
    mapping["ModelInstance"] = &Offsets::Player::ModelInstance;
    mapping["UserId"] = &Offsets::Player::UserId;
    mapping["Team"] = &Offsets::Player::Team;
    mapping["DisplayName"] = &Offsets::Player::DisplayName;
    mapping["PlayerMouse"] = &Offsets::Player::Mouse;
    mapping["CameraMode"] = &Offsets::Player::CameraMode;
    mapping["CameraMaxZoomDistance"] = &Offsets::Player::MaxZoomDistance;
    mapping["CameraMinZoomDistance"] = &Offsets::Player::MinZoomDistance;
    mapping["CharacterAppearanceId"] = &Offsets::Player::CharacterAppearanceId;

    // Workspace / World
    mapping["WorkspaceToWorld"] = &Offsets::Workspace::World;
    mapping["Gravity"] = &Offsets::World::Gravity;
    mapping["ReadOnlyGravity"] = &Offsets::Workspace::ReadOnlyGravity;

    // Lighting
    mapping["OutdoorAmbient"] = &Offsets::Lighting::OutdoorAmbient;
    mapping["FogColor"] = &Offsets::Lighting::FogColor;
    mapping["FogEnd"] = &Offsets::Lighting::FogEnd;
    mapping["FogStart"] = &Offsets::Lighting::FogStart;
    mapping["ClockTime"] = &Offsets::Lighting::ClockTime;

    // Silent / GUI
    mapping["FramePositionOffsetX"] = &Offsets::Silent::FramePositionOffsetX;
    mapping["FramePositionOffsetY"] = &Offsets::Silent::FramePositionOffsetY;
    mapping["FramePositionX"] = &Offsets::Silent::FramePositionX;
    mapping["FramePositionY"] = &Offsets::Silent::FramePositionY;
    mapping["FrameRotation"] = &Offsets::Silent::FrameRotation;
    mapping["FrameSizeOffsetX"] = &Offsets::Silent::FrameSizeOffsetX;
    mapping["FrameSizeOffsetY"] = &Offsets::Silent::FrameSizeOffsetY;
    mapping["FrameSizeX"] = &Offsets::Silent::FrameSizeX;
    mapping["FrameSizeY"] = &Offsets::Silent::FrameSizeY;

    // Misc
    mapping["AnimationId"] = &Offsets::Misc::AnimationId;
    mapping["StringLength"] = &Offsets::Misc::StringLength;
    mapping["Value"] = &Offsets::Misc::Value;
    mapping["Ping"] = &Offsets::Misc::Ping;
    mapping["InsetMaxX"] = &Offsets::Misc::InsetMaxX;
    mapping["InsetMaxY"] = &Offsets::Misc::InsetMaxY;
    mapping["InsetMinX"] = &Offsets::Misc::InsetMinX;
    mapping["InsetMinY"] = &Offsets::Misc::InsetMinY;

    // Sound
    mapping["SoundId"] = &Offsets::Sound::SoundId;

    // Sky
    mapping["SkyboxBk"] = &Offsets::Sky::SkyboxBk;
    mapping["SkyboxDn"] = &Offsets::Sky::SkyboxDn;
    mapping["SkyboxFt"] = &Offsets::Sky::SkyboxFt;
    mapping["SkyboxLf"] = &Offsets::Sky::SkyboxLf;
    mapping["SkyboxRt"] = &Offsets::Sky::SkyboxRt;
    mapping["SkyboxUp"] = &Offsets::Sky::SkyboxUp;
    mapping["StarCount"] = &Offsets::Sky::StarCount;
    mapping["MoonTextureId"] = &Offsets::Sky::MoonTextureId;
    mapping["SunTextureId"] = &Offsets::Sky::SunTextureId;

    // Beam
    mapping["BeamBrightness"] = &Offsets::Beam::Brightness;
    mapping["BeamColor"] = &Offsets::Beam::Color;
    mapping["BeamLightEmission"] = &Offsets::Beam::LightEmission;
    mapping["BeamLightInfuence"] = &Offsets::Beam::LightInfluence;

    // Script
    mapping["BanningEnabled"] = &Offsets::Script::BanningEnabled;
    mapping["RunContext"] = &Offsets::Script::RunContext;
    mapping["Sandboxed"] = &Offsets::Script::Sandboxed;

    // Scripts (bytecode)
    mapping["LocalScriptByteCode"] = &Offsets::LocalScript::ByteCode;
    mapping["LocalScriptHash"] = &Offsets::LocalScript::Hash;
    mapping["ModuleScriptByteCode"] = &Offsets::ModuleScript::ByteCode;
    mapping["ModuleScriptHash"] = &Offsets::ModuleScript::Hash;
    
    return mapping;
}

//  winhttp fetch, returns raw text body or empty on failure
static std::string FetchURL()
{
    std::string result;

    HINTERNET hSession = WinHttpOpen(
        L"xExternal/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) return {};

    HINTERNET hConnect = WinHttpConnect(
        hSession, REMOTE_HOST,
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", REMOTE_PATH,
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    if (WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, nullptr))
    {
        DWORD bytesAvail = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvail) && bytesAvail > 0)
        {
            std::string chunk(bytesAvail, '\0');
            DWORD bytesRead = 0;
            WinHttpReadData(hRequest, &chunk[0], bytesAvail, &bytesRead);
            chunk.resize(bytesRead);
            result += chunk;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

static bool ParseRemote(const std::string& src,
    std::string& outVersion,
    std::unordered_map<std::string, uintptr_t>& outValues)
{
    std::istringstream ss(src);
    std::string line;
    outVersion.clear();
    outValues.clear();

    while (std::getline(ss, line))
    {
        if (outVersion.empty())
        {
            const std::string prefix = "// Roblox Version: ";
            size_t pos = line.find(prefix);
            if (pos != std::string::npos)
            {
                outVersion = line.substr(pos + prefix.size());
                while (!outVersion.empty() &&
                    (outVersion.back() == '\r' ||
                        outVersion.back() == '\n' ||
                        outVersion.back() == ' '))
                    outVersion.pop_back();
                continue;
            }
        }

        const std::string kw = "uintptr_t ";
        size_t kwPos = line.find(kw);
        if (kwPos == std::string::npos) continue;

        size_t nameStart = kwPos + kw.size();
        size_t eqPos = line.find('=', nameStart);
        if (eqPos == std::string::npos) continue;

        std::string name = line.substr(nameStart, eqPos - nameStart);
        while (!name.empty() && (name.back() == ' ' || name.back() == '\t'))
            name.pop_back();

        size_t valStart = eqPos + 1;
        while (valStart < line.size() && (line[valStart] == ' ' || line[valStart] == '\t'))
            ++valStart;

        std::string valStr = line.substr(valStart);
        size_t semi = valStr.find(';');
        if (semi != std::string::npos) valStr = valStr.substr(0, semi);
        while (!valStr.empty() && (valStr.back() == ' ' || valStr.back() == '\t'))
            valStr.pop_back();

        if (valStr.empty()) continue;

        uintptr_t val = (uintptr_t)std::stoull(valStr, nullptr, 0);
        outValues[name] = val;
    }

    return !outVersion.empty() && !outValues.empty();
}

//  public entry point
namespace OffsetUpdater
{
    bool Initialize(bool keep_console_open)
    {
        AllocConsole();
        FILE* dummy = nullptr;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        SetConsoleTitleA("xexternal.de");

        printf("offsets updater // xexternal.de\n");
        printf("fetching remote offsets...\n");

        // 1. fetch remote
        std::string remote = FetchURL();
        if (remote.empty())
        {
            printf("fetch failed, using compiled offsets (%s)\n\n",
                Offsets::ClientVersion.c_str());
            if (keep_console_open) { printf("press ENTER to continue...\n"); getchar(); }
            else { Sleep(2500); }
            ShowWindow(GetConsoleWindow(), SW_HIDE); FreeConsole();
            return true;
        }
        printf("fetched %zu bytes\n\n", remote.size());

        // 2. parse
        std::string remoteVersion;
        std::unordered_map<std::string, uintptr_t> remoteValues;
        if (!ParseRemote(remote, remoteVersion, remoteValues))
        {
            printf("failed to parse remote file, using compiled offsets\n\n");
            if (keep_console_open) { printf("press ENTER to continue...\n"); getchar(); }
            else { Sleep(2500); }
            ShowWindow(GetConsoleWindow(), SW_HIDE); FreeConsole();
            return true;
        }

        printf("  remote version  : %s\n", remoteVersion.c_str());
        printf("  compiled version: %s\n\n", Offsets::ClientVersion.c_str());

        // build mapping and apply
        auto mapping = BuildMappingTable();

        int updated  = 0;
        int unchanged = 0;
        int unknown  = 0;

        printf("  %-45s %-12s %-12s\n", "Offset", "Old", "New");
        printf("  %s\n", std::string(72, '-').c_str());

        for (const auto& rv : remoteValues)
        {
            auto it = mapping.find(rv.first);
            if (it == mapping.end())
            {
                ++unknown;
                continue;
            }

            uintptr_t oldVal = *it->second;
            uintptr_t newVal = rv.second;

            if (oldVal != newVal)
            {
                printf("  %-45s 0x%-10llX 0x%-10llX  <-- UPDATED\n",
                    rv.first.c_str(),
                    (unsigned long long)oldVal,
                    (unsigned long long)newVal);
                *it->second = newVal;
                ++updated;
            }
            else
            {
                ++unchanged;
            }
        }

        // 4. update version string in memory
        Offsets::ClientVersion = remoteVersion;

        printf("\n");
        printf("done // %d updated, %d unchanged, %d remote-only (no mapping)\n\n",
            updated, unchanged, unknown);

        if (keep_console_open)
        {
            printf("press ENTER to continue...\n");
            getchar();
            ShowWindow(GetConsoleWindow(), SW_HIDE);
            FreeConsole();
        }
        else
        {
            printf("continuing...\n");
            Sleep(3000);
            ShowWindow(GetConsoleWindow(), SW_HIDE); FreeConsole();
        }

        return true;
    }
}