# M0-05B：验收后的少量补修

**2026-09-24 补修后更新：本清单六项已解决五项，用户追加约 20 分钟。当前只剩闪避 0 帧左上臂 Z=15→25，按 [C1 指南 §0](M0_05C1_MontagePlayback.md) 收尾后即可继续角色接入。受击真正源资产现已变为 LS_M0_HitResult_01，旧 _02 路径指向它；下面路径说明与六项清单保留作为首次补修历史，不要再次整套执行。**

日期：2026-09-24。引擎：UE 5.5.4。预计由你手动完成 **10–15 分钟**，不需要 C++ Build。

你已完成两段动画制作和独立播放，记录用时为闪避 40 分钟、受击 40 分钟。我读取已保存资产后确认骨架一致、root 与双脚没有位移，但发现两处播放范围和四个关键帧数值需要对齐。本次只修改下表，不重新制作整段动画。

当前状态：**M0-05B 待补修复验；M0-05C 暂未开始。** 完成并反馈后，我会读取最终资产再下发 Montage 接入步骤。

## 1. 先确认打开的是哪个资产

停止 PIE，保存现有改动，打开 `L_M0_AnimationLab`。继续使用已有 `M0_AnimationDummy`。

| 用途 | 这次应打开的实际资产 |
|---|---|
| 闪避制作源 | `/Game/EmberTrial/Animations/Practice/Sequence/LS_M0_Dodge_01` |
| 受击制作源 | `/Game/EmberTrial/Animations/Practice/Sequence/LS_M0_HitResult_02` |
| 闪避烘焙结果 | `/Game/EmberTrial/Animations/Practice/Clips/AS_M0_Dodge_01` |
| 受击烘焙结果 | `/Game/EmberTrial/Animations/Practice/Clips/AS_M0_HitResult_01` |

沿用实际名称 **HitResult**，不用为匹配旧指南里的 HitReact 重命名。`LS_M0_HitResult_01` 的加载结果指向 **LS_M0_Dodge_01**，不要把它当成受击制作源；受击请明确打开 **_02**，并核对 Sequencer 顶部标题。该旧路径本次保留。

每次只打开并编辑一个制作 Sequence。切换前先保存、关闭当前 Sequencer 标签，避免给错误动作打 key。

## 2. 修改闪避：一个姿势数值和播放范围

### 2.1 第 0 帧左上臂

1. 打开 `LS_M0_Dodge_01`，停止播放，关闭 Auto Key。
2. 时间指针精确放到 **第 0 帧**。
3. 在 Anim Outliner 搜索并选中 **`upperarm_l_fk_ctrl`**，注意是左侧 `l`。
4. 在 Anim Details 填入下表最终值，Enter 提交。

| 控制器 / 帧 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---|---|---|---|
| `upperarm_l_fk_ctrl` / 0 | (0,0,0) | **(0,-20,25)** | (1,1,1) |

磁盘上的当前 Rotation Z 是 **15**，这里改为 **25**。右上臂的 Z=15 是正确的，不跟着改。

5. 保持该控制器选中，鼠标焦点放回动画视口，按 **S** 更新第 0 帧 Transform key；也可以用对应通道的 Add Key 按钮。确认更新的是 0 帧原有 key，没有把它拖到别的帧。
6. 切到第 4 帧，再回第 0 帧，核对 Rotation 仍为 `(0,-20,25)`。将本次修改的 key 插值保持 **Linear**。
7. 在 0 和 20 帧来回切换，两端左臂应一致。其他四帧保留已有数值。

### 2.2 播放范围

1. 保持 Display Rate 为 **30 fps**，时间显示单位使用 **Frames**。
2. 打开 Sequencer 上方工具栏的 **Playback Options（播放选项）** 下拉菜单；将鼠标悬停在工具栏按钮上可查看名称。
3. 若 **Playback Range Locked** 勾选，先取消锁定。
4. 设置 **Start=0、End=21**，Enter 提交。重新打开菜单核对保存的数值。
5. 只缩短播放范围，**不要拉伸关键帧、缩放动作时间或删除第 20 帧**。Control Rig section 原来覆盖到 31 帧可以先保留，它仍覆盖新的播放范围。
6. 保存 Sequence。再次打开播放菜单，确认 End=21。

这里的 End 是排他端点，最后一个姿势 key 在 20 帧，所以 End 要填 **21**。最后一个 key 的帧号和动画总帧数是两个概念；之前报告中的“20 帧”不能替代烘焙后的实际时长。

Playback Options 的 Start / End 是序列播放范围；不要只改变底部用于查看时间轴的可视范围。菜单入口与字段可参考 [Epic UE 5.5 Sequencer 工具栏说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/sequencer-cinematic-toolbar-in-unreal-engine?application_version=5.5)。

## 3. 修改受击：三个关键帧数值和播放范围

保存、关闭闪避 Sequence，打开 **`LS_M0_HitResult_02`**。

按 §2.1 的“定位帧 → 选控制器 → 填值 → Enter → 打 key → 切走再切回确认”方法，修改以下三处：

| 帧 | 控制器 | Location (X,Y,Z) | Rotation (X,Y,Z)，° | Scale (X,Y,Z) |
|---:|---|---|---|---|
| 0 | `upperarm_l_fk_ctrl` | (0,0,0) | **(0,-20,25)** | (1,1,1) |
| 7 | `spine_01_ctrl` | (0,0,0) | **(-7,0,0)** | (1,1,1) |
| 12 | `spine_01_ctrl` | (0,0,0) | **(3,0,0)** | (1,1,1) |

目前磁盘上的第 7、12 帧 spine_01_ctrl Rotation X 都是 **-12**。修改后，脊柱 X 的五帧顺序应为：

```text
帧：        0      3       7      12     18
Rotation X：0     -12     -7      +3      0
表现：      准备 → 后仰 → 衰减 → 小幅前倾 → 准备
```

保持 Linear。第 3 帧的 -12 保留，不改成 -7；第 12 帧填正数 **+3**。手臂其他帧保留已有数值，三个 FK/IK 开关继续为 False。

然后打开 Playback Options，设置 **Start=0、End=19、Display Rate=30 fps**。最后一个姿势 key 仍在第 18 帧，保存。

预览时检查两点：0 与 18 帧的左臂一致；3→7→12 帧能看到后仰减小、轻微向前回弹，而不是一直保持最大后仰到第 12 帧。

## 4. 重新烘焙两段结果

**修改或保存制作 Sequence，不一定会更新已有 Animation Sequence。需要确保两段 AS 都重新导出了最新内容。**

对闪避、受击分别执行一次：

1. 核对当前 Sequence 顶部标题与 §1 表格一致，先保存。
2. 清空 Sequencer 左侧搜索，右键最上层 **M0_AnimationDummy** 绑定行，选择 **Bake Animation Sequence**。
3. 输出到 `Clips`。如果当前流程提供覆盖同名资产，闪避选 `AS_M0_Dodge_01`，受击选 `AS_M0_HitResult_01`。只更新对应动作。
4. 如果对话框不接受同名、只允许新建，就分别输出 **`AS_M0_Dodge_02` / `AS_M0_HitResult_02`**，保留旧版，在反馈中告诉我实际名称，我会对齐后续引用。无需强制删除已有资产。
5. 使用当前 Sequence 播放范围；保持骨骼变换导出、`Export in World Space=False`（若提供），`Enable Root Motion=False`。
6. 关闭 Sequencer，单独打开**这次刚烘焙的 AS**，确认动作更新并保存。

| 动作 | Sequence 范围 | 目标 AS 时长 | 按当前已观察到的导出计数方式预计 |
|---|---|---|---|
| 闪避 | 0–21，30 fps | 约 **0.700 秒** | 21 个帧间隔 / 22 个采样点 |
| 受击 | 0–19，30 fps | 约 **0.633 秒** | 19 个帧间隔 / 20 个采样点 |

预计计数用于检查，最终以 UE 资产实际读数为准；如果界面只写“帧数”而未区分采样点，原样反馈字段即可，我会补查。**两段不能仍是约 1.033 秒**。不要靠提高 Play Rate 消除多余停顿，那会连前半段动作一起加速。

重新烘焙后分别在 Manny / Quinn_Simple 上预览：root 与双脚固定，闪避完整结束，受击包含回弹，首尾没有左臂跳变。若设置范围后仍导出 1.033 秒，或烘焙报错，停在当前状态，把输出选项截图和错误文字给我，我接手检查。

## 5. 保存与反馈

保存地图、两个真正的制作 Sequence 和新烘焙的 AS。回 ThirdPersonMap，快速确认移动 / 转向 / 跳跃、E、P 正常；原攻击动画不修改。

```text
M0-05B 补修完成 / 卡在第 __ 节
闪避 0 帧左上臂 Rotation Z=25：是 / 实际结果
受击 0 帧左上臂 Z=25，7 帧脊柱 X=-7，12 帧 X=+3：是 / 实际结果
闪避 / 受击 Sequence 的 End：21 / 19（或实际结果）
新烘焙的闪避 / 受击 AS 名称：
闪避 / 受击 AS 显示时长：
首尾、受击回弹、root / 双脚与 Quinn_Simple 预览：正常 / 实际结果
已保存，基础输入 / E / P 回归：正常 / 实际结果
补修用时：约 __ 分钟
```

本次是基础操作，由你执行。看不清菜单或控制器，发包含时间指针、控制器名称和 Anim Details 的截图；动作时序问题发 5–10 秒短视频；编译、导出或运行报错优先复制文字日志。卡住约 15 分钟即可反馈，无须继续反复删除或重建资产。
