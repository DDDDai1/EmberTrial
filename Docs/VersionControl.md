# 本项目的 Git / LFS 基线

Git 仓库根目录：`E:\UE_Project\Demo`。UE 工程位于其中的 `EmberTrial` 子目录。

**2026-10-29最新现状：**本地4ed704b已含03A，起点工作区干净；两Success / 双Build和短查已验收，77来源SHA一致。本轮只更新13文档，03B七新 / 四旧Source、两新WBP / 原Pause布局与来源范围见[03B末尾](M3_03B_RelicPausePanel.md#11-本批-commit--push固定放在最后)。远端未核验，Stage / Commit / Push由用户操作。

**Commit双语约定（10/23用户新增）：**后续每份操作文档同时给英文建议标题与专业中文译文。Break Pulse使用“破势”、cooldown使用“技能冷却”、lifecycle使用“生命周期”，不机械逐词翻译。英文可作标题，中文可作正文或理解参考，不需要拼成一条重复标题；不改写已存在的历史提交。

## 1. 已配置内容

- `.gitignore` 排除编译产物、Intermediate、Saved、缓存和 IDE 临时文件；这些文件可重新生成。
- `.gitattributes` 将 `.uasset` / `.umap` 交给 Git LFS，并为后续 Blender / FBX 等源资产预留规则。
- 源码、共享Config、`.uproject`、`.vsconfig`、文档和完整Content进入版本管理；本机IDE选择与Content Browser选中路径两份配置留在本地，详见第5节。
- World Partition 的 `__ExternalActors__` / `__ExternalObjects__` 也是地图内容的一部分，已保留。
- origin沿用GitHub的DDDDai1/EmberTrial配置；本地当前HEAD见页首，不据缓存引用声称联网确认服务器或LFS状态。首次Push故障记录保留在第9节，上传完成以用户实际Push结果为准。

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

规则整理时查到**一份已经被Git跟踪、应改为本地保留的文件**。当前用户已执行取消跟踪，Git显示暂存删除，尚待Commit；**不需要重复执行下面命令**。以下保留当时在CLion底部Terminal使用的一次性步骤：

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
3. 整理前部分暂存文件出现AM / RM：表示暂存版本与工作区最新版本不同。**如果启用了Staging Area，把应提交的Unstaged最新变化也Stage进去**；不要只提交Staged中的旧版本。如果使用默认Changelist提交模式，选中需要保存的全部最新变化，检查文件预览。Character旧路径删除与新Public / Private路径新增应一起包含；目前这些开发改动已进入e916593。
4. 同时包含第6节那一条个人配置的删除。建议本次Commit标题：`chore: checkpoint EmberTrial through M1-05C1 and refine gitignore`。本次保存当前完整开发检查点，后续再按每批功能建立小提交。CLion的Commit / Staging操作见[官方说明](https://www.jetbrains.com/help/clion/commit-and-push-changes.html)。
5. 本机Git LFS已安装，当前.git/config与pre-push hook已有LFS配置，.gitattributes对.uasset / .umap已核对为lfs。提交时保留.gitattributes；不用重新添加同样的track规则。首次提交完成后，可在Terminal运行`git lfs fsck`确认本地LFS对象，再Push；若报错发完整文本，先处理对象错误。
6. 初次建立远端时，在CLion的Git远端管理中添加origin与GitHub仓库URL，再使用Push上传main；**本次origin已经创建，直接按第9节重试Push即可**。Commit保存本地历史，Push才上传；正常LFS pre-push hook会同时传输所需资产对象。以后创建用于接收现有历史的空仓库时，不额外初始化README / License / .gitignore，避免产生第二套初始历史；如果目标仓库已有内容，先核对再合并。
7. Push完成后，在GitHub检查Source、Config、Content、Docs、Tools与.gitattributes均存在；本机缓存、.run、Saved、Artifacts和Raw不会显示是预期结果。LFS的配置与提交规则见[GitHub官方说明](https://docs.github.com/en/repositories/working-with-files/managing-large-files/configuring-git-large-file-storage)。

本次规则修改与只读核对不包含提交、上传、历史重写或新功能开发。运行构建与内容保持M1-05C1当前基线；等你完成Git整理后再决定何时进入C2。

## 8. 本次规则核对结果（2026-10-09）

- 用Git实际忽略规则核对335个保留路径，包含全部51份Source、259个Content资产，以及共享Config / .vsconfig / 工具 / 文档和未来手写CMake、源美术、环境示例文件；没有误忽略。
- 25个本地文件样例全部匹配忽略规则，覆盖UE产物、CLion / VS状态、本机两份偏好、包 / Trace / Raw、Python缓存与Blender备份。
- 全部259个.uasset / .umap的filter属性为lfs；本次只核对属性和已有hook，不将它当作全部LFS对象完整性或GitHub上传成功证明。首次Commit后的fsck / Push由用户执行。
- 新规则下Untracked从95减为93：排除本机.run启动XML与DefaultEditorSettings.ini；其余大量新文件是实际开发成果，继续显示是正确结果。
- 规则修改时，已跟踪且匹配忽略规则的路径恰好一份：DefaultEditorPerProjectUserSettings.ini；随后用户已取消跟踪、删除记录待Commit。Source与C1清单40资产哈希均保持一致，.gitattributes内容未改。
- 本次两份文本文档的Git空白检查通过，无需重新编译或重做Gameplay验收。

## 9. 首次Push的curl35 / schannel TLS断连（2026-10-09）

截图提示GitHub仓库创建成功，但发送Git数据时SSL/TLS连接失败。这是传输阶段的错误；目前没有发现GitHub拒收文件大小、LFS对象或非快进分支的证据。不能仅凭错误就认定.gitignore或项目源码有问题。

### 9.1 已完成的只读检查

- origin：`https://github.com/DDDDai1/EmberTrial.git`；本地main最新e916593，工作区只有个人配置的暂存删除。
- Git 2.49.0.windows.1；全局http.proxy指向127.0.0.1:7897，监听进程为verge-mihomo。
- 在沙箱外测试代理和直连访问github.com，均HTTP200、TLS证书验证结果0；默认Git读取远端main成功 / Exit0，无该分支。
- `HTTP/1.1 + OpenSSL`通过现有代理读取全部远端refs成功 / Exit0；直连Git + HTTP/1.1读取同样成功。两次全部refs输出为空，当前远端没有已发布分支 / 标签。
- 这些测试使用临时-c参数，没有修改用户Git配置、执行Commit / Push或改变代理程序。沙箱内首次测试连接失败，不拿它作为用户真实网络故障的结论。

目前只能确定首次Push的TLS连接中断，而此刻查询可以成功；**“代理故障”或“schannel必然有缺陷”都未被证明**。下面使用已验证能连接的替代配置重试实际上传，结果仍需用户Push确认。

### 9.2 在CLion中执行的推荐处理

1. 打开CLion Terminal，执行：

```powershell
Set-Location 'E:\UE_Project\Demo'
git config --local http.version HTTP/1.1
git config --local http.sslBackend openssl
```

这两项配置只作用于当前仓库，选择HTTP/1.1和OpenSSL连接实现；保留正常证书验证。它们的含义见[Git官方配置说明](https://git-scm.com/docs/git-config)。助手已用同样参数完成只读远端查询，但没有代替用户写入.git/config。

2. 在CLion Commit窗口提交剩余的个人配置删除记录；本文排障更新也可一起提交。推荐标题：`chore: finish local editor preference cleanup and document GitHub push recovery`。删除记录保留本机文件，作用是让新版本不再跟踪它。
3. 使用CLion的**Git → Push**，确认本地main推往origin/main；直接重试现有仓库，不再执行Share Project on GitHub创建第二个仓库。首次Push设置上游即可；也可以在同一Terminal明确执行：

```powershell
git push -u origin main
```

4. 完成应看到main已推送 / 已设置跟踪。可用以下只读命令确认远端main：

```powershell
git rev-parse HEAD
git ls-remote origin refs/heads/main
```

两个提交哈希一致，再到GitHub查看Source / Content / Docs；LFS传输成功随Push结果确认。当前诊断未完成实际上传，也未宣称全部LFS对象已远端备份。

### 9.3 若仍出现同样的TLS断连

已验证本机Git直连读取远端也可用。先用**单次绕过Git代理**的方式重试，不改全局代理、不要求切换代理软件：

```powershell
git -c http.proxy= push -u origin main
```

该-c参数仅在此次命令有效，使用前一节已设置的HTTP/1.1 / OpenSSL。若仍失败，把CLion Git控制台或Terminal中的完整错误文本发给我，保留LFS上传结果和最后的RPC / TLS错误；按实际失败阶段继续判断。GitHub的网络问题排查说明见[官方文档](https://docs.github.com/en/get-started/using-github/troubleshooting-connectivity-problems)。

这里不设置sslVerify=false，也不盲目调大postBuffer：截图指向TLS连接失败；Git官方说明postBuffer增加主要针对不支持chunked传输的特定HTTP端点，不是大部分Push错误的通用修复。恢复本仓库原有协议默认值时（这两项在本次处理前未配置）可执行：

```powershell
git config --local --unset http.version
git config --local --unset http.sslBackend
```

## 10. 每章完成后的Commit / Push约定（2026-10-09用户新增）

用户按每章完成进度上传；**从M1-05C2开始，开发指南最后必须列出本章需要Push的内容**：必需源码、必需可编辑UE资产、指南与验收 / 清单文档，实际发生改动才纳入的配置 / 地图，以及留本地的产物。后续补修 / 实施收尾插在清单之前，不把清单挤到文档中间。当前完整示例见[C2最后一节](M1_05C2_BehaviorTreeMeleeAttack.md#11-本章需要commit--push的内容固定放在文档最后)。

执行顺序：用户手动实现 / Save All → 正常构建与相关检查 → 发完成反馈 → 助手读取代码、资产与日志并更新验收摘要 / AssetManifest / Progress → 用户在CLion检查最新差异、Commit → Push到origin/main → GitHub确认最新提交。无需GitHub插件、重新Share Project或改为另一个仓库。

- Stage / Changelist只纳入本章实际交付与已有明确待提交项；出现AM / RM时，将应提交的最新Unstaged变化也纳入，避免提交旧暂存版本。原生新文件与Content资产同样需要加入。
- 资产按现有LFS规则提交；临时GE None、Debug True、测试默认血量等先恢复再Commit。源码、Config共享规则、手写工具、Markdown摘要不能为减少文件数而被忽略。
- Binaries / Intermediate / Saved / DerivedDataCache、IDE本机状态 / .run、生成CMake、Artifacts、Raw / Trace / Dump按.gitignore留本地。不用清缓存或强制添加被忽略文件。
- 当前个人偏好的暂存删除记录随下一次Commit包含；本机文件保留。若已提交便不重复处理。本次未改.gitignore / .gitattributes、未执行Stage / Commit / Push。
- C1 / C2主体历史继续保留；主体已进入9cdbc75。本轮整章核查通过，待提交C2收尾与D复盘可合并为`docs: close M1-05 enemy combat milestone`，按D最后清单选择实际文档变化；不重复提交没有差异的源码 / 资产。

如果上传失败，保留完整Git / LFS输出，按实际失败阶段排查；不Force Push、不重写历史。当前源资产的授权登记仍需在对外展示时持续核对，GitHub上传结果本身不替代资产来源记录。

2026-10-10追加：M1-05文档收尾已进入8843ffa。当前新增M1-06路线 / A指南和状态更新只代表下发，不代表六新类 / 菜单资产已实现。A按最后清单将实际新增Source / BP / Widget / Map、DefaultEngine / DefaultGame共享地图配置与验收摘要提交；Raw / 生成索引 / 包仍忽略，个人偏好暂存删除随用户下次提交。

2026-10-13 M1-06D / 全章收尾：实际两新 / 六旧Source与四旧资产已验收，69 / 34 / 清单46；D报告 / 总报告 / 来源与状态更新加入实际未提交差异。本地HEAD / 缓存origin.main仍3351b53，本轮未联网核验远端，D由用户Stage / Commit / Push。M1-07A新增手写Tools/Run-StageBuild.ps1与两份指南纳入下一批；Run日志 / JSON / Trace和完整Windows包放Artifacts，保持忽略。Stage后又改过的文本 / Source / Content要把最新Unstaged变化再Stage，不能只提交旧版本；此流程不要求GitHub插件。

2026-10-13 M1-07A完成：本地HEAD193eb4e包含D实现；本轮未联网核验远端，未操作Stage / Commit / Push。Source / Content / AssetManifest不变，A验收与状态摘要提交实际文档差异；DefaultGame.ini已为Always，已进入提交的配置不重复处理。B新指南与Tools/Run-StageBuild.ps1的Profile / B目录 / Region + Log通道为新增实际变化，按B末尾清单由用户提交。原包、三Runtime、UAT副本与后续Trace / CSV / 截图保持忽略；工具WhatIf没有启动游戏或生成Trace。

2026-10-13 B分析：本地HEAD193eb4e，69Source / 46清单资产 / 两Config / 七包指纹不变；提交新增Docs/Reports/M1_07B_Verification.md、B指南8.1与状态摘要实际变化，报告必须保留Combat02待补。A报告 / B工具若尚未提交，沿用B最后清单；不人为Stage无差异Source / Content，不上传Raw / CSV / utrace / 运行期Saved。用户补采后助手更新报告，再重新Stage最新文本、Commit / Push；助手未操作Git索引或远端。

2026-10-13 Combat02收尾：真实AI补采 / 性能分析完成，Source / Content / Config与原基线不变；当前B报告保留MoveRadius待补。新增 / 修改B报告、B指南8.2、M1-06A指南7.1 / 验收补注与状态摘要可提交；用户实施后再Stage实际Task.cpp和BT资产差异、最新报告。修复后的Editor / Game源码不自动修改旧Attempt01；C收口统一Attempt02，包 / Raw / Trace不上传。未代Stage / Commit / Push。

23:50后续：Task.cpp两行换序已有真实差异；BT核对 / 保存但哈希没变，不人为Stage无差异资产。新8.3诊断尚未实施，实施后将当前完整Task.cpp与报告重新Stage。旧包 / Raw仍本地，B主动攻击衔接尚未通过，不写全项通过。

2026-10-14函数收尾：00:26自主接近 / 两次真实伤害样本通过；最终短函数已直接覆盖B指南，尚未写入Source。用户落地后将最新Task.cpp（正确绑定顺序 / Pawn.h / 精简日志）重新Stage，B报告与状态一起提交；BT无差异不重复加入，Raw / 包保持忽略。Game receipt仍旧版，最终函数正常双Build后再记实施完成；未代操作Git。

2026-10-14本轮交接：B最终Task.cpp已落地，最新报告记录Editor成功与用户功能反馈，Game转C1；旧长诊断暂存版本需要重新Stage。C1指南及路线 / 状态实际变化纳入本次文档提交；实施后的四新文件 / 三修改文件 / 合法Probe再按[C1最后清单](M1_07C1_EditorValidation.md#10-本批需要-commit--push固定放在文档最后)提交。未运行Git索引或远端操作，Source新模块仍未生成，不提前列为实际新增文件。

2026-10-14 C1收尾：e3ee554只说明本地已提交主体，不等于本轮联网核验远端。新增C1报告 / 清单与C2下发、工具实际更新可提交；Code尚未代写，C2配置未改，不提前Stage不存在的新Source / Probe。C1 HealthHUD正常保存后哈希已登记，BT无差异不重复Stage。

C2主体尚未在本地新Commit中观察到；只读HEAD保持e3ee554，不替用户Push或宣称GitHub已同步。C3只新增手写工具 /文档，不新增C++ /资产；包完成后提交实际报告。已恢复合法的GA Probe与最终Source需要重新Stage，避免上传None负例暂存版本。

2026-10-14 C3收尾：本地HEAD与缓存origin/main为e3ee554，尚未联网确认远端。Attempt02与两Runtime验收通过；本轮生成C3 / M1-07 / M1报告、M1复盘、M2路线并同步状态和排期。按C3最后清单与C2未提交实现选择真实差异，重新Stage最新文本 / 合法Probe，包与Raw留本地。M2尚无源码或资产可提交。

2026-10-14 M2-01A下发：当前新增体力指南与路线 / 状态更新可作文档提交；Source未代写，不能Stage尚不存在的三新文件。用户实施并检查通过后，按[A末尾清单](M2_01A_StaminaFoundation.md#10-本批需要-commit--push固定放在最后)重新Stage两属性文件 / 一测试、Character两文件 / Validator与三正式身体的真实资产差异。Build.cs / Target / uproject不需要计划改动；旧Attempt02保留M1基线，Raw / 生成文件本地。HEAD仍e3ee554，本轮没有索引或远端操作。

2026-10-14 M2-01A验收：实现主体已本地4fedf14，最新补充[A报告](Reports/M2_01A_Verification.md)与指南 / 当前状态 / 工时文字可提交。三正式身体与全部48清单资产哈希保持，没有新增体力UE资产，不为验收记录重复Stage无差异Content。当前读取缓存origin/main仍e3ee554，实际上传以用户Push结果确认；本轮无Git索引 / 网络操作。

2026-10-14 M2-01B1下发：新增B1指南与路线 / 状态文字可提交；五新Source / 七旧Source修改仅在指南，尚无实际实现可Stage。用户完成后按[B1最后清单](M2_01B1_StaminaRecovery.md#10-本批-commit--push固定放在最后)纳入完整实现、三身体默认组件的真实资产差异与报告。本批不新增GE蓝图 / UI资产 / Build依赖，Raw / 生成文件与旧包本地；B2视图待后续下发，Git仍由用户操作。

2026-10-15 B1验收通过，Source83 / 42cpp，五新 / 七旧实现实际存在，48资产 / 共享Config与uproject哈希保持A；按[B1报告末尾](Reports/M2_01B1_Verification.md#5-本次-commit--push固定放在最后)纳入正确目录实现与文档。旧Public / Private Effects / Resource文件仍有先前暂存记录（工作树AD），新Abilities/Effects与Resources文件未跟踪；重新Stage旧路径删除 / 新路径完整文件与测试最新内容后查看Staged Diff，避免上传旧结构。本轮HEAD4fedf14，未联网核验远端，助手不操作Git索引。

[B2指南](M2_01B2_StaminaHUD.md)目前仅文档与状态更新可提交；不能提交尚不存在的四个UI新文件或Widget。用户实施后再按该页最后清单纳入Bootstrap修改、新体力Widget与Controller Class真实资产差异，实物验收时补来源清单。Raw / 生成文件 / 日志与旧包继续本地。

2026-10-15 B2验收：HEAD与缓存origin/main已是8b725e5，B1 / B2实现和正确文件位置均纳入；当前索引无旧Effects / Resource路径，历史AD问题已解决。本轮未联网核验远端或LFS，不替用户Push。当前新增[B2报告](Reports/M2_01B2_Verification.md)、[01总验收](Reports/M2_01_Verification.md)、AssetManifest一行新增 / 一行Controller更新，以及状态 / 路线真实收尾，用户重新Stage这些文档即可；没有新差异的Source / uasset不重复处理。

[M2-02路线](M2_02_DodgeRoadmap.md)是下一章设计，无操作代码或新资产，不标作已实现。Raw / 日志 / receipt / CodeModel与历史包本地；用户检查Staged Diff后Commit / Push。

2026-10-15 M2-02A下发：当前新增[A指南](M2_02A_DodgeAction.md)与路线 / 状态文字可提交，无新Source或资产。实现完成后按A最后清单重新Stage三新 / 十旧Source、两GA / GE及玩家Class / 既有GA实际资产差异；不先Stage尚不存在的文件，不遗漏共享Practice / 两处恢复数组 / 测试 / Editor规则。CostNone / MontageNone临时配置必须恢复，不改UI / IMC / Build依赖；来源清单在实物验收更新，当前49项。Raw / 生成文件与旧包留本地，用户Commit / Push。

2026-10-15 A验收收尾：四项Success、六Valid、双构建 / 三索引与真实扣25 / 取消 / Stop清理已核对；预期None Montage Error在恢复Valid后关闭。来源清单新增GA_ET_Dodge / GE_ET_DodgeCost、更新玩家Dodge Class，总51项；三旧GA与其他48旧资产哈希保持。当前新增[A报告](Reports/M2_02A_Verification.md)和状态更新，末尾清单以报告为准；B / C尚无实现，本轮无Git索引或网络操作。

2026-10-15 B指南下发：当前新增[M2-02B完整文档](M2_02B_DirectionalDodge.md)与状态 / 路线，Source / 资产保持A；A验收 / 清单如尚未提交可一并Stage。B实物完成后按文档最后三新 / 五旧与GA / 玩家输入实际差异清单重新Stage，旧Action名字暂保留不重命名。未修改Practice / 资源 / UI / Build依赖或Git索引，Raw / 生成文件本地；不把文档候选当已编译代码。

2026-10-15 B验收收尾：核对期间用户完成c010520提交，最终本地与缓存origin同哈希；源码 / 资产索引操作由用户完成，助手没有Stage / Commit / Push。新增[B报告](Reports/M2_02B_Verification.md)、清单GA / 玩家两行以及进度 / B指南已验收状态需重新Stage最新文档；C未实施，无新源文件或Tag可提交。

2026-10-15 C指南下发：三新 / 八旧Source预计96 / 50，新增窗口GE为原生类，不创建GE蓝图或新增资产清单项；既有GA_ET_Dodge计划增加.15配置，其他资产只提交实际合法差异。C实物完成后按[最后清单](M2_02C_Invulnerability.md#11-本批-commit--push固定放在最后)重新Stage完整Source / GA配置 / 文档，来源哈希待验收更新。当前不将静态候选写成已编译或已提交实现；助手不操作Git索引 / Commit / Push。

2026-10-15 C收口：96 / 50 / 51，三项Dodge Success分布在初轮与时点补修后的单项复测日志；不为凑同一日志重跑旧项。来源清单GA说明更新但SHA保持；无Content差异不需要Force Add或人为重存资产。所有11份Source保存后重新Stage，检查Staged Diff包含.12 / .04修正，再由用户Commit / Push；完整路径清单见[C报告](Reports/M2_02C_Verification.md)。

2026-10-15 M2-03A下发：新增指南 / 路线 / Docs/Data/M2_03A_AttackPoseValues.json可提交，当前工程Source / 资产 / Config保持C验收。制作完成后按A最后清单提交新Combo目录内两LS / 两AS / 两AM及实际合法制作地图差异；来源清单待实物核对，当前51。旧M0 / 玩家GA / 输入 / AnimBP无计划修改，C已有AM状态不当作A新增Source。仍由用户Stage / Commit / Push。

2026-10-15 A验收收尾：六资产最终仅在Combo/Sequence、Clips、Montages各两份，未覆盖旧资源；AssetManifest新增六行真实SHA至57。按A报告Stage实物与文档，C已有AM源码需纳入工作区最新内容，不把A素材当作新代码。用户自行Commit / Push，助手未改暂存区或连接GitHub。

2026-10-16 M2-03B准备：当前可提交新指南与Progress / Backlog / M2路线 / 连段路线 / Architecture / 验收流程 / Introduction及A交接更新。完成手动实现后重新Stage三新 / 六旧完整源码、独立AM_ET_Light_01、玩家GA / AM02真实配置差异与核实后的来源记录，逐项见[B末尾](M2_03B_LightCombo.md#12-本批-commit--push固定放在最后)。没有差异的敌人GA / 角色 / 地图不人为重存；Raw / 候选脚本 / 生成文件 / 旧包不Force Add。本轮仅静态检查，未运行B构建或测试。

2026-10-16 B已验收：用户已提交一新AM01、四旧资产真实差异与三新 / 六旧源码；旧共用AM重存导出数据相同，清单58项如实更新新SHA。当前提交B报告 / 状态及C指南；C实施后按[最后清单](M2_03C_RecoveryDodgeCancel.md#11-本批-commit--push固定放在最后)重新Stage一新测试 / 七旧Source、Dodge标签与真实资产差异、更新清单 / 文档，未改文件不制造差异。无助手Git索引 / 网络或UE构建操作。

2026-10-16 C / 整章验收通过：双构建、三Success、100 / 53 / 58与实际清理已核对。初查新测试仅暂存创建骨架，用户随后efaae3e已纳入完整测试（含double时间与暂停推进），最终源码无差异、缓存origin同哈希；记录状态随用户新提交更新，不再要求重复Stage源码。当前仅按[C报告末尾](Reports/M2_03C_Verification.md#5-本次-commit--push固定放在最后)提交本轮报告 / 清单 / 状态收尾，无Content变化不人为重存。

2026-10-17 M2-04路线 / A指南下发：A身体离散3/3、固定原生Health GE成功扣数 / 重入保护、事件快照 / 失活保留与EndPlay解绑、仅PIE ET_Healing复用入口、玩家ET-HEAL-001与一项Healing.Resource长期自动化。五新 / 五旧候选与十个精确替换锚点、本机5.5.4 Instant成功句柄 / 应用查询 / Pauser / 默认子对象与既有Ready语义静态核对；A指南含完整代码、三身体逐项配置、5–10分钟短查与末尾Push清单，不建临时Controller验收图、不重跑未变Combo / Dodge矩阵。预计A105 / 56 / 58；当前工程仍100 / 53 / 58，未写Source / UE资产、执行Build / 新测试 / Cook或计预计工时。累计2391 / 明确235，M2为380 / 50保持；B1动画 / B2正式能力与Q / C次数HUD待后续下发。

本轮Push边界：先提交路线 / A指南与状态 / 架构 / 验收 / Introduction / 上章收尾文档；A实物完成后按[A末尾](M2_04A_HealingResource.md#13-本批-commit--push固定放在最后)重新Stage完整Source与实际资产差异。仍由用户检查Staged Diff、Commit、Push，未变资产不人为重存，Raw不Force Add。

2026-10-17 M2-04A验收：用户19:17全项通过，手动60 / 等待5。十份实现与候选C++ Token一致（九份仅排版 / 末尾换行），其余95旧Source保持C；实际105 / 56 / 来源58。Editor18:19:24 / Game18:20:47覆盖最终Source，CodeModel40+13+3；19:13三正式身体项目Valid / Issues0，19:13:57.118 Healing.Resource实际Success。正式75→100 / 2次、满血拒绝、P内75与2次保持 / Applied0、耗尽0拒绝、19:16:29 Retry新100 / 3次与原HUD / Stop清理有日志；具体输入画面 / Stop再开按用户反馈，不补造第二轮PIE。58资产 / Config / uproject / 冻结B1保持，清单只更新三个身体的来源说明 / 验收阶段。累计2451 / 明确240、M2为440 / 55，04当前60 / 5，独立检查未拆；未重跑未变旧矩阵或新Cook。[A报告](Reports/M2_04A_Verification.md)无待补，B1动画素材待下发，正式Q / GA与次数HUD仍未完成。初查十Source已Stage完整内容，核对期间用户完成685a0f3，最终HEAD / 缓存origin一致、Source无差异；当前只提交最新文档收尾，助手未代改工程 / Git索引或执行UE测试。

核对期间用户已将A完整十份Source与原指南纳入`685a0f3`，最终HEAD / 缓存origin/main一致，Source工作区无差异，不需要重复Stage源码。当前新增[A报告](Reports/M2_04A_Verification.md)、来源三个正式身体说明及状态 / 架构 / 验收 / 路线 / Introduction收尾需要Stage最新文档差异。完整提交范围见[A报告末尾](Reports/M2_04A_Verification.md#5-本次-commit--push固定放在最后)。58资产均无差异，不强制重存Content；Raw / 生成文件 / 日志 / 历史包本地，Stage / Commit / Push由用户操作，助手未联网核验远端。

2026-10-17 B1指南下发：[治疗动画完整步骤](M2_04B1_HealingAnimation.md)与[数值JSON](Data/M2_04B1_HealPoseValues.json)已保存。空白LS_ET_Heal_01 → AS_ET_Heal_01 → AM_ET_Heal_01；五帧0 / 8 / 18 / 22 / 30、七控制器完整L/R/S，8–18相同停留，B2生效预留15帧 / .5秒。旋转向量来自旧M0 / Combo，新的组合 / 时序尚未在UE求值 / 蒙皮预览，用户先查0 / 8再补余帧；未伪造视觉通过。30fps / 排他End31、DefaultSlot / Loop1 / Blend .10与.10 / AutoBlendOut，Notifies保持空，不改Skeleton；B2再添加原生生效Notify。预算45–75手动 / 检查5–10未计实耗，无新C++ / Build / 旧测试 / Cook或Controller测试图。工程仍105 / 56 / 清单58，与A验收基线保持；预计三新素材后来源61。当前累计2451 / 明确240不变；完成并保存报告且无待补项时，按用户新增约定同轮下发B2。

2026-10-23 C1收尾：本地b3cbc24已包含三份动画与完整B2，f4127ff / 77e556f补交关闭旧空文件问题。工作区起点干净，131Source归一换行后与HEAD一致；三个新LFS OID吻合本机。当前只Stage最新文档收尾，不重复Stage无变化资产。C2-01待用户写入，未来新Source / GA / IA不提前提交；未联网确认远端。

2026-10-23 C2-01验收：本地0cce598已完整提交五新 / 八旧与两新 / 两变UE资产；核对起点工作区 / 暂存区干净。136Source归一换行一致，四个LFS OID与本机SHA吻合；HitReact / Stagger原资产字节无变，继承原生新增策略并有Valid / 自动化证明，不强行重存制造差异。当前只提交报告 / 来源70项 / 状态和C2-02指南；下一批新Source / 通知未落地，不提前提交空文件，未联网核验远端。

2026-10-23 C2-02收尾：八份Source与AM已验收；初查时三新文件暂存为空，核对期间用户完成36bb869，139Source与HEAD归一换行一致、AM LFS匹配，问题关闭。当前只提交报告 / 来源 / 状态文档，不重复Stage无变化Source / 资产；测试保留，助手未改索引或联网核验远端。

2026-10-23 D指南下发：五新 / 五旧Source、一新WBP与原Controller配置均待用户实施。当前源码 / 资产保持C2-02，无新Build / 测试 / Cook证据；只提交指南与状态文档。新WBP实物完成后才登记来源71，不能把静态候选或预计19项写成已运行Success。完整实施清单见[D末尾](M2_05D_BreakPulseHUD.md#12-本批-commit--push固定放在最后)，仍由用户Stage最新完整内容 / Commit / Push。

2026-10-24 D验收收尾：实际144 / 78、来源71，三指定专项当次Success，真实敌人F归零 / 恢复后继续近战。完整实现已在用户提交c59291d，旧Core误放测试已移至UI，Widget实现文件实际名ETBreakPulseHUDWidgets.cpp保留，无需为复数命名制造源码差异。当前只提交D / 模块报告、清单 / 状态与M2-06路线 / A指南；A实物尚未创建，不预先提交空文件。自本轮起每份操作文档给出术语准确的中英Commit建议。

2026-10-24 A验收：六Source与候选一致，来源实际新增IA_ET_LockTarget至72；两个旧资产IMC / Controller的本地SHA与HEAD LFS匹配。用户5bd5354已包含完整Source / 资产，无需重复Stage。当前A报告 / 清单 / 状态与B1指南待提交，未来B1六新UI文件 / WBP和Controller按[B1末尾](M2_06B1_TargetMarker.md#12-本章-commit--push固定放在最后)处理，未实施前不创建空文件。英文建议与专业中文译文同步，助手未操作Git索引或网络。

2026-10-24 B1收尾：六新UI / 零旧Source、双Build / MarkerRules Success / 两资产保存已核对；本轮文档整理期间用户提交c902dcc，源码 / 两资产与HEAD一致，误放路径已关闭，不重复提示取消旧暂存。当前只提交B1报告 / 清单 / 状态与B2指南修订；B2代码仍仅在忽略Raw候选与MD中，不提前Stage实际Source。

2026-10-24 C指南下发：普通实现由用户手动，用户已写入C五旧Source / 两IA；本次“直接解决”仅授权助手修ETTargetLockTests.cpp，玩法 / 资产未再改。现有B2右肩未提交差异保留。C实施后五旧Core / 两新IA / 原IMC与Controller及实际文档清单见[C末尾](M2_06C_TargetSwitching.md#11-本章-commit--push固定放在最后)；当前不提前Stage未来文件 / 空报告，不改变Git索引。主张先提交B2，再做C；如合并，指南提供完整合并范围及中英文建议。

## 11. 当前批次 Commit / Push（固定放在最后）
[03B验收检查报告](Reports/M3_03B_Verification.md)已保存：主体实现和三份资产已在本地897b9fa，当前待列表Never Tick Ensure窄补修，不进入下一章。当前Stage报告、操作指南、AssetManifest与状态文档；补修后再加入有变化的ListEntry资产及新SHA。无需重复Stage无变化Source，未联网核验远端。

当前记录建议英文`docs: record relic panel verification and pending tick fix`，中文`docs: 记录遗物面板验收结果与待处理的 Tick 补修`。实际修复后英文`fix: align relic list entry tick settings with Slate lifecycle`，中文`fix: 调整遗物列表条目的 Tick 配置以兼容 Slate 生命周期`。Stage / Commit / Push由用户操作，Raw / 日志 / 缓存 / 包留本地。
