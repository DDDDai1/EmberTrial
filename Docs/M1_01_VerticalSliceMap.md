# M1-01：正式纵向测试地图

生成日期：2026-10-01；验收日期：2026-10-02。前置：[M0-06 包内运行通过](Reports/M0_06_Verification.md)、[M0-07 范围与资源校准完成](Reports/M0_07_Calibration.md)。**本批已通过**，用户手动约 20 分钟，文件与一次新地图 PIE 日志已核对；两次运行和具体操作按反馈登记。见 [M1-01 验收报告](Reports/M1_01_Verification.md)。下方步骤保留供复查，无需重做。

## 1. 本批目标与分工

由你在 UE 中创建一间约 **20 m × 20 m** 的简单房间，能可靠出生，沿用已有角色与 1 / 2 / 3、E、P。之后在此逐步接入正式玩家、敌人、HUD 和失败重试，成为 M1 的纵向玩法场景。

预计手动 **15–25 分钟**；出现问题按实际用时登记。此批使用地图和 Actor 的基础编辑操作，没有新增 C++，也无需跑自动测试或重新打包。后续 M1 阶段包再切换默认启动 / Cook 地图并验收独立运行。

| 内容 | 本批产物 |
|---|---|
| 关卡名 | **L_ET_VerticalSlice** |
| UE 资源路径 | `/Game/EmberTrial/Maps/L_ET_VerticalSlice` |
| 磁盘位置 | `E:\UE_Project\Demo\EmberTrial\Content\EmberTrial\Maps\L_ET_VerticalSlice.umap` |
| 地面 / 墙壁 | 引擎 Cube，cm 为单位，先用默认基础材质 |
| 出生与角色 | 一个 PlayerStart，项目现有 EmberTrialGameMode / BP_ThirdPersonCharacter |
| 交互练习 | 从 ThirdPersonMap 复制两个已验证实例到新地图，确认它们并不依赖旧场景 |

本批完成后获得可工作的场景基线；完整战斗、HUD 与菜单闭环按 [M1 工作顺序](Reports/M0_07_Calibration.md) 分批实现。

## 2. 创建地图

1. 打开 EmberTrial，回到 ThirdPersonMap，退出 PIE，使用 **File → Save All**。
2. 在 World Outliner 选中现有两个练习方块实例；可搜 `Target` 或按你当前给 Actor 设置的名称寻找。确认只选中这两个实例，按 **Ctrl+C**。这里复制实例的已有组件 / 标签配置。
3. 点击 **File → New Level**，选择 **Basic（基础）**，点击 Create。Basic 是带基础地面与灯光的关卡模板；选择该模板即可。
4. 按 **Ctrl+Shift+S**（Save Current Level As），在内容目录创建 / 选择 `EmberTrial/Maps`，命名 **L_ET_VerticalSlice** 并保存。
5. 打开 **Window → World Settings**，找到 GameMode → **GameMode Override**，设为 **None**，使用已验证的项目默认 EmberTrialGameMode。

后续每次编辑前确认视口显示的关卡名是 L_ET_VerticalSlice。

## 3. 搭建房间

### 3.1 地面

1. 使用顶部 **Add（+）→ Shapes → Cube** 放置一个基础立方体；也可在 Place Actors 中搜索 Cube 并拖入视口。
2. 在 Outliner 重命名为 **SM_ET_Floor**。
3. 选中它，在 Details → Transform 填下表的 Location / Rotation / Scale。
4. 确认 Static Mesh 为 `/Engine/BasicShapes/Cube`，Mobility=Static，Simulate Physics 关闭，Collision Presets=BlockAll。
5. 在这个新地图中删除 Basic 模板原来的 Floor Actor，使用刚添加的 SM_ET_Floor 作为唯一地面。保留模板的灯光 / 天空。

| Actor | Location X / Y / Z（cm） | Rotation X / Y / Z | Scale X / Y / Z |
|---|---|---|---|
| SM_ET_Floor | 0 / 0 / -10 | 0 / 0 / 0 | 20 / 20 / 0.2 |

Cube 原始边长为 100 cm，因此地面宽 2000 cm、厚 20 cm，顶面在 Z=0。Scale 是倍数，Location 是 cm，两者不要混填。

### 3.2 四面墙

再放置四个 Cube，分别命名并填下面数值；都使用 Static / BlockAll / 不模拟物理。

| Actor | Location X / Y / Z（cm） | Rotation X / Y / Z | Scale X / Y / Z |
|---|---|---|---|
| SM_ET_Wall_PosX | 1025 / 0 / 150 | 0 / 0 / 0 | 0.5 / 20 / 3 |
| SM_ET_Wall_NegX | -1025 / 0 / 150 | 0 / 0 / 0 | 0.5 / 20 / 3 |
| SM_ET_Wall_PosY | 0 / 1025 / 150 | 0 / 0 / 0 | 20 / 0.5 / 3 |
| SM_ET_Wall_NegY | 0 / -1025 / 150 | 0 / 0 / 0 | 20 / 0.5 / 3 |

房间净宽约 20 m、墙高 3 m，无屋顶。先保留默认材质，确保角色、地面和墙边界能看清。后续入口门、祭坛与遭遇布局再逐批添加。

## 4. 出生与两个交互实例

### 4.1 PlayerStart

1. 在 Outliner 搜索 **PlayerStart**。
2. Basic 模板如已有 PlayerStart，调整已有实例；没有则用 Add / Place Actors 搜索并放置一个。
3. 保留**一个** PlayerStart，填 Location **X=-600，Y=0，Z=120**，Rotation **0 / 0 / 0**，Scale **1 / 1 / 1**。
4. Play 按钮右侧小箭头中，将 Spawn Player At 设为 **Default Player Start**，避免沿用 Current Camera Location 影响出生验证。

Z=120 让胶囊在地面上方安全生成，然后由正常重力落地；零旋转朝 +X。没有必要在地图中再放一个玩家 Character。

### 4.2 粘贴已验证实例

1. 在新地图中按 **Ctrl+V**，粘贴第 2 节复制的两个练习实例。
2. 如果剪贴板内容已变，保存当前新地图，打开 ThirdPersonMap，只复制那两个实例，再打开 L_ET_VerticalSlice 粘贴。
3. 重命名新地图中的实例并调整 Actor Transform / Practice Label：

| 新实例名称 | Location X / Y / Z（cm） | Rotation | Scale | Practice Label |
|---|---|---|---|---|
| Target_VS_A | 0 / -300 / 50 | 0 / 0 / 0 | 1 / 1 / 1 | M1_Target_A |
| Target_VS_B | 300 / 300 / 50 | 0 / 0 / 0 | 1 / 1 / 1 | M1_Target_B |

Practice Label 在选中实例后的 Details → Practice 中调整。Sphere、接口和激活初始状态沿用已验证配置；保持未激活的编辑器初值。

4. File → Save All。地图、地面、四墙、PlayerStart 和两个交互实例应已保存；需要时按 F 聚焦所选 Actor 检查位置。

## 5. 最小验收

1. 在新地图 Play。默认出生在 X=-600 附近，落到地面；人物 / 动画 / 镜头正确。
2. WASD / 鼠标 / Space 正常；走向任意两面墙，确认普通移动被阻挡。
3. 站定按 1，等结束；按 2 完整右侧闪避；再做一次 2→3 受击中断。角色能恢复控制。仅做功能冒烟检查。
4. 在墙边以角色右侧朝向墙，按 2，确认没有穿过墙或持续漂移；无需重复原来的坡面 / 帧率矩阵。
5. 对两个目标分别做靠近 → 按 E → 离开，看到 M1_Target_A / B 的提示、激活更新与隐藏。
6. 轻按并松开 P 打开暂停，鼠标继续；再开一次，Enter 或 Space 继续。恢复后直接移动和跳跃。
7. Stop，再 Play，确认出生位置稳定、目标恢复未激活、动作恢复 Idle；停止并 Save All。

画面太暗、出生报 BADsize、人物落空、角色与 PIE 既有基线不同或交互失效，反馈卡住的具体项即可。布局 / Details 给截图，日志给路径 / 时间，位移和动作问题给短视频；不用自行清理引擎缓存。

## 6. 反馈与下一批

```text
M1-01 完成 / 卡在第 __ 节
地图资源路径：/Game/EmberTrial/Maps/L_ET_VerticalSlice
地面 / 四墙 / 单个 PlayerStart：已配置
两次 Play 出生、移动转向跳跃和墙壁阻挡：通过 / 异常
1/2/3、闪避中断 / 墙边位移：通过 / 异常
E-A/B 提示和重开重置、P 鼠标 / 键盘继续：通过 / 异常
地图与资产已保存：是
大致完成时间：今天 __:__，日志由你读取
实际手动用时：约 __ 分钟
```

本批已验收；下一步按 [M1-02A 模块与 CLion 索引](M1_02A_RuntimeModules.md) 手动实现。C++ 仍按文档代码由你手动实现；新增模块时配套处理 CLion 索引，不把 IDE 误报当作 UBT 编译失败。
