# M7-02 核心遗物祭坛建模与接入

正式路线已用双槽祭坛替换圆柱灰盒。下凹槽位呼应装备界面的槽 A、槽 B。玄武岩、铜边和琥珀色刻纹延续灰烬短刃的配色。[M7-02 验收报告](Reports/M7_02_Verification.md)

## 1. 已交付的 Blender 源件

| 项目 | 实际规格 |
| --- | --- |
| 源文件与交接件 | [SM_ET_RelicAltar.blend](../SourceAssets/Props/RelicAltar/SM_ET_RelicAltar.blend) 与 [SM_ET_RelicAltar.fbx](../SourceAssets/Props/RelicAltar/SM_ET_RelicAltar.fbx) |
| 尺寸与原点 | Blender X / Y / Z 为 **124 × 76 × 102.5 cm**，中心原点贴地，Z 向上，正面刻纹朝 Blender **-Y** |
| 网格 | 单个 Static Mesh、**750 个三角形**、三个材质槽、一套 UV0 |
| 槽位 | 台面高度 102 cm，槽底 92.8 cm。两个槽均有真实内壁和槽底 |
| 视觉范围 | 静态槽位和刻纹为场景美术。装备状态由既有祭坛界面反馈 |
| 本地证据 | `Artifacts/M7_02/RelicAltar_Validation.json`、`Artifacts/M7_02/RelicAltar_Preview.png` |

Blender 5.3.0 已重开 `.blend` 并回导 FBX，两份模型的边界、原点、三角形、材质槽和 UV 一致。垂直射线分别命中两个槽底与中间台面，验证凹槽随 FBX 一同交接。预览照明仅用于看模型形体。

复做时将输出指向新目录。需要手工调整时，从正式 `.blend` 另存一份，在 Edit Mode 改网格，保留地面原点和整体尺寸约定。

```powershell
Set-Location 'E:/UE_Project/Demo'
& 'E:/Blender/blender.exe' --background --factory-startup --python-exit-code 1 --python './Tools/Build-RelicAltar.py' -- './Artifacts/M7_02/Revision02'
```

## 2. 导入网格与三个材质

1. 打开 UE 5.5.4，在 Content Browser 创建 `/Game/EmberTrial/Art/Props/RelicAltar`。进入该目录，点 **Import**，选上面的 `SM_ET_RelicAltar.fbx`。
2. 导入为 **Static Mesh**，统一缩放 **1**。关闭骨骼网格、动画、自动碰撞、Nanite 和贴图生成。可见 **Generate Lightmap UVs** 时关闭。可见法线导入选项时保留源法线，以保持切角硬边。如果 Interchange 自动生成材质，可直接在步骤 4 修改这三份材质。[UE 5.5 FBX 导入参数](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-import-options-reference-in-unreal-engine?application_version=5.5)
3. 双击导入的网格，检查两槽都能看到槽底，底座平整，尺寸约 **124 × 76 × 102.5 cm**，水平两轴可能因坐标转换互换。记下实际 LOD0 三角形与槽数。若 Static Mesh Editor 显示自动凸包，用 **Collision → Remove Collision** 清除并保存。网格的 Nanite 设置应为关闭。此祭坛沿用原版不阻挡走路、导航和交互射线的配置。
4. 创建或打开下表三个同名材质。保持 **Opaque / Default Lit**。在材质图中按住 **3** 点空白处生成 Constant3Vector，接 **Base Color**。按住 **1** 点两次生成两个 Constant，分别接 **Metallic** 和 **Roughness**。填下表线性值，逐份 **Apply → Save**。若导入材质已有自动生成的转换节点，用这三个输入替换对应连接。

   | 材质 | Base Color（线性 RGB） | Metallic | Roughness | 对应部位 |
   | --- | --- | ---: | ---: | --- |
   | `M_ET_AltarBasalt` | .052 / .061 / .074 | 0 | .79 | 底座、两侧立柱、台面和槽内壁 |
   | `M_ET_AltarBronze` | .236 / .114 / .052 | .72 | .43 | 铜边、槽口和正面徽记边框 |
   | `M_ET_AltarEmber` | .61 / .194 / .046 | .18 | .40 | 正面刻纹和两个槽底符号 |

5. 回到 Static Mesh Editor 的 **Material Slots**，按槽名称赋值。保存后，从正面和上方看一遍。若只有一个槽，先重查导入选项和 FBX，确保材质按几何分槽，再接蓝图。[UE 5.5 静态网格与材质槽](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine?application_version=5.5)

### 2.1 实施与复查（2026-11-16）

- 沿用 Interchange 已导入的 `/Game/EmberTrial/Art/Props/RelicAltar/SM_ET_RelicAltar`。导入初始开启 Nanite 并生成了 1 个凸包。关闭 Nanite、移除凸包后，实测 LOD0 为 **750 个三角形**，简单碰撞与凸包均为 **0**。UE 坐标下实测 **124 × 76 × 102.5 cm**，包围盒最低点 **Z=0 cm**。网格有 1 个 LOD、1 个 UV 通道。
- 初始导入提示 FBX 缺少独立平滑组信息。FBX 中已有面法线。本次在 **LOD 0 → Build Settings** 关闭 **Recompute Normals**，保留源文件的切角硬边。**Generate Lightmap UVs** 保持关闭。
- 三份同名材质已按上表改为三个常量节点，均为 **Opaque / Default Lit**。资产的三个槽逐一核对已指向对应材质。透视及顶视图可区分玄武岩、铜边和琥珀色刻纹，两个槽口均可看见槽底。
- 网格和三份材质已保存。尺寸、导入前后设置和保留的日志在本地 `Artifacts/M7_02/UERelicAltarAssets.json` 与 `Artifacts/M7_02/UEImportVerification.log`。正式地图的照明、朝向和交互在第 3、4 节复验。

## 3. 替换已有祭坛的外观

1. 打开 `/Game/EmberTrial/Blueprints/Interaction/BP_ETRelicAltar`。在 **Components** 选已有的 **Marker**。把 **Static Mesh** 从 Cylinder 改为 `SM_ET_RelicAltar`。在 **Transform** 中将相对 **Location = 0 / 0 / 0**、**Rotation = 0 / 0 / 0**、**Scale = 1 / 1 / 1**。原圆柱的 Z=30 和 Scale=.6 必须改掉。
2. 在同一组件上核对 **Collision Presets = NoCollision**、**Generate Overlap Events = False**、**Can Ever Affect Navigation = False**。Material Overrides 如果仍指向灰盒材质，清除覆盖，让网格使用其三个材质槽。选 **Label**，勾 **Hidden In Game**，保留正式路线地图中单独摆放的 `RELIC ALTAR [E]` 指示。**Compile → Save**。
3. 打开正式地图 `/Game/EmberTrial/Maps/L_ET_TrialRoute`。在 Outliner 选 **RelicAltar_A**，确认仍是同一个实例，位置 **-970 / -270 / 0**、Actor Yaw **0°**、Scale **1 / 1 / 1**。如果 Marker 仍显示 Cylinder 或旧缩放，检查实例中 Marker 对应属性有无覆盖，恢复为保存的蓝图默认值。
4. 从 **Checkpoint_A 到祭坛** 的方向看正面符号。正式图的 **Marker → Relative Rotation → Z / Yaw** 已核对为 **0°**。底座应贴地，正面朝向玩家接近的一侧，台面与上方 `RELIC ALTAR [E]` 保持间距。测试图 `L_ET_VerticalSlice` 保留自身的 Actor Yaw **90°**。
5. 核对祭坛 **Enabled=True、Interaction Radius=180、Max Vertical Distance=180、Sight Point Offset=(0,0,80)**。原生的 `AltarRoot`、`RangePreview`、`Marker`、`Label` 均保留。保存本次实际有改动的资源。蓝图默认外观更新后，其他继承默认值的测试地图也会显示新模型。

## 4. 分组验收

2026-11-16 实机与专项均通过。日志确认槽 A 装备生息余烬、槽 B 装备裂势印，普通 P 使用暂停模式，重开正式图后 E 仍可打开。[验收报告和本地截图路径](Reports/M7_02_Verification.md)

| 检查 | 操作与通过条件 |
| --- | --- |
| 保存与外观 | 重开正式地图，核对唯一祭坛、三材质、两槽凹面、底座贴地和角色比例。在玩家从 A 点走近的镜头下，能将祭坛与周围箱体区分开。近处显示地图原有 `[E]` 指示。留一张第三人称截图到本地 `Artifacts/M7_02`。 |
| 交互与装备 | 使用当前可继续存档进入 A，或者从 Entry 击败首名近战后走到 A。在祭坛旁按 **E**，确认单个页面打开，能把一件遗物装备到空槽。继续游戏后走到范围外，按 **E** 不应打开祭坛。按 **P** 确认原普通暂停页中的遗物区仍为只读。 |
| 短回归 | Stop 后再次进正式图，核对祭坛网格和交互仍在。确认经过祭坛的原路线可通行。退出 PIE 后在 Output Log 执行一次 `Automation RunTests EmberTrial.M3.Interaction.RelicAltar`，等待 **Success**。 |

```text
M7-02 完成
UE 网格 / 实际尺寸与LOD0 / 三材质 / 碰撞：
BP Marker局部变换 / 隐藏Label / Compile-Save：
正式图唯一祭坛 / 实际Marker Yaw / 贴地与正面：
真实E打开、装备、范围外拒绝 / 普通P只读：
RelicAltar自动化 / Stop重开：
实机截图路径 / 最明显的画面或交互问题（无填无）：
完成时分 / 导入与配置分钟 / 实机检查分钟：
```

## 5. Commit / Push

### 5.1 第 2 节导入记录与进度文档

文档单独提交时，暂存 `Docs/M7_02_RelicAltarModel.md`、`Docs/Progress.md`、`Docs/Backlog.md` 与 `Introduction.md`。含资产路径与 SHA-256 的 `Docs/AssetManifest.csv` 随资产批次提交。

- 英文 `docs(art): document relic altar mesh import and material setup`
- 中文 `docs(art): 记录遗物祭坛网格导入与材质配置`

### 5.2 整章资产与接入

暂存 `Tools/Build-RelicAltar.py`、`SourceAssets/Props/RelicAltar` 两份源件、`Docs/AssetManifest.csv`、本指南、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。UE 网格和三个材质已经登记在来源清单。第 4 节复验后暂存 `EmberTrial/Content/EmberTrial/Art/Props/RelicAltar` 下网格与三个材质，以及 `BP_ETRelicAltar.uasset`。仅当地图的 Marker 朝向或实例覆盖实际保存过改动时，暂存 `L_ET_TrialRoute.umap`。预览、截图与日志留在 `Artifacts`。

- 英文 `feat(art): model and integrate the two-slot relic altar`
- 中文 `feat(art): 制作并接入双槽遗物祭坛`

### 5.3 验收文档

资产已随 `b43112b` 提交。本次只暂存报告、指南、来源表和三份进度文档。[文档暂存清单](Reports/M7_02_Verification.md#本次文档-commit--push)

- 英文 `docs(art): record relic altar integration and interaction verification`
- 中文 `docs(art): 记录遗物祭坛接入与交互验收`
