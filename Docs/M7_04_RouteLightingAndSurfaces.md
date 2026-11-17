# M7-04 正式路线的场景材质与光照

本章先在检查点 A 到第二场入口做一段具有完整质感的样板，再把同一套墙材扩到整张正式路线。地面呈现磨损石板和接缝，墙体呈现砌块与风化，闸门呈现锈蚀和反光变化。沿用现有三场遭遇、检查点、祭坛和原有路径。

2026-11-17 终版已验收。地面、十面墙和三门使用地图实例覆盖，灯光与实机交互已核对。[验收报告](Reports/M7_04_Verification.md)

## 1. 纯色基线与 PBR 材质方案

### 1.1 已保存的纯色对照

`/Game/EmberTrial/Art/Environment/Route` 中三份 `M_Solid` 实例用于保留此前的配色和前后截图对照。已经赋过纯色的对象可以直接进入贴图阶段。

| 用途 | 资产 | Base Color（线性 RGB） | Metallic | Roughness |
| --- | --- | --- | ---: | ---: |
| 冷灰石地面 | `MI_ET_RouteFloorSlate` | .167 / .181 / .187 | 0 | .87 |
| 暖灰石墙 | `MI_ET_RouteWallStone` | .335 / .302 / .251 | 0 | .92 |
| 暗铜闸门 | `MI_ET_RouteGateBronze` | .260 / .151 / .079 | .58 | .50 |

已只读核对 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。原方向光为 Movable，强度 **6.0**、色温 **6500 K**。原天空光为 Movable，强度 **1.0**，使用 **Specified Cubemap** → `DaylightAmbientCubemap`，**Real Time Capture** 关闭。材质和地图基线在本地 `Artifacts/M7_04/RouteMaterials.json`、`Artifacts/M7_04/RouteBaseline.json`。天空光沿用 M6-02 已验证的捕获配置。[M6-02 验收](Reports/M6_02_Verification.md) · [UE 5.5 天空光属性](https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-lights-in-unreal-engine?application_version=5.5)

### 1.2 PBR 贴图选型与导入

样板使用三组 Poly Haven CC0 贴图。地面和墙体从 **2K** 开始，闸门从 **1K** 开始。每组在资源页选择分辨率，分别取 `Diffuse`、`Normal (DX)` 和 `AO/Rough/Metal` 的 PNG。源文件可先放在 `Artifacts/M7_04/TextureSources`，确认使用的版本和路径后再登记资产清单。[Poly Haven 资产许可](https://polyhaven.com/license)

| 表面 | 纹理表现和比例起点 | 来源 |
| --- | --- | --- |
| 地面 | 大块石板、磨损边缘与灰缝。每张贴图约覆盖 **180 cm** | [Monastery Stone Floor](https://polyhaven.com/a/monastery_stone_floor) |
| 墙体 | 风化砌石、砂浆接缝与凿痕。每张贴图约覆盖 **210 cm** | [Stone Block Wall](https://polyhaven.com/a/stone_block_wall) |
| 闸门 | 氧化金属、残留涂层与不均匀反光。先按门面尺寸调整 UV 重复 | [Rusty Metal 04](https://polyhaven.com/a/rusty_metal_04) |

九张源图已通过官方下载地址获取。石板和砌石各为 **2048 × 2048**，闸门为 **1024 × 1024**，均为 16-bit PNG。UE 资源在 `/Game/EmberTrial/Art/Environment/Route/Textures`。三个系列分别以 `T_ET_RouteFloor`、`T_ET_RouteWall`、`T_ET_RouteGate` 命名，各含 `_D`、`_N` 和 `_ARM`。已复核尺寸、压缩类型和 sRGB。使用的是 DirectX 法线，`Flip Green Channel` 保持关闭。源下载地址、哈希与导入数据在本地 `Artifacts/M7_04/TextureSources/Sources.json` 和 `Artifacts/M7_04/RoutePBRAssets.json`。

| 贴图 | UE 设置 | 材质连接 |
| --- | --- | --- |
| Diffuse | sRGB 开，默认颜色压缩 | RGB → Base Color。可乘以弱色调，起点为线性 **1 / 1 / 1** |
| Normal (DX) | Compression Settings = Normalmap，sRGB 关，Sampler Type = Normal | Normal |
| AO/Rough/Metal | Compression Settings = Masks (no sRGB)，sRGB 关，Sampler Type = Masks | **R → Ambient Occlusion、G → Roughness、B → Metallic**。地面和墙体的 Metallic 用 0 |

### 1.3 纹理投射与样板

1. 石材的两个实例共用 `M_ET_RouteStone`。颜色与 ARM 使用 **WorldAlignedTexture**，法线使用 **WorldAlignedNormal**，三个函数都取 `XYZ Texture`。共用 `TextureSizeCm` 参数控制尺寸，地面为 **(180,180,180)**，墙体为 **(210,210,210)**。ARM 按表拆 R 和 G，石材金属度为 **0**。本机 UE 5.5 的法线函数包含 World → Tangent 转换，石材的 **Tangent Space Normal** 应打开。该投射方式用于已有的大跨度非等比缩放地面和墙体。[UE 5.5 世界对齐纹理与法线](https://dev.epicgames.com/documentation/en-us/unreal-engine/texturing-material-functions-in-unreal-engine?application_version=5.5)
2. `M_ET_RouteGateMetal` 的三个 Texture Sample Parameter 2D 接同一个 **TextureCoordinate（UV0）**，U / V 各 **1.0**。三个采样按上表连接，使用切线空间法线，贴图随闸门网格移动。两个主材质均为 Opaque / Default Lit，贴图使用常规 mip 与纹理流送。
3. 创建 `MI_ET_RouteFloorSlate_PBR`、`MI_ET_RouteWallStone_PBR` 与 `MI_ET_RouteGateMetal_PBR`。先只换地面、A 点相邻的一对侧墙和第二场闸门。在正常第三人称距离比较石缝、材质反光与敌人轮廓，近看时再查世界投射接缝和纹理拉伸。A 点与第二场对照通过后扩到第 2 节全部对象。
4. 如果样板还显得平整，下一步优先给近景门框加真实倒角、在墙脚和门边布置少量磨损贴花，再用一道侧光读出浮雕。先让两处重点区域形成明暗层次，再决定其余区域要补多少细节。当前 PBR 样板用法线表现细小凹凸，沿用现有碰撞。

### 1.4 样板实施结果（2026-11-17）

九张贴图、两个主材质和三个实例已保存并登记到 `Docs/AssetManifest.csv`。在同一光照下检查初版后，调低地面和闸门的基础色亮度，并给墙体轻微暖色。三个实例的最终 `SurfaceTint` 为线性 RGB。

| 已应用的对象 | 材质实例 | SurfaceTint |
| --- | --- | --- |
| `SM_ET_Floor` | `MI_ET_RouteFloorSlate_PBR` | 0.60 / 0.66 / 0.70 |
| `SM_IntroExit_Wall_NegY`、`SM_IntroExit_Wall_PosY` | `MI_ET_RouteWallStone_PBR` | 1.08 / 1.02 / 0.96 |
| `MixedEncounter_A` 的 `ExitGate` | `MI_ET_RouteGateMetal_PBR` | 0.48 / 0.43 / 0.36 |

在 UE 视口分别以相同机位和 **1280 × 720** 分辨率记录两组对照，文件在本地 `Artifacts/M7_04`。祭坛机位能看出石板接缝和墙面砌块，第二场能看出闸门露出的金属和锈蚀区域。直射处石板仍偏亮，第二场门框还是灰盒轮廓。

| 机位 | 修改前 | 样板 |
| --- | --- | --- |
| 检查点 A 朝祭坛 | `Route_Altar_Before.png` | `Route_Altar_PBR_Refined.png` |
| 第二场入口朝闸门 | `Route_MixedGate_Before.png` | `Route_MixedGate_PBR_Refined.png` |

已用新的 UE 进程重新加载 `L_ET_TrialRoute`，四处材质覆盖均已保存，四个组件的碰撞模式与基线一致。重开时方向光为 **6.0 / 6500 K**，天空光为 **1.0**，指定立方体贴图且关闭实时捕获。存盘与复验日志在 `Artifacts/M7_04/SaveRoutePBRSample.log` 和 `Artifacts/M7_04/VerifyRoutePBRSample.log`。实机清场、交互、角色与锁定标记的可读性归入第 4 节。

## 2. 在正式地图替换表面材质

1. 打开 `L_ET_TrialRoute`。第 1.4 节已留两组修改前对照，终版可沿用对应的机位和窗口尺寸补拍。
2. 在 **Outliner** 选 `SM_ET_Floor`。确认 **Details → Materials → Element 0** 使用已保存的 `MI_ET_RouteFloorSlate_PBR`。
3. 正式图共 **十面墙**，第一场闸门两侧已配置。把剩余八面墙的 **Element 0** 设为 `MI_ET_RouteWallStone_PBR`。可在 Outliner 用 Ctrl 多选同类静态网格，一次赋值，最后按三对门侧墙和四面外墙分别核对。

   | 区域 | Outliner 对象 |
   | --- | --- |
   | 四面外围墙 | `SM_ET_Wall_PosX`、`SM_ET_Wall_NegX`、`SM_ET_Wall_PosY`、`SM_ET_Wall_NegY` |
   | 第一场闸门两侧 | `SM_IntroExit_Wall_NegY`、`SM_IntroExit_Wall_PosY` |
   | 第二场闸门两侧 | `SM_MixedExit_Wall_NegY`、`SM_MixedExit_Wall_PosY` |
   | 守关者闸门两侧 | `SM_GuardianExit_Wall_NegY`、`SM_GuardianExit_Wall_PosY` |

4. 在 `IntroEncounter_A` 和 `GuardianEncounter_Route` 实例的 **Details** 组件列表选择 **ExitGate**（也可能显示为 **Exit Gate**），把 **Materials → Element 0** 改为 `MI_ET_RouteGateMetal_PBR`。`MixedEncounter_A` 已在样板中配置，最后一起核对。若 Details 只有 Actor 参数，先展开顶部的组件树。若采用共享蓝图配置，在 `/Game/EmberTrial/Blueprints/BP_ETMixedEncounter` 的 **Components → ExitGate** 修改材质，Compile → Save，再核对三份正式图实例和继承该默认材质的测试图。
5. 确认三门的网格、变换、碰撞、遭遇 ID 和 Cleared On Spawn 配置沿用当前值。保存地图。祭坛和角色继续用 M7-01 至 M7-03 的资产。

## 3. 调整已有光源

1. 在 Outliner 的 **Lighting** 文件夹选 **DirectionalLight**。在 Details 搜 **Intensity**，试用 **4.5**。搜索 **Temperature**，保留 **Use Temperature** 勾选，把 **Temperature** 改为 **5200 K**。灯的旋转、阴影、Mobility 和 Atmosphere Sun Light 沿用原值。
2. 选同文件夹的 **SkyLight**，改 **Intensity Scale** 为 **0.90**。核对 **Source Type = SLS Specified Cubemap**、**Cubemap = DaylightAmbientCubemap**、**Real Time Capture = Off**。保留现有曝光配置。
3. 进 PIE 走回第 2 节的两个取景位置。先看阴影里的门、敌人和祭坛正面。若颜色可辨但阴影中的细节消失，优先把天空光退回 **1.0** 再比一次。保存实际通过的强度、色温和对照图。本章只改现有两盏灯的这三项值。

## 4. 分组验收

本轮清场、祭坛 E、Stop 重开和 `Encounter.Mixed` 已通过。终版同机位截图与读取的灯光参数见[验收报告](Reports/M7_04_Verification.md)。

| 组别 | 操作与通过条件 |
| --- | --- |
| 外观对照 | 同一画质、窗口和站位，各拍 A 点祭坛与第二场入口的修改后图。地面、墙体与闸门能从结构与反光区分，护甲和锁定标记在亮处与阴影都可读。近看查大面拉伸、明显平铺重复、缝线与法线方向。 |
| 清场与互动 | 从 Entry 或继续 A 进行一次真实交战。门在清场前挡路，清场后隐藏且能通过。祭坛的 `[E]` 提示仍可看清，近处 E 可开页面。 |
| 保存复验 | Stop 后重开正式图，核对材质、最终灯光参数及天空光捕获配置。Output Log 执行一次 `Automation RunTests EmberTrial.M4.Encounter.Mixed`，等待 `Success`。 |

对照图用于验收可读性。可用 `stat unit` 看同位置的帧时间是否明显波动，记录条件与观察值。发布性能以同一打包版本的测量为准。

```text
M7-04 完成
地面、十面墙与三门 / PBR实例 / 实例或共享蓝图方式 / Save：
方向光强度与色温 / 天空光强度、Cubemap、Real Time Capture：
A 点与第二场前后截图路径：
一场真实清场 / 闸门隐藏可通行 / 祭坛E：
Encounter.Mixed / Stop重开：
最难辨认处或其他问题（无填无）：
完成时分 / 配置分钟 / 实机检查分钟：
```

## 5. Commit / Push

终版地图与路线材质已随 `bd42051` 提交。验收文档按[本次暂存清单](Reports/M7_04_Verification.md#本次文档-commit--push)提交。以下保留样板与终版素材的原提交范围，便于查找。

### 5.1 文档

暂存本指南和同步的 `Docs/Progress.md`、`Docs/Backlog.md`。

- 英文 `docs(art): record route PBR sample verification`
- 中文 `docs(art): 记录试炼路线 PBR 材质样板与复验`

### 5.2 材质与地图

暂存 `EmberTrial/Content/EmberTrial/Art/Environment/Route` 内的九张贴图、两个主材质、三个 PBR 实例和三份纯色基线，连同 `L_ET_TrialRoute.umap` 与 `Docs/AssetManifest.csv`。如果闸门采用了共享蓝图配置，再暂存 `BP_ETMixedEncounter.uasset`。源贴图下载件、对照图、地图备份和日志保留在本地 `Artifacts/M7_04`。

如果单独提交第 1 节样板，可以用下面这条提交信息。

- 英文 `feat(art): add trial route PBR material sample`
- 中文 `feat(art): 接入试炼路线 PBR 材质样板`

整条路线与灯光实机验收后，使用本章提交信息。

- 英文 `feat(art): refine trial route surfaces and lighting`
- 中文 `feat(art): 打磨试炼路线的场景材质与光照`
