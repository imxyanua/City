# City

Thiết kế dãy phố — ứng dụng **OpenGL 3.3** (C++17), menu **ImGui**: preset + **năm nút nhóm** (tổng quan / môi trường / ánh sáng / hậu kỳ / GPU–scene), thời gian trong ngày, sương mù và mưa.

Mã nguồn và CMake nằm trong thư mục [`project/`](project/). Build và chạy: [`project/BUILD.md`](project/BUILD.md).

**Model:** đặt file GLB thành phố và (tuỳ chọn) xe trong **[`project/assets/models/`](project/assets/models/)** — bắt buộc có **`procedural_city_7.glb`** để khởi động. CMake **không** dùng thư mục `project/models/`; chỉ copy nội dung `project/assets/` ra cạnh executable khi build.

Repository: [github.com/imxyanua/City](https://github.com/imxyanua/City)
