# 无砟轨道 4 台 Dalsa CamLink 相机存储后卡死/图像错乱现场修复手册

## 1. 文档用途

本文用于在现场生产机器上重新诊断、应用和验证以下问题的修复：

- 检测模式：无砟轨道。
- 相机：4 台 Dalsa CamLink 接口线阵相机。
- 触发方式：外触发。
- 未开始存储时：4 台相机均能正常连接、采集和刷新显示。
- 手动开始存储后：通常采集 10～30 米，主界面停止刷新；存储图像出现水平分段重复、错位、旧画面重复等异常。

本文记录的是针对该问题的源码热修复方案。个人开发机已完成 Debug 和 Release 编译，但由于没有 4 台 Dalsa 相机、采集卡和现场外触发信号，最终结论必须以生产机器连续采集验证为准。

## 2. 给现场 Codex 的直接指令

到现场后，可把本文件交给 Codex，并提出：

> 请完整阅读 `DALSA_4CAM_FIELD_FIX.md`，先检查生产机仓库、未提交修改、运行目录、本机 `CMakeUserPresets.json` 和当前可执行程序，不要覆盖现场配置。然后依据本文的修复原则核对当前源码，应用缺失的修复，使用生产机自己的 CMake preset 编译 Release，最后按本文的现场验证清单协助测试。不要直接套用个人电脑的 Qt、Visual Studio、OpenCV 或 SDK 路径。

现场 Codex 应先诊断当前代码是否已经包含这些修复，不能只按旧行号机械替换。若生产机代码与本文基线不同，应按语义合并。

## 3. 诊断结论

### 3.1 主要根因：DMA 地址提前释放

原 Dalsa 回调 `CCamera::CallBackSub` 的顺序是：

1. `m_Buffers->GetAddress(&pData)` 获取 Sapera/DMA 图像地址。
2. 立即调用 `m_Buffers->ReleaseAddress(pData)`。
3. 再把 `pData` 复制到显示队列和保存队列。

`pData` 只应在释放地址前使用。提前释放后，采集卡可以复用对应缓冲区；保存启动后回调负担加重，复制期间更容易读到正在被下一帧覆盖的数据。现场图片中的水平块重复和跨块错位与该问题高度一致。

正确顺序必须是：

1. 获取地址。
2. 将图像复制到程序自己管理的显示/保存内存。
3. 最后释放地址。

### 3.2 Sapera trash 帧被当成正常帧

传输模式使用 `SapXferPair::CycleNextWithTrash`。当正常 DMA 缓冲来不及周转时，Sapera 会用 `pInfo->IsTrash()` 标记溢出/不完整帧。

原代码虽然留有检查 trash 的注释，却在检查前直接调用 `CallBackSub`，实际仍把 trash 数据加入显示和保存队列。4 台外触发相机开始存储后更容易触发这一问题。

修复要求：`XferCallback` 必须先检查 `pInfo`、上下文和 `pInfo->IsTrash()`；trash 帧只计数/告警，不得进入显示或保存队列。

### 3.3 CamLink DMA 缓冲数量过少

原 CamLink 初始化只创建 2 个 `SapBufferWithTrash` 缓冲；项目的 Dalsa 网口通路使用 10 个。4 台相机并行采集、保存时，2 个缓冲没有足够的调度余量。

修复要求：CamLink 通路改为 10 个循环缓冲：

```cpp
m_Buffers = new SapBufferWithTrash(10, m_pAcquisition);
```

如果生产机显存/锁页内存受限，应先确认 10 个缓冲能成功 `Create()`，不可忽略创建失败。

### 3.4 采集回调中每帧查询相机 Feature

保存开启后，原代码在每个小帧的 Dalsa 回调中调用 `GetCamFreg`。无砟轨道 CamLink 外触发分支会查询：

- `TriggerMode`；
- `IsFeatureAvailable("measureCC1Rate")`；
- `measureCC1Rate`。

4 台相机高频采集时，这些同步 SDK 调用会延长每个回调，增加 trash 帧和队列积压。

修复要求：采集回调只给 `dHFreqPrecision` 写默认值；保存线程每取出一个 `MAXFRAME` 数据包后查询一次行频，并把该值填入该包的 16 个索引项。这样查询频率降低 16 倍，且离开 DMA 热路径。

### 3.5 显示队列无界增长

显示线程要求所有相机的显示队列都达到 `MAXFRAME` 才刷新。如果某一路短暂落后，其余相机的队列会持续增长，最终造成大量内存占用和主界面卡死。

显示允许丢弃过时画面，因此修复要求：

- 每个相机显示队列最多保留 `MAXFRAME * 2`，当前即 32 个小帧；
- 超限时删除最旧帧；
- 刷新时显示最新的一组连续 `MAXFRAME` 帧；
- 复制、删除和出队必须在同一个显示队列锁内完成。

注意：保存队列不能像显示队列一样静默丢帧。若现场观察到保存队列持续单调增长，应进一步处理磁盘吞吐或主动停止存储，不能简单删除保存数据。

## 4. 当前开发机修复范围

修复基于以下仓库状态开始：

```text
基线提交：c46abda  优化主界面起止里程初始化显示
分支：main
```

修改文件：

- `src/Camera.cpp`
- `src/Camera.h`
- `src/Globals.cpp`
- `src/Globals.h`
- `src/HikCamera.cpp`
- `src/QSaveThd.cpp`
- `src/TunnelDetection.cpp`

核心修改如下。

### 4.1 `src/Camera.h`

- 显式包含 `<atomic>`。
- 将 Dalsa 的 `m_bEnable` 改成 `std::atomic<bool>`。
- 增加 trash 计数：

```cpp
std::atomic<bool> m_bEnable;
std::atomic<unsigned long long> m_trashFrameCount;
```

### 4.2 `src/Camera.cpp`

构造函数初始化原子变量：

```cpp
m_bEnable.store(false);
m_trashFrameCount.store(0);
```

CamLink 缓冲数从 2 改为 10：

```cpp
m_Buffers = new SapBufferWithTrash(10, m_pAcquisition);
```

`XferCallback` 在进入 `CallBackSub` 前丢弃 trash 帧，参考逻辑：

```cpp
void CCamera::XferCallback(SapXferCallbackInfo *pInfo)
{
    if (pInfo == NULL)
        return;

    CCamera *camera = static_cast<CCamera *>(pInfo->GetContext());
    if (camera == NULL)
        return;

    if (pInfo->IsTrash())
    {
        const unsigned long long count =
            camera->m_trashFrameCount.fetch_add(1) + 1;
        if (count == 1 || (count % 100) == 0)
        {
            qWarning() << camera->m_camInfor.Name
                       << QStringLiteral("Sapera 丢弃不完整帧，累计：")
                       << count;
        }
        return;
    }

    camera->CallBackSub(pInfo);
}
```

`CallBackSub` 的关键约束：`ReleaseAddress` 必须在两次复制完成后调用：

```cpp
void *pData = NULL;
if (m_Buffers != NULL &&
    m_Buffers->GetAddress(&pData) &&
    pData != NULL)
{
    if (!m_bEnable.load())
    {
        WriteToShowDeque(pData);
    }
    else
    {
        WriteToSaveDeque(pData, pInfo);
        WriteToShowDeque(pData);
    }

    if (!m_Buffers->ReleaseAddress(pData))
    {
        qWarning() << m_camInfor.Name
                   << QStringLiteral("Sapera 释放图像缓冲地址失败");
    }
}
```

`WriteToSaveDeque` 不再调用 `GetCamFreg`：

```cpp
fmst.dHFreqPrecision = -2;
```

`StartAcquisition`/`StopAcquisition` 使用原子读写：

```cpp
m_bEnable.store(true);
m_bEnable.store(false);
```

显示队列加入上限：

```cpp
const size_t maxShowFrames = static_cast<size_t>(MAXFRAME) * 2;
while (m_ImgDeqShow.size() >= maxShowFrames)
{
    FRAMEIMAGESTRUCT oldFrame = m_ImgDeqShow.front();
    delete[] oldFrame.pDataPoint;
    m_ImgDeqShow.pop_front();
}
m_ImgDeqShow.push_back(fmst);
```

`GetShowDeqDataII` 应在同一个锁内：

- 检查数量是否足够；
- 删除超过 `isize` 的旧帧；
- 复制并释放剩余最新的 `isize` 帧；
- 不再先复制、解锁后又调用 `ClearShowDeg()` 清空新到帧。

### 4.3 `src/QSaveThd.cpp`

保存线程每取出一包后查询一次行频：

```cpp
double packetFrequency = -2;
if (m_cam->GetCamFreg(packetFrequency))
{
    std::fill_n(m_pPicture->picInfor.dHFreqPrecision,
                MAXFRAME,
                packetFrequency);
}
```

同时完成以下健壮性修复：

- 检查 `SaveFrameData` 返回值，失败时停止该通道存储；
- `memset` 使用 `m_pPicture->iBufferSize`，不能使用字段自身的 `sizeof`；
- 检查 `.camdatx` 和 `.camidx` 的 `fwrite` 返回值；
- `StartSave` 先打开索引和数据/Zarr 存储，确认句柄有效后再把存储状态设为 `true`；
- 写盘失败不得继续增加包号并伪装成成功。

### 4.4 `src/Globals.h` 和 `src/Globals.cpp`

`g_isSave` 同时被 UI 线程和保存线程读写，应改成原子数组：

```cpp
extern std::atomic<bool> g_isSave[MAXCAMNUM];
```

```cpp
std::atomic<bool> g_isSave[MAXCAMNUM];
```

使用处改为 `.load()`/`.store()`。

### 4.5 `src/TunnelDetection.cpp`

启动存储的顺序改为：

1. `QSaveThd::StartSave` 打开目标文件/Zarr；
2. `ICamera::StartAcquisition` 让回调开始进入保存队列。

不得在文件尚未打开时先让相机进入保存状态。

### 4.6 `src/HikCamera.cpp`

海康回调同样不再逐帧读取行频，统一由 `QSaveThd` 每包读取一次。该修改不改变海康图像数据格式。

## 5. 生产机操作前检查

### 5.1 停止现场程序

必须先正常停止存储和采集，再关闭程序。确认任务管理器中没有仍在运行的 `TunnelDetection.exe`，避免替换正在使用的文件或损坏 `.tmp` 数据。

### 5.2 记录代码和本地修改

在生产机仓库根目录执行：

```powershell
git rev-parse HEAD
git status --short --branch
git diff
git log -1 --oneline
```

如果存在未提交修改：

- 不要执行 `git reset --hard`；
- 不要直接 `pull` 覆盖；
- 先建立临时分支并提交，或至少把 `git diff` 保存到仓库外的备份目录；
- 明确哪些修改属于生产机专用代码。

### 5.3 保护生产机本地文件

以下内容不得用个人电脑版本覆盖：

- `CMakeUserPresets.json`；
- `Config.xml`；
- 所有相机 `.ccf` 文件；
- 相机、采集卡和里程设备相关本地配置；
- 当前运行目录中的生产专用 DLL；
- 现场尚未上传的采集数据。

生产机已知可能使用 VS2022、Qt 6.11.1 和 `C:/dev` 下的依赖；必须以生产机现有 `CMakeUserPresets.json` 为准。

### 5.4 备份当前可执行程序

在明确的备份目录中保存：

- 当前 `TunnelDetection.exe`；
- 当前 `TsZarrBridge.dll`；
- 当前程序目录文件清单和版本时间；
- 当前 `Config.xml` 与 `.ccf` 文件副本。

不要把备份放回编译输出目录，避免后续构建覆盖。

## 6. 生产机源码处理流程

1. 比较生产机当前 `src/Camera.cpp` 与本文第 3、4 节。
2. 检查修复是否已经通过 Git 同步；已经存在的逻辑不要重复添加。
3. 如果代码有现场专用修改，按语义合并，不能整文件覆盖。
4. 用 `git diff --check` 检查空白和冲突。
5. 用 `rg` 核对关键点：

```powershell
rg -n "SapBufferWithTrash|IsTrash|ReleaseAddress|GetCamFreg|maxShowFrames|g_isSave" src
```

关键不变量：

- `CallBackSub` 中先复制，后 `ReleaseAddress`；
- `XferCallback` 在调用 `CallBackSub` 前过滤 trash；
- CamLink 有足够 DMA 缓冲；
- Dalsa 回调中没有逐帧 `GetCamFreg`；
- 显示队列有界；
- 保存队列不得静默丢帧；
- 文件打开成功后才开始保存。

## 7. 生产机编译

先列出生产机实际预设：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --list-presets=all
```

优先使用生产机原有配置和 Release 预设。示例名称若仍为 `local`：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --preset local
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --build --preset local-release
```

如果生产机已有完整构建树且只修改主程序，可先使用：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --build --preset local-release-fast
```

若改过 `CMakeLists.txt`、`src/tszarr` 或 TensorStore，必须全量构建。不要删除有效的 TensorStore 源码和首次下载缓存，除非确认缓存损坏。

构建成功标准：

- 命令退出码为 0；
- `bin/Release/TunnelDetection.exe` 时间戳更新；
- 若使用 Zarr，`bin/Release/TsZarrBridge.dll` 存在；
- Qt/OpenCV 部署步骤没有致命错误。

## 8. 部署原则

- 优先在“开发/构建目录”和“现场运行目录”之间做明确区分。
- 不要直接把个人电脑的整个 `bin/Release` 覆盖到生产机。
- 本次主要修复位于主程序；若 `TsZarrBridge` 源码未变化，通常只需更新生产机本地编译出的 `TunnelDetection.exe`。
- 更新前必须备份旧程序；更新时确保程序已退出。
- 不覆盖 `Config.xml`、`.ccf` 和现场数据。

## 9. 现场验证方案

### 9.1 第一阶段：只显示、不存储

1. 连接 4 台 Dalsa CamLink 相机。
2. 确认均为外触发模式。
3. 连续采集显示至少 10 分钟。
4. 确认四路画面持续刷新，无连接弹窗和明显条带错位。

### 9.2 第二阶段：短距离存储

1. 开始手动存储。
2. 先低速采集 20～30 米。
3. 停止存储并正常收口文件。
4. 立即检查四路数据，不得出现约一个小帧高度为周期的水平重复块。

### 9.3 第三阶段：超过原故障距离

连续采集至少 100 米，或至少达到原故障距离的 3 倍。测试期间每隔 1 分钟记录：

- 4 路显示是否刷新；
- 进程内存占用；
- 磁盘写入速度和活动时间；
- 每路“显示队列”和“存储队列”长度；
- Sapera 是否出现 trash/overflow 告警；
- 数据盘剩余空间。

期望：

- 显示队列不会超过约 32 个小帧并长期增长；
- 保存队列可在 0～若干包之间波动，但不能持续单调上升；
- 进程内存不能随里程持续线性增长；
- 主界面持续刷新；
- 四路存储数据连续，无水平块重复、错行和旧帧回灌。

如果保存队列持续增长，即使界面暂时正常，也说明总数据率高于实际写盘处理能力。此时应先记录：

- 每台相机 `ImageWidth`、`ImageHeight`、颜色通道数；
- 外触发行频；
- 4 台相机总字节率；
- 数据盘连续写入性能；
- `SaveZarr` 是 0 还是 1。

不要通过无限增大保存队列掩盖吞吐不足；那只会把卡死推迟到更远距离。

### 9.4 第四阶段：停止并再次启动

至少执行两轮“开始存储 → 采集 → 停止存储 → 再次开始”。第二轮开头不得出现上一轮残留画面或旧数据，`.tmp` 文件应正常收口。

## 10. 数据正确性检查

每台相机至少抽查：

- 文件开头、中间、故障原发生距离附近、文件结尾；
- 连续多个 4096 高度图片块；
- 水平边界是否连续；
- 轨枕、钢轨和道砟纹理是否发生周期性重复；
- 四路时间戳、脉冲数和里程是否单调且大致同步。

旧 `.camdatx` 模式还应检查：

- 文件大小应为完整数据包大小的整数倍；
- `.camidx` 记录数量与数据包数量一致；
- 写盘失败后程序不得继续伪造包号。

Zarr 模式还应检查：

- `.zarr.tmp` 在正常停止后重命名为 `.zarr`；
- packet 维长度与 `.camidx` 记录数量一致；
- 抽样读回结果与现场画面一致。

## 11. 失败时需要保留的信息

如果修复后仍出现卡死或数据错乱，不要立即重启并删除现场证据。先保存：

- 故障发生时的里程和时间；
- 四路显示/保存队列长度；
- 任务管理器中的内存、CPU、磁盘写入速度；
- Sapera trash/overflow 计数；
- `Config.xml` 中相关相机参数；
- 4 个 `.ccf` 文件；
- 故障点前后各若干数据包和 `.camidx`；
- 程序版本、Git 提交号和 `git diff`；
- `SaveZarr` 配置值；
- 是否只有某一路先停止刷新。

如果显示队列稳定但保存队列持续增长，下一步重点排查写盘吞吐。如果保存队列不增长却仍有固定高度重复条带，下一步重点核对 Sapera 当前缓冲索引、`ImageWidth/ImageHeight/iSizeFrame` 与 `.ccf` 实际帧尺寸是否一致。

## 12. 回滚方法

现场验证失败时：

1. 正常停止存储和采集。
2. 退出程序并确认进程结束。
3. 保存本次失败数据和日志。
4. 恢复备份的旧 `TunnelDetection.exe`。
5. 不要覆盖或删除现场 `Config.xml`、`.ccf` 和采集数据。
6. 源码回滚优先使用事先保存的补丁或独立分支；不要使用 `git reset --hard` 清除不明来源的现场修改。

## 13. 当前验证记录

个人开发机已执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --build --preset local-debug-fast
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --build --preset local-release
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\cmake.ps1 --build --preset local-release-fast
```

结果：Debug、Release 全部退出码为 0，并生成 64 位主程序。构建中的第三方源码代码页警告和 `windeployqt` 的 `VCINSTALLDIR` 提示未导致失败。

尚未验证：4 台 Dalsa CamLink 相机、生产采集卡、真实外触发、现场磁盘以及 100 米以上连续存储。

