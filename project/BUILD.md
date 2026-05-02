# Build và chạy CityStreet

## 1. Đặt model đúng chỗ (theo dự án)

Tất cả file `.glb` phải nằm trong:

```text
project/assets/models/
```

| File | Bắt buộc |
|------|-----------|
| `procedural_city_7.glb` | Có — không có thì exe báo lỗi và thoát |
| `2014_bmw_z4_sdrive35is.glb`, `2021_lexus_bev_sport_concept.glb`, `2022_nio_et7.glb` | Không — thiếu thì chỉ không có ô tô trong cảnh |

**Không** đặt model trong `project/models/` hay chỉ trong thư mục gốc repo: CMake lệnh `POST_BUILD` chỉ **`copy_directory`** từ `project/assets/` → `...(thư mục CityStreet.exe)/assets/`. Nếu model nằm sai chỗ, bản build sẽ thiếu `assets/models` bên exe.

## 2. CMake (Windows, PowerShell)

Từ thư mục `City` (cha của `project/`):

```powershell
cmake -S project -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Chạy (đường dẫn tương đối từ `City`):

```powershell
.\build\Release\CityStreet.exe
```

Sau build, exe dùng: **`build/Release/assets/models/`** (bản copy từ `project/assets/`). Luôn sửa/ thêm GLB trong **`project/assets/models/`** rồi **build lại** (hoặc copy tay vào output nếu bạn chỉ đổi model).

### Debug trong VS Code / Cursor

Đặt **cwd** của launch configuration trùng thư mục chứa exe (vd. `${workspaceFolder}/build/Release`) để đường dẫn `assets/` và `shaders/` đọc đúng nếu `argv[0]` không đầy đủ.

## 3. Tuỳ chọn: đổi tên city GLB

Mặc định code tìm `procedural_city_7.glb`. Để dùng tên khác, đặt file trong `project/assets/models/` và thêm vào **`config.json` cạnh exe** khóa `cityModelFile` (xem `project/config.json.example`).

---

**Card:** OpenGL **3.3 Core**.  

**Khớp cảnh:** traffic / pedestrian trong code chỉnh theo scale city gốc; model lệch nhiều có thể cần chỉnh hằng trong `Scene.cpp` hoặc camera trong `main.cpp`.
