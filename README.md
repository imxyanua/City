# City

Thiết kế dãy phố — ứng dụng **OpenGL 3.3** (C++17) xem cảnh `procedural_city_7.glb`, có menu **Dear ImGui** (tiếng Việt), thời gian trong ngày, sương mù và mưa (hậu kỳ).

Mã nguồn và CMake nằm trong thư mục [`project/`](project/). Build và chạy: [`project/BUILD.md`](project/BUILD.md).

**Model:** đặt file GLB thành phố và (tuỳ chọn) xe trong **[`project/assets/models/`](project/assets/models/)** — bắt buộc có **`procedural_city_7.glb`** để khởi động. CMake **không** dùng thư mục `project/models/`; chỉ copy nội dung `project/assets/` ra cạnh executable khi build.

Repository: [github.com/imxyanua/City](https://github.com/imxyanua/City)
