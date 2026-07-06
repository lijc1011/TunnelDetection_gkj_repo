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

采集数据默认写 Zarr v3（`Config.xml` 里 `SaveZarr=1`；设为 `0` 回退旧 `.camdatx` 流程）。
TensorStore 源码 vendor 在 `3rdparty/TensorStore`，经 FetchContent 并入构建树；
主程序只链接窄接口桥 `TsZarrBridge.dll`（`src/tszarr/`，C ABI）。

### 首次构建须知

- **需要联网**：首次 configure 会下载 abseil/riegeli 等依赖源码到 `build/local/_deps`。
- **需要 Python ≥ 3.10**（bazel_to_cmake 在配置期运行；python.org 安装版走注册表即可被找到，无需加 PATH）。
- 首次构建用 **全量** 预设/任务，之后日常用 **增量**。

### 日常构建（增量，快）

- VS Code 小齿轮/小甲虫/小三角默认走 `local-debug-fast` 预设：
  `msbuild /p:BuildProjectReferences=false`，只编译主程序，不扫描 TensorStore 的几百个工程。
- 命令行：`.\tools\cmake.ps1 --build --preset local-debug-fast`

### 何时需要全量构建（任务：Build Debug/Release FULL (TensorStore)）

- 首次克隆/首次构建；
- 改了 `3rdparty/TensorStore` 或 `src/tszarr/`（桥接层）；
- 改了根 `CMakeLists.txt`（增量预设跳过 ZERO_CHECK，不会自动重新 configure）。

### 存储格式说明

- 每相机一次检测产出 `<同名>.zarr` 目录（数组 uint8 `[包, 行, 列]`，chunk=一包=一个文件，
  bytes codec 不压缩）+ 照写的 `.camidx` 索引。像素先按 `SaveTransform`（相机级配置，
  0无/1水平翻转/2垂直翻转/3旋转180/4顺90/5逆90）变换，再用与 camdatx 相同的
  文件名密钥 LUT 加密后写入；静态元数据在数组 attributes 里（`encrypted:true` 等）。
- 写盘基准工具：`bin/<Config>/TsZarrBench.exe <输出目录> [包数] [高] [宽]`，
  对比现方案 fwrite 与 zarr 异步写并做读回校验。
