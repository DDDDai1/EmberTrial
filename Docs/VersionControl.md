# 本项目的 Git / LFS 基线

Git 仓库根目录：`E:\UE_Project\Demo`。UE 工程位于其中的 `EmberTrial` 子目录。

## 1. 已配置内容

- `.gitignore` 排除编译产物、Intermediate、Saved、缓存和 IDE 临时文件；这些文件可重新生成。
- `.gitattributes` 将 `.uasset` / `.umap` 交给 Git LFS，并为后续 Blender / FBX 等源资产预留规则。
- 源码、Config、`.uproject`、`.vsconfig`、文档和实际 Content 进入版本管理。
- World Partition 的 `__ExternalActors__` / `__ExternalObjects__` 也是地图内容的一部分，已保留。
- 当前没有远端，没有上传。这里只是本地历史，不是异地备份。

## 2. 第一个可恢复检查点

`3562f61`：用户创建并验证的 UE 5.4.4 模板工程，作为迁移前检查点。

初始提交包含 237 个文件，其中 212 个 UE 资产由 LFS 管理；LFS 完整性检查通过。模板原有的个别尾随空白予以保留，没有为了格式重写用户的模板代码。

迁移后的编译通过检查点为 `861be2b`，保留版本变化的清晰差异。提交号和验证结果记录到 Progress 与迁移报告。

M0-03 检查点为 `6259374`，包含 Actor 练习和后续交互提示指南。

### M0-04 阶段检查点

本阶段的检查点提交标题约定为 **`feat: complete M0 interaction and pause prototype`**，用于保存用户已验收的 A / B / C 源码、蓝图、接口、UMG、输入映射、场景外部 Actor，以及验收记录和下一批动画指南。动画指南已下发不代表动画资产已制作。

用下面命令查找该提交及其实际提交号；以 Git 历史是否存在为准，不把未提交工作区当成已保存检查点：

```powershell
git log --oneline --all --grep="complete M0 interaction and pause prototype"
```

该检查点只在本地保存，不包含 Saved / Intermediate / Binaries、原始运行日志或 Artifacts 备份，也不是外部备份。项目尚未通过打包验收。

### M0-05A 阶段检查点

本阶段检查点标题为 **`feat: complete M0 attack animation prototype`**，包含已验收的攻击动画、制作 Sequence、动画试场、数值及资产检查脚本、验收记录和下一批 B 指南。B 指南及参数已校准，不代表闪避／受击 UE 资产已制作。

```powershell
git log --oneline --all --grep="complete M0 attack animation prototype"
```

以 Git 中实际提交为准。用户产生的 `DefaultEditor.ini` 共享预览场景配置改动保留在工作区，本检查点未纳入该预览配置；原始检查日志与临时报告继续排除。无远端上传。

### M0-05B 补修前检查点

本轮检查点标题为 **`chore: checkpoint M0 defensive animations before corrections`**。保存用户已制作的闪避 / 受击 AS、源 Sequence、旧路径文件、动画试场，以及只读审查脚本、六项待修记录和手动补修指南。

这是**补修前版本**，不表示 M0-05B 验收通过：播放范围与四个关键帧数值仍待修正。以实际 Git 历史是否存在为准：

```powershell
git log --oneline --all --grep="checkpoint M0 defensive animations before corrections"
```

仍保留 `DefaultEditor.ini` 预览配置改动在工作区，不纳入本检查点；未上传远端。补修完成后再保存验收通过版本。

### M0-05B 补修复验与 C1 交接检查点

本轮标题为 **`chore: record M0 correction review and prepare montage guide`**。保存用户已修正时长与受击参数的五个资产文件、复验记录、C1 前置读取脚本与手动指南。B 仍剩闪避 0 帧左上臂 Z 一项，C1 仅下发步骤，不能把本提交当作 B / C1 全部验收通过。

```powershell
git log --oneline --all --grep="record M0 correction review and prepare montage guide"
```

之前的补修前检查点为 `30a8289`。本轮仍不纳入 DefaultEditor.ini 的预览配置，不上传远端。

### C1 播放修复与 C2 组件准备检查点

本轮标题为 **`fix: correct M0 montage speed gate and prepare action component`**。记录用户保存的速度分支修复、C1 Montage / Input / Slot 资产、最终闪避补修、只读图表检查脚本和 C2-01 指南。

```powershell
git log --oneline --all --grep="correct M0 montage speed gate"
```

以实际 Git 历史为准。本检查点表示 B 补修与 C1 播放故障已处理，不表示 C1 全部边界或 C2 实现已验收。新的动作组件只在指南中提供参考代码，尚未创建。DefaultEditor.ini 的预览配置继续留在工作区，原始日志 / 报告不入库，无远端上传。

### C2-01 基础组件检查点

**92a2c63**，标题 **`feat: checkpoint M0 action component scaffold`**。保存用户手动创建的基础组件、已挂载组件的玩家蓝图以及 C1 完整验收记录。C2-01 用户实际约 40 分钟，Build / 三次 PIE / 基础功能与 C1 边界通过。

### C2-02 实现与手动交接检查点

本轮标题为 **`feat: implement M0 action lifecycle and interruption`**。包含动作组件状态 / 回调 / 打断与清理、Character 输入限制、综合自动测试、验收记录和手动接线指南。

```powershell
git log --oneline --all --grep="implement M0 action lifecycle and interruption"
```

正常 C++ Build 和 1 项综合自动测试通过；本提交**不表示 C2-02 玩家蓝图接线及 PIE 已验收**。三个 Montage 赋值、三条 IA 替换和 E 门控由用户接下来执行；之后另建资产验收检查点。DefaultEditor.ini 的原有预览配置改动继续保留在工作区，Raw 原始日志不纳入提交，无远端上传。

### C2-02 实机验收检查点

本轮标题为 **`feat: accept M0 action lifecycle and prepare dodge configuration`**，保存用户本轮已保存的玩家 / 练习物体蓝图、C2-02 实机验收记录及 C2-03A 手动参数指南。用户约 15 分钟，全部实机验收通过。C2-02 代码检查点为 `55c46cd`；本批不改 C++，C2-03 的配置和位移尚未实施。

```powershell
git log --oneline --all --grep="accept M0 action lifecycle and prepare dodge configuration"
```

以实际 Git 历史为准；DefaultEditor.ini 仍不纳入，未上传远端。

### C2-03A 验收与 B 手动交接检查点

本次检查点标题为 **`docs: accept dodge configuration and guide manual C++ implementation`**，保存用户手动新增的两个头文件属性、A 验收、B 手动代码指南与最新协作约定。仅 A 已通过；B 的 C++ 只存在于 Markdown 代码块，未写入工程或构建。DefaultEditor.ini 仍保留在工作区。

### C2-03B 首轮实现与补修前检查点

本次标题为 **`chore: checkpoint M0 dodge before guard correction`**，保留用户已实现的位移 / 清理源码、50 分钟实机反馈、源码审查、计时与帧率证据，以及局部补修指南。**这是补修前版本**：RequestPracticeAction 缺少参数 / 外部位移源保护，120 上限样本仍需核实；不表示 B 最终验收通过。

仅用户已经写入的 C++ 进入该检查点；助手本轮只更新文档，没有替用户补代码。DefaultEditor.ini 原有改动仍不纳入，原始日志摘要在 Raw 中本地保存，无远端上传。

### C2-03B 最终验收与 C 手动交接检查点

本次标题为 **`feat: accept M0 dodge guards and prepare movement regression`**，保存用户补入的保护 / SourceTime / Lifecycle 用例，以及编译排障、B 验收记录和 C2-03C 手动指南。补修前检查点为 `a2d34e0`。B 实现与实机验收完成，C 的新测试仅存在于文档，不表示已经构建或执行。

仍仅保存本地；DefaultEditor.ini 原有改动不纳入，Raw 筛选日志不入库。用户本轮约 30 分钟，与 B 首轮 50 分钟分开登记。

## 3. 你需要知道的日常规则

1. UE 中修改资产后先保存，Git 才能记录磁盘上的结果；未保存的蓝图不在提交中。
2. 不手工改二进制资产内容；同一个地图或蓝图避免双方同时修改。
3. 不把 `.git` 当作可清理的生成目录；只复制源码而漏掉 LFS 对象，不能保证保留完整历史资产。
4. 大型 Trace、视频、dump 与试玩包另行归档；小型证据摘要放在 Docs/Reports。
5. 需要恢复某个版本时告诉我提交号和目标；先保留当前改动，再选择独立副本或其他合适方式验证，不盲目 reset 或覆盖当前工作区。

## 4. 可选的只读查看命令

在工作区打开终端后，可用：

```powershell
git status --short
git log --oneline -5
git lfs ls-files
```

`git status` 看哪些文件变化，`git log` 看提交历史，`git lfs ls-files` 看哪些二进制文件由 LFS 管理。你当前不需要额外执行这些命令才能继续练习。
