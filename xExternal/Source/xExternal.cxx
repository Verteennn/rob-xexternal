#include <iostream>
#include <thread>

#include "Driver/Driver.h"
#include "Globals.hxx"
#include "Miscellaneous/Output/Output.h"
#include "Core/Graphics/Graphics.h"
#include "Engine/Engine.h"
#include "Core/Features/Cache/Cache.h"
#include "Core/Features/Cheats/Misc/Misc.h"
#include "Core/Features/Cheats/World/World.h"
#include "Core/Features/Cheats/Aimbot/Aimbot.h"
#include "Core/Features/Cheats/Aimbot/Silent/Silent.h"
#include "Engine/Offsets/OffsetUpdater.h"

#include <ShlObj.h>
#pragma comment(lib, "Shell32.lib")

std::int32_t main(std::int32_t argc, char** argv[])
{
    constexpr bool DEBUG_OFFSETS = true;
    OffsetUpdater::Initialize(DEBUG_OFFSETS);
    if (!DEBUG_OFFSETS)
        FreeConsole();

    static constexpr const char* BINARY_NAME = { "RobloxPlayerBeta.exe" };

    Driver->Find_Process(BINARY_NAME);
    Driver->Attach_Process(BINARY_NAME);
    Driver->Find_Module(BINARY_NAME);

    auto FakeDataModel = Driver->Read<std::uint64_t>(Driver->Get_Module() + Offsets::FakeDataModel::Pointer);
    Globals::Datamodel.Address = Driver->Read<std::uint64_t>(FakeDataModel + Offsets::FakeDataModel::RealDataModel);
    Globals::VisualEngine.Address = Driver->Read<std::uint64_t>(Driver->Get_Module() + Offsets::VisualEngine::Pointer);
    Globals::Players.Address = Globals::Datamodel.Find_First_Child_Of_Class("Players").Address;
    Globals::Workspace.Address = Globals::Datamodel.Find_First_Child_Of_Class("Workspace").Address;
    Globals::Camera.Address = Globals::Workspace.Find_First_Child_Of_Class("Camera").Address;
    auto Lightin = Globals::Datamodel.Find_First_Child_Of_Class("Lighting");
    Globals::Lighting = SDK::Lighting(Lightin.Address);

    std::thread(Cache::RunService).detach();
    std::thread(World::RunService).detach();
    std::thread(Aimbot::RunService).detach();
    std::thread(Silent::RunService).detach();
    std::thread(Misc::RunService).detach();

    auto workspacetoworld = Driver->Read<uintptr_t>(Globals::Workspace.Address + Offsets::Workspace::World);
    Driver->Write<float>(workspacetoworld + 0x658, 200 * 4.f);

    Graphic->Create_Window();
    Graphic->Create_Device();
    Graphic->Create_Imgui();

    for (;;)
    {
        Graphic->Start_Render();
        Graphic->Render_Visuals();
        Graphic->Running ? Graphic->Render_Menu() : void();
        Graphic->End_Render();
    }

    return 0;
}