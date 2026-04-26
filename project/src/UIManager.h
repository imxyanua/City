#pragma once

#include "AppOptions.h"
#include "Scene.h"
#include "Camera.h"
#include "imgui.h"
#include <GLFW/glfw3.h>

class UIManager {
public:
    void render(AppOptions& opts, Scene& scene, Camera& camera, GLFWwindow* window, ImGuiIO& imguiIo);
};
