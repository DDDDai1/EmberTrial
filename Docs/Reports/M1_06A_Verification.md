# M1-06A：独立主菜单与战斗入口验收

复核：2026-10-10，UE **5.5.4**。**本批功能验收通过，无待补功能检查。**用户21:13反馈全部通过、无失败；实际手动45分钟（代码10 / 资产30 / 检查5），构建等待5分钟。发现一项不阻塞的菜单空HUD初始化Warning，原因与后续处理见第4节；不把日志描述为“完全无Warning”。M1-06B尚未下发。

本轮助手只读源码、构建记录、索引、配置、资产与日志，保存核查记录并更新文档；没有改Source、Build、PIE或代Commit / Push。此前经授权完成的Designer第7–11点属于已记录的实施历史。

## 1. 源码与双目标构建

- 相对[M1-05C2最终基线](Raw/M1_05C2_Baseline.json)，新增六份菜单Source，修改游戏Controller.cpp与PursuitMove.cpp两份既有Source，其余**51份既有Source哈希不变**。当前共**59份Source / 29个cpp**；Build.cs / Target / .uproject及战斗、AI决策、Presenter / HUD实现保持原版本。
- [A指南](../M1_06A_MainMenu.md)七个全文C++片段与实际文件一致，比较时忽略空白 / 注释。追击Task另核对四处`PursuitMoveTargetActorKey` / `PursuitMoveStopDistanceKey`，黑板字符串仍为`TargetActor` / `PursuitStopDistance`，未以关闭Unity绕过C2374。
- 最后Source修改：**16:14:58.520**，早于Editor receipt **16:15:39.961**、Game receipt **16:17:39.738**。两份receipt均Win64 / Development / UE5.5.4，类型分别Editor / Game；用户确认两次Build成功、无C2374。助手没有重新构建，未取得全局UBT Log.txt，构建依据为用户结果、receipt与源码时间。
- 实际`compile_commands.json`于**16:16:46.700**更新，去重后29个项目cpp，包含菜单GameMode、Controller、Widget。不是按GLOB行数估算索引数量。

Core菜单GameMode不生成Pawn / Spectator / HUD；UI菜单Controller持有唯一View并校验软地图、保护一次请求、管理焦点与主动清理；Widget仅发送意图 / 展示状态。游戏Controller显式Game Only、隐藏鼠标、Flush输入；菜单没有ASC / 战斗业务、Tick或Text Binding。

## 2. 运行直接证据

[日志摘录](Raw/M1_06A_Acceptance_2026-10-10.log)、[结构化证据](Raw/M1_06A_Evidence_2026-10-10.json)和[本批基线](Raw/M1_06A_Baseline.json)保存在本地。下列时间为Asia/Shanghai，原日志UTC加8；运行范围**21:10:11–21:13:20**。

| 路径 | 直接证据 |
| --- | --- |
| 菜单世界隔离 | 五轮Game class为BP_ETMenuGameMode，Menu player ready / Menu ready均Pawn=None；菜单阶段无UIBootstrap创建战斗HUD、无生命 / 追击初始化 |
| 正常进入战斗 | 21:10:39.680、21:11:57.817、21:12:20.947各一次Start requested，随后Browse正式地图；旧Menu removed / controller end先于新世界Gameplay初始化 |
| 新身体 / HUD / 输入 | 三轮Game input ready；三份Health HUD创建，各收到Ready100/100、Fraction1；玩家 / 敌人 / 训练身体重新初始化100/100。后续三份HUD均解绑 / 移除，Presenter Shutdown |
| 菜单退出 | 21:12:32.760 Quit requested，随后Cmd: quit、Menu removed / controller end与PIE清理，编辑器保留。此处证明PIE退出，未证明当前包内进程退出 |
| None地图负例 | 21:13:09.700一条Game map unavailable、Path为空；该轮没有Start requested / 战斗Browse / 新Pawn或HUD。21:13:19.998 Controller又一次正常保存 |
| P短基线 | 两组Pause opened / closed成对；具体鼠标 / Enter / Space动作、无卡键 / 补播按用户实测反馈 |

共**5次Menu ready / removed / end闭合，3次Start / Game input ready / HUD创建 / HUD移除闭合**。相关Blueprint Error / Accessed None / Fatal / 项目LogET Error为0。不同PIE世界允许再次使用`_C_0`等名字，不能用名字重复推断对象复用；生命周期顺序与各轮新初始化共同证明世界重新装配。

## 3. 配置、四项资产与证据边界

直接核对共享Config：Game Default Map为`L_ET_MainMenu`，Editor Startup Map为`L_ET_VerticalSlice`；Cook列表保留ThirdPersonMap并新增菜单 / 正式地图两项。`DefaultGame.ini`还由UE保存了`Build=Always → IfProjectHasCode`：本项目含C++，仍会选择构建，不据此宣称本轮已经Cook / 打包。

| 新资产 | 最后正常保存（2026-10-10） |
| --- | --- |
| WBP_ETMainMenu | 21:03:54.867 |
| BP_ETMenuGameMode | 21:06:08.082 |
| L_ET_MainMenu | 21:07:49.814 |
| BP_ETMainMenuController | 21:13:19.998，晚于None负例 |

四项资产的实际来源、用途、SHA-256已登记到[AssetManifest](../AssetManifest.csv)，**清单40→44**；原40项哈希全部不变。BP Classes、四BindWidget / 无Binding / Never、键盘导航、首次输入与最终软地图恢复按用户完整反馈接受；本轮未重新逐字段解析BP或操控编辑器。

保留日志没有None负例中的Quit requested，也没有恢复引用后再次从菜单进入战斗的记录；这两项按用户明确“通过 / 已恢复保存”登记，不虚构直接日志。真正加载失败的TravelFailure、当前阶段独立包和性能不属于本轮已测结果。方案B允许复用用户实测与已有证据，不为没有矛盾的记录缺口重跑整套流程。

## 4. 已定位的非阻塞Warning

五次菜单初始化各有一条`LogSpawn: Warning: SpawnActor failed because no class was specified`。本机引擎源码中，`GameModeBase.cpp:894`默认InitializeHUDForPlayer调用ClientSetHUD(HUDClass)，`PlayerController.cpp:1354`的实现仍在1368行尝试SpawnActor<AHUD>；菜单明确设置HUDClass=null，所以留下这条提示。不是缺失BindWidget或玩家生成失败，菜单Pawn=None符合设计。

处理方案已加入[A指南第11.1节](../M1_06A_MainMenu.md#111-菜单空hud初始化warning本轮发现非阻塞)：菜单GameMode覆盖InitializeHUDForPlayer_Implementation为空实现，保留游戏GameMode的HUD流程，不调用Super。**当前Source未加入该补修，未声称Warning消失**；在下一批原生代码构建时一并处理，只核对一次菜单启动 / 开始，不重复本批输入矩阵。

## 5. 工时、Push与交接

本批**手动45 / 等待5分钟**，检查5分钟；不把此前助手GUI操作时间算进用户投入，不以单次样本推断精确节省率。项目累计手动**1566分钟（26小时06分钟）**、明确等待**125分钟**。M1-06当前仅A功能已验收，B / C / D待下发；旧P菜单保留。

本机HEAD与缓存origin/main均为`8843ffa`，本轮未联网核验远端。**六个新文件在暂存区仍是初始类骨架，工作区才有完整实现（AM）；用户Commit前必须重新加入最新修改。**按[A最后Push清单](../M1_06A_MainMenu.md#13-本批需要commit--push的内容固定放在文档最后)上传八份Source、四资产、两共享Config、指南 / 本报告 / 清单 / 状态文档，以及已有个人设置取消跟踪删除记录；个人配置磁盘文件保留。Raw / 缓存 / 构建产物继续留本地，助手未操作暂存区或代提交。
