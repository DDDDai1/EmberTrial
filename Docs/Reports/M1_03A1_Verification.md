# M1-03A1：ASC 基础接入验收

日期：2026-10-03。结论：**A1 通过。** Character ASC / 接口、ActorInfo 刷新与结束清理完成；生命属性、GameplayReady 与 HUD 尚未实现。

## 用户实机反馈

用户约 **20:43** 完成，手动 **45 min**、构建 / 索引等待 **3 min**。插件 / 依赖 / 角色代码保存，正常 Editor / Game Build 与 CLion 导航通过；单个继承 ASC / PracticeAction、Owner / Avatar / Controller、Blueprint ASC 查询、空 Pawn 拒绝、三轮失去 / 恢复、重复键保护、三次 PIE 清理通过。临时节点 / ETTempASCPawn 已删除并保存，原动作 / E/P / 基础输入全部正常。

## 直接核对

| 项目 | 结果 |
|---|---|
| .uproject | GameplayAbilities 启用；两个项目 Runtime 模块与编辑器建模插件保留 |
| 核心依赖 | GameplayAbilities 公开依赖，Tags / Tasks 私有依赖；输入依赖 / 根目录私有路径保留，核心不含 UI / UMG |
| 角色 | 原类 / API 保持；IAbilitySystemInterface、单个原生 ASC、BeginPlay 检查、控制变化刷新、结束保护 / 取消 / Cue 清理 / ClearActorInfo 存在 |
| 源码范围 | Source 共 27 文件；与前一基线相比只改核心 Build.cs 和角色 .h/.cpp；其他 24 份保持相同 |
| Editor receipt | 20:24:24；UE 5.5.4 / Win64 / Development / Editor，包含 GAS 编译产物 |
| Game receipt | 20:26:03；UE 5.5.4 / Win64 / Development / Game |
| CLion | 编译数据库 13 个 cpp；GAS include 路径已入索引 |
| Controller 保存 | 20:43:36；SHA256 `7CB7BBDF77126D945745762743C60F1E6F42BFB84D6E87577132653E99F70285` |
| 玩家 Blueprint | 既有 BP_ThirdPersonCharacter；磁盘保存时间仍为 10/02 21:58:58，原生 ASC 由 C++ 父类新增，不需要另造 Blueprint |

未读取本次完整 UBT 动作日志，不虚构编译动作数 / 耗时；双目标成功按用户反馈与 receipt 登记。源码声明 / 实现已按本机接口审查，原输入逻辑保持。资产保存时间不独立证明临时图表节点已删除；删除按用户反馈记录。

## 日志覆盖

当前 `EmberTrial/Saved/Logs/EmberTrial.log` 直接保留两轮：

- **20:34:57–20:35:01**：BeginPlay 的 Mode=Refresh，Owner / Avatar 正确；Stop 前 Controller=None，随后 Widget / LocalPlayer 与 ActorInfo 清理。
- **20:42:54–20:43:24**：BeginPlay；20:42:59 / 20:43:12 查询有效 ASC；20:43:02→10、14→15、15→15、16→16、18→19 有五轮完整失去 / 恢复，Owner / Avatar / ASC 不变，Controller None / 恢复正确；Stop 清理。

计数：2 条 BeginPlay、5 条 PossessedBy、7 条 UnPossessed（包含 Stop 前解绑）、2 条 ActorInfo cleared、2 次 Blueprint ASC 查询，以及各 2 组 UI 初始化 / Widget 创建 / 清理。全部 ready 为 Mode=Refresh，符合引擎默认初始化后的刷新路径；每轮 cleared 后没有重新 ready。

未匹配到本批 Missing ASC / ActorInfo、Accessed None、蓝图 / 脚本错误、Fatal / Assert。**第三轮 PIE、空 Pawn 8 拒绝、重复 5 不执行、最终清理后的完整原玩法矩阵按用户反馈验收；现存日志不覆盖这些操作的全部细节。** 无需为补日志重复完整测试。

片段保留在 `Docs/Reports/Raw/M1_03A1_Acceptance_2026-10-03.log`。本轮助手只读证据、更新文档 / 清单，未修改工程代码 / 资产或代执行 Build / PIE。

## 工时与下一批

累计手动 **918 min（15 h 18 min）**，等待 **58 min**，由先前 873 / 55 加本次 45 / 3 得到。没有新增 UE 资产；来源清单更新 Controller 与既有玩家 Blueprint 的阶段 / 哈希，维持 **20 项**。

A2 分两小批：先按 [A2-01 生命属性 / GameplayReady](../M1_03A2_01_HealthReady.md) 手动实现属性、初始化与就绪契约；A2-02 再用真实 Gameplay Effect 验证修改、边界与初始化不重置。HUD 在 B 接入；A1 通过不表示 M1-03 总任务完成。
