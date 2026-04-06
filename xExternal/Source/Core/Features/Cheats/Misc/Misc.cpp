#include <Windows.h>
#include <thread>
#include <chrono>
#pragma comment(lib, "Winmm.lib")

#include "Engine/Engine.h"
#include "Globals.hxx"
#include "Core/Features/Cache/Cache.h"
#include "Engine/Offsets/Offsets.h"

namespace {
    SDK::Vector3 LookVector(const SDK::Matrix3& rot)
    {
        return { -rot.data[2], -rot.data[5], -rot.data[8] };
    }

    SDK::Vector3 RightVector(const SDK::Matrix3& rot)
    {
        return { rot.data[0], rot.data[3], rot.data[6] };
    }

    int MiscKeyToVK(ImGuiKey key)
    {
        if (key >= ImGuiKey_A && key <= ImGuiKey_Z)
            return 'A' + (key - ImGuiKey_A);
        if (key >= ImGuiKey_0 && key <= ImGuiKey_9)
            return '0' + (key - ImGuiKey_0);
        if (key >= ImGuiKey_F1 && key <= ImGuiKey_F12)
            return VK_F1 + (key - ImGuiKey_F1);

        switch (key)
        {
        case ImGuiKey_Tab:          return VK_TAB;
        case ImGuiKey_Space:        return VK_SPACE;
        case ImGuiKey_Enter:        return VK_RETURN;
        case ImGuiKey_Escape:       return VK_ESCAPE;
        case ImGuiKey_Backspace:    return VK_BACK;
        case ImGuiKey_Insert:       return VK_INSERT;
        case ImGuiKey_Delete:       return VK_DELETE;
        case ImGuiKey_LeftArrow:    return VK_LEFT;
        case ImGuiKey_RightArrow:   return VK_RIGHT;
        case ImGuiKey_UpArrow:      return VK_UP;
        case ImGuiKey_DownArrow:    return VK_DOWN;
        case ImGuiKey_PageUp:       return VK_PRIOR;
        case ImGuiKey_PageDown:     return VK_NEXT;
        case ImGuiKey_Home:         return VK_HOME;
        case ImGuiKey_End:          return VK_END;
        case ImGuiKey_LeftCtrl:     return VK_LCONTROL;
        case ImGuiKey_RightCtrl:    return VK_RCONTROL;
        case ImGuiKey_LeftShift:    return VK_LSHIFT;
        case ImGuiKey_RightShift:   return VK_RSHIFT;
        case ImGuiKey_LeftAlt:      return VK_LMENU;
        case ImGuiKey_RightAlt:     return VK_RMENU;
        case ImGuiKey_MouseLeft:    return VK_LBUTTON;
        case ImGuiKey_MouseRight:   return VK_RBUTTON;
        case ImGuiKey_MouseMiddle:  return VK_MBUTTON;
        default:                    return 0;
        }
    }
    uintptr_t GetWorldPtr()
    {
        if (!Globals::Datamodel.Address)
            return 0;

        uintptr_t WorkspaceAddr = Driver->Read<uintptr_t>(Globals::Datamodel.Address + Offsets::DataModel::Workspace);
        if (!WorkspaceAddr)
            return 0;

        return Driver->Read<uintptr_t>(WorkspaceAddr + Offsets::Workspace::World);
    }

    float ReadGravity(uintptr_t world)
    {
        return Driver->Read<float>(world + Offsets::World::Gravity);
    }

    void WriteGravity(uintptr_t world, float value)
    {
        Driver->Write<float>(world + Offsets::World::Gravity, value);
    }

    void SetPlatformStand(uintptr_t humanoidAddress, bool value)
    {
        Driver->Write<bool>(humanoidAddress + Offsets::Humanoid::PlatformStand, value);
    }
}

namespace Misc {

    void Fly()
    {
        timeBeginPeriod(1);

        bool prevKeyState = false;
        bool wasFlying = false;
        float savedGravity = 196.2f;

        while (true)
        {
            Sleep(1);

            HWND robloxWnd = FindWindowA(0, "Roblox");
            bool robloxFocused = robloxWnd && GetForegroundWindow() == robloxWnd;

            int  vk = MiscKeyToVK(Globals::Misc::Fly_Key);
            bool keyDown = robloxFocused && vk && (GetAsyncKeyState(vk) & 0x8000);

            if (Globals::Misc::Fly_Mode == ImKeyBindMode_Toggle)
            {
                if (keyDown && !prevKeyState)
                    Globals::Misc::Fly = !Globals::Misc::Fly;
            }
            else
            {
                Globals::Misc::Fly = keyDown;
            }

            prevKeyState = keyDown;

            auto& lp = Globals::LocalPlayer;
            if (!lp.HumanoidRootPart.Address || !lp.Humanoid.Address || !Globals::Camera.Address)
            {
                wasFlying = false;
                continue;
            }

            const uintptr_t world = GetWorldPtr();
            if (!world)
                continue;

            if (!Globals::Misc::Fly)
            {
                if (wasFlying)
                {
                    WriteGravity(world, savedGravity);
                    SetPlatformStand(lp.Humanoid.Address, false);
                    SDK::Part hrp(lp.HumanoidRootPart.Address);
                    hrp.Write_Velocity({ 0.f, 0.f, 0.f });
                    wasFlying = false;
                }
                continue;
            }

            if (!wasFlying)
            {
                savedGravity = ReadGravity(world);
                wasFlying = true;
            }

            WriteGravity(world, 0.f);
            SetPlatformStand(lp.Humanoid.Address, true);

            const bool W = GetAsyncKeyState('W') & 0x8000;
            const bool S = GetAsyncKeyState('S') & 0x8000;
            const bool A = GetAsyncKeyState('A') & 0x8000;
            const bool D = GetAsyncKeyState('D') & 0x8000;
            const bool Up = GetAsyncKeyState(VK_SPACE) & 0x8000;
            const bool Down = GetAsyncKeyState(VK_LCONTROL) & 0x8000;

            SDK::Camera cam(Globals::Camera.Address);
            const SDK::Matrix3 camRot = cam.Get_CameraRot();
            const SDK::Vector3 look = LookVector(camRot);
            const SDK::Vector3 right = RightVector(camRot);

            SDK::Vector3 dir(0.f, 0.f, 0.f);

            if (W)  dir = dir + look;
            if (S)  dir = dir - look;
            if (A)  dir = dir - right;
            if (D)  dir = dir + right;
            if (Up)   dir.y += 1.f;
            if (Down) dir.y -= 1.f;

            if (dir.magnitude() > 0.f)
                dir = dir.normalize();

            SDK::Part hrp(lp.HumanoidRootPart.Address);
            hrp.Write_Velocity({
                dir.x * Globals::Misc::Fly_Speed,
                dir.y * Globals::Misc::Fly_Speed,
                dir.z * Globals::Misc::Fly_Speed
                });
        }
    }

    void RunService()
    {
        std::thread(Fly).detach();
    }
}