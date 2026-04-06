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
    return {
        // FakeDataModel
        { "FakeDataModelPointer",               &Offsets::FakeDataModel::Pointer               },
        { "FakeDataModelToDataModel",           &Offsets::FakeDataModel::RealDataModel         },

        // VisualEngine
        { "VisualEnginePointer",                &Offsets::VisualEngine::Pointer                },
        { "Dimensions",                         &Offsets::VisualEngine::Dimensions             },
        { "VisualEngineToDataModel1",           &Offsets::VisualEngine::FakeDataModel          },
        { "viewmatrix",                         &Offsets::VisualEngine::ViewMatrix             },

        // TaskScheduler
        { "TaskSchedulerPointer",               &Offsets::TaskScheduler::Pointer               },
        { "TaskSchedulerMaxFPS",                &Offsets::TaskScheduler::MaxFPS                },
        { "JobStart",                           &Offsets::TaskScheduler::JobStart              },
        { "JobEnd",                             &Offsets::TaskScheduler::JobEnd                },
        { "Job_Name",                           &Offsets::TaskScheduler::JobName               },

        // MouseService
        { "MouseSensitivity",                   &Offsets::MouseService::SensitivityPointer     },
        { "MousePosition",                      &Offsets::MouseService::MousePosition          },
        { "InputObject",                        &Offsets::MouseService::InputObject            },

        // PlayerConfigurer
        { "PlayerConfigurerPointer",            &Offsets::PlayerConfigurer::Pointer            },

        // Instance
        { "Children",                           &Offsets::Instance::ChildrenStart              },
        { "ChildrenEnd",                        &Offsets::Instance::ChildrenEnd                },
        { "Name",                               &Offsets::Instance::Name                       },
        { "Parent",                             &Offsets::Instance::Parent                     },
        { "ClassDescriptor",                    &Offsets::Instance::ClassDescriptor            },
        { "ClassDescriptorToClassName",         &Offsets::Instance::ClassName                  },
        { "InstanceAttributePointer1",          &Offsets::Instance::AttributeContainer         },
        { "InstanceAttributePointer2",          &Offsets::Instance::AttributeList              },
        { "AttributeToNext",                    &Offsets::Instance::AttributeToNext            },
        { "AttributeToValue",                   &Offsets::Instance::AttributeToValue           },
        { "InstanceCapabilities",               &Offsets::Instance::Capabilities               },
        { "OnDemandInstance",                   &Offsets::Instance::OnDemand                   },

        // DataModel
        { "CreatorId",                          &Offsets::DataModel::CreatorId                 },
        { "GameId",                             &Offsets::DataModel::GameId                    },
        { "PlaceId",                            &Offsets::DataModel::PlaceId                   },
        { "JobId",                              &Offsets::DataModel::JobId                     },
        { "Workspace",                          &Offsets::DataModel::Workspace                 },
        { "GameLoaded",                         &Offsets::DataModel::GameLoaded                },
        { "ScriptContext",                      &Offsets::DataModel::ScriptContext             },
        { "DataModelPrimitiveCount",            &Offsets::DataModel::PrimitiveCount            },
        { "DataModelDeleterPointer",            &Offsets::DataModel::DeleterPointer            },
        { "DataModelToRenderView1",             &Offsets::DataModel::ToRenderView1             },
        { "DataModelToRenderView2",             &Offsets::DataModel::ToRenderView2             },
        { "DataModelToRenderView3",             &Offsets::DataModel::ToRenderView3             },

        // FFlag
        { "FFlagList",                          &Offsets::FFlag::List                          },
        { "FFlagToValueGetSet",                 &Offsets::FFlag::ToValueGetSet                 },

        // ValueGetSet
        { "ValueGetSetToValue",                 &Offsets::ValueGetSet::ToValue                 },

        // Camera
        { "Camera",                             &Offsets::Workspace::CurrentCamera             },
        { "CameraPos",                          &Offsets::Camera::Position                     },
        { "CameraRotation",                     &Offsets::Camera::Rotation                     },
        { "CameraSubject",                      &Offsets::Camera::CameraSubject                },
        { "CameraType",                         &Offsets::Camera::CameraType                   },
        { "FOV",                                &Offsets::Camera::FieldOfView                  },
        { "ViewportSize",                       &Offsets::Camera::ViewportSize                 },

        // Humanoid
        { "Health",                             &Offsets::Humanoid::Health                     },
        { "MaxHealth",                          &Offsets::Humanoid::MaxHealth                  },
        { "WalkSpeed",                          &Offsets::Humanoid::Walkspeed                  },
        { "WalkSpeedCheck",                     &Offsets::Humanoid::WalkspeedCheck             },
        { "JumpPower",                          &Offsets::Humanoid::JumpPower                  },
        { "HipHeight",                          &Offsets::Humanoid::HipHeight                  },
        { "RigType",                            &Offsets::Humanoid::RigType                    },
        { "Sit",                                &Offsets::Humanoid::Sit                        },
        { "MoveDirection",                      &Offsets::Humanoid::MoveDirection              },
        { "HumanoidState",                      &Offsets::Humanoid::HumanoidState              },
        { "HumanoidStateId",                    &Offsets::Humanoid::HumanoidStateID            },
        { "HumanoidDisplayName",                &Offsets::Humanoid::DisplayName                },
        { "AutoJumpEnabled",                    &Offsets::Humanoid::AutoJumpEnabled            },
        { "EvaluateStateMachine",               &Offsets::Humanoid::EvaluateStateMachine       },
        { "HealthDisplayDistance",              &Offsets::Humanoid::HealthDisplayDistance       },
        { "NameDisplayDistance",                &Offsets::Humanoid::NameDisplayDistance         },
        { "RootPartR15",                        &Offsets::Humanoid::HumanoidRootPart           },
        { "RootPartR6",                         &Offsets::Humanoid::RootPartR6                 },

        // Primitive / BasePart
        { "Position",                           &Offsets::Primitive::Position                  },
        { "CFrame",                             &Offsets::Primitive::Rotation                  },
        { "Velocity",                           &Offsets::Primitive::AssemblyLinearVelocity    },
        { "PartSize",                           &Offsets::Primitive::Size                      },
        { "Primitive",                          &Offsets::BasePart::Primitive                  },
        { "Transparency",                       &Offsets::BasePart::Transparency               },
        { "Anchored",                           &Offsets::Primitive::Flags                     },
        { "MaterialType",                       &Offsets::Primitive::Material                  },

        // Player
        { "LocalPlayer",                        &Offsets::Player::LocalPlayer                  },
        { "ModelInstance",                      &Offsets::Player::ModelInstance                },
        { "UserId",                             &Offsets::Player::UserId                       },
        { "Team",                               &Offsets::Player::Team                         },
        { "DisplayName",                        &Offsets::Player::DisplayName                  },
        { "PlayerMouse",                        &Offsets::Player::Mouse                        },
        { "CameraMode",                         &Offsets::Player::CameraMode                   },
        { "CameraMaxZoomDistance",              &Offsets::Player::MaxZoomDistance              },
        { "CameraMinZoomDistance",              &Offsets::Player::MinZoomDistance              },
        { "CharacterAppearanceId",              &Offsets::Player::CharacterAppearanceId        },

        // Workspace / World
        { "WorkspaceToWorld",                   &Offsets::Workspace::World                     },
        { "Gravity",                            &Offsets::World::Gravity                       },
        { "ReadOnlyGravity",                    &Offsets::Workspace::ReadOnlyGravity           },

        // Lighting
        { "OutdoorAmbient",                     &Offsets::Lighting::OutdoorAmbient             },
        { "FogColor",                           &Offsets::Lighting::FogColor                   },
        { "FogEnd",                             &Offsets::Lighting::FogEnd                     },
        { "FogStart",                           &Offsets::Lighting::FogStart                   },
        { "ClockTime",                          &Offsets::Lighting::ClockTime                  },

        // Silent / GUI
        { "FramePositionOffsetX",               &Offsets::Silent::FramePositionOffsetX         },
        { "FramePositionOffsetY",               &Offsets::Silent::FramePositionOffsetY         },
        { "FramePositionX",                     &Offsets::Silent::FramePositionX               },
        { "FramePositionY",                     &Offsets::Silent::FramePositionY               },
        { "FrameRotation",                      &Offsets::Silent::FrameRotation                },
        { "FrameSizeOffsetX",                   &Offsets::Silent::FrameSizeOffsetX             },
        { "FrameSizeOffsetY",                   &Offsets::Silent::FrameSizeOffsetY             },
        { "FrameSizeX",                         &Offsets::Silent::FrameSizeX                   },
        { "FrameSizeY",                         &Offsets::Silent::FrameSizeY                   },

        // Misc
        { "AnimationId",                        &Offsets::Misc::AnimationId                    },
        { "StringLength",                       &Offsets::Misc::StringLength                   },
        { "Value",                              &Offsets::Misc::Value                          },
        { "Ping",                               &Offsets::Misc::Ping                           },
        { "InsetMaxX",                          &Offsets::Misc::InsetMaxX                      },
        { "InsetMaxY",                          &Offsets::Misc::InsetMaxY                      },
        { "InsetMinX",                          &Offsets::Misc::InsetMinX                      },
        { "InsetMinY",                          &Offsets::Misc::InsetMinY                      },

        // Sound
        { "SoundId",                            &Offsets::Sound::SoundId                       },

        // Sky
        { "SkyboxBk",                           &Offsets::Sky::SkyboxBk                        },
        { "SkyboxDn",                           &Offsets::Sky::SkyboxDn                        },
        { "SkyboxFt",                           &Offsets::Sky::SkyboxFt                        },
        { "SkyboxLf",                           &Offsets::Sky::SkyboxLf                        },
        { "SkyboxRt",                           &Offsets::Sky::SkyboxRt                        },
        { "SkyboxUp",                           &Offsets::Sky::SkyboxUp                        },
        { "StarCount",                          &Offsets::Sky::StarCount                       },
        { "MoonTextureId",                      &Offsets::Sky::MoonTextureId                   },
        { "SunTextureId",                       &Offsets::Sky::SunTextureId                    },

        // Beam
        { "BeamBrightness",                     &Offsets::Beam::Brightness                     },
        { "BeamColor",                          &Offsets::Beam::Color                          },
        { "BeamLightEmission",                  &Offsets::Beam::LightEmission                  },
        { "BeamLightInfuence",                  &Offsets::Beam::LightInfluence                 },

        // Script
        { "BanningEnabled",                     &Offsets::Script::BanningEnabled               },
        { "RunContext",                          &Offsets::Script::RunContext                   },
        { "Sandboxed",                          &Offsets::Script::Sandboxed                    },

        // Scripts (bytecode)
        { "LocalScriptByteCode",                &Offsets::LocalScript::ByteCode                },
        { "LocalScriptHash",                    &Offsets::LocalScript::Hash                    },
        { "ModuleScriptByteCode",               &Offsets::ModuleScript::ByteCode               },
        { "ModuleScriptHash",                   &Offsets::ModuleScript::Hash                   },
    };
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