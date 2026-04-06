#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "../../Globals.hxx"

namespace fs = std::filesystem;

namespace ConfigManager
{
    inline std::string GetConfigDir()
    {
        char* appdata = nullptr;
        size_t size = 0;
        if (_dupenv_s(&appdata, &size, "APPDATA") == 0 && appdata)
        {
            fs::path dir = fs::path(appdata) / "discord";
            free(appdata);
            if (!fs::exists(dir))
                fs::create_directories(dir);
            return dir.string() + "\\";
        }
        fs::path fallback = "C:\\discord";
        if (!fs::exists(fallback))
            fs::create_directories(fallback);
        return fallback.string() + "\\";
    }

    inline void Refresh(std::vector<std::string>& list)
    {
        list.clear();
        for (auto& entry : fs::directory_iterator(GetConfigDir()))
            if (entry.path().extension() == ".ini")
                list.push_back(entry.path().stem().string());
    }

    inline void Delete(const std::string& name)
    {
        fs::path p = GetConfigDir() + name + ".ini";
        if (fs::exists(p)) fs::remove(p);
    }

    // helpers

    static void WriteStr(const char* sec, const char* key, const std::string& val, const char* path)
    {
        WritePrivateProfileStringA(sec, key, val.c_str(), path);
    }
    static void WriteBool(const char* sec, const char* key, bool v, const char* path)
    {
        WritePrivateProfileStringA(sec, key, v ? "1" : "0", path);
    }
    static void WriteFloat(const char* sec, const char* key, float v, const char* path)
    {
        WritePrivateProfileStringA(sec, key, std::to_string(v).c_str(), path);
    }
    static void WriteInt(const char* sec, const char* key, int v, const char* path)
    {
        WritePrivateProfileStringA(sec, key, std::to_string(v).c_str(), path);
    }
    static void WriteColor(const char* sec, const char* key, float col[4], const char* path)
    {
        auto k = [&](int i) { return std::string(key) + std::to_string(i); };
        for (int i = 0; i < 4; i++)
            WritePrivateProfileStringA(sec, k(i).c_str(), std::to_string(col[i]).c_str(), path);
    }

    static bool ReadBool(const char* sec, const char* key, bool def, const char* path)
    {
        return GetPrivateProfileIntA(sec, key, def ? 1 : 0, path) != 0;
    }
    static float ReadFloat(const char* sec, const char* key, float def, const char* path)
    {
        char buf[64];
        GetPrivateProfileStringA(sec, key, std::to_string(def).c_str(), buf, sizeof(buf), path);
        try { return std::stof(buf); }
        catch (...) { return def; }
    }
    static int ReadInt(const char* sec, const char* key, int def, const char* path)
    {
        return GetPrivateProfileIntA(sec, key, def, path);
    }
    static void ReadColor(const char* sec, const char* key, float col[4], const char* path)
    {
        for (int i = 0; i < 4; i++)
        {
            auto k = std::string(key) + std::to_string(i);
            col[i] = ReadFloat(sec, k.c_str(), col[i], path);
        }
    }

    // save

    inline void Save(const std::string& name)
    {
        std::string p = GetConfigDir() + name + ".ini";
        const char* path = p.c_str();

        // settings
        WriteBool("Settings", "Team_Check", Globals::Settings::Team_Check, path);
        WriteBool("Settings", "Client_Check", Globals::Settings::Client_Check, path);
        WriteBool("Settings", "Streamproof", Globals::Settings::Streamproof, path);
        WriteInt("Settings", "Performance_Mode", Globals::Settings::Performance_Mode, path);

        // world
        WriteBool("World", "Skybox", Globals::World::Skybox, path);
        WriteBool("World", "Rotate", Globals::World::Rotate, path);
        WriteBool("World", "Ambience", Globals::World::Ambience, path);
        WriteBool("World", "Fog", Globals::World::Fog, path);
        WriteBool("World", "Brightness", Globals::World::Brightness, path);
        WriteBool("World", "Exposure", Globals::World::Exposure, path);
        WriteBool("World", "FOV", Globals::World::FOV, path);
        WriteInt("World", "Skybox_Type", Globals::World::Skybox_Type, path);
        WriteFloat("World", "Skybox_Rotate_Speed", Globals::World::Skybox_Rotate_Speed, path);
        WriteFloat("World", "Fog_Distance", Globals::World::Fog_Distance, path);
        WriteFloat("World", "FOV_Distance", Globals::World::FOV_Distance, path);
        WriteFloat("World", "ExposureI", Globals::World::ExposureI, path);
        WriteFloat("World", "BrightnessI", Globals::World::BrightnessI, path);
        WriteColor("World", "Ambience", Globals::World::Colors::Ambience, path);
        WriteColor("World", "Fog", Globals::World::Colors::Fog, path);

        // aimbot
        WriteBool("Aimbot", "Enabled", Globals::Aimbot::Enabled, path);
        WriteBool("Aimbot", "useFov", Globals::Aimbot::useFov, path);
        WriteBool("Aimbot", "DrawFov", Globals::Aimbot::DrawFov, path);
        WriteBool("Aimbot", "FovSpin", Globals::Aimbot::FovSpin, path);
        WriteBool("Aimbot", "FillFov", Globals::Aimbot::FillFov, path);
        WriteBool("Aimbot", "AimbotSticky", Globals::Aimbot::AimbotSticky, path);
        WriteBool("Aimbot", "Shake", Globals::Aimbot::Shake, path);
        WriteBool("Aimbot", "KnockedCheck", Globals::Aimbot::KnockedCheck, path);
        WriteFloat("Aimbot", "FovSize", Globals::Aimbot::FovSize, path);
        WriteFloat("Aimbot", "ShakeX", Globals::Aimbot::ShakeX, path);
        WriteFloat("Aimbot", "ShakeY", Globals::Aimbot::ShakeY, path);
        WriteFloat("Aimbot", "ShakeZ", Globals::Aimbot::ShakeZ, path);
        WriteInt("Aimbot", "HitPart", Globals::Aimbot::HitPart, path);
        WriteInt("Aimbot", "Aimbot_type", Globals::Aimbot::Aimbot_type, path);
        WriteInt("Aimbot", "FovSpinSpeed", Globals::Aimbot::FovSpinSpeed, path);
        WriteInt("Aimbot", "Aimbot_Key", (int)Globals::Aimbot::Aimbot_Key, path);
        WriteInt("Aimbot", "Aimbot_Mode", (int)Globals::Aimbot::Aimbot_Mode, path);
        WriteColor("Aimbot", "FovColor", Globals::Aimbot::FovColor, path);
        WriteFloat("Aimbot", "CamSmoothX", Globals::Aimbot::Camera::Smoothing_X, path);
        WriteFloat("Aimbot", "CamSmoothY", Globals::Aimbot::Camera::Smoothing_Y, path);
        WriteFloat("Aimbot", "MouseSmoothX", Globals::Aimbot::Mouse::Smoothing_X, path);
        WriteFloat("Aimbot", "MouseSmoothY", Globals::Aimbot::Mouse::Smoothing_Y, path);
        WriteFloat("Aimbot", "MouseSens", Globals::Aimbot::Mouse::Mouse_Sensitivty, path);

        // silent
        WriteBool("Silent", "Enabled", Globals::Silent::Enabled, path);
        WriteBool("Silent", "DrawFov", Globals::Silent::DrawFov, path);
        WriteBool("Silent", "StickyAim", Globals::Silent::StickyAim, path);
        WriteBool("Silent", "SpoofMouse", Globals::Silent::SpoofMouse, path);
        WriteBool("Silent", "UseFov", Globals::Silent::UseFov, path);
        WriteBool("Silent", "KnockedCheck", Globals::Silent::KnockedCheck, path);
        WriteBool("Silent", "GunBasedFov", Globals::Silent::GunBasedFov, path);
        WriteBool("Silent", "FovSpin", Globals::Silent::FovSpin, path);
        WriteBool("Silent", "FillFov", Globals::Silent::FillFov, path);
        WriteFloat("Silent", "Fov", Globals::Silent::Fov, path);
        WriteFloat("Silent", "FovDoubleBarrel", Globals::Silent::FovDoubleBarrel, path);
        WriteFloat("Silent", "FovTacticalShotgun", Globals::Silent::FovTacticalShotgun, path);
        WriteFloat("Silent", "FovRevolver", Globals::Silent::FovRevolver, path);
        WriteInt("Silent", "AimPart", Globals::Silent::AimPart, path);
        WriteInt("Silent", "FovSpinSpeed", Globals::Silent::FovSpinSpeed, path);
        WriteInt("Silent", "Silent_Key", (int)Globals::Silent::Silent_Key, path);
        WriteInt("Silent", "Silent_Mode", (int)Globals::Silent::Silent_Mode, path);
        WriteColor("Silent", "FovColor", Globals::Silent::FovColor, path);

        // visuals
        WriteBool("Visuals", "Enabled", Globals::Visuals::Enabled, path);
        WriteBool("Visuals", "Box", Globals::Visuals::Box, path);
        WriteBool("Visuals", "Box_Fill", Globals::Visuals::Box_Fill, path);
        WriteBool("Visuals", "Box_Fill_Gradient", Globals::Visuals::Box_Fill_Gradient, path);
        WriteBool("Visuals", "Box_Fill_Gradient_Rotate", Globals::Visuals::Box_Fill_Gradient_Rotate, path);
        WriteBool("Visuals", "Healthbar", Globals::Visuals::Healthbar, path);
        WriteBool("Visuals", "Health", Globals::Visuals::Health, path);
        WriteBool("Visuals", "Name", Globals::Visuals::Name, path);
        WriteBool("Visuals", "Distance", Globals::Visuals::Distance, path);
        WriteBool("Visuals", "Rig_Type", Globals::Visuals::Rig_Type, path);
        WriteBool("Visuals", "Tool", Globals::Visuals::Tool, path);
        WriteBool("Visuals", "Skeleton", Globals::Visuals::Skeleton, path);
        WriteBool("Visuals", "Chams", Globals::Visuals::Chams, path);
        WriteBool("Visuals", "ChamsFade", Globals::Visuals::ChamsFade, path);
        WriteFloat("Visuals", "Render_Distance", Globals::Visuals::Render_Distance, path);
        WriteInt("Visuals", "ChamsFadeSpeed", Globals::Visuals::ChamsFadeSpeed, path);
        WriteInt("Visuals", "BoxFillSpeed", Globals::Visuals::BoxFillSpeed, path);
        WriteInt("Visuals", "Healthbar_Type", Globals::Visuals::Healthbar_Type, path);
        WriteInt("Visuals", "Box_Type", Globals::Visuals::Box_Type, path);
        WriteInt("Visuals", "Box_Fill_Type", Globals::Visuals::Box_Fill_Type, path);
        WriteInt("Visuals", "Name_Type", Globals::Visuals::Name_Type, path);
        WriteInt("Visuals", "Gap", Globals::Visuals::Gap, path);
        WriteInt("Visuals", "Thickness", Globals::Visuals::Thickness, path);
        WriteColor("Visuals", "Box", Globals::Visuals::Colors::Box, path);
        WriteColor("Visuals", "BoxFill_Top", Globals::Visuals::Colors::BoxFill_Top, path);
        WriteColor("Visuals", "BoxFill_Bottom", Globals::Visuals::Colors::BoxFill_Bottom, path);
        WriteColor("Visuals", "Healthbar", Globals::Visuals::Colors::Healthbar, path);
        WriteColor("Visuals", "Name", Globals::Visuals::Colors::Name, path);
        WriteColor("Visuals", "Distance", Globals::Visuals::Colors::Distance, path);
        WriteColor("Visuals", "Rig_Type", Globals::Visuals::Colors::Rig_Type, path);
        WriteColor("Visuals", "Tool", Globals::Visuals::Colors::Tool, path);
        WriteColor("Visuals", "Health", Globals::Visuals::Colors::Health, path);
        WriteColor("Visuals", "Skeleton", Globals::Visuals::Colors::Skeleton, path);
        WriteColor("Visuals", "Chams", Globals::Visuals::Colors::Chams, path);
        WriteColor("Visuals", "ChamsOutline", Globals::Visuals::Colors::ChamsOutline, path);
        WriteColor("Visuals", "Healthbar_Top", Globals::Visuals::Colors::Healthbar_Top, path);
        WriteColor("Visuals", "Healthbar_Middle", Globals::Visuals::Colors::Healthbar_Middle, path);
        WriteColor("Visuals", "Healthbar_Bottom", Globals::Visuals::Colors::Healthbar_Bottom, path);

        // misc
        WriteBool("Misc", "Fly", Globals::Misc::Fly, path);
        WriteFloat("Misc", "Fly_Speed", Globals::Misc::Fly_Speed, path);
        WriteInt("Misc", "Fly_Key", (int)Globals::Misc::Fly_Key, path);
        WriteInt("Misc", "Fly_Mode", (int)Globals::Misc::Fly_Mode, path);
    }

    // load

    inline void Load(const std::string& name)
    {
        std::string p = GetConfigDir() + name + ".ini";
        if (!fs::exists(p)) return;
        const char* path = p.c_str();

        // settings
        Globals::Settings::Team_Check = ReadBool("Settings", "Team_Check", Globals::Settings::Team_Check, path);
        Globals::Settings::Client_Check = ReadBool("Settings", "Client_Check", Globals::Settings::Client_Check, path);
        Globals::Settings::Streamproof = ReadBool("Settings", "Streamproof", Globals::Settings::Streamproof, path);
        Globals::Settings::Performance_Mode = ReadInt("Settings", "Performance_Mode", Globals::Settings::Performance_Mode, path);

        // world
        Globals::World::Skybox = ReadBool("World", "Skybox", Globals::World::Skybox, path);
        Globals::World::Rotate = ReadBool("World", "Rotate", Globals::World::Rotate, path);
        Globals::World::Ambience = ReadBool("World", "Ambience", Globals::World::Ambience, path);
        Globals::World::Fog = ReadBool("World", "Fog", Globals::World::Fog, path);
        Globals::World::Brightness = ReadBool("World", "Brightness", Globals::World::Brightness, path);
        Globals::World::Exposure = ReadBool("World", "Exposure", Globals::World::Exposure, path);
        Globals::World::FOV = ReadBool("World", "FOV", Globals::World::FOV, path);
        Globals::World::Skybox_Type = ReadInt("World", "Skybox_Type", Globals::World::Skybox_Type, path);
        Globals::World::Skybox_Rotate_Speed = ReadFloat("World", "Skybox_Rotate_Speed", Globals::World::Skybox_Rotate_Speed, path);
        Globals::World::Fog_Distance = ReadFloat("World", "Fog_Distance", Globals::World::Fog_Distance, path);
        Globals::World::FOV_Distance = ReadFloat("World", "FOV_Distance", Globals::World::FOV_Distance, path);
        Globals::World::ExposureI = ReadFloat("World", "ExposureI", Globals::World::ExposureI, path);
        Globals::World::BrightnessI = ReadFloat("World", "BrightnessI", Globals::World::BrightnessI, path);
        ReadColor("World", "Ambience", Globals::World::Colors::Ambience, path);
        ReadColor("World", "Fog", Globals::World::Colors::Fog, path);

        // aimbot
        Globals::Aimbot::Enabled = ReadBool("Aimbot", "Enabled", Globals::Aimbot::Enabled, path);
        Globals::Aimbot::useFov = ReadBool("Aimbot", "useFov", Globals::Aimbot::useFov, path);
        Globals::Aimbot::DrawFov = ReadBool("Aimbot", "DrawFov", Globals::Aimbot::DrawFov, path);
        Globals::Aimbot::FovSpin = ReadBool("Aimbot", "FovSpin", Globals::Aimbot::FovSpin, path);
        Globals::Aimbot::FillFov = ReadBool("Aimbot", "FillFov", Globals::Aimbot::FillFov, path);
        Globals::Aimbot::AimbotSticky = ReadBool("Aimbot", "AimbotSticky", Globals::Aimbot::AimbotSticky, path);
        Globals::Aimbot::Shake = ReadBool("Aimbot", "Shake", Globals::Aimbot::Shake, path);
        Globals::Aimbot::KnockedCheck = ReadBool("Aimbot", "KnockedCheck", Globals::Aimbot::KnockedCheck, path);
        Globals::Aimbot::FovSize = ReadFloat("Aimbot", "FovSize", Globals::Aimbot::FovSize, path);
        Globals::Aimbot::ShakeX = ReadFloat("Aimbot", "ShakeX", Globals::Aimbot::ShakeX, path);
        Globals::Aimbot::ShakeY = ReadFloat("Aimbot", "ShakeY", Globals::Aimbot::ShakeY, path);
        Globals::Aimbot::ShakeZ = ReadFloat("Aimbot", "ShakeZ", Globals::Aimbot::ShakeZ, path);
        Globals::Aimbot::HitPart = ReadInt("Aimbot", "HitPart", Globals::Aimbot::HitPart, path);
        Globals::Aimbot::Aimbot_type = ReadInt("Aimbot", "Aimbot_type", Globals::Aimbot::Aimbot_type, path);
        Globals::Aimbot::FovSpinSpeed = ReadInt("Aimbot", "FovSpinSpeed", Globals::Aimbot::FovSpinSpeed, path);
        Globals::Aimbot::Aimbot_Key = (ImGuiKey)ReadInt("Aimbot", "Aimbot_Key", (int)Globals::Aimbot::Aimbot_Key, path);
        Globals::Aimbot::Aimbot_Mode = (ImKeyBindMode)ReadInt("Aimbot", "Aimbot_Mode", (int)Globals::Aimbot::Aimbot_Mode, path);
        ReadColor("Aimbot", "FovColor", Globals::Aimbot::FovColor, path);
        Globals::Aimbot::Camera::Smoothing_X = ReadFloat("Aimbot", "CamSmoothX", Globals::Aimbot::Camera::Smoothing_X, path);
        Globals::Aimbot::Camera::Smoothing_Y = ReadFloat("Aimbot", "CamSmoothY", Globals::Aimbot::Camera::Smoothing_Y, path);
        Globals::Aimbot::Mouse::Smoothing_X = ReadFloat("Aimbot", "MouseSmoothX", Globals::Aimbot::Mouse::Smoothing_X, path);
        Globals::Aimbot::Mouse::Smoothing_Y = ReadFloat("Aimbot", "MouseSmoothY", Globals::Aimbot::Mouse::Smoothing_Y, path);
        Globals::Aimbot::Mouse::Mouse_Sensitivty = ReadFloat("Aimbot", "MouseSens", Globals::Aimbot::Mouse::Mouse_Sensitivty, path);

        // silent
        Globals::Silent::Enabled = ReadBool("Silent", "Enabled", Globals::Silent::Enabled, path);
        Globals::Silent::DrawFov = ReadBool("Silent", "DrawFov", Globals::Silent::DrawFov, path);
        Globals::Silent::StickyAim = ReadBool("Silent", "StickyAim", Globals::Silent::StickyAim, path);
        Globals::Silent::SpoofMouse = ReadBool("Silent", "SpoofMouse", Globals::Silent::SpoofMouse, path);
        Globals::Silent::UseFov = ReadBool("Silent", "UseFov", Globals::Silent::UseFov, path);
        Globals::Silent::KnockedCheck = ReadBool("Silent", "KnockedCheck", Globals::Silent::KnockedCheck, path);
        Globals::Silent::GunBasedFov = ReadBool("Silent", "GunBasedFov", Globals::Silent::GunBasedFov, path);
        Globals::Silent::FovSpin = ReadBool("Silent", "FovSpin", Globals::Silent::FovSpin, path);
        Globals::Silent::FillFov = ReadBool("Silent", "FillFov", Globals::Silent::FillFov, path);
        Globals::Silent::Fov = ReadFloat("Silent", "Fov", Globals::Silent::Fov, path);
        Globals::Silent::FovDoubleBarrel = ReadFloat("Silent", "FovDoubleBarrel", Globals::Silent::FovDoubleBarrel, path);
        Globals::Silent::FovTacticalShotgun = ReadFloat("Silent", "FovTacticalShotgun", Globals::Silent::FovTacticalShotgun, path);
        Globals::Silent::FovRevolver = ReadFloat("Silent", "FovRevolver", Globals::Silent::FovRevolver, path);
        Globals::Silent::AimPart = ReadInt("Silent", "AimPart", Globals::Silent::AimPart, path);
        Globals::Silent::FovSpinSpeed = ReadInt("Silent", "FovSpinSpeed", Globals::Silent::FovSpinSpeed, path);
        Globals::Silent::Silent_Key = (ImGuiKey)ReadInt("Silent", "Silent_Key", (int)Globals::Silent::Silent_Key, path);
        Globals::Silent::Silent_Mode = (ImKeyBindMode)ReadInt("Silent", "Silent_Mode", (int)Globals::Silent::Silent_Mode, path);
        ReadColor("Silent", "FovColor", Globals::Silent::FovColor, path);

        // visuals
        Globals::Visuals::Enabled = ReadBool("Visuals", "Enabled", Globals::Visuals::Enabled, path);
        Globals::Visuals::Box = ReadBool("Visuals", "Box", Globals::Visuals::Box, path);
        Globals::Visuals::Box_Fill = ReadBool("Visuals", "Box_Fill", Globals::Visuals::Box_Fill, path);
        Globals::Visuals::Box_Fill_Gradient = ReadBool("Visuals", "Box_Fill_Gradient", Globals::Visuals::Box_Fill_Gradient, path);
        Globals::Visuals::Box_Fill_Gradient_Rotate = ReadBool("Visuals", "Box_Fill_Gradient_Rotate", Globals::Visuals::Box_Fill_Gradient_Rotate, path);
        Globals::Visuals::Healthbar = ReadBool("Visuals", "Healthbar", Globals::Visuals::Healthbar, path);
        Globals::Visuals::Health = ReadBool("Visuals", "Health", Globals::Visuals::Health, path);
        Globals::Visuals::Name = ReadBool("Visuals", "Name", Globals::Visuals::Name, path);
        Globals::Visuals::Distance = ReadBool("Visuals", "Distance", Globals::Visuals::Distance, path);
        Globals::Visuals::Rig_Type = ReadBool("Visuals", "Rig_Type", Globals::Visuals::Rig_Type, path);
        Globals::Visuals::Tool = ReadBool("Visuals", "Tool", Globals::Visuals::Tool, path);
        Globals::Visuals::Skeleton = ReadBool("Visuals", "Skeleton", Globals::Visuals::Skeleton, path);
        Globals::Visuals::Chams = ReadBool("Visuals", "Chams", Globals::Visuals::Chams, path);
        Globals::Visuals::ChamsFade = ReadBool("Visuals", "ChamsFade", Globals::Visuals::ChamsFade, path);
        Globals::Visuals::Render_Distance = ReadFloat("Visuals", "Render_Distance", Globals::Visuals::Render_Distance, path);
        Globals::Visuals::ChamsFadeSpeed = ReadInt("Visuals", "ChamsFadeSpeed", Globals::Visuals::ChamsFadeSpeed, path);
        Globals::Visuals::BoxFillSpeed = ReadInt("Visuals", "BoxFillSpeed", Globals::Visuals::BoxFillSpeed, path);
        Globals::Visuals::Healthbar_Type = ReadInt("Visuals", "Healthbar_Type", Globals::Visuals::Healthbar_Type, path);
        Globals::Visuals::Box_Type = ReadInt("Visuals", "Box_Type", Globals::Visuals::Box_Type, path);
        Globals::Visuals::Box_Fill_Type = ReadInt("Visuals", "Box_Fill_Type", Globals::Visuals::Box_Fill_Type, path);
        Globals::Visuals::Name_Type = ReadInt("Visuals", "Name_Type", Globals::Visuals::Name_Type, path);
        Globals::Visuals::Gap = ReadInt("Visuals", "Gap", Globals::Visuals::Gap, path);
        Globals::Visuals::Thickness = ReadInt("Visuals", "Thickness", Globals::Visuals::Thickness, path);
        ReadColor("Visuals", "Box", Globals::Visuals::Colors::Box, path);
        ReadColor("Visuals", "BoxFill_Top", Globals::Visuals::Colors::BoxFill_Top, path);
        ReadColor("Visuals", "BoxFill_Bottom", Globals::Visuals::Colors::BoxFill_Bottom, path);
        ReadColor("Visuals", "Healthbar", Globals::Visuals::Colors::Healthbar, path);
        ReadColor("Visuals", "Name", Globals::Visuals::Colors::Name, path);
        ReadColor("Visuals", "Distance", Globals::Visuals::Colors::Distance, path);
        ReadColor("Visuals", "Rig_Type", Globals::Visuals::Colors::Rig_Type, path);
        ReadColor("Visuals", "Tool", Globals::Visuals::Colors::Tool, path);
        ReadColor("Visuals", "Health", Globals::Visuals::Colors::Health, path);
        ReadColor("Visuals", "Skeleton", Globals::Visuals::Colors::Skeleton, path);
        ReadColor("Visuals", "Chams", Globals::Visuals::Colors::Chams, path);
        ReadColor("Visuals", "ChamsOutline", Globals::Visuals::Colors::ChamsOutline, path);
        ReadColor("Visuals", "Healthbar_Top", Globals::Visuals::Colors::Healthbar_Top, path);
        ReadColor("Visuals", "Healthbar_Middle", Globals::Visuals::Colors::Healthbar_Middle, path);
        ReadColor("Visuals", "Healthbar_Bottom", Globals::Visuals::Colors::Healthbar_Bottom, path);

        // misc
        Globals::Misc::Fly = ReadBool("Misc", "Fly", Globals::Misc::Fly, path);
        Globals::Misc::Fly_Speed = ReadFloat("Misc", "Fly_Speed", Globals::Misc::Fly_Speed, path);
        Globals::Misc::Fly_Key = (ImGuiKey)ReadInt("Misc", "Fly_Key", (int)Globals::Misc::Fly_Key, path);
        Globals::Misc::Fly_Mode = (ImKeyBindMode)ReadInt("Misc", "Fly_Mode", (int)Globals::Misc::Fly_Mode, path);
    }
}