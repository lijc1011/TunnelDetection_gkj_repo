# 两台电脑接力开发

源码和公共配置通过 GitHub 的 `main` 分支同步。Qt、Visual Studio、OpenCV、vcpkg、z5 和相机 SDK 的安装路径只写在本机的 `CMakeUserPresets.json` 中；该文件已被 Git 忽略。

## 个人电脑首次设置

1. 安装 VS Code 的 C/C++ 与 CMake Tools 扩展。
2. 在 Visual Studio Installer 中确认已安装“使用 C++ 的桌面开发”和 MSVC v143 x64/x86 生成工具。
3. VS2026 的 CMake generator 需要 CMake 4.2 或更高版本。
4. 克隆仓库后，在仓库根目录执行：

```powershell
Copy-Item CMakeUserPresets.personal.example.json CMakeUserPresets.json
```

5. 打开 `CMakeUserPresets.json`，核对本机的 Qt、vcpkg、OpenCV、z5、Sapera 和海康 MVS 路径。模板中的 Qt 根目录尤其要以实际安装目录为准；该目录下应能找到 `lib/cmake/Qt6/Qt6Config.cmake`。
6. 配置和构建：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --preset local
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --build --preset local-debug
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --preset local-app
```

也可以直接在 VS Code 中运行默认生成任务，或按 `F5` 构建并调试。

个人电脑模板使用 VS2026 generator 和 v143 工具集，以匹配 Qt `msvc2022_64` 及现有 OpenCV 二进制库。如果 CMake 报找不到 v143，请先通过 Visual Studio Installer 添加该组件。

## 工控机

工控机根目录已经有自己的 `CMakeUserPresets.json`，使用 VS2022、Qt 6.11.1 和 `C:/dev` 下的依赖。它不会出现在 `git status` 中，也不会被个人电脑的配置覆盖。

如果重新克隆工控机仓库，只需从个人电脑模板复制一份并把 generator、Qt、vcpkg、OpenCV 和 z5 路径改回工控机值。

## 每次切换电脑

开始修改前：

```powershell
git switch main
git pull --ff-only
git status
```

离开当前电脑前：

```powershell
git status
git diff
git add <本次修改的文件>
git commit -m "说明本次修改"
git push origin main
```

未完成的工作应提交到临时分支并推送；不要提交 `CMakeUserPresets.json`、`build/`、`bin/` 或 DLL/PDB 等构建产物。不要依赖 `git stash` 跨电脑。

## TensorStore（Zarr v3 存储后端）

采集格式由运行目录的 `Config.xml` 控制：`SaveZarr=1` 写 Zarr v3，
`SaveZarr=0` 使用旧 `.camdatx` 流程。预编译桥接只调整构建，不改变此开关。
TensorStore 源码 vendor 在 `3rdparty/TensorStore`，经 FetchContent 并入构建树；
主程序只链接窄接口桥 `TsZarrBridge.dll`（`src/tszarr/`，C ABI）。

### 首次构建须知

- **需要联网**：首次 configure 会下载 abseil/riegeli 等依赖源码到 `build/local/_deps`。
- **需要 Python ≥ 3.10**（bazel_to_cmake 在配置期运行；python.org 安装版走注册表即可被找到，无需加 PATH）。
- 首次构建用 **全量** 预设/任务，之后日常用 **增量**。

### 日常构建（增量，快）

- VS Code 默认构建任务和 F5 使用 `local-app-debug`。
- CMake Tools 工具栏的预设选择保存在 VS Code 工作区状态中，需要首次手动选择：
  `CMake: Select Configure Preset` → `local-app`，
  `CMake: Select Build Preset` → `local-app-debug`。
  `settings.json` 中的 `cmake.configurePreset` / `cmake.buildPreset` 不是该扩展支持的设置项。
- 日常构建树位于 `build/local-app`，直接链接 `bin/Debug` 或 `bin/Release` 中
  已构建的 `TsZarrBridge.dll/.lib`，不加载 TensorStore 源码、不运行其转换脚本。
- 主程序输出仍在 `bin/Debug`、`bin/Release`，F5 的启动路径不变。
- Debug：`.\tools\cmake.ps1 --build --preset local-app-debug`
- Release：`.\tools\cmake.ps1 --build --preset local-app-release`
- 两个配置分别引用对应的桥接库，不混用 Debug/Release。
- 日常入口只生成 Debug/Release 两种配置；不再生成 TensorStore 的第三方工程。
- `CMakeUserPresets.json` 不随 Git 更新。已有电脑需要把模板中的 `local-app` 配置预设、
  `local-app-debug` / `local-app-release` 构建预设合入自己的文件，保留原有 SDK 路径和工具集。
- 其他配置仍可通过完整构建入口生成；日常入口也必须先具备对应配置的 DLL/LIB。
- 原 `local-debug-fast` / `local-release-fast` 预设保留，供旧流程使用。

### 避免重复配置（Configure / Generate）

- VS Code 不再在打开项目或保存 `CMakeLists.txt` 时自动配置；已有缓存时跳过自动配置。
- 仅修改已有 `.cpp/.h` 内容时，直接使用日常增量构建。
- 添加或删除源码、修改 CMake 文件、预设或依赖路径后，先运行任务
  `CMake Configure Local`，或执行 `.\tools\cmake.ps1 --preset local-app`，再构建。
- 保留 `build/local-app`，以及完整构建树 `build/local` 及其 `_deps`；
  只在排查缓存问题时删除缓存并重新配置。
- FULL 构建任务保留配置前置步骤，继续用于首次构建或依赖变动后的完整构建。

### 何时需要全量构建（任务：Build Debug/Release FULL (TensorStore)）

- 首次克隆/首次构建；
- 改了 `3rdparty/TensorStore` 或 `TsZarrBridge.cpp/.h`（桥接层）；
- 调整了桥接库的编译选项、依赖、工具链或架构。

完整构建仍使用原来的 `local` 配置预设和 `build/local` 构建树，
会更新 `bin/<Config>/TsZarrBridge.dll/.lib`，供日常构建直接使用。
VS Code 任务 `Build Debug/Release FULL (TensorStore)` 会先执行
`CMake Configure FULL (TensorStore)`，不会使用日常入口跳过存储模块。

命令行示例（Debug；Release 改用 `local-release`）：

```powershell
.\tools\cmake.ps1 --preset local
.\tools\cmake.ps1 --build --preset local-debug
.\tools\cmake.ps1 --preset local-app
```

桥接库属于本机构建产物，不提交到 Git。新电脑先完成对应配置的完整构建，
再使用日常入口；也可用 `TUNNEL_TSBRIDGE_PREBUILT_DIR` 指向匹配的预编译库根目录。
更新桥接库后可运行 `bin/<Config>/TsZarrBench.exe --self-test` 校验写入和读回。

### 预编译桥接库的匹配检查

完整构建在生成桥接 DLL 后，自动写入同目录的 `TsZarrBridge.dll.build.json`，记录
桥接 `.h/.cpp`、DLL/LIB 的 SHA-256，以及配置、MSVC 工具集和架构信息。
日常配置和每次构建都会核对这些信息；缺失记录、源码变动、DLL/LIB 被替换或
配置不匹配时直接报错，要求先执行对应配置的 FULL 构建，避免静默使用旧接口。
只需重建当前要使用的配置；另一配置未更新不会阻止已匹配配置的日常构建。
已有的旧 DLL 首次使用本机制时也需要完整构建一次，不能手动补写记录冒充重建。
使用外部预编译目录时，须同时提供对应的 DLL、LIB 和 `TsZarrBridge.dll.build.json`。

这些检查只影响构建过程，不改变 `SaveZarr`、采集流程、文件格式或界面交互。

### 存储格式说明

- 每相机一次检测产出 `<同名>.zarr` 目录（数组 uint8 `[包, 行, 列]`，chunk=一包=一个文件，
  bytes codec 不压缩）+ 照写的 `.camidx` 索引。像素先按 `SaveTransform`（相机级配置，
  0无/1水平翻转/2垂直翻转/3旋转180/4顺90/5逆90）变换，再用与 camdatx 相同的
  文件名密钥 LUT 加密后写入；静态元数据在数组 attributes 里（`encrypted:true` 等）。
- 写盘基准工具：`bin/<Config>/TsZarrBench.exe <输出目录> [包数] [高] [宽]`，
  对比现方案 fwrite 与 zarr 异步写并做读回校验。
