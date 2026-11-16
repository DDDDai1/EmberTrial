# M7-01 主武器「灰烬短刃」Blender 试做与 UE 接入

灰烬短刃已导入 UE 5.5.4 并挂到正式玩家右手。材质、挂载、三段轻击随动和重开结果见第 3、4 节。[M7-01 验收报告](Reports/M7_01_Verification.md)

## 1. 模型规格

保留短而偏重的轮廓。刀身用深灰冷调金属，握柄用暗棕缠带，两面加一块低饱和琥珀色嵌片。先看玩家原有越肩镜头里能否一眼分辨刀身方向和握持位置，再决定是否增加细节。

| 项目 | 试做值 |
| --- | --- |
| UE 资源 | `/Game/EmberTrial/Art/Weapons/CinderBlade/SM_ET_CinderBlade` |
| Blender 与 FBX | `SourceAssets/Weapons/CinderBlade/SM_ET_CinderBlade.blend` 和同名 `.fbx` |
| Blender 坐标 | 单位米，原点在握柄中心，刀尖沿 +Z，宽度沿 X，两面朝 ±Y |
| 导入边界 | 约 **8.8 × 3.4 × 56 cm**，武器组件 Scale **1 / 1 / 1**。长边尺寸允许 1 cm 的测量差 |
| 几何预算 | 单个 Static Mesh，脚本预期 276 个三角形。LOD0，Nanite 关闭，静态网格无碰撞体和物理模拟 |
| 材质预算 | 三个纯色材质槽 Steel / Wrap / Ember。预留 UV0，零贴图，本轮以几何、颜色和粗糙度区分材质 |
| UE 连接 | 正式玩家蓝图上的一个 Static Mesh Component，挂到现有 Skeletal Mesh 的 `hand_r`，以组件局部变换对位 |

当前项目的轻击用 `hand_r` 骨骼位置、25 cm 半径和 Montage 命中窗口做扫掠。本章用短刃试做对照视觉和已有判定。如果出现清楚的刀尖穿过敌人但不掉血，或者刀身明显没接触却产生伤害，把它录成下一批调整判定的输入。先留一组旧半径基线，方便判断是握持、动画轨迹还是 Trace 范围造成的。现有逻辑见 [攻击 Trace](M1_04B1_AttackTrace.md)。

## 2. 连接 Blender 并生成源资产

本机使用 `E:/Blender/blender.exe`，实测为 Blender **5.3.0**，构建 `02f0899d78e3`。已生成 [SM_ET_CinderBlade.blend](../SourceAssets/Weapons/CinderBlade/SM_ET_CinderBlade.blend) 和 [SM_ET_CinderBlade.fbx](../SourceAssets/Weapons/CinderBlade/SM_ET_CinderBlade.fbx)。Blender 重开源文件及回导 FBX 后，单个网格、**8.8 × 3.4 × 56 cm**、握柄原点、**276 个三角形**、三个材质槽及 **1 套 UV** 一致。导出校验和预览在本地 `Artifacts/M7_01`。

下方是重新生成的命令。生成器使用独立的后台 Blender 进程。现有正式文件会保留，复做时把 `$weaponOutput` 指向新目录，例如 `Artifacts/M7_01/CinderBlade_Revision02`。

```powershell
$weaponBlender = 'E:/Blender/blender.exe'
Set-Location 'E:/UE_Project/Demo'
if (!(Test-Path -LiteralPath $weaponBlender -PathType Leaf)) {
    throw '把 $weaponBlender 改为你的 blender.exe 完整路径。'
}
$weaponOutput = Join-Path (Get-Location).Path 'SourceAssets/Weapons/CinderBlade'
& $weaponBlender --version
& $weaponBlender --background --factory-startup --python-exit-code 1 --python './Tools/Build-CinderBlade.py' -- $weaponOutput
if ($LASTEXITCODE -ne 0) { throw '主武器生成失败，请保留 PowerShell 中的错误。' }
```

1. 看到 `XYZ (8.8, 3.4, 56.0) cm`、`276 triangles` 和两条输出路径后，用 Blender 打开 `.blend`。Outliner 应只有一把短刃，选中后按数字键盘 **.** 聚焦。按小键盘 **1** 看正面，按小键盘 **3** 看厚度，再打开 **Material Preview** 核对三种材质。
2. 按 **N → Item**，检查对象 Location / Rotation 为 0，Scale 为 1。打开 **Viewport Overlays → Origins** 或选中对象，用橙色原点确认它处于握柄中心。模型保留了分离的部件，在 Edit Mode 将鼠标移到某部件上按 **L** 可以选中其相连几何。
3. 如要试改轮廓，先 **Save As** 到本地 `Artifacts/M7_01` 再做一版。保留对象原点和方向，移动几何时进 Edit Mode。导出时选中短刃，走 **File → Export → FBX**，开启 Selected Objects，类型只选 Mesh。Scale 1、Apply Unit 开、Apply Scalings 选 FBX Units Scale、Forward `-Y`、Up `Z`、Geometry 的 Triangulate Faces 开、Animation 关闭。确认可用后再同步正式 `.blend` 和 `.fbx`。生成器遇到已存在的正式文件会退出，可用不同输出目录生成一份新版再比较。[Blender FBX 导出参数](https://docs.blender.org/api/4.5/bpy.ops.export_scene.html)

## 3. 导入 UE 与材质

1. 打开 EmberTrial。在 Content Browser 进入 `EmberTrial/Art/Weapons`，创建 **CinderBlade** 文件夹，点 **Import** 选择源目录的 `SM_ET_CinderBlade.fbx`。UE 5.5 的 FBX 导入可能显示 Interchange 面板。保留 **Static Mesh**，关闭骨骼网格、动画、自动碰撞、Nanite、材质和贴图自动生成中能看到的对应选项。导入比例 **1.0**。若可见 **Generate Lightmap UVs**，关闭它。使用一份网格与三个材质槽。[UE 5.5 FBX 静态网格流程](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine?application_version=5.5)
2. 打开导入的 Static Mesh。查看编辑器显示的尺寸、LOD0 三角形、材质槽和碰撞。长边应约 56 cm，原点应在握柄。若尺寸差约 100 倍，在 FBX 的导出单位与导入设置里排查并对同一资产 Reimport。若整个模型绕错轴，也在导入阶段查证，避免在挂载时用缩放或多层旋转抵消。
3. 在同目录创建下表的三个 Opaque / Default Lit 材质。在材质图按住 **3** 点空白生成 Constant3Vector，接 **Base Color**，按住 **1** 各建一个 Constant 接 **Metallic** 和 **Roughness**，填下表线性值。每份点 **Apply → Save**。在 Static Mesh Editor 的 **Material Slots** 逐槽赋值，以导入槽名和网格对应部位为准。赋完再看 Preview，金属、握柄和两枚嵌片应各自正确。若导入只剩一个槽，重查 FBX 的材质导入设置和实际槽名，先保存错误界面的截图再排查。

   | 材质 | Base Color（R / G / B） | Metallic | Roughness |
   | --- | --- | ---: | ---: |
   | `M_ET_BladeSteel` | .105 / .117 / .138 | .72 | .38 |
   | `M_ET_BladeWrap` | .031 / .025 / .022 | 0 | .78 |
   | `M_ET_BladeEmber` | .46 / .11 / .022 | .18 | .42 |

**第 3.3 节实施记录（2026-11-16）**

FBX 导入时 Interchange 已生成三份同名材质，并分配给 Steel、Wrap、Ember 三个槽。沿用这些材质资产，把自动生成的 Phong 转换图换成上表的线性颜色、金属度和粗糙度常量，Compile / Save 后确认保留三个槽。随后关闭网格的 Nanite 并清除导入时生成的凸包。已保存的网格为 **8.8 × 3.4 × 56 cm**、LOD0 **276 三角形**、简单碰撞和凸包均为 **0**。三种材质可在实机近景中区分。材质复查数据在本地 `Artifacts/M7_01/UEMaterialsVerified.json`，其中网格字段为关闭 Nanite 和移除凸包前的快照。

## 4. 挂到玩家右手

1. 打开 `/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter`，进 **Viewport**。在 **Components** 选中继承的 **Mesh (CharacterMesh0)**，点 **Add → Static Mesh**，命名 **CinderBladeMesh**。如果它出现在 Capsule 下，将它拖到 Mesh 下，确认树中缩进一层。
2. 选 **CinderBladeMesh**。在 Details 中把 **Static Mesh** 指向 `SM_ET_CinderBlade`。搜索 **Parent Socket**，打开选择框选 **hand_r**。部分界面把骨骼和 Socket 放在同一个列表，选普通右手骨即可。第一次填写后清掉搜索框，对照 **Transform** 把 Location、Rotation 重置为 0，Scale 为 1，再进行对位。[UE 5.5 骨架的挂载定义](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/Animation/FPreviewAttachedObjectPair?application_version=5.5)
3. 在 **Collision** 把 **Collision Presets** 设为 **NoCollision**，核对 **Generate Overlap Events** 和 **Simulate Physics** 为 False。Details 可见 **Can Ever Affect Navigation** 时将其关闭。组件保留可见和移动性，方便跟随手骨动画。
4. 在蓝图视口围绕右手看正面和侧面，先调 Rotation，再调 Location。用局部旋转轴以 **90°** 步进把短刃对到掌心接近指节的握柄方向，随后用 **5–15°** 微调厚度朝向。移动用 **1 cm** 步进，将握柄中心放进掌内，护手靠食指侧、尾帽靠小指侧。每次用 **Compile → Play** 看游戏中的待机与连段。保留实际对位好的 Location、Rotation 三轴值。持握动画原为徒手，本轮以掌心贴合、动态穿插可辨、镜头内可读为验收。若姿势全程明显穿插，保留截图供后续修改动画。
5. 确认只有玩家蓝图新增一个组件。编译保存后回正式路线 `L_ET_TrialRoute`，验证重开时挂载正常。骨骼、攻击和敌人资产沿用原配置。

**第 4 节实施与复查记录（2026-11-16）**

- `BP_ThirdPersonCharacter` 新增单个 `CinderBladeMesh`，父级为 `Mesh (CharacterMesh0)`，**Parent Socket = `hand_r`**。Static Mesh 为 `SM_ET_CinderBlade`，Mobility 为 Movable，Collision 为 NoCollision，Overlap、物理模拟、导航影响均关闭。玩家蓝图已 Compile / Save。
- 用 `SKM_Quinn_Simple` 参考姿势中四个右手指根的位置确定握柄方向，把护手对到食指侧、尾帽对到小指侧。下面是已保存的局部变换，可直接在组件 Details 中复用。

  | 变换 | X | Y | Z |
  | --- | ---: | ---: | ---: |
  | Location（cm） | -7.556183 | 1.515425 | -0.376300 |
  | Rotation（°，Roll / Pitch / Yaw） | 10.741906 | -13.116233 | 178.911335 |
  | Scale | 1 | 1 | 1 |

- 正式地图 `L_ET_TrialRoute` 第一次 PIE 验证待机和 `AM_ET_Light_01 → 02 → 03`。每段播放中采样一次，刀柄在 `hand_r` 坐标系中的位置与待机一致，挂载组件始终只有一份。Stop 后再 Play，重新出生时同样只有一份挂载。运行记录在本地 `Artifacts/M7_01/UEBladeGripRuntime.json`。
- 右手正前侧与外侧近景可见握柄贴掌。当前徒手动画的手指仍有张开，轻击第 2 段能看到刀身，默认待机视角里右臂遮住了大部分刀身。近景通过临时调整 PIE 镜头采集，已退出 PIE。截图在 `Artifacts/M7_01`，文件为 `BladeGrip_Front.png`、`BladeGrip_Side.png`、`BladeRuntime_Idle.png` 和 `BladeRuntime_Combo02.png`。

## 5. 分组验收

2026-11-16 已完成。实机轻击每次 25 点伤害，`EmberTrial.M2.Combo.Flow` 返回 Success，键鼠操作通过。刀尖对照的单次命中或落空细节未随验收反馈记录。[完整验收](Reports/M7_01_Verification.md)

| 检查 | 操作 | 通过条件 |
| --- | --- | --- |
| 源文件与导入 | 保存并重开 `.blend`，在 UE 打开同名 Static Mesh | 单个网格、三槽、实际长边约 56 cm，原点仍在握柄，LOD0 和 NoCollision 设置可复查 |
| 持握与镜头 | 正式图待机、走动转身、左键依次打三段、2 闪避、Tab 锁定 | 武器跟随同一右手，切动作与解锁无脱落或额外生成。记下最明显的手掌或躯干穿插与近景遮挡 |
| 判定一致性 | 对第一个真实近战敌人在正常距离单点轻击，查看伤害日志。让一次挥击只在可见刀尖附近掠过，再记录实际命中 | 原轻击仍能造成实际伤害，新网格无额外碰撞或重复伤害。记录刀尖对照的实测结果 |
| 短回归 | 运行 `Automation RunTests EmberTrial.M2.Combo.Flow` 一次。实机按 P 继续，Stop 后再进一次 | 原连段自动化通过，重开仍只挂一把短刃 |

截图取待机持握近景和常规战斗视角各一张，保留在 `Artifacts/M7_01`。若只有灰盒镜头，保留它作为后续升级的前景对照。

```text
M7-01 完成
Blender 版本 / 实际 blender.exe 路径：
.blend / .fbx 文件路径 / 重开结果：
UE 网格路径 / 长宽厚 / LOD0 三角形 / 三材质 / 碰撞：
BP 手骨 / 组件 / 最终 Location XYZ / Rotation XYZ：
待机、移动、三段轻击、闪避与锁定镜头：
正常距离实际一次伤害 / 刀尖可见范围对照：
Combo.Flow / P 与 Stop 重开：
两张截图路径 / 最明显穿插、遮挡或失败（无填无）：
完成时分 / 模型与导入分钟 / 对位与实机分钟：
```

## 6. Commit / Push

生成器、资产清单与首版指南已随 `a9f7683` 提交，源 `.blend`、交接 `.fbx`、UE 武器目录和 `BP_ThirdPersonCharacter.uasset` 随 `676e8fc` 提交。验收文档的暂存范围和提交文案见 [M7-01 验收报告](Reports/M7_01_Verification.md)。

原资产提交文案

- 英文 `feat(art): model and attach the player cinder blade prototype`
- 中文 `feat(art): 制作灰烬短刃原型并挂载到玩家右手`
