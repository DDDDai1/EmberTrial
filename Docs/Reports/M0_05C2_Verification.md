# M0-05C2：组件验收、动作生命周期与交接记录

更新：2026-09-28。工程：EmberTrial；引擎：UE 5.5.4。

**C2-01 验收通过；C2-02 C++ 实现、正常 Build 与综合自动测试通过。C2-02 的玩家蓝图配置 / 接线 / PIE 验收尚未完成。C2-03 实际闪避位移未开始。**

## 1. C2-01：用户结果与本地复核

用户报告实际约 **40 分钟**，已保存源码和资产：

- 新类使用向导默认路径，关闭 UE 后 C++ Build 成功。
- BP_ThirdPersonCharacter 仅挂一个 PracticeAction，两个 BlueprintPure 查询节点可用，Compile 通过。
- 每次 Play 一条 Component ready，连续三次通过。
- 原 1/2/3、E/P、移动 / 转向 / 跳跃与 C1 边界回归全部通过。

助手读取用户源文件及当时的 `EmberTrial/Saved/Logs/EmberTrial.log`：

- 三条初始化记录的日志 UTC 时间为 2026-09-27 12:53:21.997、12:55:52.497、12:56:10.723，中间存在旧 C1 播放记录。
- 检查的相关日志未匹配到 Blueprint Runtime Error / Accessed None 等错误；这不代替用户对具体输入和画面的验收。
- 原实现的 CurrentState 放在 public 且未加 Transient，功能装配通过，但封装与运行时字段约定需修正。C2-02 已修成 private + Transient + BlueprintReadOnly，保持枚举名与查询函数接口。

原始基础组件、角色资产及 C1 验收文档已保存为本地检查点 **92a2c63**，标题 `feat: checkpoint M0 action component scaffold`。

## 2. C2-02 实现范围

| 位置 | 已实现的职责 |
|---|---|
| ETPracticeActionComponent.h / .cpp | 三个 Montage 配置、单一动作状态、请求校验、受击打断、Montage 实例回调、统一退出清理 |
| EmberTrialCharacter.h / .cpp | BeginPlay 缓存已有动作组件；Move / Jump Started 读取动作状态，忙碌时拒绝输入；Look 不加动作限制 |
| Tests/ETPracticeActionTests.cpp | 临时世界、实际 Montage 的生命周期和异常路径综合测试 |
| M0_05C2_02_ActionLifecycle.md | 用户资源配置、三条输入替换、E 门控、日志说明、实机回归与反馈模板 |

状态规则：Idle 下攻击要求地面且水平速度 <=3 cm/s；闪避 / 受击允许地面移动时发起。动作开始时清除旧移动输入和速度。攻击、闪避不能互相打断；受击能打断两者，受击自身不能重复重启。空中、暂停或组件未就绪时拒绝，不缓存请求。

结束使用 **Montage 实例的 OnMontageEnded**，包括淡出完成。每次动作带序号，先使旧序号失效再停止旧实例，忽略迟到回调。取消只停止组件保存的实例，不执行全局 Stop All Montages。失败、取消、外部中断、AnimInstance 更换、Deactivate 与 EndPlay 均释放状态。

组件仅在忙碌时启用 Tick，检查 AnimInstance 身份和实例是否仍存在；空闲关闭。角色不叠加计数式 Ignore Move Input 锁，也不使用 Delay 猜测动画结束。E 的动作门控由用户在既有交互函数中接入，组件不依赖 Widget。

本批是 M0 验证组件，未加入 GAS、伤害、无敌帧、输入缓存或实际闪避位移。Root Motion Source / CharacterMovement 位移属于 C2-03。

## 3. 正常 C++ 构建

用户确认已保存并关闭 UE 后执行正常构建。最终成功运行日期为 **2026-09-27**；2026-09-28 继续工作时复核保存的结果。

```powershell
& 'D:\UE5\UE_5.5\Engine\Build\BatchFiles\Build.bat' EmberTrialEditor Win64 Development '-Project=E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' -WaitMutex -NoHotReloadFromIDE
```

- 目标：EmberTrialEditor / Win64 / Development，正常 DLL / LIB 链接完成。
- MSVC 14.38.33145，Windows SDK 10.0.22621.0。
- 最终构建退出码 0，7 个构建步骤，**20.92 秒**。
- 初次真实编译发现测试子目录无法解析平铺模块头文件，已修正 include 为 `../ETPracticeActionComponent.h`；最终构建通过，不把中途失败隐藏为“从未报错”。
- 首次沙箱运行因 UBT 外部缓存权限失败，随后按授权完成构建；未更改全局 UBT 配置。
- 本地原始证据：`Docs/Reports/Raw/M0_05C2_Build.log`，已复制保留。Raw 目录不进入 Git。

## 4. 自动化测试结果与边界

使用 UnrealEditor-Cmd 在 `/Engine/Maps/Entry` 启动测试，无界面、NullRHI、无自动保存。测试不写入用户蓝图或动画资产。

```powershell
& 'D:\UE5\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject' /Engine/Maps/Entry '-ExecCmds=Automation RunTests EmberTrial.M0.PracticeAction' '-TestExit=Automation Test Queue Empty' '-ReportExportPath=E:\UE_Project\Demo\Docs\Reports\Raw\M0_05C2_Automation' -unattended -nullrhi -nosound -NoSplash -NoAutoSave -stdout -FullStdOutLogOutput '-abslog=E:\UE_Project\Demo\Docs\Reports\Raw\M0_05C2_Automation.log'
```

实际报告 UTC 时间：**2026-09-27 13:20:36**，北京时间 21:20:36。

| 字段 | 结果 |
|---|---|
| 测试 | EmberTrial.M0.PracticeAction.Lifecycle |
| 成功 / 失败 / 未运行 | 1 / 0 / 0 |
| 测试内 warnings / errors | 0 / 0 |
| 单项测试耗时 | 约 0.256 秒，不包含引擎启动 |
| 日志末尾 | Test Completed Result={Success}；1 tests performed；RequestExitWithStatus(..., 0, ...) |

这是 **1 项包含多个断言的综合测试**，不把其中每个断言计为独立测试。引擎启动日志仍有可选工具 DLL / 在线服务等警告；“0 警告”仅指该测试报告的统计。

覆盖的场景：

| 场景 | 验证内容 |
|---|---|
| 基础状态和准入 | 初始 Idle / 不 Tick；Idle 非法请求、移动攻击、空中三动作被拒绝 |
| 动作优先级 | 攻击期间拒绝攻击 / 闪避；受击打断攻击和闪避；受击期间拒绝三动作 |
| 迟到事件 | 复制旧 Montage 的真实结束委托，进入受击后执行；新受击状态不被释放 |
| 自然结束 | 推进测试世界，受击自然结束回 Idle，安全 Tick 关闭；攻击恢复后自然结束 |
| 移动与暂停 | 地面移动时可开始闪避且旧速度归零；暂停位置不推进、请求拒绝，恢复后完成 |
| 缺失 / 失败 | 缺失受击资源不取消当前攻击；零长度 Montage 播放失败回 Idle，无残留 Tick |
| 取消 / 所有权 | 重复取消安全；拒绝抢占外部正在播放的 Montage；Idle 取消不停止外部 Montage |
| 生命周期异常 | 外部停止、AnimInstance 替换、Deactivate / Activate、Owner 销毁后状态释放和请求拒绝 |

测试使用 `FTestWorldWrapper` 创建临时 Game World，生成基础 **ACharacter + UAnimInstance**，载入项目 Quinn_Simple 与三段实际 Montage。为隔离地形，关闭测试角色 CharacterMovement 的 Tick 并指定 Walking。

因此以下项目**未由这项自动测试证明**，仍需本批用户 PIE：

- BP_ThirdPersonCharacter 上的三个 Montage 引用、三条新输入与 E 门控是否实际接好。
- 项目 Character 的真实增强输入、Move / Jump 按住或松开、镜头与菜单焦点。
- ABP_Manny / ABP_Quinn 最终输出、动作外观和打断混合。
- 实际地图中的暂停 / A-B 交互 / 三次 PIE 回归。
- C2-03 位移、墙壁、坡面、残留速度；打包和性能基线也未在本批执行。

原始报告：`Docs/Reports/Raw/M0_05C2_Automation/index.json` 与 `Docs/Reports/Raw/M0_05C2_Automation.log`。摘要入库，原始机器日志继续忽略。

## 5. 当前交接与后续复盘

现在按 [C2-02 操作指南](../M0_05C2_02_ActionLifecycle.md) 第 3–6 节操作，第 7 节反馈。C2-01 不重做；不要将历史示例重新覆盖现有 C++。本批用户实机验收通过后再进入 C2-03。

复盘时重点解释：为什么状态只有一个写入入口；为什么结束回调比固定 Delay 可靠；旧回调如何影响新动作、序号如何防护；为什么保留跳跃 Completed；为什么只清理自己持有的实例；为什么自动测试成功仍需要蓝图 / 输入 / 画面验收。能用本项目的真实故障和证据说明，比背诵概念更有价值。
