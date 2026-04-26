cmake_minimum_required(VERSION 3.16)
project(CityStreet LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# glm (header-only)
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# nlohmann_json (tinygltf dependency)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# tinygltf (header-only)
FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG        v2.8.20
)
FetchContent_MakeAvailable(tinygltf)

# Dear ImGui (menu / debug UI)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.9
)
FetchContent_MakeAvailable(imgui)

set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad_generated")
set(IMGUI_DIR "${imgui_SOURCE_DIR}")

add_executable(CityStreet
  src/main.cpp
  src/Shader.cpp
  src/Camera.cpp
  src/GLTFLoader.cpp
  src/Mesh.cpp
  src/Model.cpp
  src/Scene.cpp
  src/CityBuilder.cpp
  src/RainSystem.cpp
  src/PostProcessor.cpp
  src/UIManager.cpp
  ${GLAD_DIR}/src/gl.c
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set_source_files_properties(
  "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
  PROPERTIES COMPILE_DEFINITIONS "IMGUI_IMPL_OPENGL_LOADER_GLAD2"
)

target_include_directories(CityStreet PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${GLAD_DIR}/include
  ${tinygltf_SOURCE_DIR}
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

target_link_libraries(CityStreet PRIVATE
  glfw
  glm::glm
  nlohmann_json::nlohmann_json
)

if(WIN32)
  target_link_libraries(CityStreet PRIVATE opengl32)
endif()

target_compile_definitions(CityStreet PRIVATE
  GLFW_INCLUDE_NONE
  GLM_ENABLE_EXPERIMENTAL
)

if(MSVC)
  # /utf-8: chuỗi tiếng Việt có dấu trong mã nguồn.
  # /W3 matches typical VS CMake presets (avoids D9025 vs /W4); suppress tinygltf headers included in our .cpp.
  target_compile_options(CityStreet PRIVATE /utf-8 /W3 /permissive- /wd4018 /wd4267)
else()
  target_compile_options(CityStreet PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy runtime assets next to the executable (models, shaders)
add_custom_command(TARGET CityStreet POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:CityStreet>/assets"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:CityStreet>/shaders"
  COMMENT "Copying assets and shaders to output directory"
)
