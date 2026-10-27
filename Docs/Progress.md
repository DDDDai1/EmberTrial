# 当前开发进度

更新2026-10-27。[M2-07A新版包](Reports/M2_07A_Verification.md)与[B性能样本分析](Reports/M2_07B_Verification.md)已通过，当前进入[M2-07C技术复盘](M2_07C_TechnicalReview.md)。材料已生成，待用户阅读 / 简短反馈；整个M2尚未提前关闭。首次PIE等待需求保持关闭。

## 当前工程与独立包

- 157 Source / 86cpp，Core60 / UI23 / Editor3，23项自动化，来源75 / 本地294份UE资产。当前本地HEAD92ace0c；未联网核验远端。
- 最新功能基线为`Artifacts/Builds/M2_07A/Attempt01/Windows/EmberTrial.exe`，Build / Cook / Stage / Package / Archive成功，两轮独立运行通过。B核对157 Source / 294 Content / 6 Config及7份包文件指纹均与A一致，本轮不改运行工程。
- 已有四类主动战斗能力、五HUD、真实近战AI、菜单 / Pause / Failure、当前进程检查点、锁定 / 标记 / 右肩镜头及Z / C切换。双敌人成功切换沿用C编辑器 / Selection，A包只检查单敌人空侧保持，不扩充证据。
- 镜头Pitch-18 / MaxShoulderYaw12、Shoulder(0,75,55)cm / Focus45；臂长400与碰撞保留。生成前按实际胶囊贴地，未改保存的出生点 / Mesh / FootIK。

## M2-07证据与限制

A的23项阶段全组Success沿用10/25实际执行，不因文档收尾重跑。两轮包内日志支持三段各25、闪避成本 / 恢复、F归零 / 反应、Q消费、真实敌人10伤害、检查点Retry / Continue与新进程清空；中文 / 输入 / 取景由用户实机确认。

B用同包完成待机与真实AI交战采样。待机 / 交战内部主循环间隔P95约5.915 / 5.714ms；完整交战Region另有3次>16.7ms Tick、最大30.013ms，未唯一归因，不隐藏，也未发现需立即改工程的持续瓶颈。GPU场景范围不是完整GPU帧，主循环间隔不是呈现FPS。

请求1920×1080、Medium=1，PNG实际2048×1081；用户确认窗口未缩小 / 裁剪，电源 / 风扇档位不清楚。启动可用物理内存80.10MB只是单时点，不证明稳态峰值或尖峰根因。本轮不做严格1080p / Shipping达标声明，不要求重采。

## 当前只需进行C复盘

打开[C指南](M2_07C_TechnicalReview.md)，约15–25分钟对照攻击链、提交时点、生命周期、锁定分工及实际性能边界；没有新代码、Build、测试、视频或验收蓝图。只反馈不理解的节号和实际用时；有问题先解释，无问题后关闭M2并下发M3-01。

M3保留6遗物 / 2槽目标，先2件最小装备闭环，再列表 / 详情、祭坛页面与设置。静态数据、运行状态、跨World装备语义先设计；当前不宣称已有Inventory或磁盘存档。最终简历 / 整体作品复盘仍在M8共同完成。

## 工时与协作

- B用户手动30 / 等待10；M2-07累计60 / 20，M2已量化1400 / 139分钟，项目3411分钟（56小时51分）/ 明确等待324分钟（5小时24分），另有M2-06C未登记。C预计阅读时间不计实耗，助手分析 / 整理不计用户工时。
- 普通C++由用户手动；复杂排错按当次授权。方案B保持小批查影响、阶段全组一次，避免重搭旧验收入口。
- 每周30小时 / 百元预算 / 键鼠 / 无硬期限保持；Blender重要模型安排在中后期，当前仍为模板 / Control Rig占位。

## 当前批次 Commit / Push（固定放在最后）

[M2-07B分析报告](Reports/M2_07B_Verification.md)已保存，当前进入[M2-07C技术复盘](M2_07C_TechnicalReview.md)，待用户阅读 / 简短反馈。本轮仅两份新文档与11份当前入口同步，精确13文件清单见[C末尾](M2_07C_TechnicalReview.md#10-本批-commit--push固定放在最后)。建议英文`docs: analyze M2 performance and prepare the technical review`，中文译文`docs: 分析M2性能并准备阶段技术复盘`。

Source / Content / Config / AssetManifest / Tools与原包保持，Raw / Trace / CSV / 截图 / 日志留本地。CLion刷新后Stage最新文本、Review Diff、Commit / Push由用户操作；助手未改索引或联网核验远端。
