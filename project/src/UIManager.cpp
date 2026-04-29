#include "UIManager.h"
#include "CityBuilder.h"
#include "imgui.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <string>

namespace {
void formatHourVi(float hour, char* buf, size_t bufSize) {
    float h = std::fmod(hour, 24.0f);
    if (h < 0.0f) {
        h += 24.0f;
    }
    int hi = static_cast<int>(std::floor(h));
    int mi = static_cast<int>(std::floor((h - static_cast<float>(hi)) * 60.0f + 0.5f));
    int hd = hi;
    if (mi >= 60) {
        mi = 0;
        hd = (hd + 1) % 24;
    }
    std::snprintf(buf, bufSize, "%02d:%02d", hd, mi);
}

std::string toLower(std::string v) {
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return v;
}

bool matchQuery(const char* label, const char* query) {
    if (!query || !query[0]) return true;
    std::string l = toLower(label ? label : "");
    std::string q = toLower(query);
    return l.find(q) != std::string::npos;
}

enum class UIPreset {
    Day,
    Night,
    Rain,
    Sunny,
    Noir
};

void applyPreset(UIPreset preset, AppOptions& opts, Scene& scene, Camera& camera) {
    (void)camera;
    if (preset == UIPreset::Day) {
        opts.syncTimeOfDay = false;
        opts.timeOfDayHour = 13.1f;
        opts.rainIntensity = 0.0f;
        opts.windDir = glm::normalize(glm::vec2(-0.48f, -0.1f));
        opts.cloudHeight = 140.0f;
        scene.setWetness(opts.rainIntensity);
        scene.setFogDensity(0.012f);
        scene.setFogBaseHeight(0.0f);
        scene.setFogHeightFalloff(0.022f);
        scene.setFogColor(glm::vec3(0.54f, 0.62f, 0.74f));
        scene.setExposure(1.34f);
        scene.setWindowEmissive(0.8f);
        opts.bloomIntensity = 0.48f;
        opts.godRayIntensity = 0.7f;
        opts.colorGradeIntensity = 0.5f;
        opts.vignetteIntensity = 0.22f;
        opts.chromaticAberration = 0.008f;
        opts.ssaoIntensity = 0.9f;
        opts.ssrIntensity = 0.12f;
    }
    if (preset == UIPreset::Night) {
        opts.syncTimeOfDay = false;
        opts.timeOfDayHour = 22.1f;
        opts.rainIntensity = 0.1f;
        opts.windDir = glm::normalize(glm::vec2(-0.42f, -0.22f));
        opts.cloudHeight = 112.0f;
        scene.setWetness(opts.rainIntensity);
        scene.setFogDensity(0.14f);
        scene.setFogBaseHeight(0.0f);
        scene.setFogHeightFalloff(0.06f);
        scene.setFogColor(glm::vec3(0.12f, 0.16f, 0.24f));
        scene.setExposure(0.72f);
        scene.setWindowEmissive(3.6f);
        opts.bloomIntensity = 0.95f;
        opts.godRayIntensity = 0.1f;
        opts.colorGradeIntensity = 0.9f;
        opts.vignetteIntensity = 0.88f;
        opts.chromaticAberration = 0.016f;
        opts.ssaoIntensity = 1.2f;
        opts.ssrIntensity = 0.55f;
    }
    if (preset == UIPreset::Rain) {
        opts.syncTimeOfDay = false;
        opts.timeOfDayHour = 17.7f;
        opts.rainIntensity = 1.0f;
        opts.windDir = glm::normalize(glm::vec2(-1.05f, -0.45f));
        opts.cloudHeight = 80.0f;
        scene.setWetness(opts.rainIntensity);
        scene.setFogDensity(0.5f);
        scene.setFogBaseHeight(-2.0f);
        scene.setFogHeightFalloff(0.12f);
        scene.setFogColor(glm::vec3(0.20f, 0.26f, 0.36f));
        scene.setExposure(0.9f);
        scene.setWindowEmissive(2.8f);
        opts.bloomIntensity = 0.28f;
        opts.godRayIntensity = 0.03f;
        opts.colorGradeIntensity = 1.0f;
        opts.vignetteIntensity = 0.82f;
        opts.chromaticAberration = 0.01f;
        opts.ssrIntensity = 1.3f;
        opts.ssaoIntensity = 1.45f;
    }
    if (preset == UIPreset::Sunny) {
        opts.syncTimeOfDay = false;
        opts.timeOfDayHour = 10.0f;
        opts.rainIntensity = 0.0f;
        opts.windDir = glm::normalize(glm::vec2(-0.28f, -0.06f));
        opts.cloudHeight = 150.0f;
        scene.setWetness(opts.rainIntensity);
        scene.setFogDensity(0.004f);
        scene.setFogBaseHeight(0.0f);
        scene.setFogHeightFalloff(0.018f);
        scene.setFogColor(glm::vec3(0.60f, 0.66f, 0.76f));
        scene.setExposure(1.55f);
        scene.setWindowEmissive(0.6f);
        opts.bloomIntensity = 0.95f;
        opts.godRayIntensity = 1.1f;
        opts.colorGradeIntensity = 0.4f;
        opts.vignetteIntensity = 0.18f;
        opts.chromaticAberration = 0.006f;
        opts.ssaoIntensity = 0.85f;
        opts.ssrIntensity = 0.12f;
    }
    if (preset == UIPreset::Noir) {
        opts.syncTimeOfDay = false;
        opts.timeOfDayHour = 20.4f;
        opts.rainIntensity = 0.3f;
        opts.windDir = glm::normalize(glm::vec2(-0.75f, -0.32f));
        opts.cloudHeight = 92.0f;
        scene.setWetness(opts.rainIntensity);
        scene.setFogDensity(0.26f);
        scene.setFogBaseHeight(-0.2f);
        scene.setFogHeightFalloff(0.08f);
        scene.setFogColor(glm::vec3(0.10f, 0.12f, 0.18f));
        scene.setExposure(0.7f);
        scene.setWindowEmissive(3.2f);
        opts.bloomIntensity = 0.15f;
        opts.godRayIntensity = 0.05f;
        opts.colorGradeIntensity = 1.1f;
        opts.vignetteIntensity = 0.98f;
        opts.chromaticAberration = 0.0f;
        opts.ssrIntensity = 0.7f;
        opts.ssaoIntensity = 1.9f;
    }
}

void resetEnvironment(AppOptions& opts, Scene& scene) {
    AppOptions d;
    opts.syncTimeOfDay = d.syncTimeOfDay;
    opts.timeOfDayHour = d.timeOfDayHour;
    opts.rainIntensity = d.rainIntensity;
    opts.windDir = d.windDir;
    opts.cloudHeight = d.cloudHeight;
    scene.setWetness(opts.rainIntensity);
    scene.setFogDensity(0.0f);
    scene.setFogBaseHeight(0.0f);
    scene.setFogHeightFalloff(0.04f);
    scene.setFogColor(glm::vec3(0.45f, 0.52f, 0.62f));
}

void resetLighting(Scene& scene) {
    scene.setExposure(1.18f);
    scene.setSkyColor(glm::vec3(0.38f, 0.48f, 0.62f));
    scene.setGroundColor(glm::vec3(0.20f, 0.18f, 0.16f));
    scene.setSunColor(glm::vec3(1.05f, 0.97f, 0.88f));
    scene.setLightDir(glm::vec3(0.4f, -0.92f, 0.15f));
    scene.setWindowEmissive(1.5f);
}

void resetPost(AppOptions& opts) {
    AppOptions d;
    opts.bloomThreshold = d.bloomThreshold;
    opts.bloomIntensity = d.bloomIntensity;
    opts.godRayIntensity = d.godRayIntensity;
    opts.colorGradeIntensity = d.colorGradeIntensity;
    opts.vignetteIntensity = d.vignetteIntensity;
    opts.fxaaIntensity = d.fxaaIntensity;
    opts.chromaticAberration = d.chromaticAberration;
    opts.ssaoIntensity = d.ssaoIntensity;
    opts.ssrIntensity = d.ssrIntensity;
}

void resetSystem(AppOptions& opts, Scene& scene, Camera& camera, GLFWwindow* window) {
    AppOptions d;
    scene.m_debugDrawCarOnly = false;
    scene.m_debugCarIndex = 0;
    scene.setCarScale(100.0f);
    scene.setCarYOffset(0.0f);

    camera.setMoveSpeed(18.0f);
    camera.setMouseSensitivity(0.12f);
    camera.setFovDegrees(60.0f);

    opts.useGridLayout = d.useGridLayout;
    opts.gridRows = d.gridRows;
    opts.gridCols = d.gridCols;
    opts.gridSpaceX = d.gridSpaceX;
    opts.gridSpaceZ = d.gridSpaceZ;
    if (opts.useGridLayout) {
        scene.setInstanceTransforms(CityBuilder::buildGrid(opts.gridRows, opts.gridCols, opts.gridSpaceX, opts.gridSpaceZ, 0.0f));
    } else {
        scene.setInstanceTransforms({glm::mat4(1.0f)});
    }

    opts.wireframe = d.wireframe;
    glPolygonMode(GL_FRONT_AND_BACK, opts.wireframe ? GL_LINE : GL_FILL);
    opts.cullBack = d.cullBack;
    if (opts.cullBack) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    opts.vsync = d.vsync;
    if (window) glfwSwapInterval(opts.vsync ? 1 : 0);
    opts.clearCol[0] = d.clearCol[0];
    opts.clearCol[1] = d.clearCol[1];
    opts.clearCol[2] = d.clearCol[2];
}

void drawHotkeysOverlay(bool show, bool menuOpen, const ImVec4& colBg, const ImVec4& colAccent, const ImVec4& colText) {
    if (!show) return;
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 pos = ImVec2(12.0f, 12.0f);
    (void)io;
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(menuOpen ? 0.18f : 0.72f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(colBg.x, colBg.y, colBg.z, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(colAccent.x, colAccent.y, colAccent.z, 0.35f));
    ImGui::PushStyleColor(ImGuiCol_Text, colText);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##HotkeysOverlay", nullptr, flags);
    ImGui::TextColored(colAccent, u8"PHÍM TẮT");
    ImGui::Text(u8"Tab: mở/đóng menu");
    ImGui::Text(u8"WASD + Space/Shift: di chuyển");
    ImGui::Text(u8"Chuột: nhìn | Lăn: zoom");
    ImGui::Text(u8"F: lái xe | Esc: thoát");
    ImGui::End();

    ImGui::PopStyleColor(3);
}

}

void UIManager::render(AppOptions& opts, Scene& scene, Camera& camera, GLFWwindow* window, ImGuiIO& imguiIo) {
    static char search[64] = "";
    static bool searchInit = false;

    int& section = opts.uiSection;
    if (section < 0) section = 0;
    if (section > 3) section = 3;

    if (!searchInit) {
        std::snprintf(search, sizeof(search), "%s", opts.uiSearch.c_str());
        searchInit = true;
    }

    bool& showFavorites = opts.uiShowFavorites;
    bool& compactMode = opts.uiCompact;
    bool& showHotkeys = opts.uiShowHotkeys;
    auto& favorites = opts.uiFavorites;

    enum FavId {
        FavTime,
        FavRain,
        FavCloud,
        FavWind,
        FavFog,
        FavExposure,
        FavBloom,
        FavFov,
        FavSpeed,
        FavCount
    };

    static_assert(FavCount == AppOptions::kUiFavoriteCount, "UI favorites mismatch");

    const char* cameraModes[] = {
        u8"Tự do (Free)",
        u8"Theo dõi xe (Follow)",
        u8"Camera an ninh (CCTV)",
        u8"Lái xe (Drive)",
        u8"Quay phim (Cinematic)"
    };

    const ImVec4 colBg = ImVec4(0.06f, 0.07f, 0.08f, 0.98f);
    const ImVec4 colPanel = ImVec4(0.12f, 0.13f, 0.15f, 1.0f);
    const ImVec4 colPanelHover = ImVec4(0.18f, 0.19f, 0.22f, 1.0f);
    const ImVec4 colPanelActive = ImVec4(0.22f, 0.24f, 0.28f, 1.0f);
    const ImVec4 colAccent = ImVec4(0.20f, 0.78f, 0.68f, 1.0f);
    const ImVec4 colAccentStrong = ImVec4(0.23f, 0.86f, 0.74f, 1.0f);
    const ImVec4 colBorder = ImVec4(0.20f, 0.22f, 0.26f, 1.0f);
    const ImVec4 colText = ImVec4(0.93f, 0.94f, 0.96f, 1.0f);
    const ImVec4 colTextMuted = ImVec4(0.62f, 0.65f, 0.70f, 1.0f);

    drawHotkeysOverlay(showHotkeys, opts.showMenu, colBg, colAccentStrong, colText);
    if (!opts.showMenu) return;

    // Theme đậm, tương phản cao cho menu
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiStyle styleBackup = style;
    const float pad = compactMode ? 10.0f : 16.0f;
    const float framePadX = compactMode ? 8.0f : 10.0f;
    const float framePadY = compactMode ? 4.0f : 6.0f;
    const float itemX = compactMode ? 8.0f : 10.0f;
    const float itemY = compactMode ? 6.0f : 8.0f;

    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 10.0f;
    style.ScrollbarRounding = 10.0f;
    style.GrabRounding = 8.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;
    style.WindowPadding = ImVec2(pad, pad - 2.0f);
    style.FramePadding = ImVec2(framePadX, framePadY);
    style.ItemSpacing = ImVec2(itemX, itemY);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, colBg);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, colPanel);
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, colPanelActive);
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, colPanel);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, colPanel);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colPanelHover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colPanelActive);
    ImGui::PushStyleColor(ImGuiCol_Header, colPanel);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colPanelHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, colPanelActive);
    ImGui::PushStyleColor(ImGuiCol_Button, colPanel);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colPanelHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colPanelActive);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, colAccentStrong);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, colAccent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, colAccentStrong);
    ImGui::PushStyleColor(ImGuiCol_Border, colBorder);
    ImGui::PushStyleColor(ImGuiCol_Separator, colBorder);
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, colAccent);
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, colAccentStrong);
    ImGui::PushStyleColor(ImGuiCol_Text, colText);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, colTextMuted);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, colBg);
    const int colorCount = 23;

    ImGui::SetNextWindowBgAlpha(0.98f);
    ImVec2 screen = imguiIo.DisplaySize;
    float menuW = std::min(std::max(screen.x * 0.36f, 360.0f), 560.0f);
    float menuH = std::min(std::max(screen.y * 0.82f, 520.0f), 760.0f);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 420.0f), ImVec2(screen.x * 0.95f, screen.y * 0.95f));
    ImGui::SetNextWindowSize(ImVec2(menuW, menuH), ImGuiCond_FirstUseEver);
    ImGui::Begin(u8"Cài đặt", &opts.showMenu, ImGuiWindowFlags_NoCollapse);

    ImGui::SetWindowFontScale(compactMode ? 1.0f : 1.05f);
    ImGui::TextColored(colAccentStrong, u8"CÀI ĐẶT");
    ImGui::SameLine();
    ImGui::TextDisabled(u8"Tab: đóng/mở menu • Esc: thoát");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::Text(u8"FPS: %.0f", static_cast<double>(imguiIo.Framerate));
    ImGui::Separator();

    float contentWidth = ImGui::GetContentRegionAvail().x;
    float fieldWidth = std::min(260.0f, contentWidth);
    ImGui::PushItemWidth(fieldWidth);
    ImGui::Combo(u8"Góc máy", &opts.cameraMode, cameraModes, IM_ARRAYSIZE(cameraModes));
    ImGui::PopItemWidth();
    ImGui::Spacing();

    ImGui::PushItemWidth(fieldWidth);
    if (ImGui::InputText(u8"##search", search, sizeof(search))) {
        opts.uiSearch = search;
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button(u8"Xóa lọc")) {
        search[0] = '\0';
        opts.uiSearch.clear();
    }
    bool narrowLayout = ImGui::GetWindowWidth() < 560.0f;
    if (narrowLayout) {
        ImGui::Checkbox(u8"Yêu thích", &showFavorites);
        ImGui::Checkbox(u8"Gọn giao diện", &compactMode);
        ImGui::Checkbox(u8"Hiện phím tắt", &showHotkeys);
    } else {
        ImGui::SameLine();
        ImGui::Checkbox(u8"Yêu thích", &showFavorites);
        ImGui::SameLine();
        ImGui::Checkbox(u8"Gọn giao diện", &compactMode);
        ImGui::SameLine();
        ImGui::Checkbox(u8"Hiện phím tắt", &showHotkeys);
    }

    ImGui::Separator();
    ImGui::TextDisabled(u8"Preset nhanh");
    if (ImGui::Button(u8"Ngày")) applyPreset(UIPreset::Day, opts, scene, camera);
    if (!narrowLayout) ImGui::SameLine();
    if (ImGui::Button(u8"Đêm")) applyPreset(UIPreset::Night, opts, scene, camera);
    if (!narrowLayout) ImGui::SameLine();
    if (ImGui::Button(u8"Mưa")) applyPreset(UIPreset::Rain, opts, scene, camera);
    if (!narrowLayout) ImGui::SameLine();
    if (ImGui::Button(u8"Nắng")) applyPreset(UIPreset::Sunny, opts, scene, camera);
    if (!narrowLayout) ImGui::SameLine();
    if (ImGui::Button(u8"Noir")) applyPreset(UIPreset::Noir, opts, scene, camera);

    ImGui::Separator();

    bool stackLayout = ImGui::GetWindowWidth() < 520.0f;
    if (!stackLayout) {
        ImGui::Columns(2, "SettingsColumns", false);
        float sidebarWidth = std::min(std::max(ImGui::GetWindowWidth() * 0.28f, 180.0f), 240.0f);
        ImGui::SetColumnWidth(0, sidebarWidth);
    }

    if (!stackLayout) {
        ImGui::BeginChild("Sidebar", ImVec2(0.0f, 0.0f), true);
    } else {
        ImGui::BeginChild("Sidebar", ImVec2(0.0f, 150.0f), true);
    }
        ImGui::TextDisabled(u8"NHÓM");
        const char* sections[] = { u8"Môi trường", u8"Ánh sáng", u8"Hậu kỳ", u8"Hệ thống" };
        for (int i = 0; i < 4; ++i) {
            if (ImGui::Selectable(sections[i], section == i)) section = i;
        }
        ImGui::Separator();
        ImGui::TextDisabled(u8"PHÍM TẮT");
        ImGui::TextDisabled(u8"Tab: menu");
        ImGui::TextDisabled(u8"WASD: di chuyển");
        ImGui::TextDisabled(u8"F: lái xe");
    ImGui::EndChild();

    if (!stackLayout) {
        ImGui::NextColumn();
    } else {
        ImGui::Separator();
    }
    ImGui::BeginChild("Content", ImVec2(0.0f, 0.0f), false);

        auto show = [&](const char* label, bool pinned) {
            return matchQuery(label, search) || pinned;
        };

        bool anyFav = std::any_of(favorites.begin(), favorites.end(), [](bool v) { return v; });
        if (showFavorites && anyFav) {
            ImGui::TextColored(colAccentStrong, u8"Yêu thích");
            ImGui::Separator();

            if (favorites[FavTime]) {
                ImGui::Checkbox(u8"Đồng bộ thời gian thực", &opts.syncTimeOfDay);
                if (opts.syncTimeOfDay) {
                    ImGui::TextDisabled(u8"Giờ (0–24): %.2f", opts.timeOfDayHour);
                } else {
                    if (ImGui::SliderFloat(u8"Giờ (0–24)", &opts.timeOfDayHour, 0.0f, 24.0f, "%.2f")) {
                        (void)opts.timeOfDayHour;
                    }
                }
                char tbuf[8];
                formatHourVi(opts.timeOfDayHour, tbuf, sizeof(tbuf));
                ImGui::Text(u8"Thời gian trong game: %s", tbuf);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_time", &favorites[FavTime]);
            }

            if (favorites[FavRain]) {
                if (ImGui::SliderFloat(u8"Cường độ mưa", &opts.rainIntensity, 0.0f, 1.0f)) scene.setWetness(opts.rainIntensity);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_rain", &favorites[FavRain]);
            }
            if (favorites[FavCloud]) {
                ImGui::SliderFloat(u8"Độ cao tầng mây", &opts.cloudHeight, 40.0f, 140.0f, "%.1f");
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_cloud", &favorites[FavCloud]);
            }
            if (favorites[FavWind]) {
                ImGui::SliderFloat2(u8"Hướng gió", &opts.windDir.x, -1.0f, 1.0f);
                opts.windDir = glm::normalize(opts.windDir + glm::vec2(1e-5f));
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_wind", &favorites[FavWind]);
            }
            if (favorites[FavFog]) {
                float fogD = scene.fogDensity();
                if (ImGui::SliderFloat(u8"Mật độ sương", &fogD, 0.0f, 1.2f)) scene.setFogDensity(fogD);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_fog", &favorites[FavFog]);
            }
            if (favorites[FavExposure]) {
                float exp = scene.exposure();
                if (ImGui::SliderFloat(u8"Phơi sáng", &exp, 0.2f, 3.5f)) scene.setExposure(exp);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_exp", &favorites[FavExposure]);
            }
            if (favorites[FavBloom]) {
                ImGui::SliderFloat(u8"Bloom Intensity", &opts.bloomIntensity, 0.0f, 2.0f);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_bloom", &favorites[FavBloom]);
            }
            if (favorites[FavFov]) {
                float fov = camera.fovDegrees();
                if (ImGui::SliderFloat(u8"FOV", &fov, 20.0f, 90.0f)) camera.setFovDegrees(fov);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_fov", &favorites[FavFov]);
            }
            if (favorites[FavSpeed]) {
                float spd = camera.moveSpeed();
                if (ImGui::SliderFloat(u8"Tốc độ camera", &spd, 2.0f, 80.0f)) camera.setMoveSpeed(spd);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fav_speed", &favorites[FavSpeed]);
            }

            ImGui::Separator();
        }

        if (section == 0) {
            ImGui::TextColored(colAccentStrong, u8"Môi trường");
            ImGui::SameLine();
            if (ImGui::Button(u8"Đặt lại nhóm")) resetEnvironment(opts, scene);
            ImGui::Separator();

            if (show(u8"Đồng bộ thời gian thực", favorites[FavTime])) {
                ImGui::Checkbox(u8"Đồng bộ thời gian thực", &opts.syncTimeOfDay);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##time", &favorites[FavTime]);
            }
            if (show(u8"Giờ (0–24)", favorites[FavTime])) {
                if (opts.syncTimeOfDay) {
                    ImGui::TextDisabled(u8"Giờ (0–24): %.2f", opts.timeOfDayHour);
                } else {
                    ImGui::SliderFloat(u8"Giờ (0–24)", &opts.timeOfDayHour, 0.0f, 24.0f, "%.2f");
                }
                char tbuf[8];
                formatHourVi(opts.timeOfDayHour, tbuf, sizeof(tbuf));
                ImGui::Text(u8"Thời gian trong game: %s", tbuf);
            }

            if (show(u8"Mật độ (0 = tắt)", favorites[FavFog])) {
                float fogD = scene.fogDensity();
                if (ImGui::SliderFloat(u8"Mật độ (0 = tắt)", &fogD, 0.0f, 1.2f)) scene.setFogDensity(fogD);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fog", &favorites[FavFog]);
            }
            if (show(u8"Độ cao nền", false)) {
                float fBase = scene.fogBaseHeight();
                if (ImGui::SliderFloat(u8"Độ cao nền", &fBase, -10.0f, 50.0f)) scene.setFogBaseHeight(fBase);
            }
            if (show(u8"Giảm theo độ cao", false)) {
                float fFalloff = scene.fogHeightFalloff();
                if (ImGui::SliderFloat(u8"Giảm theo độ cao", &fFalloff, 0.0f, 0.2f)) scene.setFogHeightFalloff(fFalloff);
            }
            if (show(u8"Màu sương", false)) {
                glm::vec3 fc = scene.fogColor();
                float fc3[3] = {fc.x, fc.y, fc.z};
                if (ImGui::ColorEdit3(u8"Màu sương", fc3)) scene.setFogColor(glm::vec3(fc3[0], fc3[1], fc3[2]));
            }

            ImGui::Separator();
            ImGui::Text(u8"--- Mưa & Ướt ---");
            if (show(u8"Cường độ mưa", favorites[FavRain])) {
                if (ImGui::SliderFloat(u8"Cường độ mưa", &opts.rainIntensity, 0.0f, 1.0f)) scene.setWetness(opts.rainIntensity);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##rain", &favorites[FavRain]);
            }
            if (show(u8"Độ cao tầng mây", favorites[FavCloud])) {
                ImGui::SliderFloat(u8"Độ cao tầng mây", &opts.cloudHeight, 40.0f, 140.0f, "%.1f");
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##cloud", &favorites[FavCloud]);
            }
            if (show(u8"Hướng gió", favorites[FavWind])) {
                ImGui::SliderFloat2(u8"Hướng gió", &opts.windDir.x, -1.0f, 1.0f);
                opts.windDir = glm::normalize(opts.windDir + glm::vec2(1e-5f));
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##wind", &favorites[FavWind]);
            }
        }

        if (section == 1) {
            ImGui::TextColored(colAccentStrong, u8"Ánh sáng");
            ImGui::SameLine();
            if (ImGui::Button(u8"Đặt lại nhóm")) resetLighting(scene);
            ImGui::Separator();

            if (show(u8"Phơi sáng (exposure)", favorites[FavExposure])) {
                float exp = scene.exposure();
                if (ImGui::SliderFloat(u8"Phơi sáng (exposure)", &exp, 0.2f, 3.5f)) scene.setExposure(exp);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##exposure", &favorites[FavExposure]);
            }

            if (opts.syncTimeOfDay) {
                ImGui::TextDisabled(u8"Màu trời/đất/nắng: đang đồng bộ theo thời gian");
            } else {
                if (show(u8"Màu trời (ambient trên)", false)) {
                    glm::vec3 sky = scene.skyColor();
                    float sky3[3] = {sky.x, sky.y, sky.z};
                    if (ImGui::ColorEdit3(u8"Màu trời (ambient trên)", sky3)) scene.setSkyColor(glm::vec3(sky3[0], sky3[1], sky3[2]));
                }
                if (show(u8"Màu đất (ambient dưới)", false)) {
                    glm::vec3 gnd = scene.groundColor();
                    float gnd3[3] = {gnd.x, gnd.y, gnd.z};
                    if (ImGui::ColorEdit3(u8"Màu đất (ambient dưới)", gnd3)) scene.setGroundColor(glm::vec3(gnd3[0], gnd3[1], gnd3[2]));
                }
                if (show(u8"Màu nắng", false)) {
                    glm::vec3 sun = scene.sunColor();
                    float sun3[3] = {sun.x, sun.y, sun.z};
                    if (ImGui::ColorEdit3(u8"Màu nắng", sun3)) scene.setSunColor(glm::vec3(sun3[0], sun3[1], sun3[2]));
                }
                if (show(u8"Hướng tia sáng", false)) {
                    glm::vec3 ld = scene.lightDir();
                    float lx = ld.x, ly = ld.y, lz = ld.z;
                    if (ImGui::DragFloat3(u8"Hướng tia sáng", &lx, 0.01f)) scene.setLightDir(glm::vec3(lx, ly, lz));
                }
            }

            ImGui::Separator();
            if (show(u8"Độ sáng cửa sổ", false)) {
                float em = scene.windowEmissive();
                if (ImGui::SliderFloat(u8"Độ sáng cửa sổ", &em, 0.0f, 5.0f)) scene.setWindowEmissive(em);
            }
        }

        if (section == 2) {
            ImGui::TextColored(colAccentStrong, u8"Hậu kỳ");
            ImGui::SameLine();
            if (ImGui::Button(u8"Đặt lại nhóm")) resetPost(opts);
            ImGui::Separator();

            if (show(u8"Bloom Threshold", false)) ImGui::SliderFloat(u8"Bloom Threshold", &opts.bloomThreshold, 0.5f, 5.0f);
            if (show(u8"Bloom Intensity", favorites[FavBloom])) {
                ImGui::SliderFloat(u8"Bloom Intensity", &opts.bloomIntensity, 0.0f, 2.0f);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##bloom", &favorites[FavBloom]);
            }
            if (show(u8"God Ray (Sun)", false)) ImGui::SliderFloat(u8"God Ray (Sun)", &opts.godRayIntensity, 0.0f, 1.5f);
            if (show(u8"Color Grade (Lạnh)", false)) ImGui::SliderFloat(u8"Color Grade (Lạnh)", &opts.colorGradeIntensity, 0.0f, 1.0f);
            if (show(u8"Vignette (Viền tối)", false)) ImGui::SliderFloat(u8"Vignette (Viền tối)", &opts.vignetteIntensity, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::Text(u8"--- Khử răng cưa & Ống kính ---");
            if (show(u8"FXAA (Anti-aliasing)", false)) ImGui::SliderFloat(u8"FXAA (Anti-aliasing)", &opts.fxaaIntensity, 0.0f, 1.0f);
            if (show(u8"Chromatic Aberration", false)) ImGui::SliderFloat(u8"Chromatic Aberration", &opts.chromaticAberration, 0.0f, 0.05f, "%.3f");

            ImGui::Separator();
            ImGui::Text(u8"--- Chiều sâu & Phản chiếu ---");
            if (show(u8"SSAO (Ambient Occlusion)", false)) ImGui::SliderFloat(u8"SSAO (Ambient Occlusion)", &opts.ssaoIntensity, 0.0f, 2.0f);
            if (show(u8"SSR (Phản chiếu mưa)", false)) ImGui::SliderFloat(u8"SSR (Phản chiếu mưa)", &opts.ssrIntensity, 0.0f, 1.5f);
        }

        if (section == 3) {
            ImGui::TextColored(colAccentStrong, u8"Hệ thống");
            ImGui::SameLine();
            if (ImGui::Button(u8"Đặt lại nhóm")) resetSystem(opts, scene, camera, window);
            ImGui::Separator();

            ImGui::Text(u8"--- Xe & Giao thông ---");
            ImGui::Text(u8"Model xe: %d | Xe trên đường: %d", scene.carModelCount(), scene.carCount());
            if (show(u8"Chỉ hiển thị xe (Debug View)", false)) {
                ImGui::Checkbox(u8"Chỉ hiển thị xe (Debug View)", &scene.m_debugDrawCarOnly);
                if (scene.m_debugDrawCarOnly && scene.carModelCount() > 0) {
                    ImGui::SliderInt(u8"Xem model số", &scene.m_debugCarIndex, 0, scene.carModelCount() - 1);
                }
            }

            if (show(u8"Tỷ lệ xe (Scale)", false)) {
                float cs = scene.carScale();
                if (ImGui::SliderFloat(u8"Tỷ lệ xe (Scale)", &cs, 1.0f, 200.0f, "%.1f")) scene.setCarScale(cs);
            }
            if (show(u8"Độ cao xe (Y Offset)", false)) {
                float cy = scene.carYOffset();
                if (ImGui::SliderFloat(u8"Độ cao xe (Y Offset)", &cy, -5.0f, 5.0f, "%.2f")) scene.setCarYOffset(cy);
            }

            ImGui::Separator();
            ImGui::Text(u8"--- Camera ---");
            if (show(u8"Tốc độ di chuyển", favorites[FavSpeed])) {
                float spd = camera.moveSpeed();
                if (ImGui::SliderFloat(u8"Tốc độ di chuyển", &spd, 2.0f, 80.0f)) camera.setMoveSpeed(spd);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##speed", &favorites[FavSpeed]);
            }
            if (show(u8"Độ nhạy chuột", false)) {
                float sens = camera.mouseSensitivity();
                if (ImGui::SliderFloat(u8"Độ nhạy chuột", &sens, 0.02f, 0.35f)) camera.setMouseSensitivity(sens);
            }
            if (show(u8"Trường nhìn FOV", favorites[FavFov])) {
                float fov = camera.fovDegrees();
                if (ImGui::SliderFloat(u8"Trường nhìn FOV", &fov, 20.0f, 90.0f)) camera.setFovDegrees(fov);
                ImGui::SameLine();
                ImGui::Checkbox(u8"Ghim##fov", &favorites[FavFov]);
            }
            if (show(u8"Đặt lại vị trí camera", false)) {
                if (ImGui::Button(u8"Đặt lại vị trí camera")) camera = Camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);
            }

            ImGui::Separator();
            ImGui::Text(u8"--- Bố cục ---");
            if (show(u8"Lưới nhiều bản sao", false)) ImGui::Checkbox(u8"Lưới nhiều bản sao", &opts.useGridLayout);
            if (opts.useGridLayout) {
                if (show(u8"Hàng", false)) ImGui::SliderInt(u8"Hàng", &opts.gridRows, 1, 10);
                if (show(u8"Cột", false)) ImGui::SliderInt(u8"Cột", &opts.gridCols, 1, 10);
                if (show(u8"Khoảng cách X", false)) ImGui::SliderFloat(u8"Khoảng cách X", &opts.gridSpaceX, 20.0f, 200.0f);
                if (show(u8"Khoảng cách Z", false)) ImGui::SliderFloat(u8"Khoảng cách Z", &opts.gridSpaceZ, 20.0f, 250.0f);
            }
            if (show(u8"Áp dụng bố cục", false)) {
                if (ImGui::Button(u8"Áp dụng bố cục")) {
                    if (opts.useGridLayout) scene.setInstanceTransforms(CityBuilder::buildGrid(opts.gridRows, opts.gridCols, opts.gridSpaceX, opts.gridSpaceZ, 0.0f));
                    else scene.setInstanceTransforms({glm::mat4(1.0f)});
                }
            }

            ImGui::Separator();
            ImGui::Text(u8"--- Hiển thị ---");
            if (show(u8"Khung dây", false)) {
                if (ImGui::Checkbox(u8"Khung dây", &opts.wireframe)) glPolygonMode(GL_FRONT_AND_BACK, opts.wireframe ? GL_LINE : GL_FILL);
            }
            if (show(u8"Loại mặt sau (cull)", false)) {
                if (ImGui::Checkbox(u8"Loại mặt sau (cull)", &opts.cullBack)) {
                    if (opts.cullBack) glEnable(GL_CULL_FACE);
                    else glDisable(GL_CULL_FACE);
                }
            }
            if (show(u8"VSync", false)) {
                if (ImGui::Checkbox(u8"VSync", &opts.vsync)) glfwSwapInterval(opts.vsync ? 1 : 0);
            }

            if (opts.syncTimeOfDay) {
                ImGui::TextDisabled(u8"Màu nền trời: đang đồng bộ theo thời gian");
            } else if (show(u8"Màu nền trời", false)) {
                ImGui::ColorEdit3(u8"Màu nền trời", opts.clearCol);
            }
        }

    ImGui::EndChild();
    if (!stackLayout) {
        ImGui::Columns(1);
    }

    ImGui::End();
    ImGui::PopStyleColor(colorCount);
    style = styleBackup;
}
