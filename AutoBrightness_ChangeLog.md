# 会话记录：海康双相机连接修复 + 自动亮度自适应功能

> 记录日期：2026-07-02　　分支：main　　构建预设：`local-debug`（VS2022 x64）

本文件汇总本次会话的两项改动，供现场分项调试与后续维护参考。

---

## 一、海康双相机只连上 1 台 / 不刷新 / 不存储 —— 已修复

### 根因
- `Config.xml` 里两台相机 `<CamLinkName />` 都为空，而海康匹配原先用 `CamLinkName` 字段 → 两台都命中枚举到的第 0 台设备 → 第二台独占打开失败。
- `initCamera` 仅当**全部相机连上**才返回 true，否则显示线程/存储线程都不启动（`TunnelDetection.cpp` 约 1937 行）→ 表现为「只连 1 台 + 不刷新 + 不存储」。

### 匹配约定（现场纠正后）
- DALSA 网口相机（CameraType=0）：靠 **`Name`** 匹配。
- DALSA CamLink 相机（CameraType=1）：靠 **`CamLinkName`** 匹配。
- **海康是网口相机，也用 `Name` 匹配**（对应 MVS 的 Device User ID / 固定IP / 序列号之一）。

### 改动
- `src/HikCamera.cpp` `FindDeviceIndex`：改用 `m_camInfor.Name`（为空才退化取第一台）；顶部与 `HikCamera.h` 注释同步更正。

### 现场使用要求
- 每台海康的 MVS **Device User ID 必须与 Config 的 `<Name>` 完全一致且互不相同**
  （例：`9Cam_YGH9`、`10Cam_YGH10`）。

---

## 二、自动亮度自适应（曝光/增益）功能 —— 新增

### 设计要点
- **总开关**：全局 `std::atomic<bool> g_autoBrightnessEnabled`，默认 `false`（手动）。
- **完全隔离**：AE 全部逻辑包在 `QShowThd` 热路径的 `if (g_autoBrightnessEnabled.load())` 内。
  手动模式下该段一行不进，显示/存储与未加功能时**逐字节一致**。默认即「开箱等于老程序」。
- **控制策略**：曝光优先 → 曝光顶到限值仍不足时增益兜底；两者都在 config 限值内。
  增益按 dB 在线性域换算（亮度 ∝ 10^(dB/20)），不做除以增益的运算，杜绝除零/NaN。
- **稳定性**：亮度死区 + 单步限速 + 抽样测亮度 + 每 N 幅降频执行 + 每台相机独立控制器。
- **写盘联动**：免费。曝光/增益下发后，后续帧自动带新亮度同时进 show/save 队列，`QSaveThd` 照常写盘，无需额外代码。

### 改动文件清单
| 模块 | 文件 | 内容 |
|---|---|---|
| 总开关 | `src/Globals.h` / `src/Globals.cpp` | 新增 `g_autoBrightnessEnabled{false}` |
| 对话框(B4) | `src/QSetCameraDlg.cpp` / `.h` | 上下 GroupBox：上=「亮度模式」手动/自动 radio(默认手动)，下=「相机参数」原控件；radio 仅翻转全局开关 |
| 状态回显(C1) | `src/QSetCameraDlg.cpp` | 自动模式下 timer 把相机真实曝光/增益实时刷到界面框；手动模式不刷新 |
| 控制器(A) | `src/AutoExposureController.cpp` / `.h` | 曝光优先+增益兜底(dB换算)、死区、限速、抽样、`reset()` |
| 集成(A1/B1-B3) | `src/QShowThd.cpp` / `.h` | 每台独立控制器数组；上升沿用 `GetExpTime/GetCamGain` 播种+清积分；入参(height,width)摆正；每 N 幅降频执行；总开关门控 |

### 可整定参数
- `src/AutoExposureController.cpp`：`kp_`(0.3)、`ki_`(0.02)、`deadband_`(4)、`max_step_ratio_`(0.15)、`sample_stride_`(8)、构造默认 `target=128`。
- `src/QShowThd.h`：`m_aeInterval`(3，每 N 幅执行一次 AE)。

### 现场分项调试顺序
1. **先验隔离**：默认(手动)跑一遍，确认连接/刷新/存储与以前完全一致。
2. **开自动**：对话框选「自动亮度」，观察曝光框是否随画面亮度动态变化、能否收敛到中等亮度不振荡。
3. **整定**：振荡→调小 `kp_`/`max_step_ratio_` 或调大 `m_aeInterval`；太慢→反之。

### 已知取舍 / 待办(暂缓)
- `SetCamGain(int)` 目前四舍五入取整；增益范围仅 1~3dB，整数步进够用。需更细再按方案 E1 改 double。
- AE 触发周期 = 攒满一整幅图 × `m_aeInterval`，实际快慢取决于行频。
- 参数集中到 config、主界面(而非对话框)显示状态等，为后续可选项。

---

## 三、构建与验证
- 命令：`cmake --build --preset local-debug`
- 状态：**编译链接通过**（唯一告警 `TunnelDetection.cpp:1114` 为既有告警，与本次改动无关）。
- 注意：若报 `LNK1168 无法打开 TunnelDetection.exe`，是程序正在运行占用文件，关掉再构建即可。
- 运行期硬件行为（两台相机实际连接/刷新/存储、AE 收敛）需现场用真实相机验证。
