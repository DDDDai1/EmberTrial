# 当前开发状态

更新日期：2026-09-24。已复核 M0-05B 补修资产，C1 操作指南已下发。

## 当前阶段

**M0-01 / M0-02 / M0-03 / M0-04 / M0-05A 已完成。M0-05B 首轮 80 分钟 + 补修 20 分钟，六项差异已解决五项，只剩闪避 0 帧左上臂 Z。当前下发 C1：先按 §0 收尾这一项，再直接做 Montage / Slot 与按键播放。B 尚未完全通过，C1 待用户操作；M0-05 总任务及 M0 整体未完成。**

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
23. 2026-09-21 观察到 L_M0_AnimationLab、LS_M0_Attack_01、CR_Mannequin_Body 与 Anim Details；用户提出需要具体姿势数值。新增 M0_05A_AttackPoseValues.md，包含五帧六个 FK 控制器的完整 L/R/S。独立引擎临时 Rig 求值与 31 帧线性采样通过；没有给原 Sequence 打 key、保存或烘焙，蒙皮外观及播放仍待用户确认。
24. 用户随后确认数值姿势检查通过：0 / 12 帧正常，五帧均已打 key，两个 FK/IK 开关为 False，Linear 已设置，连续播放正常。将姿势制作与 Sequencer 预览记为用户验收通过；资产保存、动画烘焙、独立播放、root 漂移、玩家绑定和实际用时仍待反馈。
25. 用户约 80 分钟完成 M0-05A，确认 Control Rig、关键帧、独立播放、无 root 漂移、保存及回原场景基础输入 / E / P 均正常。独立 UE API 检查确认 SKM_Quinn_Simple 与攻击动画共用 SK_Mannequin，Anim Class 为 ABP_Quinn_C，ABP_Quinn 的父类为 ABP_Manny_C；动画为 31 个帧间隔 / 32 个采样点 / 1.0333333 秒，Root Motion=False，root 采样不漂移。A 验收完成。
26. 下发 M0-05B：新建两个 Sequence，使用双臂 FK 与 spine_01_ctrl 完成原地右侧闪避和受击后仰，之后烘焙并在 Manny / Quinn_Simple 上预览。参数已通过独立临时 Rig 求值；下发时用户尚未实施 B。
27. 用户报告 M0-05B 两段五帧预览、独立播放、无 root / 脚漂移、Quinn_Simple、原攻击和基础输入 / E / P 均正常，已保存；闪避 40 分钟、受击 40 分钟。Skeleton 未填写，帧数填写 20 / 18。
28. 独立 UE API 核对：实际受击资产名为 AS_M0_HitResult_01，制作源为 LS_M0_HitResult_02；两段和玩家共用 SK_Mannequin。两段实际均为 30 fps、31 个帧间隔 / 32 个采样点 / 1.033 秒，Sequence End 均为 31；用户填写的 20 / 18 对应最后姿势 key 的位置，不能据此记为烘焙帧数。
29. 对比关键帧发现四个标量差异：两段 0 帧左上臂 Z=15，应为 25；受击 7 / 12 帧 spine_01_ctrl 的 X 均为 -12，应为 -7 / +3。两段 root / pelvis / 双脚位置采样均固定；左手起止相差约 5.27 cm。B 暂记待补修复验，已下发约 10–15 分钟的局部补修指南；本轮未改写 UE 资产。
30. 用户补修约 20 分钟，报告全部修正、重烘焙、外观和基础输入回归正常；AS 沿用 _01。独立 UE 复验确认闪避 21 帧间隔 / 22 采样点 / 0.700 秒，受击 19 / 20 / 0.633 秒，均为 30 fps、相同骨架、无 root / 脚位置漂移；受击的三处 key 已正确，左手首尾位置一致。
31. 闪避保存后的 0 帧 upperarm_l_fk_ctrl.Rotation.Z 仍是 15，末帧为 25，AS 左手首尾仍差约 5.27 cm；review_findings 剩 1 项，未误记 B 全部通过。当前受击实际源已变为 LS_M0_HitResult_01，旧 _02 加载后指向 _01；更新指南避免沿用过期路径说明。
32. C1 前置只读盘点确认玩家仍为 SKM_Quinn_Simple / ABP_Quinn_C，IMC_Default 正在使用；ABP_Manny 已有 DefaultSlot，Always Update Source Pose=False，ABP_Quinn 无独立 AnimGraph。下发 C1 细化指南：先关闭重开确认闪避最后一项，再复用已有 Slot 创建三段 Montage 与 1/2/3 调试播放。助手未修改 UE 资产，C1 实机待用户实施。

详细证据见 [M0-05B 验收与补修复验](Reports/M0_05B_Verification.md)。此前的 [B 数值校验](Reports/M0_05B_PoseValuesVerification.md) 只针对临时 Rig 参数，不能替代用户制作资产的复核。区分用户实机反馈、我直接读取的数据与尚未实测的方案。

## 现在由用户执行

执行 [M0-05C1：Montage 与角色播放](M0_05C1_MontagePlayback.md)，预计 45–70 分钟，无需 C++ Build：

1. 先按 §0 更新闪避 0 帧左上臂 Z=25，明确打 key，保存并关闭重开确认，再重烘焙；受击无需重复制作。若 key 不保留，反馈后由助手排障。
2. 直接继续创建三个 Montage，在 ABP_Manny 复用现有 DefaultSlot 并开启 Always Update Source Pose。
3. 在已有 IMC_Default 加入 1/2/3 的三个 Started 输入，共用 TryPlayPracticeMontage。站定落地才接受请求，用 Is Any Montage Playing 防止重复启动。
4. 完成单次播放、长按 / 忙碌拒绝、暂停恢复、E / P 和三次 PIE 验收，再按指南末尾模板反馈。当前闪避仍为原地姿势，实际位移留到 C2。

## 下一轮我优先做什么

- 同时复核 B 最后一个 key / AS 首尾，以及 C1 的 Montage、Slot、输入接线和播放日志；用户若导出新 AS 版本，明确引用关系。
- 若报错，读取本地文件和日志直接排查；需要蓝图上下文时再请用户补图。
- B 与 C1 通过后进入 C2 动作状态 / CharacterMovement 位移，再完成 C3 回归 / 录制；首次打包仍属于 M0-06。

## 仍待确认

- 每周可投入时间、期望投递日期、是否有手柄：已询问，尚未收到答案。
- 日志报告约 15554 MB 可用物理内存总量；可接受资源预算仍待补充。
- M0-05B：待闪避 0 帧左上臂 Z=25 与重烘焙首尾复验。M0-04 约 190 分钟，M0-05A 约 80 分钟，M0-05B 约 100 分钟，三批已报告手动用时合计 370 分钟；C1、助手排障、参数校准和此前未记录投入另计，不直接外推整个 Demo 工期。

## 持续协作原则

基础 C++ / 蓝图与创建工程 / 类等任务由用户先做；我提供明确步骤并审查。复杂实现、编译 / 链接 / 打包 / 崩溃、跨系统或难以描述的问题由我主动读取工程与日志接手。双方不要同时修改同一批文件。需要用户手动操作时，写明操作、预期与验证；没有执行的检查不标为通过。

报错材料遵循“文字日志优先，连线 / 布局用截图，时序 / 动画用短视频”；用户无须先分析根因。重要模型中后期再通过 Blender 设计。
