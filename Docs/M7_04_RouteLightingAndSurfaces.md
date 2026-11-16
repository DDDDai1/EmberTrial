# M7-04 正式路线的场景材质与光照

目前白色地面、白墙和灰格闸门明度接近。后续实机用冷灰石地面、暖灰石墙和暗铜闸门区分可走区域与关卡边界，用原方向光和天空光控制明暗。保留三场遭遇、检查点、祭坛和原有路径。预计手动 **25–35 分钟**。

## 1. 已准备的材质与现有光照

已在 `/Game/EmberTrial/Art/Environment/Route` 生成三份材质实例，共用项目已有的 `/Game/LevelPrototyping/Materials/M_Solid`。材质用纯色与粗糙度区分表面，沿用现有网格的材质槽。

| 用途 | 资产 | Base Color（线性 RGB） | Metallic | Roughness |
| --- | --- | --- | ---: | ---: |
| 冷灰石地面 | `MI_ET_RouteFloorSlate` | .167 / .181 / .187 | 0 | .87 |
| 暖灰石墙 | `MI_ET_RouteWallStone` | .335 / .302 / .251 | 0 | .92 |
| 暗铜闸门 | `MI_ET_RouteGateBronze` | .260 / .151 / .079 | .58 | .50 |

已只读核对 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。原方向光为 Movable，强度 **6.0**、色温 **6500 K**。原天空光为 Movable，强度 **1.0**，使用 **Specified Cubemap** → `DaylightAmbientCubemap`，**Real Time Capture** 关闭。材质和地图基线在本地 `Artifacts/M7_04/RouteMaterials.json`、`Artifacts/M7_04/RouteBaseline.json`。天空光沿用 M6-02 已验证的捕获配置。[M6-02 验收](Reports/M6_02_Verification.md) · [UE 5.5 天空光属性](https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-lights-in-unreal-engine?application_version=5.5)

## 2. 在正式地图替换表面材质

1. 打开 `L_ET_TrialRoute`，先留两张同视角的修改前截图。一张站在检查点 A 朝祭坛拍，一张从第二场入口朝闸门拍。记住站位、朝向和窗口尺寸，截图放在本地 `Artifacts/M7_04`。
2. 在 **Outliner** 选 `SM_ET_Floor`。在 **Details → Materials → Element 0** 选 `MI_ET_RouteFloorSlate`。这个设置是当前地图实例的覆盖。网格资产、位置、缩放和碰撞使用当前值。
3. 同样设置以下六面墙的 **Element 0** 为 `MI_ET_RouteWallStone`。可在 Outliner 用 Ctrl 多选同类静态网格，给共同的 Element 0 一次赋值，最后逐个核对。

   | 区域 | Outliner 对象 |
   | --- | --- |
   | 四面外围墙 | `SM_ET_Wall_PosX`、`SM_ET_Wall_NegX`、`SM_ET_Wall_PosY`、`SM_ET_Wall_NegY` |
   | 出口两侧墙 | `SM_GuardianExit_Wall_PosY`、`SM_GuardianExit_Wall_NegY` |

4. 依次选 `IntroEncounter_A`、`MixedEncounter_A`、`GuardianEncounter_Route`。在各实例 **Details** 的组件列表选择 **ExitGate**（有空格时显示为 **Exit Gate**），把 **Materials → Element 0** 改为 `MI_ET_RouteGateBronze`。若 Details 只有 Actor 参数，先点展开顶部的组件树。若实例中的材质仍无法修改，在 `/Game/EmberTrial/Blueprints/BP_ETMixedEncounter` 的 **Components → ExitGate** 修改共享默认材质，Compile → Save，再核对正式图三份实例。用共享蓝图方式时，其他继承默认外观的测试图也会显示暗铜闸门。
5. 确认三门的网格、变换、碰撞、遭遇 ID 和 Cleared On Spawn 配置沿用当前值。保存地图。祭坛和角色继续用 M7-01 至 M7-03 的资产。

## 3. 调整已有光源

1. 在 Outliner 的 **Lighting** 文件夹选 **DirectionalLight**。在 Details 搜 **Intensity**，试用 **4.5**。搜索 **Temperature**，保留 **Use Temperature** 勾选，把 **Temperature** 改为 **5200 K**。灯的旋转、阴影、Mobility 和 Atmosphere Sun Light 沿用原值。
2. 选同文件夹的 **SkyLight**，改 **Intensity Scale** 为 **0.90**。核对 **Source Type = SLS Specified Cubemap**、**Cubemap = DaylightAmbientCubemap**、**Real Time Capture = Off**。保留现有曝光配置。
3. 进 PIE 走回第 2 节的两个取景位置。先看阴影里的门、敌人和祭坛正面。若颜色可辨但阴影中的细节消失，优先把天空光退回 **1.0** 再比一次。保存实际通过的强度、色温和对照图。本章只改现有两盏灯的这三项值。

## 4. 分组验收

| 组别 | 操作与通过条件 |
| --- | --- |
| 外观对照 | 同一画质、窗口和站位，各拍 A 点祭坛与第二场入口的修改后图。地面、墙体与闸门可区分，护甲和锁定标记在亮处与阴影都可读。记录最难辨认的一处。 |
| 清场与互动 | 从 Entry 或继续 A 进行一次真实交战。门在清场前挡路，清场后隐藏且能通过。祭坛的 `[E]` 提示仍可看清，近处 E 可开页面。 |
| 保存复验 | Stop 后重开正式图，核对材质、最终灯光参数及天空光捕获配置。Output Log 执行一次 `Automation RunTests EmberTrial.M4.Encounter.Mixed`，等待 `Success`。 |

对照图用于验收可读性。可用 `stat unit` 看同位置的帧时间是否明显波动，记录条件与观察值。发布性能以同一打包版本的测量为准。

```text
M7-04 完成
地面、六面墙与三门 / 实例或共享蓝图方式 / Save：
方向光强度与色温 / 天空光强度、Cubemap、Real Time Capture：
A 点与第二场前后截图路径：
一场真实清场 / 闸门隐藏可通行 / 祭坛E：
Encounter.Mixed / Stop重开：
最难辨认处或其他问题（无填无）：
完成时分 / 配置分钟 / 实机检查分钟：
```

## 5. Commit / Push

### 5.1 文档

暂存本指南和同步的 `Docs/Progress.md`、`Docs/Backlog.md`、`Introduction.md`。本批开始时也生成了 M7-03 验收报告，可按其清单一并提交。

- 英文 `docs(art): document route surface and lighting setup`
- 中文 `docs(art): 记录正式路线场景材质与灯光配置`

### 5.2 材质与地图

验收后暂存 `EmberTrial/Content/EmberTrial/Art/Environment/Route` 内三份材质实例、`L_ET_TrialRoute.umap` 和 `Docs/AssetManifest.csv`。如果闸门采用了共享蓝图配置，再暂存 `BP_ETMixedEncounter.uasset`。前后对照图和日志留在本地。

- 英文 `feat(art): refine trial route surfaces and lighting`
- 中文 `feat(art): 打磨试炼路线的场景材质与光照`
