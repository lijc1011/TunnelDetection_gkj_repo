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

未完成的工作应提交到临时分支并推送；不要依赖 `git stash` 跨电脑。不要提交 `CMakeUserPresets.json`、`build/`、`bin/` 或 DLL/PDB 等构建产物。
