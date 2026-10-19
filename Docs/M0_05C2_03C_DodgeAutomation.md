# M0-05C2-03C：真实位移的专项自动回归

2026-10-19第19批清理同步：以下配置、接入及验收属于M0历史，当前左键 / 1、2、Q已走正式GAS；Practice只保留兼容及原回归入口。完整代码以Source链接为准，本轮按用户授权由助手直接清理；实际新检查和提交清单见[Cleanup](Cleanup.md)。

> 最终验收（2026-10-01 整理）：用户 9 月 29 日约 **30 分钟**完成，Build、DodgeMovement 两次和 Lifecycle 回归报告 Success；源码与指南一致，本地日志确认一次新测试 Success，三种固定步长均为 150.05 cm / 0.3001 s。C2-03 / C2 已完成，本页不用重做。下一步见 [C3 组合演示](M0_05C3_CombinedDemo.md)。

日期：2026-09-29；UE 5.5.4。预计手动 **30–45 分钟**。C2-03B 已验收：首次实现 50 分钟、补修 30 分钟；无需重做 B。

本页记录当时新增专项测试的过程，现已实现并保留。完整代码只在Source维护，原测试和断言未因本轮清理而减少；实际新构建与测试结果见Cleanup。

## 1. 这项测试解决什么问题

已有 Lifecycle 为了隔离动画和状态，关闭了测试角色的移动 Tick，因此它无法发现“动画正常但实际没移动”“打断后继续滑行”等回归。本批新建独立测试，保留真实 CharacterMovement 更新和胶囊碰撞，在临时世界自动生成一块平地。

| 用例 | 验证内容 |
|---|---|
| 1/30、1/60、1/120 秒三种固定步长 | 150 cm 右侧位移、位移源累计约 0.30 秒、先停位移后解锁、结束后无漂移 |
| 1/60 秒步长下中途受击 | 已经移动但尚未走满；受击后立刻停位移、结束恢复 Idle |
| 1/60 秒步长下暂停 / 恢复 | 暂停时位置、动画进度和位移源时间都冻结，恢复后总距离不增加 |

固定步长是**模拟输入条件**，不代表 GPU 达到 30 / 60 / 120 FPS，也不是性能基准。B 中的墙壁 / 坡面 / 键鼠 / UI 仍以实机结果为据；本批不增加整套重复手测，也不扩展为测试框架工程。

## 2. 在头文件中增加一个测试友元

保存并关闭 UE。打开：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\ETPracticeActionComponent.h
```

在文件末尾已有的 `#if WITH_DEV_AUTOMATION_TESTS` 区域，保留旧行并增加一行，使该区域为：

```cpp
#if WITH_DEV_AUTOMATION_TESTS
    friend class FETPracticeActionLifecycleTest;
    friend class FETPracticeDodgeMovementTest;
#endif
```

这里只让新测试设置临时组件的资源、检查持有的位移源；不把这些字段改成 public，不新增 UFUNCTION / UPROPERTY，不修改玩家蓝图参数。

## 3. 新建一个普通 .cpp 文件

如果 VS 看不到 **Tests**（不是 Texts），先按 [VS 目录查看与同步指南](VS_FileVisibility.md) 切换文件夹视图。目录已经存在，不用清缓存或重复创建。解决方案视图中的逻辑分组不一定与当前磁盘同步。

准确路径：

```text
E:\UE_Project\Demo\EmberTrial\Source\EmberTrial\Tests\ETPracticeDodgeMovementTests.cpp
```

这是普通 C++ 测试文件，**不用 UE 的 Add C++ Class 向导**，不创建 .h，也不需要 generated.h。可以在 VS 对 Tests 文件夹使用“添加 → 新建项 → C++ 文件”，确认实际保存到上面的 Source 目录，而不是 Intermediate。必要时用“文件 → 新建文件”另存到此路径；避免生成 `.cpp.txt`。

下面是**新文件的全部内容**，一次完整写入，不要插进原 Lifecycle 文件中，也不要覆盖它。`State` 别名已放在函数最前面，避免前次的声明顺序问题。

当前源码入口：[ETPracticeDodgeMovementTests.cpp](../EmberTrial/Source/EmberTrial/Tests/ETPracticeDodgeMovementTests.cpp)。保留下面的职责 / 验收说明，不用历史全文覆盖已完成工程。

### 3.1 理解这几个关键点

- 地板是临时 UBoxComponent，顶面 Z=0；角色从 Z=200 自然落地。不能为了通过测试直接一直设 Walking 或关闭移动 Tick，那会掩盖没有地板 / 没有真实移动的问题。
- `bRunPhysicsWithNoController=true` 只属于本测试临时角色，因为它没有玩家 Controller。不要复制到玩家蓝图来“修”其他问题。
- 每种步长都使用新的 FTestWorldWrapper；析构负责销毁自己的世界。测试不保存地图或资产，不修改全局 `t.MaxFPS`、时间膨胀或玩家参数。
- 测试保存 Source 的共享指针只是读取完成后的时间，不推进它、不延长实际移动。移动权仍由 CharacterMovement 持有 / 移除。
- 1 cm / 0.002 s 是本次平地自动回归的容差，不要求浮点值逐位相等。如果超差先定位，不直接放宽阈值。
- AttackMontage 在这项测试中不赋值，因为没有攻击请求；攻击仍由原 Lifecycle 覆盖。

## 4. Build 与运行

1. 保存头文件和新 `.cpp`，确认 UE 已关闭。
2. VS 使用原来的 **Development Editor / Win64** 配置正常 Build。
3. 如果 VS 的项目树没显示新文件，先用文件夹视图或 Ctrl+O 按完整路径确认；若需要同步解决方案树，再按 [目录同步指南](VS_FileVisibility.md) 轻量生成项目文件，无需清缓存。不要往 Intermediate 中补一份，也不要删除 Saved 排障。
4. 成功后打开 UE，**停止 PIE**，在 Output Log 控制台执行：

```text
Automation RunTests EmberTrial.M0.PracticeAction.DodgeMovement
```

预期 `Test Completed. Result={Success}`，路径为 `EmberTrial.M0.PracticeAction.DodgeMovement`，并出现三组 StepRate 摘要。它是一项综合测试，不是三个独立测试。

首次通过后，再执行一次同一命令，确认临时世界销毁和重复运行没有残留。最后回归旧测试一次：

```text
Automation RunTests EmberTrial.M0.PracticeAction.Lifecycle
```

这两项通过后，本批不要求再跑一次完整手动矩阵；后续 C3 做约 30 秒组合演示，随后首次打包。

### 4.1 常见失败入口

| 现象 | 先检查 |
|---|---|
| 私有成员无法访问 | 头文件 friend 名称必须是 `FETPracticeDodgeMovementTest`，与宏第一参数一致 |
| 找不到测试 | 新文件真实路径、Build 是否成功、是否仍打开旧编辑器进程；先确认日志，不反复复制文件 |
| `Character landed on real floor` 失败 | 临时 Box 的注册 / 碰撞、角色出生点、无 Controller 的物理开关 |
| Travel 为 0 或位移超差 | 是否保留移动 Tick、Source 时间、碰撞地板以及移动更新；不要修改断言凑通过 |
| 动画状态不符合预期 | Montage 路径是否使用项目实际的 `_Montage` / `HitResult` 名称 |
| 暂停断言失败 | 保留日志，由我对比本机引擎时序；不把暂停用例直接删除 |

## 5. 最简反馈

不用手工抄 Begin / End 或逐条数日志。我默认读取本地 `EmberTrial/Saved/Logs/EmberTrial.log`；告诉我完成时间，避免后续启动覆盖需要的片段即可。有错误时优先贴第一条 error。

```text
M0-05C2-03C 完成 / 卡在第 __ 节
测试 friend / 新文件路径：已按文档完成
关闭 UE 后 Build：成功 / 第一条错误
DodgeMovement 第一次 / 第二次：Success / 实际结果
Lifecycle 回归：Success / 实际结果
大致完成时间：今天 __:__，日志由你读取
源码已保存：是
实际用时：约 __ 分钟
```

C2-03历史已验收；当前固定步长测试只验证位移正确性，不作真实帧率 / 性能证明。键鼠、墙坡与UI仍由独立实机检查覆盖。

## 本次清理 Commit / Push（固定放在最后）

第19批四份Source与十份Markdown，共14项；当前检查、最新差异和逐文件清单见[Cleanup末尾](Cleanup.md)。历史资产 / 测试未改，不重复制造差异。用户重新Stage后Commit / Push；Raw、报告、索引留本地，助手不操作Git索引。
