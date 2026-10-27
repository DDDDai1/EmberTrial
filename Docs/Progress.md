# 当前开发进度

更新2026-10-25。**[M2-07A新版包集成验收通过](Reports/M2_07A_Verification.md)，当前进入[M2-07B必要性能窗口](M2_07B_PerformanceCapture.md)，待用户采集。**首次PIE创建等待需求已关闭，不再优化 / 计时 / 验收。

## 当前实际工程与可运行基线

- 157 Source / 86cpp，Core60 / UI23 / Editor3，23项自动化，来源75 / 本地294份UE资产。本地HEAD183b80c含A相关实现 / 工具与下发材料，当前未联网核验远端。
- 最新功能基线为`Artifacts/Builds/M2_07A/Attempt01/Windows/EmberTrial.exe`；实际Game二进制与工程Binaries一致，UAT Build / Cook / Stage / Package / Archive全成功，ExitCode0。旧Cleanup/Attempt01和M1包只留历史证据。
- 四能力 / 五HUD、真实近战AI、菜单 / Pause / Failure / 进程内检查点、锁定 / 标记 / 右肩镜头及Z-C空候选保持已在新包串联检查。包内双敌人成功切换正例沿用C编辑器 / Selection，不冒充新增样本。
- 镜头Pitch-18 / MaxShoulderYaw12、Shoulder(0,75,55)cm / Focus45、原臂长400与碰撞保留；出生贴地按实际胶囊处理，原出生点 / Mesh / FootIK保持。

## A已完成证据

2026-10-25 18:38:10北京时间阶段全组23成功 / 0警告 / 0失败 / 0未执行，不要求再跑。用户18:54报告A完成、手动30 / 等待10、无失败；UAT工具段117.75秒与用户等待口径分开。两轮运行18:49:29–18:52:52、18:53:01–18:53:17，完整Runtime无实际Error / Warning / Fatal。

日志含三段各25、真实方向闪避 / 体力消费恢复、F两次Poise100→40→0、Q消费后2/3、Enemy真实10伤害、Z / C两方向NoCandidate及同一当前目标、Checkpoint_A激活 / Retry / Continue、新进程Continue=0与正常退出。中文 / 输入 / 取景 / 无持续陷地由用户实机确认，详细证据 / 覆盖限制见报告。

## B本次下发边界

只用A已验证包采稳定待机与真实AI交战两段：1920×1080请求、Medium=1、未限帧、ScreenPercentage100 / 动态分辨率0；一张引擎截图辅助确认视口，两个Region清楚命名。用户负责采集 / 打开正确文件，助手负责有效窗、导出与分析，不要求手抄性能统计。

脚本语法与WhatIf已验证，M2输出路由独立、M1原High=2行为保持；**B实际Trace / 运行 / FPS结论尚未产生**。不重Build / Cook、23项或A功能矩阵，不追加已关闭的PIE等待；完成分析后进入C技术复盘。

## 工时与协作

- A实际手动30 / 等待10；M2-07累计30 / 10，M2已量化1370 / 129分钟，项目3381分钟（56小时21分）/ 明确等待314分钟（5小时14分），另有M2-06C未登记。B预计20–35分钟不计为实耗。
- 普通后续C++继续用户手动；本轮只改已有脚本和文档，Source / Content / Config保持。Stage / Commit / Push由用户操作。
- 方案B：小批只查受影响项，阶段全组只跑一次；每周30小时 / 百元预算 / 键鼠 / 暂无硬期限保持。

## 当前批次 Commit / Push（固定放在最后）

A已验收，最新功能包为M2_07A/Attempt01，实际[A报告](Reports/M2_07A_Verification.md)已保存，当前进入[M2-07B必要性能窗口](M2_07B_PerformanceCapture.md)，尚未采集 / 分析。精确本批文档与Tools范围见[B末尾](M2_07B_PerformanceCapture.md#9-本批-commit--push固定放在最后)。当前建议英文`docs: record M2 package acceptance and prepare profiling`，中文译文`docs: 记录M2开发包验收并准备性能采集`。用户自行Stage最新内容 / Review Diff / Commit / Push；Source / Content / Config / AssetManifest本轮保持，Raw / 包 / 日志 / JSON / 截图 / Trace本地保留，助手未改Git索引或联网核验远端。
