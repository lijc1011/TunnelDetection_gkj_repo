# TunnelDetection Zarr 现场试验说明

版本：`0.2.0-fieldtrial-20260826`

## 移植前状态评估

本项目原来已经具备 TensorStore Zarr v3 直写链路：`QSaveThd` 按相机异步写
`uint8 [packet,y,x]`，一包一个 chunk，保留 `.camidx`，支持 `SaveTransform`，正常停止时
由 `.zarr.tmp` 改名为 `.zarr`，并有 `TsZarrBench` 写入/回读基准。

原实现尚不适合直接作为现场验证闭环，主要缺口是：桥接层无条件覆盖已有目录；关闭失败后仍可能
改名为正式结果；多通道逐个启动会出现部分通道已经采集、后续通道创建失败；没有磁盘阈值、
camidx 数量校验、采集结果清单、失败清单和程序运行日志；`SaveTransform` 也不是现场 Excel 的完整
多通道转正规则。

## 本版本新增

- 全通道先准备存储目标，全部成功后才统一启动相机。
- 停止时先统一停止全部相机，再逐通道等待 Zarr 收口，减少通道尾部长度偏差。
- Zarr 默认拒绝覆盖；始终写 `.zarr.tmp`，关闭、索引校验和抽样回读全部成功后才收口。
- 每 32 包检查磁盘剩余空间，阈值由 `ZarrMinFreeGB` 配置。
- 收口时校验 `.camidx` 记录数等于 Zarr 包数，回读首末包的三段 SHA-256 指纹。
- 正式目录包含 `acquisition_manifest.json`；失败临时目录保留 `acquisition_failure.json`。
- 程序日志写入应用目录的 `logs/`，写盘失败会显示在对应通道状态和状态栏。
- 接入 Excel 数字化规则；规则 3/6 未标定时拒绝在线转正。
- 程序退出时如仍在采集，会先提示并同步停止相机、收口 Zarr/camidx。
- `Field_Self_Test.bat` 独立验证 Zarr 写读、覆盖保护和规则边界。

## Config.xml 配置

在 `<program_infor>` 中使用：

```xml
<SaveZarr>1</SaveZarr>
<ZarrVerifyOnClose>1</ZarrVerifyOnClose>
<ZarrMinFreeGB>10</ZarrMinFreeGB>
<ZarrNormalize>0</ZarrNormalize>
<ZarrMachineId></ZarrMachineId>
```

第一轮现场试验必须保持 `ZarrNormalize=0`，即 raw Zarr。在线转正仅建议在方向基准确认后启用，
并填写 `ZarrMachineId=A` 或 `B`。

`ZarrNormalize=0` 时程序不会执行规则旋转/翻转，也不会应用相机项中的旧 `SaveTransform`，
Zarr 数组按相机采集到的原始方向和几何尺寸写入；仅继续执行与旧格式一致的 LUT 加密。

在线流式采集不知道最终包数，因此无法安全执行跨包倒序：规则 4/7/8 会主动拦截；规则 3/6 因
原规则未标定也会拦截。当前可在线转正的是无需跨包倒序且已标定的规则 1/2/5。其他工况应采集
raw Zarr，后续再做离线正规化。

规则表“正向/反向”当前按采集界面换算：`增加+车头朝大里程` 或 `减小+车头朝小里程` 为正向，
另外两种组合为反向。在线转正前必须用现场方向基准确认该换算；raw Zarr 不受此换算影响。

## 现场调试步骤

1. 把整个 Release 包解压到现场机器本地 SSD；不要只复制 EXE/DLL。
2. 确认现场机已安装与采集卡匹配的 Teledyne DALSA Sapera 驱动；Release 包带运行 DLL，不能替代采集卡驱动和许可。海康运行 DLL 已随包携带。
3. 备份现场原程序目录、`Config.xml`、相机 `.ccf` 和台账；把现场有效 `Config.xml` 复制到新包。
4. 在 `Config.xml` 增加上述 Zarr 字段，第一轮设置 `SaveZarr=1`、`ZarrNormalize=0`。
5. 双击 `Field_Self_Test.bat`，必须看到 `FIELD SELF-TEST PASS`。
6. 启动程序，只连接一台相机，先显示 5 分钟，不存储，确认帧稳定。
7. 开始存储 20～30 米后正常停止。确认 `.zarr.tmp` 消失，生成 `.zarr`、同名 `.camidx`，
   且 `acquisition_manifest.json` 中 `status=ok`、`packet_count=index_records`、
   `verified_samples` 为 1 或 2。
8. 扩展到全部相机，先低速 20～30 米，再连续采集至少 100 米或原故障距离的 3 倍。
9. 至少执行两轮“开始存储→停止→再次开始”，第二轮不得混入上一轮数据。
10. 测试强制故障：将 `ZarrMinFreeGB` 临时设为大于当前空闲空间，确认程序在启动前拒绝采集，
   然后恢复正确值。
11. 如需验证在线转正，先用规则 1/2/5 的一个通道短采，核对方向后再扩大；不要绕过拦截。

## 问题反馈

提供 `field_self_test.log`、`logs/` 最新日志、Zarr 内的 `acquisition_manifest.json`，或失败
`.zarr.tmp` 内的 `acquisition_failure.json`。可运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Collect_Field_Diagnostics.ps1 -DataRoot D:\现场数据根目录
```

本机已验证 Release 编译、桥接写读、覆盖保护、规则自检和 GUI 无相机启动；尚需现场验证真实相机、
采集卡、触发、里程设备、多通道持续吞吐和现场磁盘。
