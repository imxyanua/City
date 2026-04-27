#include "UIManager.h"
#include "CityBuilder.h"
#include "imgui.h"
#include <cstdio>
#include <cmath>

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
}

void UIManager::render(AppOptions& opts, Scene& scene, Camera& camera, GLFWwindow* window, ImGuiIO& imguiIo) {
    if (!opts.showMenu) return;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.05f, 0.98f));
    ImGui::SetNextWindowBgAlpha(0.98f);
    ImGui::Begin(u8"Cài đặt (Tab để đóng/mở)", &opts.showMenu, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text(u8"FPS: %.0f", static_cast<double>(imguiIo.Framerate));
    ImGui::Separator();
    const char* cameraModes[] = { u8"Tự do (Free)", u8"Theo dõi xe (Follow)", u8"Camera an ninh (CCTV)", u8"Lái xe (Drive)", u8"Quay phim (Cinematic)" };
    ImGui::Combo(u8"Góc máy", &opts.cameraMode, cameraModes, IM_ARRAYSIZE(cameraModes));
    ImGui::Separator();

    if (ImGui::BeginTabBar("SettingsTabs")) {

        if (ImGui::BeginTabItem(u8"Môi trường")) {
            ImGui::Text(u8"--- Thời gian ---");
            ImGui::Checkbox(u8"Đồng bộ thời gian thực", &opts.syncTimeOfDay);
            
            ImGui::BeginDisabled(opts.syncTimeOfDay);
            ImGui::SliderFloat(u8"Giờ (0–24)", &opts.timeOfDayHour, 0.0f, 24.0f, "%.2f");
            ImGui::EndDisabled();
            
            char tbuf[8];
            formatHourVi(opts.timeOfDayHour, tbuf, sizeof(tbuf));
            ImGui::Text(u8"Thời gian trong game: %s", tbuf);
            ImGui::Separator();
            ImGui::Text(u8"--- Sương mù ---");
            float fogD = scene.fogDensity();
            if (ImGui::SliderFloat(u8"Mật độ (0 = tắt)", &fogD, 0.0f, 1.2f)) scene.setFogDensity(fogD);
            float fBase = scene.fogBaseHeight();
            if (ImGui::SliderFloat(u8"Độ cao nền", &fBase, -10.0f, 50.0f)) scene.setFogBaseHeight(fBase);
            float fFalloff = scene.fogHeightFalloff();
            if (ImGui::SliderFloat(u8"Giảm theo độ cao", &fFalloff, 0.0f, 0.2f)) scene.setFogHeightFalloff(fFalloff);
            glm::vec3 fc = scene.fogColor();
            float fc3[3] = {fc.x, fc.y, fc.z};
            if (ImGui::ColorEdit3(u8"Màu sương", fc3)) scene.setFogColor(glm::vec3(fc3[0], fc3[1], fc3[2]));
            ImGui::Separator();
            ImGui::Text(u8"--- Mưa & Ướt ---");
            if (ImGui::SliderFloat(u8"Cường độ mưa", &opts.rainIntensity, 0.0f, 1.0f)) scene.setWetness(opts.rainIntensity);
            ImGui::SliderFloat2(u8"Hướng gió", &opts.windDir.x, -1.0f, 1.0f);
            opts.windDir = glm::normalize(opts.windDir + glm::vec2(1e-5f));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(u8"Ánh sáng")) {
            float exp = scene.exposure();
            if (ImGui::SliderFloat(u8"Phơi sáng (exposure)", &exp, 0.2f, 3.5f)) scene.setExposure(exp);
            
            ImGui::BeginDisabled(opts.syncTimeOfDay);
            glm::vec3 sky = scene.skyColor();
            glm::vec3 gnd = scene.groundColor();
            glm::vec3 sun = scene.sunColor();
            float sky3[3] = {sky.x, sky.y, sky.z};
            float gnd3[3] = {gnd.x, gnd.y, gnd.z};
            float sun3[3] = {sun.x, sun.y, sun.z};
            if (ImGui::ColorEdit3(u8"Màu trời (ambient trên)", sky3)) scene.setSkyColor(glm::vec3(sky3[0], sky3[1], sky3[2]));
            if (ImGui::ColorEdit3(u8"Màu đất (ambient dưới)", gnd3)) scene.setGroundColor(glm::vec3(gnd3[0], gnd3[1], gnd3[2]));
            if (ImGui::ColorEdit3(u8"Màu nắng", sun3)) scene.setSunColor(glm::vec3(sun3[0], sun3[1], sun3[2]));
            glm::vec3 ld = scene.lightDir();
            float lx = ld.x, ly = ld.y, lz = ld.z;
            if (ImGui::DragFloat3(u8"Hướng tia sáng", &lx, 0.01f)) scene.setLightDir(glm::vec3(lx, ly, lz));
            ImGui::EndDisabled();

            ImGui::Separator();
            ImGui::Text(u8"--- Tòa nhà (Đêm) ---");
            float em = scene.windowEmissive();
            if (ImGui::SliderFloat(u8"Độ sáng cửa sổ", &em, 0.0f, 5.0f)) scene.setWindowEmissive(em);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(u8"Hậu kỳ")) {
            ImGui::SliderFloat(u8"Bloom Threshold", &opts.bloomThreshold, 0.5f, 5.0f);
            ImGui::SliderFloat(u8"Bloom Intensity", &opts.bloomIntensity, 0.0f, 2.0f);
            ImGui::SliderFloat(u8"God Ray (Sun)", &opts.godRayIntensity, 0.0f, 1.5f);
            ImGui::SliderFloat(u8"Color Grade (Lạnh)", &opts.colorGradeIntensity, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"Vignette (Viền tối)", &opts.vignetteIntensity, 0.0f, 1.0f);
            ImGui::Separator();
            ImGui::Text(u8"--- Khử răng cưa & Ống kính ---");
            ImGui::SliderFloat(u8"FXAA (Anti-aliasing)", &opts.fxaaIntensity, 0.0f, 1.0f);
            ImGui::SliderFloat(u8"Chromatic Aberration", &opts.chromaticAberration, 0.0f, 0.05f, "%.3f");
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(u8"Hệ thống")) {
            ImGui::Text(u8"--- Xe & Giao thông ---");
            ImGui::Text(u8"Model xe: %d | Xe trên đường: %d", scene.carModelCount(), scene.carCount());
            
            ImGui::Checkbox(u8"Chỉ hiển thị xe (Debug View)", &scene.m_debugDrawCarOnly);
            if (scene.m_debugDrawCarOnly && scene.carModelCount() > 0) {
                ImGui::SliderInt(u8"Xem model số", &scene.m_debugCarIndex, 0, scene.carModelCount() - 1);
            }

            float cs = scene.carScale();
            if (ImGui::SliderFloat(u8"Tỷ lệ xe (Scale)", &cs, 1.0f, 200.0f, "%.1f")) scene.setCarScale(cs);
            float cy = scene.carYOffset();
            if (ImGui::SliderFloat(u8"Độ cao xe (Y Offset)", &cy, -5.0f, 5.0f, "%.2f")) scene.setCarYOffset(cy);
            ImGui::Separator();
            ImGui::Text(u8"--- Camera ---");
            float spd = camera.moveSpeed();
            if (ImGui::SliderFloat(u8"Tốc độ di chuyển", &spd, 2.0f, 80.0f)) camera.setMoveSpeed(spd);
            float sens = camera.mouseSensitivity();
            if (ImGui::SliderFloat(u8"Độ nhạy chuột", &sens, 0.02f, 0.35f)) camera.setMouseSensitivity(sens);
            float fov = camera.fovDegrees();
            if (ImGui::SliderFloat(u8"Trường nhìn FOV", &fov, 20.0f, 90.0f)) camera.setFovDegrees(fov);
            if (ImGui::Button(u8"Đặt lại vị trí camera")) camera = Camera(glm::vec3(0.0f, 38.0f, 85.0f), -90.0f, -16.0f);
            
            ImGui::Separator();
            ImGui::Text(u8"--- Bố cục ---");
            ImGui::Checkbox(u8"Lưới nhiều bản sao", &opts.useGridLayout);
            if (opts.useGridLayout) {
                ImGui::SliderInt(u8"Hàng", &opts.gridRows, 1, 10);
                ImGui::SliderInt(u8"Cột", &opts.gridCols, 1, 10);
                ImGui::SliderFloat(u8"Khoảng cách X", &opts.gridSpaceX, 20.0f, 200.0f);
                ImGui::SliderFloat(u8"Khoảng cách Z", &opts.gridSpaceZ, 20.0f, 250.0f);
            }
            if (ImGui::Button(u8"Áp dụng bố cục")) {
                if (opts.useGridLayout) scene.setInstanceTransforms(CityBuilder::buildGrid(opts.gridRows, opts.gridCols, opts.gridSpaceX, opts.gridSpaceZ, 0.0f));
                else scene.setInstanceTransforms({glm::mat4(1.0f)});
            }

            ImGui::Separator();
            ImGui::Text(u8"--- Hiển thị ---");
            if (ImGui::Checkbox(u8"Khung dây", &opts.wireframe)) glPolygonMode(GL_FRONT_AND_BACK, opts.wireframe ? GL_LINE : GL_FILL);
            if (ImGui::Checkbox(u8"Loại mặt sau (cull)", &opts.cullBack)) {
                if (opts.cullBack) glEnable(GL_CULL_FACE);
                else glDisable(GL_CULL_FACE);
            }
            if (ImGui::Checkbox(u8"VSync", &opts.vsync)) glfwSwapInterval(opts.vsync ? 1 : 0);
            
            ImGui::BeginDisabled(opts.syncTimeOfDay);
            ImGui::ColorEdit3(u8"Màu nền trời", opts.clearCol);
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleColor();
}
