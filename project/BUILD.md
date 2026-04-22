# City Street — Build & Run (Windows)

## Phân tích nhanh `procedural_city_7.glb`

- **Mesh**: glTF 2.0 thường gồm nhiều `mesh` / `primitive` (tam giác), có thể phân cấp qua `node` (transform lồng nhau). Loader duyệt cây scene, nhân transform lên position/normal rồi upload GPU — phù hợp tái sử dụng toàn bộ chunk thành phố như một “module”.
- **Material / texture**: PBR `baseColorTexture` + `baseColorFactor`; ảnh thường nhúng trong GLB. Ảnh được tạo texture OpenGL với **mipmap**, lọc `LINEAR_MIPMAP_LINEAR`, không gọi `glTexImage` lặp lại cho cùng một image index.
- **Hình học**: Asset gốc ~**10 tòa nhà** trong một scene; ứng dụng mặc định vẽ **một instance** (`mat4` đơn vị) đúng bố cục file GLB. `CityBuilder` vẫn có trong project nếu sau này muốn nhân bản có chủ đích.
- **Reuse**: Một `Model` load một lần; có thể thêm nhiều transform trong `Scene` nếu cần nhiều bản sao.

## Kiến trúc (tóm tắt)

| Module        | Vai trò |
|---------------|---------|
| `GLTFLoader`  | tinygltf + STB: đọc `.glb`, tạo `Mesh` + `GpuMaterial`, texture GPU |
| `Model`       | Tập `Mesh`, `draw()` theo material / texture |
| `Scene`       | Model + ánh sáng + danh sách transform instance |
| `CityBuilder` | (Tùy chọn) sinh lưới transform; mặc định `main` không dùng |
| `Camera`      | WASD, mouse look, scroll FOV |
| `Shader`      | Compile/link, log lỗi GLSL |

Pipeline: **đọc glTF → VAO/VBO/EBO + texture → shader Phong (ambient + diffuse + specular nhẹ) → vẽ từng instance.**

## Yêu cầu

- Windows, Visual Studio 2019+ hoặc build tool có **CMake 3.16+**, **Git** (FetchContent).
- GPU hỗ trợ **OpenGL 3.3 Core**.

## Đặt model

File `procedural_city_7.glb` **không** được commit lên GitHub. Cần có sẵn file (bản gốc asset), rồi đặt tại:

`project/assets/models/procedural_city_7.glb`

Khi build, CMake chép thư mục `assets/` và `shaders/` cạnh file `.exe`.

## Build

```powershell
cd d:\City\project
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Hoặc Ninja (nếu cài):

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Chạy

Từ thư mục chứa `CityStreet.exe` (thường `build\Release` hoặc `build`):

```powershell
.\Release\CityStreet.exe
```

Ứng dụng tìm `assets` và `shaders` **cùng thư mục với executable** (đã copy sau build).

**Điều khiển**: WASD, chuột nhìn, cuộn zoom FOV, Space lên, Shift xuống, Esc thoát.

## Xử lý lỗi thường gặp

- **Thiếu GLB**: thông báo đường dẫn mong đợi — copy file vào `assets/models/`, build lại để copy sang output.
- **Shader lỗi**: log compile/link in ra console; kiểm tra `shaders/*.glsl` cạnh `.exe`.
- **Màn hình đen / không mesh**: mở GLB trong viewer khác để kiểm tra scene/mesh; xem log `[tinygltf]` / `[GLTFLoader]`.
