// ─────────────────────────────────────────────────────────────────────────────
//  Graphics.cpp  —  xExternal  |  "Obsidian" UI redesign
//
//  FONT UPGRADE (recommended):
//    1. Download Inter-Regular.ttf + Inter-Medium.ttf  →  rsms.me/inter
//    2. Run:  xxd -i Inter-Regular.ttf > Fonts/Inter_Regular.h
//             xxd -i Inter-Medium.ttf  > Fonts/Inter_Medium.h
//    3. Replace both Tahoma AddFontFromMemoryTTF calls in Create_Imgui()
//       with your Inter arrays. The FreeType pipeline is identical.
// ─────────────────────────────────────────────────────────────────────────────

#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <cmath>

#include "Graphics.h"
#include <Globals.hxx>

#include <imgui/misc/imgui_freetype.h>
#include "imgui/imgui_internal.h"

#include "Fonts/Inter_Regular.h"
#include "Fonts/Inter_SemiBold.h"
#include "../Features/Cheats/Visuals/Visuals.h"
#include <ImGui/addons/colors/colors.h>
#include "Core/Features/Cheats/Aimbot/Silent/Silent.h"
#include "Core/Config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Window procedure  (unchanged)
// ─────────────────────────────────────────────────────────────────────────────

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);

LRESULT CALLBACK WndProc(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    if (ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, WParam, LParam))
        return true;

    switch (Msg)
    {
    case WM_SYSCOMMAND:
        if ((WParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_SYSKEYDOWN:
        if (WParam == VK_F4) { DestroyWindow(Hwnd); return 0; }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        return 0;
    }
    return DefWindowProcA(Hwnd, Msg, WParam, LParam);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Graphics lifecycle  (unchanged)
// ─────────────────────────────────────────────────────────────────────────────

Graphics::Graphics() { Detail = std::make_unique<detail_t>(); }
Graphics::~Graphics() { Destroy_Imgui(); Destroy_Window(); Destroy_Device(); }

bool Graphics::Create_Window()
{
    Detail->WindowClass.cbSize = sizeof(Detail->WindowClass);
    Detail->WindowClass.style = CS_CLASSDC;
    Detail->WindowClass.lpszClassName = "xExternal";
    Detail->WindowClass.hInstance = GetModuleHandleA(0);
    Detail->WindowClass.lpfnWndProc = WndProc;
    RegisterClassExA(&Detail->WindowClass);

    Detail->Window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        Detail->WindowClass.lpszClassName, "xExternal", WS_POPUP,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        0, 0, Detail->WindowClass.hInstance, 0);

    if (!Detail->Window) return false;

    SetLayeredWindowAttributes(Detail->Window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    RECT ClientArea{}, WindowArea{};
    GetClientRect(Detail->Window, &ClientArea);
    GetWindowRect(Detail->Window, &WindowArea);
    POINT Diff{};
    ClientToScreen(Detail->Window, &Diff);

    MARGINS Margins{
        WindowArea.left + (Diff.x - WindowArea.left),
        WindowArea.top + (Diff.y - WindowArea.top),
        WindowArea.right, WindowArea.bottom
    };
    DwmExtendFrameIntoClientArea(Detail->Window, &Margins);
    ShowWindow(Detail->Window, SW_SHOW);
    UpdateWindow(Detail->Window);
    return true;
}

bool Graphics::Create_Device()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.OutputWindow = Detail->Window;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Windowed = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL fl;
    const D3D_FEATURE_LEVEL fll[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        fll, 2, D3D11_SDK_VERSION, &sd, &Detail->SwapChain, &Detail->Device, &fl, &Detail->DeviceContext);

    if (hr == DXGI_ERROR_UNSUPPORTED)
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
            fll, 2, D3D11_SDK_VERSION, &sd, &Detail->SwapChain, &Detail->Device, &fl, &Detail->DeviceContext);

    if (hr != S_OK)
        MessageBoxA(nullptr, "This software can not run on your computer.", "Critical Problem", MB_ICONERROR | MB_OK);

    ID3D11Texture2D* bb = nullptr;
    Detail->SwapChain->GetBuffer(0, IID_PPV_ARGS(&bb));
    if (bb) {
        Detail->Device->CreateRenderTargetView(bb, nullptr, &Detail->GraphicsTargetView);
        bb->Release();
        return true;
    }
    return false;
}

bool Graphics::Create_Imgui()
{
    ImGui::CreateContext();

    float dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(
        ::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    ImGuiIO& IO = ImGui::GetIO();
    IO.IniFilename = nullptr;

    // ── FreeType setup ────────────────────────────────────────────────────────
    const unsigned int ftFlags =
        ImGuiFreeTypeLoaderFlags_LoadColor;
    IO.Fonts->SetFontLoader(ImGuiFreeType::GetFontLoader());
    IO.Fonts->FontLoaderFlags = ftFlags;

    ImFontConfig cfg{};
    cfg.PixelSnapH = false;
    cfg.OversampleH = 3;
    cfg.OversampleV = 3;
    cfg.RasterizerMultiply = 1.0f;
    cfg.FontLoaderFlags = 0;
    cfg.FontDataOwnedByAtlas = false;

    const float sz = 14.0f * dpiScale;
    IO.Fonts->AddFontFromMemoryTTF(
        Inter_24pt_Regular_ttf, (int)Inter_24pt_Regular_ttf_len, sz, &cfg);
    Tahoma_BoldXP = IO.Fonts->AddFontFromMemoryTTF(
        Inter_24pt_SemiBold_ttf, (int)Inter_24pt_SemiBold_ttf_len, sz, &cfg);

    // ── Base ImGui style  ─────────────────────────────────────────────────────
    // Visual style is applied per-frame in Render_Menu via Push/Pop so it never
    // bleeds into Render_Visuals.  Only baseline rounding/spacing lives here.
    ImGui::StyleColorsDark();
    ImGuiStyle& S = ImGui::GetStyle();
    S.WindowRounding = 0.f;
    S.ChildRounding = 5.f;
    S.FrameRounding = 3.f;
    S.PopupRounding = 4.f;
    S.ScrollbarRounding = 2.f;
    S.GrabRounding = 2.f;
    S.ItemSpacing = { 6.f, 5.f };
    S.FramePadding = { 6.f, 4.f };
    S.WindowPadding = { 0.f, 0.f };
    S.PopupBorderSize = 1.f;
    S.FrameBorderSize = 0.f;
    S.ScrollbarSize = 5.f;

    if (!ImGui_ImplWin32_Init(Detail->Window))                return false;
    if (!Detail->Device || !Detail->DeviceContext)            return false;
    if (!ImGui_ImplDX11_Init(Detail->Device, Detail->DeviceContext)) return false;
    return true;
}

void Graphics::Destroy_Device()
{
    if (Detail->GraphicsTargetView) Detail->GraphicsTargetView->Release();
    if (Detail->SwapChain)          Detail->SwapChain->Release();
    if (Detail->DeviceContext)      Detail->DeviceContext->Release();
    if (Detail->Device)             Detail->Device->Release();
}
void Graphics::Destroy_Window()
{
    DestroyWindow(Detail->Window);
    UnregisterClassA(Detail->WindowClass.lpszClassName, Detail->WindowClass.hInstance);
}
void Graphics::Destroy_Imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Graphics::Start_Render()
{
    MSG Msg;
    while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Msg); DispatchMessage(&Msg);
    }

    SetWindowDisplayAffinity(Detail->Window,
        Globals::Settings::Streamproof ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    {
        HWND _roblox = FindWindowA(0, "Roblox");
        HWND _fg = GetForegroundWindow();
        if ((GetAsyncKeyState(VK_INSERT) & 1) && (_fg == _roblox || _fg == Detail->Window))
        {
            Running = !Running;
            LONG exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED;
            if (!Running) exStyle |= WS_EX_TRANSPARENT;
            SetWindowLong(Detail->Window, GWL_EXSTYLE, exStyle);
        }
    }
}

void Graphics::End_Render()
{
    ImGui::Render();
    const float cc[4]{ 0.f, 0.f, 0.f, 0.f };
    Detail->DeviceContext->OMSetRenderTargets(1, &Detail->GraphicsTargetView, nullptr);
    Detail->DeviceContext->ClearRenderTargetView(Detail->GraphicsTargetView, cc);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    switch (Globals::Settings::Performance_Mode)
    {
    case 0:  std::this_thread::sleep_for(std::chrono::milliseconds(1));
        Detail->SwapChain->Present(0, 0); break;
    case 1:  Detail->SwapChain->Present(1, 0); break;
    default: Detail->SwapChain->Present(0, 0); break;
    }
}

static void DrawCursor()  // unchanged
{
    if (!SilentAimInstance.Address) return;
    if (!Driver->Read<bool>(SilentAimInstance.Address + Offsets::GuiObject::Visible)) return;
    POINT pt;
    if (!GetCursorPos(&pt)) return;

    const bool rmbHeld = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
    const float gap = rmbHeld ? 4.f : 10.f;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImU32 col = IM_COL32(255, 255, 255, 255);
    const float dot = 4.f, lw = 2.f, ll = 10.f;
    const ImVec2 c = { (float)pt.x, (float)pt.y };

    dl->AddRectFilled(c - ImVec2(dot * .5f, dot * .5f), c + ImVec2(dot * .5f, dot * .5f), col);
    dl->AddRectFilled({ c.x - lw * .5f, c.y - gap - ll }, { c.x + lw * .5f, c.y - gap }, col);
    dl->AddRectFilled({ c.x - lw * .5f, c.y + gap }, { c.x + lw * .5f, c.y + gap + ll }, col);
    dl->AddRectFilled({ c.x - gap - ll, c.y - lw * .5f }, { c.x - gap,    c.y + lw * .5f }, col);
    dl->AddRectFilled({ c.x + gap,    c.y - lw * .5f }, { c.x + gap + ll, c.y + lw * .5f }, col);
}


// ═════════════════════════════════════════════════════════════════════════════
//
//  ██████  ██████  ███████ ██████  ██████  ██  █████  ███    ██
//  ██    █ ██   ██ ██      ██   ██ ██   ██ ██ ██   ██ ████   ██
//  ██    █ ██████  ███████ ██   ██ ██████  ██ ███████ ██ ██  ██
//  ██    █ ██   ██      ██ ██   ██ ██      ██ ██   ██ ██  ██ ██
//  ██████  ██████  ███████ ██████  ██      ██ ██   ██ ██   ████
//
//  "Obsidian"  —  amber-on-near-black, vertical icon sidebar, animated toggles
//
// ═════════════════════════════════════════════════════════════════════════════


// ─────────────────────────────────────────────────────────────────────────────
//  Color palette
// ─────────────────────────────────────────────────────────────────────────────
namespace Clr
{
    // surfaces
    static constexpr ImU32 Win = IM_COL32(11, 11, 16, 255);  // outermost window
    static constexpr ImU32 Surface = IM_COL32(17, 17, 23, 255);  // content area bg
    static constexpr ImU32 Card = IM_COL32(23, 23, 31, 255);  // raised card bg
    static constexpr ImU32 CardHov = IM_COL32(29, 29, 40, 255);  // card hover state
    static constexpr ImU32 Border = IM_COL32(38, 38, 52, 255);  // card borders
    static constexpr ImU32 BorderDim = IM_COL32(24, 24, 33, 255);  // divider between sidebar/content
    static constexpr ImU32 Divider = IM_COL32(32, 32, 44, 255);  // interior section divider

    // accent  (warm amber — precision-instrument, not aggressive)
    static constexpr ImU32 Accent = IM_COL32(232, 175, 67, 255);
    static constexpr ImU32 AccentDim = IM_COL32(80, 58, 20, 255);
    static constexpr ImU32 AccentHov = IM_COL32(245, 196, 100, 255);
    static constexpr ImU32 AccentFg = IM_COL32(255, 210, 120, 255);  // amber on text

    // text
    static constexpr ImU32 Text = IM_COL32(200, 200, 210, 255);
    static constexpr ImU32 TextMid = IM_COL32(128, 128, 148, 255);
    static constexpr ImU32 TextDim = IM_COL32(68, 68, 88, 255);
    static constexpr ImU32 White = IM_COL32(255, 255, 255, 255);

    // special
    static constexpr ImU32 Danger = IM_COL32(209, 75, 75, 255);
    static constexpr ImU32 DangerDim = IM_COL32(76, 26, 26, 255);
    static constexpr ImU32 DangerBrd = IM_COL32(140, 48, 48, 255);
    static constexpr ImU32 Track = IM_COL32(38, 38, 54, 255);  // toggle/slider track
    static constexpr ImU32 Transp = IM_COL32(0, 0, 0, 0);

    // linear lerp between two packed ABGR colors
    static ImU32 Lerp(ImU32 a, ImU32 b, float t)
    {
        auto ch = [](ImU32 c, int s) { return (int)((c >> s) & 0xFF); };
        auto mix = [&](int s) { return (int)(ch(a, s) * (1.f - t) + ch(b, s) * t); };
        return IM_COL32(mix(0), mix(8), mix(16), mix(24));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Per-widget animation  (toggle lerp stored by ImGuiID)
// ─────────────────────────────────────────────────────────────────────────────
namespace FX
{
    static std::unordered_map<ImGuiID, float> s_t;

    static float ToggleT(ImGuiID id, bool on)
    {
        float& v = s_t[id];
        v = ImLerp(v, on ? 1.f : 0.f,
            ImClamp(ImGui::GetIO().DeltaTime * 12.f, 0.f, 1.f));
        return v;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sidebar vector icons  (drawn with DrawList primitives — no texture atlas)
// ─────────────────────────────────────────────────────────────────────────────
namespace Icon
{
    static void Crosshair(ImDrawList* dl, ImVec2 c, float sz, ImU32 col)
    {
        const float r = sz * .38f, gap = sz * .13f, ll = sz * .26f, lw = 1.5f;
        dl->AddCircle(c, r, col, 32, lw);
        dl->AddCircleFilled(c, 2.0f, col);
        dl->AddLine({ c.x,       c.y - r - gap }, { c.x,       c.y - r - gap - ll }, col, lw);
        dl->AddLine({ c.x,       c.y + r + gap }, { c.x,       c.y + r + gap + ll }, col, lw);
        dl->AddLine({ c.x - r - gap, c.y }, { c.x - r - gap - ll, c.y }, col, lw);
        dl->AddLine({ c.x + r + gap, c.y }, { c.x + r + gap + ll, c.y }, col, lw);
    }

    static void Eye(ImDrawList* dl, ImVec2 c, float sz, ImU32 col)
    {
        const float rx = sz * .44f, ry = sz * .22f, lw = 1.5f;
        const ImVec2 L = { c.x - rx,c.y }, R = { c.x + rx,c.y };
        const ImVec2 T = { c.x,c.y - ry }, B = { c.x,c.y + ry };
        dl->AddBezierCubic(L, { L.x + rx * .65f,T.y - ry * .3f }, { R.x - rx * .65f,T.y - ry * .3f }, R, col, lw);
        dl->AddBezierCubic(L, { L.x + rx * .65f,B.y + ry * .3f }, { R.x - rx * .65f,B.y + ry * .3f }, R, col, lw);
        dl->AddCircleFilled(c, sz * .13f, col);
    }

    static void Globe(ImDrawList* dl, ImVec2 c, float sz, ImU32 col)
    {
        const float r = sz * .40f, cx = sz * .22f, lw = 1.5f;
        dl->AddCircle(c, r, col, 32, lw);
        dl->AddLine({ c.x - r, c.y }, { c.x + r, c.y }, col, lw);
        dl->AddBezierCubic({ c.x,c.y - r }, { c.x + cx,c.y - r * .5f }, { c.x + cx,c.y + r * .5f }, { c.x,c.y + r }, col, lw);
        dl->AddBezierCubic({ c.x,c.y - r }, { c.x - cx,c.y - r * .5f }, { c.x - cx,c.y + r * .5f }, { c.x,c.y + r }, col, lw);
    }

    static void Layers(ImDrawList* dl, ImVec2 c, float sz, ImU32 col)
    {
        const float hw = sz * .35f, hh = sz * .07f, gap = sz * .17f;
        for (int i = -1; i <= 1; i++)
            dl->AddRectFilled({ c.x - hw, c.y + i * gap - hh }, { c.x + hw, c.y + i * gap + hh }, col, 1.f);
    }

    static void Gear(ImDrawList* dl, ImVec2 c, float sz, ImU32 col)
    {
        const float ri = sz * .28f, ro = sz * .42f;
        dl->AddCircle(c, ri, col, 24, 1.5f);
        for (int i = 0; i < 6; i++)
        {
            const float a = (float)i / 6.f * 6.2832f;
            const float a1 = a - .24f, a2 = a + .24f;
            dl->AddQuadFilled(
                { c.x + cosf(a1) * ri, c.y + sinf(a1) * ri },
                { c.x + cosf(a2) * ri, c.y + sinf(a2) * ri },
                { c.x + cosf(a2) * ro, c.y + sinf(a2) * ro },
                { c.x + cosf(a1) * ro, c.y + sinf(a1) * ro }, col);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sidebar tab renderer
// ─────────────────────────────────────────────────────────────────────────────
namespace Sidebar
{
    static constexpr float kW = 46.f;
    static constexpr float kTabH = 48.f;
    static constexpr float kLogoH = 46.f;

    using IconFn = void(*)(ImDrawList*, ImVec2, float, ImU32);

    // Returns true if this tab was clicked this frame.
    static bool Tab(ImDrawList* dl, ImVec2 wp, float startY,
        int idx, int cur, IconFn icon)
    {
        const bool   active = (idx == cur);
        const ImVec2 tMin = { wp.x, wp.y + startY + idx * kTabH };
        const ImVec2 tMax = tMin + ImVec2(kW, kTabH);
        const ImVec2 center = { tMin.x + kW * .5f, tMin.y + kTabH * .5f };

        if (active)
            dl->AddRectFilled(tMin, tMax, IM_COL32(23, 23, 32, 255));

        // active accent bar — 3 px left edge
        if (active)
            dl->AddRectFilled({ tMin.x, tMin.y + 7.f }, { tMin.x + 3.f, tMax.y - 7.f }, Clr::Accent, 2.f);

        ImGui::SetCursorScreenPos(tMin);
        ImGui::PushID(idx);
        ImGui::InvisibleButton("##tab", { kW, kTabH });
        const bool hov = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();
        ImGui::PopID();

        ImU32 iconCol = active ? Clr::Accent
            : (hov ? Clr::TextMid : Clr::TextDim);
        icon(dl, center, 22.f, iconCol);

        return clicked;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Widget library  —  namespace UI
// ─────────────────────────────────────────────────────────────────────────────
namespace UI
{
    static constexpr float kTW = 30.f;  // toggle width
    static constexpr float kTH = 16.f;  // toggle height
    static constexpr float kTrkH = 3.f;  // slider track thickness
    static constexpr float kColW = 14.f;  // color swatch size
    static constexpr float kColH = 14.f;

    // ── Section label  ───────────────────────────────────────────────────────
    // 2 px amber left bar + uppercase muted text + divider line below.
    static void SectionLabel(const char* text)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 p = ImGui::GetCursorScreenPos();

        dl->AddRectFilled(p, { p.x + 2.f, p.y + ImGui::GetFontSize() }, Clr::Accent);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.f);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::TextDim), "%s", text);

        ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::ColorConvertU32ToFloat4(Clr::Divider));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Dummy({ 0.f, 3.f });
    }

    // ── Gap / spacing helper  ────────────────────────────────────────────────
    static void Gap(float px = 4.f) { ImGui::Dummy({ 0.f, px }); }

    // ── Toggle switch  ───────────────────────────────────────────────────────
    // 30×16 px animated pill.  Returns true when clicked.
    static bool Toggle(const char* label, bool* v)
    {
        ImGui::PushID(label);
        const ImVec2   p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##t", { kTW, kTH });
        const bool clicked = ImGui::IsItemClicked();
        const bool hov = ImGui::IsItemHovered();
        if (clicked) *v = !*v;

        const float t = FX::ToggleT(ImGui::GetItemID(), *v);
        const ImU32 trk = Clr::Lerp(Clr::Track, Clr::AccentDim, t);

        // track
        dl->AddRectFilled(p, p + ImVec2(kTW, kTH), trk, kTH * .5f);
        // amber outline when on
        if (t > .05f)
            dl->AddRect(p, p + ImVec2(kTW, kTH),
                IM_COL32(232, 175, 67, (int)(t * 90.f)), kTH * .5f, 0, 1.f);
        // thumb
        const float tx = p.x + 2.f + t * (kTW - kTH);
        const float tr = (kTH - 4.f) * .5f;
        const float cy = p.y + kTH * .5f;
        dl->AddCircleFilled({ tx + tr, cy }, tr,
            hov ? Clr::White : IM_COL32(218, 218, 228, 255), 16);

        // label on same line, vertically centred
        ImGui::SameLine(0.f, 8.f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (kTH - ImGui::GetFontSize()) * .5f);
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(*v ? Clr::Text : Clr::TextMid),
            "%s", label);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (kTH - ImGui::GetFontSize()) * .5f);

        ImGui::PopID();
        return clicked;
    }

    // ── Toggle + inline right-side color swatch  ─────────────────────────────
    static bool ToggleColor(const char* tLabel, bool* v,
        const char* cLabel, float col[4])
    {
        const bool changed = Toggle(tLabel, v);
        ImGui::SameLine(ImGui::GetContentRegionMax().x - kColW);
        ImGui::PushID(cLabel);
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            const ImU32  sw = IM_COL32((int)(col[0] * 255), (int)(col[1] * 255),
                (int)(col[2] * 255), (int)(col[3] * 255));
            dl->AddRectFilled(p, p + ImVec2(kColW, kColH), sw, 2.f);
            dl->AddRect(p, p + ImVec2(kColW, kColH), Clr::Border, 2.f);
            ImGui::InvisibleButton("##s", { kColW, kColH });
            if (ImGui::IsItemClicked()) ImGui::OpenPopup("##cp");
            ImGui::SetNextWindowPos({ p.x + kColW + 4.f, p.y - 4.f });
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(Clr::Card));
            ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Clr::Border));
            if (ImGui::BeginPopup("##cp")) {
                ImGui::ColorPicker4("##pk", col,
                    ImGuiColorEditFlags_NoSidePreview |
                    ImGuiColorEditFlags_AlphaBar |
                    ImGuiColorEditFlags_PickerHueBar);
                ImGui::EndPopup();
            }
            ImGui::PopStyleColor(2);
        }
        ImGui::PopID();
        return changed;
    }

    // ── Standalone color swatch (for inline multi-color rows)  ───────────────
    static void ColorEdit4(const char* label, float col[4])
    {
        ImGui::PushID(label);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImU32  sw = IM_COL32((int)(col[0] * 255), (int)(col[1] * 255),
            (int)(col[2] * 255), (int)(col[3] * 255));
        dl->AddRectFilled(p, p + ImVec2(kColW, kColH), sw, 2.f);
        dl->AddRect(p, p + ImVec2(kColW, kColH), Clr::Border, 2.f);
        ImGui::InvisibleButton("##sw", { kColW, kColH });
        if (ImGui::IsItemClicked()) ImGui::OpenPopup("##cpe");
        ImGui::SetNextWindowPos({ p.x + kColW + 4.f, p.y - 4.f });
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(Clr::Card));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Clr::Border));
        if (ImGui::BeginPopup("##cpe")) {
            ImGui::ColorPicker4("##pk", col,
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_AlphaBar |
                ImGuiColorEditFlags_PickerHueBar);
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopID();
    }

    // ── Two color swatches anchored right  ───────────────────────────────────
    static void DualColor(const char* la, float* ca, const char* lb, float* cb)
    {
        ImGui::SameLine(ImGui::GetContentRegionMax().x - kColW * 2.f - 4.f);
        ColorEdit4(la, ca);
        ImGui::SameLine(0.f, 4.f);
        ColorEdit4(lb, cb);
    }

    // ── Three color swatches anchored right  ─────────────────────────────────
    static void TriColor(const char* la, float* ca, const char* lb, float* cb, const char* lc, float* cc)
    {
        ImGui::SameLine(ImGui::GetContentRegionMax().x - kColW * 3.f - 8.f);
        ColorEdit4(la, ca); ImGui::SameLine(0.f, 4.f);
        ColorEdit4(lb, cb); ImGui::SameLine(0.f, 4.f);
        ColorEdit4(lc, cc);
    }

    // ── Slider Float  ────────────────────────────────────────────────────────
    // label + value on one line; track + handle below.
    static bool SliderFloat(const char* label, float* v, float mn, float mx)
    {
        ImGui::PushID(label);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float avail = ImGui::GetContentRegionAvail().x;

        // label row
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::TextMid), "%s", label);
        char buf[32]; snprintf(buf, sizeof(buf), "%.2f", *v);
        ImGui::SameLine(avail - ImGui::CalcTextSize(buf).x);
        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::AccentFg), "%s", buf);

        // track
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float  w = ImGui::GetContentRegionAvail().x;
        const float  cy = p.y + 10.f;  // vertical center of hit area

        ImGui::InvisibleButton("##sl", { w, 20.f });
        const bool active = ImGui::IsItemActive();
        const bool hov = ImGui::IsItemHovered();
        if (active)
        {
            float rel = (ImGui::GetIO().MousePos.x - p.x) / w;
            *v = mn + ImClamp(rel, 0.f, 1.f) * (mx - mn);
        }
        const float t = (*v - mn) / (mx - mn);

        // full track
        dl->AddRectFilled({ p.x, cy - kTrkH * .5f }, { p.x + w, cy + kTrkH * .5f }, Clr::Track, kTrkH);
        // filled portion
        dl->AddRectFilled({ p.x, cy - kTrkH * .5f }, { p.x + w * t, cy + kTrkH * .5f },
            active ? Clr::AccentHov : Clr::Accent, kTrkH);
        // handle
        const float hh = active ? 7.5f : (hov ? 6.5f : 5.5f);
        const float hx = p.x + w * t;
        dl->AddRectFilled({ hx - hh * .5f, cy - hh }, { hx + hh * .5f, cy + hh },
            active ? Clr::AccentHov : Clr::Accent, 2.f);

        ImGui::PopID();
        return ImGui::IsItemDeactivatedAfterEdit();
    }

    // ── Slider Int  ──────────────────────────────────────────────────────────
    static bool SliderInt(const char* label, int* v, int mn, int mx)
    {
        float fv = (float)*v;
        bool  r = SliderFloat(label, &fv, (float)mn, (float)mx);
        *v = (int)roundf(fv);
        return r;
    }

    // ── Combo  ───────────────────────────────────────────────────────────────
    static bool Combo(const char* label, int* idx, const std::vector<const char*>& items)
    {
        ImGui::PushID(label);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float avail = ImGui::GetContentRegionAvail().x;

        ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::TextMid), "%s", label);

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float  w = avail, h = 20.f;

        ImGui::InvisibleButton("##cb", { w, h });
        const bool hov = ImGui::IsItemHovered();
        const bool clicked = ImGui::IsItemClicked();

        dl->AddRectFilled(p, p + ImVec2(w, h), hov ? Clr::CardHov : Clr::Card, 3.f);
        dl->AddRect(p, p + ImVec2(w, h), Clr::Border, 3.f, 0, 1.f);

        const char* cur = (*idx >= 0 && *idx < (int)items.size()) ? items[*idx] : "---";
        dl->AddText({ p.x + 7.f, p.y + (h - ImGui::GetFontSize()) * .5f }, Clr::Text, cur);

        // chevron ▾
        const float cx = p.x + w - 13.f, cy = p.y + h * .5f;
        dl->AddTriangleFilled({ cx - 4.f,cy - 2.f }, { cx + 4.f,cy - 2.f }, { cx,cy + 3.f }, Clr::TextMid);

        bool changed = false;
        if (clicked) ImGui::OpenPopup("##cop");
        ImGui::SetNextWindowPos({ p.x, p.y + h + 2.f });
        ImGui::SetNextWindowSize({ w, 0.f });
        ImGui::SetNextWindowSizeConstraints({ w,0.f }, { w,160.f });
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(Clr::Card));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Clr::Border));
        if (ImGui::BeginPopup("##cop"))
        {
            for (int i = 0; i < (int)items.size(); i++)
            {
                const bool sel = (i == *idx);
                ImGui::PushStyleColor(ImGuiCol_Header,
                    ImGui::ColorConvertU32ToFloat4(sel ? Clr::AccentDim : Clr::Transp));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                    ImGui::ColorConvertU32ToFloat4(Clr::AccentDim));
                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::ColorConvertU32ToFloat4(sel ? Clr::AccentFg : Clr::Text));
                if (ImGui::Selectable(items[i], sel))
                {
                    *idx = i; changed = true; ImGui::CloseCurrentPopup();
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopID();
        return changed;
    }

    // ── KeyBind  ─────────────────────────────────────────────────────────────
    // [KEY] [HOLD/TOGG]  — click KEY to enter listen mode, press key to bind.
    static float KeyBindWidth(ImGuiKey key, ImKeyBindMode mode)
    {
        const char* kn = ImGui::GetKeyName(key);
        const char* mn = (mode == ImKeyBindMode_Hold) ? "HOLD" : "TOGG";
        const float kw = ImMax(ImGui::CalcTextSize(kn).x + 14.f, 38.f);
        const float mw = ImGui::CalcTextSize(mn).x + 10.f;
        return kw + 3.f + mw;
    }
    static void KeyBind(const char* id, ImGuiKey* key, ImKeyBindMode* mode)
    {
        static bool    s_listen = false;
        static ImGuiID s_ownerId = 0;
        ImGui::PushID(id);
        const ImGuiID self = ImGui::GetID("##kb");

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float h = 18.f;
        const char* kn = ImGui::GetKeyName(*key);
        const char* mn = (*mode == ImKeyBindMode_Hold) ? "HOLD" : "TOGG";
        const float kw = ImMax(ImGui::CalcTextSize(kn).x + 14.f, 38.f);
        const float mw = ImGui::CalcTextSize(mn).x + 10.f;
        const bool  listening = s_listen && (s_ownerId == self);

        // key button
        const ImVec2 kp = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##k", { kw, h });
        if (ImGui::IsItemClicked()) { s_listen = true; s_ownerId = self; }
        if (listening) {
            for (int k = ImGuiKey_Tab; k < ImGuiKey_COUNT; k++)
                if (ImGui::IsKeyPressed((ImGuiKey)k, false))
                {
                    *key = (ImGuiKey)k; s_listen = false; break;
                }
        }
        dl->AddRectFilled(kp, kp + ImVec2(kw, h), listening ? Clr::AccentDim : Clr::Card, 3.f);
        dl->AddRect(kp, kp + ImVec2(kw, h), listening ? Clr::Accent : Clr::Border, 3.f, 0, 1.f);
        dl->AddText({ kp.x + (kw - ImGui::CalcTextSize(kn).x) * .5f,
                     kp.y + (h - ImGui::GetFontSize()) * .5f },
            listening ? Clr::AccentFg : Clr::Text, kn);

        ImGui::SameLine(0.f, 3.f);

        // mode button
        const ImVec2 mp = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##m", { mw, h });
        const bool mhov = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            *mode = (*mode == ImKeyBindMode_Hold) ? ImKeyBindMode_Toggle : ImKeyBindMode_Hold;
        dl->AddRectFilled(mp, mp + ImVec2(mw, h), mhov ? Clr::CardHov : Clr::Card, 3.f);
        dl->AddRect(mp, mp + ImVec2(mw, h), Clr::Border, 3.f, 0, 1.f);
        dl->AddText({ mp.x + (mw - ImGui::CalcTextSize(mn).x) * .5f,
                     mp.y + (h - ImGui::GetFontSize()) * .5f }, Clr::TextMid, mn);

        ImGui::PopID();
    }

    // ── Action button (full-width or explicit width)  ─────────────────────────
    static bool Button(const char* label, ImU32 bg, ImU32 brd, ImU32 txt,
        float w = -1.f, float h = 22.f)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float W = (w < 0.f) ? ImGui::GetContentRegionAvail().x : w;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton(label, { W,h });
        const bool hov = ImGui::IsItemHovered();
        const bool act = ImGui::IsItemActive();
        const ImU32 fill = act ? Clr::Lerp(bg, Clr::White, .06f)
            : (hov ? Clr::Lerp(bg, Clr::White, .10f) : bg);
        dl->AddRectFilled(p, p + ImVec2(W, h), fill, 3.f);
        dl->AddRect(p, p + ImVec2(W, h), brd, 3.f, 0, 1.f);
        const float tw = ImGui::CalcTextSize(label).x;
        dl->AddText({ p.x + (W - tw) * .5f, p.y + (h - ImGui::GetFontSize()) * .5f }, txt, label);
        return ImGui::IsItemClicked();
    }

    // ── Card  ─────────────────────────────────────────────────────────────────
    // Styled BeginChild wrapper.  Optional title draws a SectionLabel inside.
    struct Card {
        static bool Begin(const char* id, ImVec2 size, const char* title = nullptr)
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(Clr::Card));
            ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Clr::Border));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.f, 9.f));
            const bool open = ImGui::BeginChild(id, size, true,
                ImGuiWindowFlags_NoScrollbar);
            if (open && title) { SectionLabel(title); Gap(2.f); }
            return open;
        }
        static void End()
        {
            ImGui::EndChild();
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);
        }
    };

}  // namespace UI


// ═════════════════════════════════════════════════════════════════════════════
//  Render_Menu  —  Obsidian layout
// ═════════════════════════════════════════════════════════════════════════════

void Graphics::Render_Menu()
{
    static int Section = 0;

    ImGuiIO& IO = ImGui::GetIO();
    ImGuiStyle& Style = ImGui::GetStyle();

    constexpr float kWinW = 590.f, kWinH = 546.f;

    ImGui::SetNextWindowSize({ kWinW, kWinH }, ImGuiCond_Once);
    ImGui::SetNextWindowPos(IO.DisplaySize / 2.f, ImGuiCond_Once, { 0.5f, 0.5f });

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(Clr::Win));

    const bool open = ImGui::Begin("##obsidian", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBackground);

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    if (!open) { ImGui::End(); return; }

    const ImVec2 WP = ImGui::GetWindowPos();
    const ImVec2 WS = ImGui::GetWindowSize();
    ImDrawList* DL = ImGui::GetWindowDrawList();

    // ── Outer frame  ─────────────────────────────────────────────────────────
    DL->AddRectFilled(WP, WP + WS, Clr::Win);
    DL->AddRect(WP, WP + WS, Clr::Border, 0.f, 0, 1.f);
    // inner accent border (amber, very subtle)
    DL->AddRect(WP + ImVec2(1.f, 1.f), WP + WS - ImVec2(1.f, 1.f),
        IM_COL32(232, 175, 67, 18), 0.f, 0, 1.f);

    // ── Sidebar background  ───────────────────────────────────────────────────
    constexpr float sbW = Sidebar::kW;
    DL->AddRectFilled(WP, { WP.x + sbW, WP.y + WS.y }, Clr::Win);
    DL->AddLine({ WP.x + sbW, WP.y + 1.f }, { WP.x + sbW, WP.y + WS.y - 1.f }, Clr::BorderDim, 1.f);

    // ── Brand mark  ──────────────────────────────────────────────────────────
    {
        const ImVec2 c = { WP.x + sbW * .5f, WP.y + Sidebar::kLogoH * .5f };
        DL->AddCircleFilled(c, 13.f, Clr::AccentDim, 32);
        DL->AddCircle(c, 13.f, Clr::Accent, 32, 1.5f);
        const float arm = 6.f;
        DL->AddLine({ c.x - arm,c.y - arm }, { c.x + arm,c.y + arm }, Clr::Accent, 2.f);
        DL->AddLine({ c.x + arm,c.y - arm }, { c.x - arm,c.y + arm }, Clr::Accent, 2.f);
        DL->AddLine({ WP.x + 7.f, WP.y + Sidebar::kLogoH },
            { WP.x + sbW - 7.f, WP.y + Sidebar::kLogoH }, Clr::Divider, 1.f);
    }

    // ── Sidebar tabs  ────────────────────────────────────────────────────────
    using Fn = void(*)(ImDrawList*, ImVec2, float, ImU32);
    constexpr Fn icons[] = { Icon::Crosshair, Icon::Eye, Icon::Globe, Icon::Layers, Icon::Gear };
    constexpr int kTabs = 5;
    const float   tabY = Sidebar::kLogoH + 5.f;

    for (int i = 0; i < kTabs; i++)
        if (Sidebar::Tab(DL, WP, tabY, i, Section, icons[i]))
            Section = i;

    // ── Content area  ────────────────────────────────────────────────────────
    constexpr float kHeaderH = 32.f;
    const float contentX = WP.x + sbW + 1.f;
    const float contentW = WS.x - sbW - 1.f;
    const float bodyY = WP.y + kHeaderH + 1.f;
    const float bodyH = WS.y - kHeaderH - 1.f;

    // header bar
    DL->AddRectFilled({ contentX, WP.y }, { contentX + contentW, WP.y + kHeaderH }, Clr::Surface);
    DL->AddLine({ contentX, WP.y + kHeaderH }, { contentX + contentW, WP.y + kHeaderH }, Clr::Divider, 1.f);

    // section title + brand in header
    constexpr const char* kTabTitles[] = { "AIMBOT","VISUALS","WORLD","MISC","SETTINGS" };
    {
        const ImVec2 tp = { contentX + 12.f, WP.y + (kHeaderH - ImGui::GetFontSize()) * .5f };
        if (Tahoma_BoldXP)
            DL->AddText(Tahoma_BoldXP, 0.f, tp, Clr::Accent, kTabTitles[Section]);
        else
            DL->AddText(tp, Clr::Accent, kTabTitles[Section]);
        const char* brand = "xexternal.de";
        const float bw = ImGui::CalcTextSize(brand).x;
        DL->AddText({ contentX + contentW - bw - 10.f, tp.y }, Clr::TextDim, brand);
    }

    // body surface
    DL->AddRectFilled({ contentX, bodyY }, { contentX + contentW, WP.y + WS.y }, Clr::Surface);

    // position cursor at body origin, push clip rect, begin group
    constexpr float kPad = 8.f;
    ImGui::SetCursorScreenPos({ contentX + kPad, bodyY + kPad });
    ImGui::PushClipRect({ contentX, bodyY }, { contentX + contentW, WP.y + WS.y }, true);
    ImGui::BeginGroup();

    const float bInW = contentW - kPad * 2.f;           // inner usable width
    const float bInH = bodyH - kPad * 2.f;           // inner usable height
    const float halfW = (bInW - 6.f) * .5f;            // half-width card
    const float halfH = (bInH - 6.f) * .5f;            // half-height card


    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 0  —  AIMBOT
    // ═══════════════════════════════════════════════════════════════════════
    if (Section == 0)
    {
        // ── Row 1 ────────────────────────────────────────────────────────────
        if (UI::Card::Begin("##abt", { halfW, halfH }, "AIMBOT"))
        {
            UI::Toggle("Enabled", &Globals::Aimbot::Enabled);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - UI::KeyBindWidth(Globals::Aimbot::Aimbot_Key, Globals::Aimbot::Aimbot_Mode) - 4.f);
            UI::KeyBind("##abk", &Globals::Aimbot::Aimbot_Key, &Globals::Aimbot::Aimbot_Mode);

            UI::Gap(2.f);
            UI::Toggle("Sticky Aim", &Globals::Aimbot::AimbotSticky);
            UI::Toggle("Knocked Check", &Globals::Aimbot::KnockedCheck);
            UI::Gap(4.f);

            UI::Combo("Type", &Globals::Aimbot::Aimbot_type, { "Mouse","Camera" });
            UI::Gap(2.f);
            UI::Combo("HitPart", &Globals::Aimbot::HitPart, { "Head","Torso","LowerTorso" });
            UI::Gap(6.f);

            if (Globals::Aimbot::Aimbot_type == 0) {
                UI::SliderFloat("Smooth X", &Globals::Aimbot::Mouse::Smoothing_X, 0.f, 12.f);
                UI::SliderFloat("Smooth Y", &Globals::Aimbot::Mouse::Smoothing_Y, 0.f, 12.f);
                UI::SliderFloat("Sensitivity", &Globals::Aimbot::Mouse::Mouse_Sensitivty, 0.f, 5.f);
            }
            else {
                UI::SliderFloat("Smooth X", &Globals::Aimbot::Camera::Smoothing_X, 0.f, 12.f);
                UI::SliderFloat("Smooth Y", &Globals::Aimbot::Camera::Smoothing_Y, 0.f, 12.f);
            }
        }
        UI::Card::End();

        ImGui::SameLine(0.f, 6.f);

        if (UI::Card::Begin("##sil", { halfW, halfH }, "SILENT AIM"))
        {
            UI::Toggle("Enabled", &Globals::Silent::Enabled);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - UI::KeyBindWidth(Globals::Silent::Silent_Key, Globals::Silent::Silent_Mode) - 4.f);
            UI::KeyBind("##sik", &Globals::Silent::Silent_Key, &Globals::Silent::Silent_Mode);

            UI::Gap(2.f);
            UI::Toggle("Sticky Aim", &Globals::Silent::StickyAim);
            UI::Toggle("Spoof Mouse", &Globals::Silent::SpoofMouse);
            UI::Toggle("Knocked Check", &Globals::Silent::KnockedCheck);
            UI::Gap(4.f);

            UI::Combo("Hit Part", &Globals::Silent::AimPart, { "Head","Torso","LowerTorso" });
        }
        UI::Card::End();

        // ── Row 2 ────────────────────────────────────────────────────────────
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.f);
        const float r2H = halfH - 6.f;

        if (UI::Card::Begin("##abfov", { halfW, r2H }, "AIMBOT FOV"))
        {
            if (Globals::Aimbot::Enabled)
            {
                UI::ToggleColor("Draw FOV", &Globals::Aimbot::DrawFov,
                    "##abfc", Globals::Aimbot::FovColor);
                if (Globals::Aimbot::DrawFov) {
                    UI::SliderFloat("Radius", &Globals::Aimbot::FovSize, 1.f, 500.f);
                    UI::Toggle("Spin", &Globals::Aimbot::FovSpin);
                    if (Globals::Aimbot::FovSpin)
                        UI::SliderInt("Spin Speed", &Globals::Aimbot::FovSpinSpeed, 1, 5);
                    UI::Toggle("Constrain to FOV", &Globals::Aimbot::useFov);
                }
            }
            else {
                ImGui::Dummy({ 0.f, 4.f });
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::TextDim),
                    "Enable aimbot first");
            }
        }
        UI::Card::End();

        ImGui::SameLine(0.f, 6.f);

        if (UI::Card::Begin("##sifov", { halfW, r2H }, "SILENT FOV"))
        {
            if (Globals::Silent::Enabled)
            {
                UI::ToggleColor("Draw FOV", &Globals::Silent::DrawFov,
                    "##sifc", Globals::Silent::FovColor);
                UI::Gap(2.f);
                UI::Toggle("Gun-Based FOV", &Globals::Silent::GunBasedFov);
                if (Globals::Silent::GunBasedFov) {
                    UI::SliderFloat("Default", &Globals::Silent::Fov, 0.f, 300.f);
                    UI::SliderFloat("Dbl Barrel", &Globals::Silent::FovDoubleBarrel, 0.f, 300.f);
                    UI::SliderFloat("Tactical", &Globals::Silent::FovTacticalShotgun, 0.f, 300.f);
                    UI::SliderFloat("Revolver", &Globals::Silent::FovRevolver, 0.f, 300.f);
                }
                else {
                    UI::SliderFloat("Static FOV", &Globals::Silent::Fov, 0.f, 500.f);
                }
                UI::Toggle("Spin FOV", &Globals::Silent::FovSpin);
                if (Globals::Silent::FovSpin)
                    UI::SliderInt("Spin Speed", &Globals::Silent::FovSpinSpeed, 1, 5);
                UI::Toggle("Constrain to FOV", &Globals::Silent::UseFov);
            }
            else {
                ImGui::Dummy({ 0.f, 4.f });
                ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::TextDim),
                    "Enable silent aim first");
            }
        }
        UI::Card::End();
    }


    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 1  —  VISUALS
    // ═══════════════════════════════════════════════════════════════════════
    if (Section == 1)
    {
        if (UI::Card::Begin("##esp", { halfW, bInH }, "ESP TOGGLES"))
        {
            UI::Toggle("Master Enable", &Globals::Visuals::Enabled);
            UI::Gap(4.f);

            // BOX
            UI::SectionLabel("BOX");
            UI::ToggleColor("Box", &Globals::Visuals::Box, "##bxc", Globals::Visuals::Colors::Box);
            if (Globals::Visuals::Box) {
                UI::Combo("Style", &Globals::Visuals::Box_Type, { "Bounding","Corner" });
                UI::Gap(2.f);
                UI::Toggle("Box Fill", &Globals::Visuals::Box_Fill);
                if (Globals::Visuals::Box_Fill) {
                    UI::DualColor("##bft", Globals::Visuals::Colors::BoxFill_Top,
                        "##bfb", Globals::Visuals::Colors::BoxFill_Bottom);
                    UI::Toggle("Gradient", &Globals::Visuals::Box_Fill_Gradient);
                    if (Globals::Visuals::Box_Fill_Gradient) {
                        UI::Toggle("Rotation", &Globals::Visuals::Box_Fill_Gradient_Rotate);
                        if (Globals::Visuals::Box_Fill_Gradient_Rotate) {
                            UI::Combo("Rotation Type", &Globals::Visuals::Box_Fill_Type,
                                { "Side","Bottom","Spin" });
                            UI::SliderInt("Speed", &Globals::Visuals::BoxFillSpeed, 1, 5);
                        }
                    }
                }
            }
            UI::Gap(4.f);

            // HEALTH
            UI::SectionLabel("HEALTH");
            UI::ToggleColor("Health Bar", &Globals::Visuals::Healthbar,
                "##hbc", Globals::Visuals::Colors::Healthbar);
            if (Globals::Visuals::Healthbar) {
                UI::Combo("Bar Style", &Globals::Visuals::Healthbar_Type, { "Static","Gradient" });
                if (Globals::Visuals::Healthbar_Type == 1) {
                    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Clr::TextMid),
                        "Gradient (top \xE2\x80\x94 mid \xE2\x80\x94 bottom)");
                    UI::TriColor("##hbt", Globals::Visuals::Colors::Healthbar_Top,
                        "##hbm", Globals::Visuals::Colors::Healthbar_Middle,
                        "##hbb", Globals::Visuals::Colors::Healthbar_Bottom);
                }
                UI::SliderInt("Bar Gap", &Globals::Visuals::Gap, 1, 5);
                UI::SliderInt("Bar Thickness", &Globals::Visuals::Thickness, 1, 5);
            }
            UI::ToggleColor("Health Text", &Globals::Visuals::Health,
                "##htc", Globals::Visuals::Colors::Health);
            UI::Gap(4.f);

            // LABELS
            UI::SectionLabel("LABELS");
            UI::ToggleColor("Name", &Globals::Visuals::Name, "##nc", Globals::Visuals::Colors::Name);
            if (Globals::Visuals::Name)
                UI::Combo("Name Format", &Globals::Visuals::Name_Type,
                    { "Name","Display Name","Name & Display" });
            UI::ToggleColor("Distance", &Globals::Visuals::Distance, "##dc", Globals::Visuals::Colors::Distance);
            UI::ToggleColor("Rig Type", &Globals::Visuals::Rig_Type, "##rc", Globals::Visuals::Colors::Rig_Type);
            UI::ToggleColor("Tool", &Globals::Visuals::Tool, "##tc", Globals::Visuals::Colors::Tool);
            UI::Gap(4.f);

            // 3D
            UI::SectionLabel("3D");
            UI::ToggleColor("Skeleton", &Globals::Visuals::Skeleton, "##skc", Globals::Visuals::Colors::Skeleton);
            UI::Toggle("Chams", &Globals::Visuals::Chams);
            if (Globals::Visuals::Chams) {
                UI::DualColor("##chc", Globals::Visuals::Colors::Chams,
                    "##choc", Globals::Visuals::Colors::ChamsOutline);
                UI::Toggle("Fade", &Globals::Visuals::ChamsFade);
                if (Globals::Visuals::ChamsFade)
                    UI::SliderInt("Fade Speed", &Globals::Visuals::ChamsFadeSpeed, 1, 5);
            }
        }
        UI::Card::End();

        ImGui::SameLine(0.f, 6.f);

        if (UI::Card::Begin("##vopt", { halfW, bInH }, "OPTIONS"))
        {
            UI::SectionLabel("FILTERS");
            UI::Toggle("Exclude Team", &Globals::Settings::Team_Check);
            UI::Toggle("Exclude Client", &Globals::Settings::Client_Check);
            UI::Gap(4.f);
            UI::SectionLabel("RENDERING");
            UI::SliderFloat("Render Distance", &Globals::Visuals::Render_Distance, 0.f, 500.f);
        }
        UI::Card::End();
    }


    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 2  —  WORLD
    // ═══════════════════════════════════════════════════════════════════════
    if (Section == 2)
    {
        if (UI::Card::Begin("##wld", { bInW, bInH }, "WORLD MANIPULATION"))
        {
            UI::SectionLabel("SKYBOX");
            UI::Toggle("Skybox Changer", &Globals::World::Skybox);
            if (Globals::World::Skybox) {
                UI::Combo("Preset", &Globals::World::Skybox_Type, {
                    "xexternal.de","Space","Pink Sky","Minecraft","Night Cloudy",
                    "Sparkling Night","Winterness","Dark Crimson","Nebula","Tropical","Green Sky" });
                UI::Toggle("Rotation", &Globals::World::Rotate);
                if (Globals::World::Rotate)
                    UI::SliderFloat("Rotate Speed", &Globals::World::Skybox_Rotate_Speed, 0.f, 5.f);
            }
            UI::Gap(4.f);

            UI::SectionLabel("LIGHTING");
            UI::ToggleColor("Atmosphere", &Globals::World::Ambience,
                "##atmc", Globals::World::Colors::Ambience);
            UI::Toggle("Fog", &Globals::World::Fog);
            if (Globals::World::Fog) {
                ImGui::SameLine(ImGui::GetContentRegionMax().x - UI::kColW);
                UI::ColorEdit4("##fogc", Globals::World::Colors::Fog);
                UI::SliderFloat("Fog Distance", &Globals::World::Fog_Distance, 0.f, 1000.f);
            }
            UI::Toggle("Brightness", &Globals::World::Brightness);
            if (Globals::World::Brightness)
                UI::SliderFloat("Brightness", &Globals::World::BrightnessI, 0.f, 10.f);
            UI::Toggle("Exposure", &Globals::World::Exposure);
            if (Globals::World::Exposure)
                UI::SliderFloat("Exposure", &Globals::World::ExposureI, -3.f, 3.f);
            UI::Gap(4.f);

            UI::SectionLabel("CAMERA");
            UI::Toggle("Custom FOV", &Globals::World::FOV);
            if (Globals::World::FOV)
                UI::SliderFloat("FOV", &Globals::World::FOV_Distance, 70.f, 120.f);
        }
        UI::Card::End();
    }


    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 3  —  MISC
    // ═══════════════════════════════════════════════════════════════════════
    if (Section == 3)
    {
        if (UI::Card::Begin("##msc", { bInW * .5f, bInH }, "MISC"))
        {
            UI::SectionLabel("MOVEMENT");
            UI::Toggle("Fly", &Globals::Misc::Fly);
            ImGui::SameLine(ImGui::GetContentRegionMax().x - UI::KeyBindWidth(Globals::Misc::Fly_Key, Globals::Misc::Fly_Mode) - 4.f);
            UI::KeyBind("##flyk", &Globals::Misc::Fly_Key, &Globals::Misc::Fly_Mode);
            if (Globals::Misc::Fly)
                UI::SliderFloat("Fly Speed", &Globals::Misc::Fly_Speed, 0.f, 200.f);
        }
        UI::Card::End();
    }


    // ═══════════════════════════════════════════════════════════════════════
    //  TAB 4  —  SETTINGS
    // ═══════════════════════════════════════════════════════════════════════
    if (Section == 4)
    {
        static char           cfgName[128] = "";
        static std::vector<std::string> cfgList;
        static bool           cfgRefreshed = false;
        if (!cfgRefreshed) { ConfigManager::Refresh(cfgList); cfgRefreshed = true; }

        // ── Config card  ─────────────────────────────────────────────────────
        if (UI::Card::Begin("##cfg", { halfW, bInH }, "CONFIGS"))
        {
            // file list
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(Clr::Win));
            ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Clr::Border));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 3.f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
            ImGui::BeginChild("##cl", { ImGui::GetContentRegionAvail().x, 108.f }, true);
            for (auto& cfg : cfgList)
            {
                const bool sel = (strcmp(cfgName, cfg.c_str()) == 0);
                ImGui::PushStyleColor(ImGuiCol_Header,
                    ImGui::ColorConvertU32ToFloat4(Clr::AccentDim));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                    ImGui::ColorConvertU32ToFloat4(Clr::AccentDim));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive,
                    ImGui::ColorConvertU32ToFloat4(Clr::Accent));
                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::ColorConvertU32ToFloat4(sel ? Clr::AccentFg : Clr::TextMid));
                if (ImGui::Selectable(cfg.c_str(), sel))
                    strcpy_s(cfgName, sizeof(cfgName), cfg.c_str());
                ImGui::PopStyleColor(4);
            }
            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);

            UI::Gap(4.f);

            // name field
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(Clr::Win));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImGui::ColorConvertU32ToFloat4(Clr::CardHov));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImGui::ColorConvertU32ToFloat4(Clr::CardHov));
            ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(Clr::Border));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(Clr::Text));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##cn", cfgName, sizeof(cfgName));
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(5);

            UI::Gap(4.f);

            const float bw = (ImGui::GetContentRegionAvail().x - 8.f) / 3.f;
            if (UI::Button("Load", Clr::Card, Clr::Border, Clr::Text, bw)) {
                if (cfgName[0]) ConfigManager::Load(cfgName);
            }
            ImGui::SameLine(0.f, 4.f);
            if (UI::Button("Save", Clr::AccentDim, Clr::Accent, Clr::AccentFg, bw)) {
                if (cfgName[0]) { ConfigManager::Save(cfgName); ConfigManager::Refresh(cfgList); }
            }
            ImGui::SameLine(0.f, 4.f);
            if (UI::Button("Delete", Clr::DangerDim, Clr::DangerBrd, Clr::Danger, bw)) {
                if (cfgName[0]) {
                    ConfigManager::Delete(cfgName);
                    ConfigManager::Refresh(cfgList);
                    cfgName[0] = '\0';
                }
            }
        }
        UI::Card::End();

        ImGui::SameLine(0.f, 6.f);

        // ── General card  ────────────────────────────────────────────────────
        if (UI::Card::Begin("##gen", { halfW, bInH }, "GENERAL"))
        {
            UI::SectionLabel("PERFORMANCE");
            UI::Toggle("Streamproof", &Globals::Settings::Streamproof);
            UI::Gap(2.f);
            UI::Combo("Mode", &Globals::Settings::Performance_Mode, { "Low","Medium","High" });

            // push cursor to bottom of card for the danger button
            const float remaining = ImGui::GetContentRegionAvail().y - 34.f;
            if (remaining > 0.f) ImGui::Dummy({ 0.f, remaining });

            if (UI::Button("  UNLOAD  ", Clr::DangerDim, Clr::DangerBrd, Clr::Danger,
                ImGui::GetContentRegionAvail().x, 26.f))
                ExitProcess(0);
        }
        UI::Card::End();
    }

    ImGui::EndGroup();
    ImGui::PopClipRect();
    ImGui::End();
}

// ───────────────────────────────────────────────d──────────────────────────────
//  Render_Visuals  (unchanged)
// ─────────────────────────────────────────────────────────────────────────────
void Graphics::Render_Visuals()
{
    DrawCursor();
    Visuals::RunService();
}