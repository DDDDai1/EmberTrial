# M1-06B：真实失败、会话整图重试与返回菜单验收

复核：2026-10-11，UE **5.5.4**。**本批功能验收通过，无待补功能检查。**用户15:05反馈全部完成、无失败；实际手动**80分钟**（代码40 / 资产30 / 检查10），构建等待5分钟。菜单空HUD初始化Warning已收口；期间出现过Widget未完成时的编译提示和一次FailureWidgetClass配置缺失，恢复及证据边界见第4节。

本轮助手只读工程源码、receipt、索引、资产和运行日志，更新验收文档 / 来源清单并保存本地证据；没有修改工程Source / UE资产、运行Build / PIE或代Stage / Commit / Push。C会话检查点与D暂停迁移尚未下发。

## 1. 源码、构建与对象归属

- 相对[A基线](Raw/M1_06A_Baseline.json)，新增六份Session / FailureWidget / FailureFlow文件，修改七份CombatLife / GameMode / MenuController文件；其余**52份既有Source哈希不变**，无源码移除。当前**65份Source / 32个cpp**。Build.cs / Target / .uproject、旧Bootstrap / Presenter / HUD、战斗能力与AI实现保持既有版本。
- [B指南](../M1_06B_FailureAndRetry.md)八个全文C++片段与实际文件一致，比较忽略空白 / 注释；八个局部片段亦核对。其中Life前向声明与原生delegate之间保留既有属性结构声明，位置合法，不要求两条声明紧邻。Git差异确认Broadcast在原Death sealed日志之后，Clear在EndPlay结束保护之后；原取消能力 / Trace / 移动 / 动画冻结与Dead Tag清理未删。
- 最后Source修改**14:24:33.538**，早于Editor receipt **14:25:28.657**、Game receipt **14:27:50.817**。两份receipt均Win64 / Development / UE5.5.4，类型分别Editor / Game；用户确认双目标Build成功。助手未重新构建，构建结论依据用户结果、receipt与源码时间。
- 实际`compile_commands.json`于**14:25:39.677**更新，去重32个项目cpp，包含三份新实现；没有按CMake GLOB行数估算索引数量。

Core RunSession属于GameInstance，只保存地图包名 / 请求状态，当前切图期间保留OriginWorld弱引用，完成后释放；没有保存旧Pawn / ASC / Widget。正式GameMode登记当前地图，支持直接PIE；菜单和失败按钮统一向Session发请求。请求校验通过后先解除暂停再OpenLevel，所属GI的地图到达确认清除pending。

FailureFlow属于本地游戏Controller，仅观察当前身体的Life，查死亡快照并用一次NextTick呈现一份View；Widget只展示状态 / 发意图。死亡原规则先封锁，HUD仍收到Ready0，再显示失败页；不治疗旧尸体或移除Dead Tag复活。旧Pawn变化 / 切图 / EndPlay主动解除Life、UI与Session订阅并取消未执行任务，结束世界不重置输入 / 暂停。旧P死亡Branch按用户实测通过，存活P仍可继续。

## 2. 运行直接证据

[日志摘录](Raw/M1_06B_Acceptance_2026-10-11.log)、[结构化证据](Raw/M1_06B_Evidence_2026-10-11.json)与[本批基线](Raw/M1_06B_Baseline.json)只保留本地。以下时间为Asia/Shanghai，原日志UTC加8；验收运行范围**14:58:48–15:04:40**。

| 路径 | 直接证据 |
| --- | --- |
| 非玩家死亡隔离 | 14:59:37.878训练目标Death sealed，没有为该身体显示玩家失败页；玩家随后14:59:48.611归零才出现Failure queued / shown |
| 真实玩家死亡 / 新页 | 七次Failure shown均Body为当前玩家且Paused1；对应七次Failure removed。HUD收到Ready0，死亡封锁TraceTick0 / PracticeIdle1 / MoveMode0 / AnimPaused1在页显示之前 |
| 敌人真实致死 | 15:01:19.620，Enemy攻击Serial10、10→0、Actual10 / BaseAfter0；玩家Death sealed / HUD0后15:01:19.621显示失败页 |
| 三次整图重试 | 15:00:10.135、15:01:22.086、15:04:16.292各一条Retry requested，随后均Retry arrived Expected1 / Pending0；请求Paused0，旧Life / 页 / HUD / AI清理，新身体与HUD100/100 |
| 返回菜单 / 再开始 | 四组MainMenu requested / arrived闭合，菜单Pawn=None；五组NewRun requested / arrived闭合，进入新世界后Game input ready与生命初始化 |
| None预检及恢复 | 15:04:09.508一条Travel rejected，紧接Failure recovered；本次没有MainMenu请求。15:04:16正常Retry；Controller15:04:22保存后，15:04:33.112正常MainMenu到达 |
| 会话 / UI清理 | 七组Session ready / end、八组Menu ready / removed / end、十一组Life bound / unbound及Flow ready / end闭合；十一份Health HUD创建 / 移除、十一份Ready100快照；三次菜单Quit requested有清理 |

上述重复命名如`_C_0`可来自不同PIE / 新世界，不能据名字重复推断复用旧对象；旧世界结束、新世界初始化和到达确认共同支持重建。重试回现有入口，检查点尚未实现。

## 3. 资产保存与来源清单

| 本批资产 | 最后正常保存（2026-10-11） |
| --- | --- |
| 新WBP_ETFailure | 14:42:51.714 |
| 既有BP_ThirdPersonCharacter | 14:46:34.688 |
| 既有BP_ETPlayerController | 15:04:22.193，晚于临时None预检前的配置操作 |

三BindWidget名称 / 类型、原生父类、无Binding / Tick Never、单UIBootstrap + 单FailureFlow、最终两Class / MainMenu Map以及旧P连线按用户明确反馈接受；本轮没有逐字段解析二进制BP或操控编辑器。运行View可正常创建并响应按钮提供了额外行为证据。

[AssetManifest](../AssetManifest.csv)新增原创失败Widget，更新两份既有BP的实际用途 / 保存记录 / SHA-256，**清单44→45**；其他**42项既有资产哈希不变**。45个路径唯一，全部磁盘哈希与清单一致。菜单BP / 地图 / 启动和Cook配置没有本批必需变化，未强行再次保存。

## 4. 已恢复提示与证据边界

- **A菜单空HUD Warning已关闭：**Menu GameMode实际增加空InitializeHUDForPlayer_Implementation且不调用Super；验收窗口八轮Menu ready，空类Spawn Warning为0。
- **Designer施工期间提示：**14:32–14:33存在尚未搭完三BindWidget的编译Error；Widget最后保存14:42:51，后续验收窗口Blueprint Error / Accessed None / Fatal / 项目LogET Error为0。不能把整份编辑器日志称为“从未报错”。
- **一次Class配置缺失已恢复：**15:03:23.752 Missing concrete FailureWidgetClass，死亡未能显示页；Controller15:03:46正常保存后，15:03:55失败页正常显示，随后None地图预检 / Retry和恢复后的MainMenu均成功。将其记录为已恢复配置过程，不把这条Warning伪称为预期None地图提示。

鼠标 / Enter / Space的具体触发、无补跳 / 卡键、P死亡拒绝、最终基础输入与“失败页显示时工具栏Stop→重开100 / 无旧页”按用户完整反馈登记。保留日志最后一轮是失败→返回菜单→退出，并未单独留下该Stop子用例；不虚构该样本，也不因没有矛盾的记录缺口要求重跑整套检查。

None地图是请求前校验，不是引擎真正TravelFailure实机结果。真正加载失败、当前阶段Cook / 独立包、性能测量和检查点恢复不属于本批已测结论；M1-07包仍待对应阶段。没有据源码一致宣称已自动化覆盖所有生命周期。

## 5. 工时、Push与交接

本批**手动80 / 等待5分钟**，检查10分钟，使用正式按钮 / 既有4入口，无新Controller临时验收图。M1-06 A+B共**手动125 / 等待10分钟**；项目累计**手动1646分钟（27小时26分钟）/ 等待130分钟**。不把助手复核计为用户工时，不按单样本推断精确节省率。

当前HEAD与缓存origin/main仍`d5eb9aa`（A提交），本轮未联网核验远端；B功能通过但尚未Commit。**六个新Source当前均为AM：暂存区仅有类骨架，完整实现位于工作区，必须在CLion重新Stage最新修改再Commit。**按[B第14节Push清单](../M1_06B_FailureAndRetry.md#14-本批需要commit--push的内容固定放在文档最后)提交13份Source、三份资产、本报告 / 指南 / 来源清单 / 状态文档；已有个人设置取消跟踪删除可一并提交，磁盘文件保留。Raw / 缓存 / 构建产物继续忽略，助手没有操作暂存区。

下一开发批为C会话检查点与“继续本次试炼”，由用户明确进入后下发；验收当轮完成B收尾，不追加开发或重复旧战斗 / HUD矩阵。

## 6. C下发时的交接更新（2026-10-11）

上述第5节保留验收当时的Git状态。后续已核对HEAD / 缓存origin/main为4b83b66，B完整实现和个人设置取消跟踪删除均已提交，旧AM骨架已关闭；本轮未联网核验远端。报告本身仍未跟踪，已列入[C第13节](../M1_06C_SessionCheckpoints.md#13-本批需要commit--push的内容固定放在文档最后)。

65Source中仅ETRunSessionSubsystem.cpp因换行字节调整与B原哈希不同，其完整功能代码与B指南一致；45资产保持验收清单哈希。保留原B实测快照，并为C下发另记当前基线，不覆盖历史证据。C下发当时尚无实际Source / 资产变化、构建或PIE结果；B结论与当时累计1646 / 等待130分钟不变。

2026-10-12交接：B报告已随3351b53提交，旧未跟踪提醒关闭；[C报告](M1_06C_Verification.md)确认67Source / 33cpp / 清单46、检查点重试 / 菜单继续 / 开始重置及缺失ID修正。当前累计1731 / 等待135分钟，本轮只追加C验收收尾文档，不重新验收B。
