# 当前开发进度

更新2026-11-05。[M3-05B3-02已验收](Reports/M3_05B3_02_Verification.md)，21:29首屏窄补修通过，无待补；当前[B4-01画质 / 帧率上限 / VSync原生接口](M3_05B4_01_GraphicsPreferences.md)待手动。

## 当前实际工程

- **214 Source / 121cpp（Core83 / UI35 / Editor3）、37自动化、301 Content / 来源82**。B3原实现已在本地01d00a0，本次两个Widget补修尚未提交；Git由用户操作。
- 主体双Build / 两Success / 三Valid与真实预览 / 关闭回退、键鼠 / 视觉通过；首屏补修后双Build / 索引与全新进程自行就绪、草稿不被重试覆盖、Pause / 返回菜单按用户专项反馈通过。
- 本轮独立日志未另存，21:29编辑器日志不作为-game证据；报告区分已有日志和实机反馈，不补造Deferred→Ready时间线。有限CoreTicker仅捕获失败时运行，成功 / 超时 / 关页结束；原Never及PIE保护保留。
- B4-01当前只有指南 / 候选：三新 / 两旧Core，预计217 Source / 123cpp（Core85 / UI35 / Editor3）、38测试；不新增资产 / 来源 / UI。复用同一GameUserSettings，不新增设置单例或并行窗口事务。
- 画质 / 帧率 / VSync正式卡、音量、完整6遗物 / 压力、M5存档与最终美术后续；旧M2_07A/Attempt01不含M3。

## 工时与协作

B3-02主体155 / 等待5，首屏补修15 / 等待5，本章**170 / 等待10分钟**。M3累计**1572分钟（26小时12分）/ 等待106**；全项目**4983分钟（83小时3分）/ 明确等待430分钟（7小时10分）**。助手工作不计用户实耗；M2仍1400 / 等待139。

B4-01预算代码45–70 / 阅读5–10 / 检查8–12分钟，构建等待另记；只一GraphicsPreferences专项、PIE拒绝及专用-game真实应用 / 保护期间拒绝 / 重启读取。不搭临时蓝图、不重跑旧全组或Cook。普通C++用户手动，复杂排错按当次授权；首次PIE等待需求保持关闭。

## 当前批次 Commit / Push（固定放在最后）

B3-02原实现 / 三资产已在本地`01d00a0`。当前Stage实际修改的两个Widget补修源码、本章验收报告 / 指南、来源与共享状态，以及新B4-01指南。B4-01三新 / 两旧Core仍只有候选，不提前Stage；Artifacts运行日志、个人ini和候选留本地。

- 英文合并建议：`fix: initialize video settings after viewport readiness`
- 专业中文：`fix: 在显示窗口就绪后完成设置界面初始化`
- 英文独立文档建议：`docs: verify video settings UI and prepare graphics preferences`
- 专业中文：`docs: 完成显示设置界面验收并准备画质偏好接口`

B4-01未来五源码与提交建议见新指南末尾。Stage后继续修改需重新Stage → Review → Commit / Push由用户操作，助手未核验远端。
