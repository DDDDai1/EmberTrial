# M0-01 工程与运行结果核对

日期：2026-09-18。用户反馈：Build 无错误，移动 / 转向 / 跳跃正常。

## 1. 工程确实已创建并运行

- 工程：`E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`。
- 主 Runtime 模块：EmberTrial，包含第三人称 Character / GameMode C++。
- 模块依赖：Core、CoreUObject、Engine、InputCore、EnhancedInput。
- 游戏 / 编辑器 Target 均存在；DefaultBuildSettings 为 V5。
- `Binaries/Win64/EmberTrialEditor.target` 是 Win64 / Development / Editor 构建记录，DLL 与 PDB 均存在。
- 最新日志记录了 ThirdPersonMap 的 PIE 启动与停止。
- 移动 / 转向 / 跳跃的交互结果来自用户实机确认，我本轮未代操作键鼠进行复验。

## 2. 发现并确认的版本差异

本次实际创建和验证的是 **UE 5.4.4**，依据互相一致：

1. `.uproject` 的 EngineAssociation 为 `5.4`。
2. 两个 Target.cs 使用 `EngineIncludeOrderVersion.Unreal5_4`。
3. 构建 receipt 的引擎版本为 `5.4.4`。
4. `Saved/Logs/EmberTrial.log` 显示 `Engine Version: 5.4.4-35576357`，引擎路径为 `D:/UE5/UE_5.4/Engine/Binaries/Win64/`。

原计划基线为 5.5.4。用户已明确选择“按原计划统一为 UE 5.5.4”，并确认已保存、关闭 UE 与 Visual Studio。因此保留 5.4.4 本地基线后再迁移；不能把本次 5.4 的结果当作 5.5 的验证。

## 3. 已查到的工具链与设备依据

- 编译响应文件 `Intermediate/Build/Win64/x64/UnrealEditor/Development/EmberTrial/EmberTrial.Shared.rsp` 引用 MSVC `14.38.33130` 与 Windows SDK `10.0.26100.0`。
- 日志显示 D3D12，选择的 adapter 0 为 NVIDIA GeForce RTX 4060 Laptop GPU。
- CPU：AMD Ryzen 7 7840H。
- 日志报告可用物理内存总量约 15554 MB，约属 16 GB 设备；这不是独立读取的内存条安装规格。
- 初次资产编译日志有可用内存预算紧张提示；本次启动时空闲约 1346 MB。后续编译控制并发、避免同时启动多个 UE，不据此断言系统内存泄漏或必须升级硬件。

最新日志还出现 EOS 编辑器服务权限警告，但未见其阻断此单机模板 PIE；本阶段不因此加入联网功能或改动账号配置。

## 4. 本地版本管理准备

- 引入 `.gitignore`，排除 Binaries、Intermediate、Saved、DerivedDataCache、IDE 临时文件和发布产物。
- `.gitattributes` 将 `.uasset` / `.umap` 交给 Git LFS；当前为 211 个 uasset 和 1 个 umap，约 405 MiB。
- 保留外部 Actor / Object 资源目录，不把地图依赖错误地忽略。
- 仓库只在本地建立；没有配置远端、上传或发布资产。
- 首次空仓库由沙盒账户创建，出现本机账户 ownership 检查冲突；已在确认没有对象、引用或暂存数据后，仅重建该空 `.git` 并用用户账户初始化，工程文件未删除。

迁移及其复验结果继续记录到 `Docs/Reports/M0_EngineMigration.md`。
