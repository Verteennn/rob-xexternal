#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <immintrin.h>
#include <cmath>
#include <limits>
#include <iostream>
#include "Silent.h"
#include <Globals.hxx>
#include "Engine/Engine.h"
#include "Engine/Math/Math.h"

std::uint64_t SilentHelper::CachedInputObject = 0;

static float GetEffectiveFov()
{
    if (!Globals::Silent::GunBasedFov)
        return Globals::Silent::Fov;

    std::string ToolName = Globals::LocalPlayer.Tool_Name;

    if (ToolName.empty())
        return Globals::Silent::Fov;

    std::transform(ToolName.begin(), ToolName.end(), ToolName.begin(), ::tolower);

    if (ToolName.find("double-barrel") != std::string::npos ||
        ToolName.find("double barrel") != std::string::npos ||
        ToolName.find("doublebarrel") != std::string::npos)
    {
        return Globals::Silent::FovDoubleBarrel;
    }
    else if (ToolName.find("tacticalshotgun") != std::string::npos ||
        ToolName.find("tactical shotgun") != std::string::npos)
    {
        return Globals::Silent::FovTacticalShotgun;
    }
    else if (ToolName.find("revolver") != std::string::npos)
    {
        return Globals::Silent::FovRevolver;
    }

    return Globals::Silent::Fov;
}

static SDK::Instance GetTargetPart(SDK::Player& Player, int AimPart)
{
    SDK::Instance TargetPart{};

    if (AimPart == 0)
    {
        TargetPart = Player.Head;
    }
    else if (AimPart == 1)
    {
        if (Player.UpperTorso.Address != 0)
            TargetPart = Player.UpperTorso;
        else
            TargetPart = Player.Torso;
    }

    return TargetPart;
}

static bool IsPlayerKnocked(SDK::Player& Player)
{
    if (Player.Character.Address == 0)
        return false;

    SDK::Instance BodyEffects = Player.Character.Find_First_Child("BodyEffects");
    if (BodyEffects.Address == 0)
        return false;

    SDK::Instance Ko = BodyEffects.Find_First_Child("K.O");
    if (Ko.Address == 0)
        return false;

    bool KoValue = Driver->Read<bool>(Ko.Address + Offsets::Misc::Value);

    return KoValue;
}

static bool IsTargetWithinFov(SDK::Player& Player)
{
    if (Player.Character.Address == 0)
        return false;

    POINT CursorPoint;
    HWND Window = FindWindowA(nullptr, "Roblox");
    if (!Window || !GetCursorPos(&CursorPoint) || !ScreenToClient(Window, &CursorPoint))
        return false;

    SDK::Vector2 Cursor = { static_cast<float>(CursorPoint.x), static_cast<float>(CursorPoint.y) };

    SDK::Instance TargetPart = GetTargetPart(Player, Globals::Silent::AimPart);
    if (TargetPart.Address == 0)
        return false;

    SDK::Part PartObj(TargetPart.Address);
    SDK::Vector3 PartPosition = PartObj.Get_PartPosition();

    SDK::Vector2 PartScreen = Globals::VisualEngine.World_To_Screen(PartPosition);

    // FIX: World_To_Screen returns {-1,-1} for off-screen — treat anything <= -0.5 as invalid
    if (PartScreen.x <= -0.5f || PartScreen.y <= -0.5f)
        return false;

    float DistanceFromCursor = PartScreen.distance(Cursor);

    return DistanceFromCursor <= GetEffectiveFov();
}

static SDK::Player GetClosestPlayerFromCursor()
{
    POINT CursorPoint;
    HWND Window = FindWindowA(nullptr, "Roblox");
    if (!Window || !GetCursorPos(&CursorPoint) || !ScreenToClient(Window, &CursorPoint))
        return {};

    SDK::Vector2 Cursor = { static_cast<float>(CursorPoint.x), static_cast<float>(CursorPoint.y) };

    std::vector<SDK::Player> PlayersSnapshot;
    {
        PlayersSnapshot = Globals::Player_Cache;
    }

    if (PlayersSnapshot.empty())
        return {};

    SDK::Player ClosestPlayer{};
    float ShortestDistance = std::numeric_limits<float>::max();

    for (SDK::Player& Player : PlayersSnapshot)
    {
        if (Player.Character.Address == 0)
            continue;

        if (Globals::LocalPlayer.Character.Address != 0 &&
            Player.Character.Address == Globals::LocalPlayer.Character.Address)
            continue;

        SDK::Instance TargetPart = GetTargetPart(Player, Globals::Silent::AimPart);
        if (TargetPart.Address == 0)
            continue;

        SDK::Part PartObj(TargetPart.Address);
        SDK::Vector3 PartPosition = PartObj.Get_PartPosition();
        SDK::Vector2 PartScreen = Globals::VisualEngine.World_To_Screen(PartPosition);

        // FIX: use -0.5 sentinel check instead of <= 0
        if (PartScreen.x <= -0.5f || PartScreen.y <= -0.5f)
            continue;

        float DistanceFromCursor = PartScreen.distance(Cursor);

        if (Globals::Silent::UseFov && DistanceFromCursor > GetEffectiveFov())
            continue;

        if (Globals::Silent::KnockedCheck && IsPlayerKnocked(Player))
            continue;

        if (DistanceFromCursor < ShortestDistance)
        {
            ShortestDistance = DistanceFromCursor;
            ClosestPlayer = Player;
        }
    }

    return ClosestPlayer;
}

// FIX: Removed the erroneous + sizeof(std::shared_ptr<void*>) that was offsetting the read
// by 16 bytes past InputObject, producing a garbage pointer that caused mouse to fly to (0,0).
static std::uint64_t GetCurrentInputObject(std::uint64_t BaseAddress)
{
    return Driver->Read<std::uint64_t>(BaseAddress + Offsets::MouseService::InputObject);
}

void SilentHelper::SetFramePosX(std::uint64_t Position)
{
    Driver->Write<std::uint64_t>(Address + Offsets::Silent::FramePositionOffsetX, Position);
}

void SilentHelper::SetFramePosY(std::uint64_t Position)
{
    Driver->Write<std::uint64_t>(Address + Offsets::Silent::FramePositionOffsetY, Position);
}

void SilentHelper::InitializeMouseService(std::uint64_t Address)
{
    CachedInputObject = GetCurrentInputObject(Address);

    if (CachedInputObject && CachedInputObject != 0xFFFFFFFFFFFFFFFF)
    {
        const char* BasePointer = reinterpret_cast<const char*>(CachedInputObject);
        _mm_prefetch(BasePointer + Offsets::MouseService::MousePosition, _MM_HINT_T0);
        _mm_prefetch(BasePointer + Offsets::MouseService::MousePosition + sizeof(SDK::Vector2), _MM_HINT_T0);
    }
}

void SilentHelper::WriteMousePosition(std::uint64_t Address, float X, float Y)
{
    // Re-read every call so we always have a fresh InputObject pointer
    CachedInputObject = GetCurrentInputObject(Address);
    if (CachedInputObject != 0 && CachedInputObject != 0xFFFFFFFFFFFFFFFF)
    {
        SDK::Vector2 NewPosition = { X, Y };
        Driver->Write<SDK::Vector2>(CachedInputObject + Offsets::MouseService::MousePosition, NewPosition);
    }
}

static bool ShouldSilentAimBeActive()
{
    if (!Globals::Silent::Enabled)
        return false;

    return SilentAimLocked;
}

static void UpdateSilentAimKeyState()
{
    int Vk = ImGuiKeyToVK(Globals::Silent::Silent_Key);
    if (!Vk) return;

    HWND RobloxHwnd = FindWindowA(nullptr, "Roblox");
    bool RobloxFocused = RobloxHwnd && GetForegroundWindow() == RobloxHwnd;
    bool Pressed = RobloxFocused && (GetAsyncKeyState(Vk) & 0x8000) != 0;

    if (Globals::Silent::Silent_Mode == ImKeyBindMode_Toggle)
    {
        if (Pressed && !SilentAimKeyWasPressed)
        {
            SilentAimLocked = !SilentAimLocked;
        }

        if (!SilentAimLocked)
        {
            SilentCachedTarget = {};
            IsSilentReady = false;
        }
    }
    else
    {
        if (Pressed)
        {
            SilentAimLocked = true;
        }
        else
        {
            SilentAimLocked = false;
            SilentCachedTarget = {};
            IsSilentReady = false;
        }
    }

    SilentAimKeyWasPressed = Pressed;
}

void Silent::SilentFramePos() {

    SDK::Player Target{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    HWND Window = FindWindowA(0, "Roblox");

    for (;;) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        MouseService = std::make_unique<SDK::Instance>(Globals::Datamodel.Find_First_Child_Of_Class("MouseService"));
        if (!MouseService || !Globals::Datamodel.Address || !Globals::VisualEngine.Address) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        UpdateSilentAimKeyState();

        if (SilentAimInstance.Address != 0 && SilentHasOriginalSizes) {
            if (Globals::Silent::Enabled) {
                Driver->Write<SDK::Vector2>(SilentAimInstance.Address + Offsets::GuiObject::Size, { 0, 0 });

                auto Children = SilentAimInstance.Children();
                for (auto& Child : Children) {
                    if (Child.Address)
                        Driver->Write<SDK::Vector2>(Child.Address + Offsets::GuiObject::Size, { 0, 0 });
                }
            }
            else {
                Driver->Write<SDK::Vector2>(SilentAimInstance.Address + Offsets::GuiObject::Size, SilentOriginalSize);
                for (const auto& [ChildAddr, OrigSize] : SilentOriginalChildrenSizes) {
                    Driver->Write<SDK::Vector2>(ChildAddr, OrigSize);
                }
            }
        }

        if (!ShouldSilentAimBeActive()) {
            IsSilentReady = false;
            SilentCachedTarget = {};
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        static int AimInstanceCheckCounter = 0;
        if (AimInstanceCheckCounter++ % 10 == 0) {
            try {
                SDK::Instance LocalPlayer = SDK::Instance(Driver->Read<std::uintptr_t>(Globals::Datamodel.Find_First_Child_Of_Class("Players").Address + Offsets::Player::LocalPlayer));
                SDK::Instance PlayerGui = LocalPlayer.Find_First_Child("PlayerGui");

                if (PlayerGui.Address != 0) {
                    SDK::Instance FoundAimFrame{};
                    auto GuiChildren = PlayerGui.Children();

                    for (auto& Child : GuiChildren) {
                        if (!Child.Address) continue;

                        std::string Name = Child.Name();
                        if (Name == "Aim") {
                            FoundAimFrame = Child;
                            break;
                        }

                        std::string Class = Child.Class();
                        if (Class == "Frame" || Class == "ScreenGui" || Class == "GuiObject") {
                            std::string LowerName = Name;
                            std::transform(LowerName.begin(), LowerName.end(), LowerName.begin(), ::tolower);

                            if (LowerName.find("main") != std::string::npos) {
                                auto Grandchildren = Child.Children();
                                for (auto& GChild : Grandchildren) {
                                    if (GChild.Address && GChild.Name() == "Aim") {
                                        FoundAimFrame = GChild;
                                        break;
                                    }
                                }
                            }
                        }
                        if (FoundAimFrame.Address) break;
                    }

                    if (FoundAimFrame.Address != SilentAimInstance.Address) {
                        SilentAimInstance = FoundAimFrame;
                        SilentHasOriginalSizes = false;
                        SilentOriginalChildrenSizes.clear();

                        if (SilentAimInstance.Address != 0) {
                            SilentOriginalSize = Driver->Read<SDK::Vector2>(SilentAimInstance.Address + Offsets::GuiObject::Size);
                            auto AimChildren = SilentAimInstance.Children();
                            for (auto& C : AimChildren) {
                                if (C.Address) {
                                    SDK::Vector2 CSize = Driver->Read<SDK::Vector2>(C.Address + Offsets::GuiObject::Size);
                                    SilentOriginalChildrenSizes.push_back({ C.Address, CSize });
                                }
                            }
                            SilentHasOriginalSizes = true;
                        }
                    }
                }
            }
            catch (...) {}
        }

        if (!SilentFTarget || SilentCachedTarget.Character.Address == 0) {
            Target = GetClosestPlayerFromCursor();
            SilentCachedLastTarget = Target;
            SDK::Instance TargetPart = GetTargetPart(Target, Globals::Silent::AimPart);
            SilentFTarget = (TargetPart.Address != 0);
            SilentCachedTarget = Target;
        }
        else {
            if (!Globals::Silent::StickyAim) {
                // Always re-acquire closest target when sticky is off
                Target = GetClosestPlayerFromCursor();
                SilentFTarget = (Target.Character.Address != 0);
                SilentCachedTarget = Target;
            }
            else if (Globals::Silent::UseFov) {
                if (!IsTargetWithinFov(SilentCachedTarget)) {
                    SilentFTarget = false;
                    SilentCachedTarget = {};
                    continue;
                }
            }
        }

        if (SilentFTarget && SilentCachedTarget.Character.Address != 0) {
            if (Globals::Silent::KnockedCheck && IsPlayerKnocked(SilentCachedTarget)) {
                SilentFTarget = false;
                SilentCachedTarget = {};
                continue;
            }

            SDK::Instance TargetPart = GetTargetPart(SilentCachedTarget, Globals::Silent::AimPart);
            if (TargetPart.Address != 0) {
                SDK::Part PartObj(TargetPart.Address);
                SDK::Vector3 Part3D = PartObj.Get_PartPosition();
                SDK::Vector2 PartScreen = Globals::VisualEngine.World_To_Screen(Part3D);

                // FIX: validate screen position with -0.5 sentinel before using it
                if (PartScreen.x <= -0.5f || PartScreen.y <= -0.5f) {
                    IsSilentReady = false;
                    continue;
                }

                POINT CursorPos;
                GetCursorPos(&CursorPos);
                if (Window) ScreenToClient(Window, &CursorPos);

                SDK::Vector2 Dims = Globals::VisualEngine.Get_Dimensions();
                SilentPartPos = PartScreen;
                SilentCachedPositionX = static_cast<std::uint64_t>(CursorPos.x);
                SilentCachedPositionY = static_cast<std::uint64_t>(Dims.y - std::abs(Dims.y - static_cast<float>(CursorPos.y)) - 58);
                IsSilentReady = true;
            }
        }
        else {
            IsSilentReady = false;
        }
    }
}

void Silent::SilentMouse()
{
    SilentHelper MouseServiceInstance{};
    bool MouseServiceInitialized = false;

    for (;;)
    {
        if (!MouseService)
        {
            MouseServiceInitialized = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (!ShouldSilentAimBeActive())
        {
            MouseServiceInitialized = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (SilentCachedTarget.Character.Address != 0 && IsSilentReady)
        {
            // FIX: use -0.5 sentinel instead of -5000 which could still pass invalid values through
            if (SilentPartPos.x <= -0.5f || SilentPartPos.y <= -0.5f ||
                SilentPartPos.x > 15000.0f || SilentPartPos.y > 15000.0f)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            try
            {
                if (!MouseServiceInitialized)
                {
                    MouseServiceInstance.InitializeMouseService(MouseService->Address);
                    MouseServiceInitialized = true;
                }

                MouseServiceInstance.WriteMousePosition(MouseService->Address, SilentPartPos.x, SilentPartPos.y);
            }
            catch (...)
            {
                MouseServiceInitialized = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

void Silent::RunService()
{
    std::thread(SilentFramePos).detach();
    std::thread(SilentMouse).detach();
}