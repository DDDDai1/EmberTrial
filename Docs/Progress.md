# 当前开发状态

更新日期：2026-09-20。

## 当前阶段

**M0-01 / M0-02 / M0-03 / M0-04 已完成。当前进入 M0-05 动画四动作试验，下发 M0-05A：零成本动画路线与第一段攻击占位，等待用户手动实施。M0 整体尚未完成。**

- 工作区：`E:\UE_Project\Demo`。
- 实际 UE 工程：`E:\UE_Project\Demo\EmberTrial\EmberTrial.uproject`，已存在。
- 引擎：UE 5.5.4，`D:\UE5\UE_5.5`。
- 5.5.4 构建成功，基础交互已由用户确认，最新运行日志核对完成；打包尚未执行。

## 截至当前已完成

1. 阅读并更新 Introduction 至 v1.1，加入用户手动实践 / 复杂问题接管原则。
2. 将重要模型的 Blender 协作设计列入中后期资源路线，保留 M0 占位资源方案。
3. 核实引擎与 Third Person C++ 模板、VS 2022、MSVC、Windows SDK、Git / LFS。
4. 核实 CPU 为 Ryzen 7 7840H；5.5.4 最新运行日志确认 D3D12 选择 RTX 4060 Laptop GPU。
5. 读取 UBT 用户配置，确认没有显式工具链覆盖；未修改全局配置。
6. 建立 M0 操作指南、问题提交模板与任务板。
7. 核对用户创建的 5.4.4 工程，保存本地迁移前检查点 `3562f61`，包括源码 / 配置及 212 个 LFS 资产；完整性检查通过。
8. 将项目关联和两个 Target 对齐到 5.5，重新生成解决方案并通过 5.5.4 正常 C++ 构建（59.18 秒）。
9. 新构建使用 MSVC 14.38.33145 / Windows SDK 10.0.22621.0；receipt 确认为 5.5.4 / Win64 / Development / Editor。
10. 迁移后检查点为 `861be2b`；用户回复“5.5.4 复验正常”，最新日志确认新版本与 ThirdPersonMap 的 PIE 启停。
11. 用户手动完成 M0-03，实际约 15 分钟。源码符合组件 / 反射 / Tick 契约，运行日志确认两个实例每次各输出一次；A 改为 Target_Changed 后独立生效，基础输入正常。
12. M0-03 蓝图子类和两个外部 Actor 文件均已落盘，随后下发 M0-04A 指南。
13. 用户手动实现 M0-04A，首次报告约 60 分钟：C++ Build / 蓝图编译通过，范围切换 / 回退和基础输入正常，连续三次 Play 未见 Accessed None；当时离开隐藏、出生重叠和销毁清理失败。GetPracticeLabel、接口和 Widget 资产现已存在。
14. 此前已接管编辑器排障，确认并修正 RefreshCandidate 中 HideTarget 的错误分支连接；用户随后确认离开隐藏与销毁清理问题已解决。
15. 用户完成第 6.4 节初始范围同步，约 10 分钟：蓝图编译、出生 A / B / 范围外 / 双范围、切换 / 离开隐藏、出生重叠后销毁清理、基础输入全部正常，已恢复临时测试设置并保存。M0-04A 按用户实机结果验收通过。
16. 核对最新日志：UE 5.5.4，5 次 PIE 启动记录，BP_ETPracticeActor 的保存记录；未匹配到 Blueprint Runtime Error / Accessed None / 蓝图脚本错误 / Fatal error。日志不替代对画面与交互结果的用户验收。
17. 下发 M0-04B 手动指南后，用户约 40 分钟完成输入、接口交互、一次性状态与 UI 更新；最新确认全部验收项通过，临时测试设置已恢复、资产已保存。
18. 本轮日志确认 A / B 激活记录及新接口 / IA_Interact / IMC / 蓝图保存。日志保留一条中途 Widget Target 未连接的编译错误，后续用户编译及实机结果通过；未检出 Blueprint Runtime Error / Accessed None / LogScript 错误 / Fatal error。详细证据见 B 验收报告。
19. 用户约 80 分钟完成 M0-04C，确认暂停、鼠标 / 键盘继续、输入阻断 / 恢复、焦点、无卡键、10 次开关 / 3 次 PIE、A / B 回归全部通过并保存。M0-04 总任务验收完成。
20. 核对最新资产及日志：最终角色保存后观察到 9 组完整 opened / Resume requested / closed，最终片段无匹配的蓝图错误；全日志有两条已解决的中途编译错误。详细区分用户验收与日志样本，见 C 报告。
21. 用户确认没有现成攻击 / 闪避 / 受击动画，选择零成本占位。已盘点现有 Manny / Quinn、移动动画及 CR_Mannequin_Body；尚未读取当前玩家实际 Mesh / Skeleton / Anim Class 绑定。
22. 下发 M0-05A：独立动画试场、模板 Control Rig、第一段原地挥击和 Animation Sequence 烘焙。后续再补闪避 / 受击与 Montage 接入，本轮未制作动画资产。

详细证据见 `Docs/Reports/M0_01_Verification.md`、`Docs/Reports/M0_EngineMigration.md`、`Docs/Reports/M0_01_UE55_Runtime.md`、`Docs/Reports/M0_03_Verification.md`、`Docs/Reports/M0_04A_Debugging.md`、`Docs/Reports/M0_04A_Verification.md`、`Docs/Reports/M0_04B_Verification.md` 和 `Docs/Reports/M0_04C_Verification.md`。区分用户实机反馈、我直接观察的排障结果与尚未实测的方案。

## 现在由用户执行

按 [M0-05A：零成本动画路线与第一段攻击占位](M0_05A_AnimationSource.md) 操作，预计 60–120 分钟。本批无需改 C++ 或进行 C++ Build：

1. 记录当前玩家 Mesh、Skeleton、Anim Class 和 AnimBP Parent，保持这些绑定不变。
2. 建立 L_M0_AnimationLab 和 LS_M0_Attack_01，用 SKM_Manny + CR_Mannequin_Body 制作原地挥击占位。
3. 烘焙 AS_M0_Attack_01，在关闭 Sequencer 后独立播放，检查时长、Skeleton 和 root 漂移。
4. 保存制作源和输出，回 ThirdPersonMap 快速验证基础输入、E、P 没有变化。
5. 按文档反馈结果 / 用时；动画时序用短视频，控制器或菜单用截图，报错用文字。复杂 Rig / 烘焙问题由我接管。

## 下一轮我优先做什么

- 核对实际骨架 / Anim Class、第一段动画独立播放结果、root 是否漂移和制作成本。
- 若报错，读取本地文件和日志直接排查；需要蓝图上下文时再请用户补图。
- A 通过后推进 M0-05B 闪避 / 受击占位，再做 C 的 Montage / Slot 与位移实机试验；首次打包仍属于 M0-06。

## 仍待确认

- 每周可投入时间、期望投递日期、是否有手柄：已询问，尚未收到答案。
- 日志报告约 15554 MB 可用物理内存总量；可接受资源预算仍待补充。
- M0-05A：目前只有操作指南，未实施或测试。M0-04 已报告手动用时约 190 分钟（60 + 10 + 40 + 80），未计入此前未记录的协助排障；不直接外推整个 Demo 工期。

## 持续协作原则

基础 C++ / 蓝图与创建工程 / 类等任务由用户先做；我提供明确步骤并审查。复杂实现、编译 / 链接 / 打包 / 崩溃、跨系统或难以描述的问题由我主动读取工程与日志接手。双方不要同时修改同一批文件。需要用户手动操作时，写明操作、预期与验证；没有执行的检查不标为通过。

报错材料遵循“文字日志优先，连线 / 布局用截图，时序 / 动画用短视频”；用户无须先分析根因。重要模型中后期再通过 Blender 设计。
