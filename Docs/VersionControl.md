# 本项目的 Git / LFS 基线

Git 仓库根目录：`E:\UE_Project\Demo`。UE 工程位于其中的 `EmberTrial` 子目录。

**2026-10-09现状：**本地已有16次提交，分支`main`，最近一次为`5d84916`（M0闪避回归与组合演示指南）；当前没有Git远端。M0后续与M1最新开发成果仍有大量未提交改动，不代表没有Git，也不代表GitHub已备份。当前运行验收已到M1-05C1，开发暂不进入C2。下面新增第5–7节给出本次提交范围和CLion操作；前面的阶段检查点说明保留为历史记录。

## 1. 已配置内容

- `.gitignore` 排除编译产物、Intermediate、Saved、缓存和 IDE 临时文件；这些文件可重新生成。
- `.gitattributes` 将 `.uasset` / `.umap` 交给 Git LFS，并为后续 Blender / FBX 等源资产预留规则。
- 源码、共享Config、`.uproject`、`.vsconfig`、文档和完整Content进入版本管理；本机IDE选择与Content Browser选中路径两份配置留在本地，详见第5节。
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

### C2-03C 验收与 C3 交接检查点

本次标题为 **`feat: accept M0 dodge movement regression and prepare combined demo`**，保存用户手动完成的新位移测试和测试友元、C2-03 / C2 验收记录、VS 目录同步说明，以及 C3 组合演示指南。上个 B 验收检查点为 `ac9eaa1`。

用户 C 实际约 30 分钟，Build / 两次新测试 / 旧测试报告通过；本地直接核实一次 DodgeMovement 和一次 Lifecycle Success，重复通过依用户反馈登记。C3 视频和首次打包仍待完成。本次保持用户源码原样，DefaultEditor.ini 原有改动不纳入，无远端上传，视频与 Raw 筛选证据不入普通 Git。

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

## 5. 本次.gitignore设计：哪些提交，哪些留在本地

本次直接修改根目录[.gitignore](../.gitignore)，保留[.gitattributes](../.gitattributes)原有LFS规则。只隐藏可重新生成的文件、本机偏好与原始产物，不通过忽略源码或蓝图来缩小Commit列表。

| 路径 / 文件 | 本次处理 | 原因 |
| --- | --- | --- |
| 根.gitignore / .gitattributes / Introduction.md | 提交 | 忽略规则、LFS规则与项目介绍必须随仓库保存 |
| EmberTrial/EmberTrial.uproject | 提交 | 引擎关联、模块与插件定义 |
| EmberTrial/Source全部，包括Tests、Build.cs、Target.cs | 提交 | 运行源码、模块构建规则与已有自动回归；测试源码也属于项目 |
| EmberTrial/Content全部 | 提交，二进制资产走LFS | Blueprint、UMG、GA、GE、BB、BT、输入、动画、Sequence、模型、材质及地图均可能是运行或制作依赖；保留模板资源与Tests样例 |
| Content/__ExternalActors__与__ExternalObjects__ | 提交，走LFS | 外部Actor / 对象是地图内容，不是缓存；不能只提交.umap而遗漏它们 |
| Config/DefaultEngine.ini、DefaultGame.ini、DefaultInput.ini、DefaultEditor.ini | 提交 | 共享输入、碰撞 / 引擎、打包与编辑器预览设置；DefaultEditor.ini现有改动是共享预览配置，可纳入本次完整检查点 |
| Config/DefaultEditorSettings.ini | 本地忽略 | 当前只有PreferredAccessor=CLionSourceCodeAccessor，属于本机代码编辑器偏好 |
| Config/DefaultEditorPerProjectUserSettings.ini | 本地忽略，先按第6节取消跟踪 | 当前只有Content Browser选择路径；已在旧历史中跟踪，单改.gitignore不会取消跟踪 |
| EmberTrial/.vsconfig | 提交 | 描述VS2022构建组件；使用CLion依然需要对应编译工具链 |
| Docs，包括ADR、操作指南、Reports中的.md、Data和AssetManifest.csv | 提交 | 设计、进度、参数、资源来源与可读验收摘要；Raw子目录除外 |
| Scripts中的.py、Tools/Refresh-CLion.ps1 | 提交 | 资产检查与索引再生成工具；脚本的EngineRoot可显式传入其他安装路径 |
| Build中的手工图标 / 平台资源 | 保留提交资格 | 不忽略整个Build文件夹；仅忽略Cook生成的FileOpenOrder |
| Binaries / Intermediate / Saved / DerivedDataCache | 本地忽略 | 构建产物、生成代码、自动保存、运行日志和缓存 |
| .idea / .vs、.sln / .vcxproj等 | 本地忽略 | IDE生成状态；更换IDE或重新生成即可恢复 |
| EmberTrial/.run | 本地忽略 | 当前EmberTrialEditor.run.xml含本机E盘工程、D盘引擎绝对路径；可按CLion指南重新配置 |
| EmberTrial/CMakeLists.txt、cmake-build-*、compile_commands.json、CMake缓存 | 本地忽略 | 当前CMakeLists由Refresh-CLion脚本生成；其他位置未来手写的CMakeLists不会被此规则误忽略 |
| CMakeUserPresets.json | 本地忽略 | 个人CMake参数；共享CMakePresets.json保留提交资格 |
| Artifacts、Docs/Reports/Raw、.log / .utrace / .ucache / .dmp | 本地忽略 | 安装包、视频、备份、原始日志与Trace；验收摘要照常提交 |
| Python缓存 / 虚拟环境、临时文件、Blender编号备份 | 本地忽略 | 可再生成；实际.blend / .fbx / .psd源资产保留并按现有LFS规则管理 |
| .env及其环境变体 | 本地忽略 | 本机环境配置；.env.example / .env.sample / .env.template保留 |

当前Content有259个.uasset / .umap提交候选；AssetManifest的40项是重点资源来源记录，**不是仅提交40个资产的白名单**。不要按文件名看不懂、是不是模板或是不是Tests来删选Content。

原始证据留在本地，因此GitHub上的验收文档仍可读，但其中指向Raw / Artifacts的本地证据链接不会有仓库文件。需要交付作品时，再按发布安排提供视频、试玩包与整理后的证据附件。本次规则用于开发仓库，不把Commit视为发布试玩包。

## 6. 在CLion提交前，仅需一次取消旧个人配置跟踪

本次查到**一份已经被Git跟踪、现在应忽略的文件**。在CLion底部Terminal中运行：

```powershell
Set-Location 'E:\UE_Project\Demo'
git rm --cached -- EmberTrial/Config/DefaultEditorPerProjectUserSettings.ini
```

只移除索引中的这一个路径，磁盘文件保留，当前编辑器设置继续可用；不删除文件，也不清空整个索引。.gitignore本身不会影响已跟踪文件，这是[Git官方规则](https://git-scm.com/docs/gitignore)。

运行后Commit列表出现该文件的删除记录是正确结果：**把这条删除和.gitignore一起提交**。该删除表示新版本不再管理它，不表示删除本机设置或擦除旧提交历史。助手本次只改忽略文件与本文，没有替你修改索引 / Commit / 远端。

可选检查，输出应为空：

```powershell
git ls-files --cached --ignored --exclude-standard
```

若不为空，只把输出发给我；不要自行执行整个仓库的清缓存或强制添加忽略文件。

## 7. 在CLion提交并上传的顺序

1. 保存UE源码 / 资产与文档，回到CLion并刷新版本控制文件列表；无需为了.gitignore改动重新Build。仓库根应为E:\UE_Project\Demo，包含EmberTrial和Docs，不要另建嵌套仓库。
2. 打开**Commit工具窗口**。把第5节应提交的修改、新文件与真实重命名纳入本次提交；Unversioned Files里新增源码、GA / BT / UI等资产、文档与Tools都需要加入Git。忽略目录里的文件不需要强制加入。
3. 当前已有部分暂存文件出现AM / RM：表示暂存版本与工作区最新版本不同。**如果启用了Staging Area，把应提交的Unstaged最新变化也Stage进去**；不要只提交现在Staged中的旧版本。如果使用默认Changelist提交模式，选中需要保存的全部最新变化，检查文件预览。Character旧路径删除与新Public / Private路径新增应一起包含。
4. 同时包含第6节那一条个人配置的删除。建议本次Commit标题：`chore: checkpoint EmberTrial through M1-05C1 and refine gitignore`。本次保存当前完整开发检查点，后续再按每批功能建立小提交。CLion的Commit / Staging操作见[官方说明](https://www.jetbrains.com/help/clion/commit-and-push-changes.html)。
5. 本机Git LFS已安装，当前.git/config与pre-push hook已有LFS配置，.gitattributes对.uasset / .umap已核对为lfs。提交时保留.gitattributes；不用重新添加同样的track规则。首次提交完成后，可在Terminal运行`git lfs fsck`确认本地LFS对象，再Push；若报错发完整文本，先处理对象错误。
6. 当前远端列表为空。由你在GitHub创建目标仓库后，在CLion的Git远端管理中添加`origin`及该仓库URL，再使用Push上传main。Commit保存本地历史，Push才上传；正常LFS pre-push hook会同时传输所需资产对象。创建用于接收现有历史的空仓库时，不额外初始化README / License / .gitignore，避免产生第二套初始历史；如果目标仓库已经有内容，先告诉我具体状态再合并，不强制覆盖。
7. Push完成后，在GitHub检查Source、Config、Content、Docs、Tools与.gitattributes均存在；本机缓存、.run、Saved、Artifacts和Raw不会显示是预期结果。LFS的配置与提交规则见[GitHub官方说明](https://docs.github.com/en/repositories/working-with-files/managing-large-files/configuring-git-large-file-storage)。

本次规则修改与只读核对不包含提交、上传、历史重写或新功能开发。运行构建与内容保持M1-05C1当前基线；等你完成Git整理后再决定何时进入C2。

## 8. 本次规则核对结果（2026-10-09）

- 用Git实际忽略规则核对335个保留路径，包含全部51份Source、259个Content资产，以及共享Config / .vsconfig / 工具 / 文档和未来手写CMake、源美术、环境示例文件；没有误忽略。
- 25个本地文件样例全部匹配忽略规则，覆盖UE产物、CLion / VS状态、本机两份偏好、包 / Trace / Raw、Python缓存与Blender备份。
- 全部259个.uasset / .umap的filter属性为lfs；本次只核对属性和已有hook，不将它当作全部LFS对象完整性或GitHub上传成功证明。首次Commit后的fsck / Push由用户执行。
- 新规则下Untracked从95减为93：排除本机.run启动XML与DefaultEditorSettings.ini；其余大量新文件是实际开发成果，继续显示是正确结果。
- 已跟踪且匹配忽略规则的路径恰好一份：DefaultEditorPerProjectUserSettings.ini，等待用户按第6节取消跟踪。Source与C1清单40资产哈希均保持一致，.gitattributes内容未改。
- 本次两份文本文档的Git空白检查通过，无需重新编译或重做Gameplay验收。
