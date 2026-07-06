# TensorStore 本地补丁存档

`3rdparty/TensorStore/`（vendored 源码，约 30MB/2400 文件）**不进版本库**（见根 .gitignore），
本目录只保存我们在其上打过的本地补丁文件，随仓库版本化。

## 恢复 3rdparty/TensorStore 的步骤（新机器 / 目录丢失时）

1. 下载 upstream 源码：google/tensorstore，commit `c0ea7e41ff708a9069f8058b41bef6dde6ab6a0d`（2026-06-04）
   https://github.com/google/tensorstore
2. 解压到 `3rdparty/TensorStore/`（不要保留 `.git` 目录）。
3. 用本目录下的文件**按相同相对路径覆盖**：
   - `tools/cmake/bazel_to_cmake/util.py`
   - `tools/cmake/run_bazel_to_cmake.py`

## 补丁内容（为什么必须打）

工程路径含中文（`d:\升级\...`）。bazel_to_cmake 用 `json.dumps` 生成 CMake 字符串时
默认 `ensure_ascii=True`，把中文转成 `\uXXXX` 转义，CMake 解析报
`Invalid character escape '\u'`，configure 直接失败。补丁将其改为
`ensure_ascii=False` 直接输出 UTF-8（文件内有 `LOCAL PATCH(TunnelDetection)` 注释标记）。

## 其他环境前提（同样是 configure 必需）

- 联网（首次 FetchContent 下载 abseil/riegeli 等依赖源码）
- Python >= 3.10（python.org 安装版即可，注册表可发现，无需加 PATH）
- NASM（CMakeUserPresets.json 里 `CMAKE_ASM_NASM_COMPILER` 指向 `C:/dev/nasm-2.16.03/nasm.exe`）
