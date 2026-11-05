# M3-05B4-01：原生画质与帧率偏好验收记录

2026-11-05，主体22:00、重启读取窄补验22:10完成。**本章验收通过，无待补，进入[B4-02共享画质设置卡](../M3_05B4_02_GraphicsSettingsUI.md)。**最终合法配置0 / 30 / 0，Graphics03已证明全新进程读取；保留前两轮曾缺查询的历史事实，不要求把合法配置改回模板值。

## 1. 源码、构建与专项

- 三新 / 两旧Core与候选去除空白后一致；本地`d20de4e`已包含实现，本轮起点工作区干净，助手未Stage / Commit / Push或联网核验远端。
- 实际**217 Source / 123cpp（Core85 / UI35 / Editor3）、38自动化、301 Content / 来源82**。资产与共享Config保持；原显示预览 / 输入偏好仍由同一GameUserSettings负责。
- Editor / Game receipt保存时间21:51:22 / 21:52:39，双Build / 三索引按用户反馈通过。
- Editor日志21:53:46.184，`EmberTrial.M3.Settings.GraphicsPreferences` Result=Success；21:53:57的PIE ApplyAccepted=0，快照3 / 0 / 0不变。不重复旧全组或Cook。

## 2. 两轮独立日志实际结果

| 检查 | 证据 |
| --- | --- |
| 启动与环境 | Graphics01 / 02均有-game、同一专用GraphicsUser.ini和各自abslog；输入偏好 / 窗口隔离及移动短查按用户反馈通过 |
| 低画质 / 帧率 | 第一轮21:55:18应用Quality0 / Cap30 / VSync0，Accepted1；查询有效CVar同为30 / 0 |
| 非法拒绝 | 21:55:37和21:55:43两次Accepted0，仍为0 / 30 / 0 |
| 暂停应用 | 21:55:54应用1 / 60 / 1；21:56:01保持画质关闭VSync后为1 / 60 / 0，有对应SaveRequested |
| 预览前额外应用 | **21:56:25又成功应用0 / 30 / 0**，这是最后一次成功提交；预览21:56:31才开始 |
| 预览期间拒绝 | 21:56:34.554发生在Serial1 / Phase2、Paused1期间，Accepted0；快照保持0 / 30 / 0。21:56:39取消恢复1280×720 / Result3，未见被拒绝设置补执行 |
| 文件与退出 | 专用ini十组画质0、FrameRateLimit30、VSync=False；第一轮正常退出21:56:50，第二轮正常退出21:59:43 |
| 第二轮读取 | Graphics02未找到查询输出，故当时保留待补验；现由Graphics03关闭，见第3节 |

两轮未发现项目Fatal、Ensure或Accessed None。Graphics01首次菜单另外出现Initial capture deferred→ready（21:54:59.406→.943），为B3首屏有限重试补充了新的独立进程日志证据；不冒充此前未另存的轮次。

## 3. 重启窄补验与工时

Graphics03.log命令行含-game、原GraphicsUser.ini与新abslog；22:09:51.724有唯一查询输出`Valid=1 Quality=0 Cap=30.00 VSync=0 EffectiveCap=30.00 EffectiveVSync=0`，没有先执行apply或SaveRequested。22:10:01.029正常退出，未见项目Fatal / Ensure / Accessed None。与用户22:10反馈一致，重启读取待补项关闭，无需重复旧矩阵。

主体代码50 / 阅读10 / 检查10，共70分钟 / 等待5；本次补验额外手动1分钟，本章共**71分钟 / 等待5**。M3累计**1643分钟（27小时23分）/ 等待111**；项目累计**5054分钟（84小时14分）/ 明确等待435分钟（7小时15分）**。助手工作不计用户实耗，B4-02预计不计实耗。

## 4. Commit / Push（固定放在最后）

B4-01五份Core实现已在本地`d20de4e`。当前Stage其验收报告 / 指南收尾、新B4-02指南与实际更新的共享状态文档；02五新 / 四旧UI及一新 / 两旧Widget尚未实施，不提前Stage候选、空报告或来源行。

- 英文文档建议：`docs: verify graphics preferences and prepare shared graphics settings UI`
- 专业中文：`docs: 完成画质偏好验收并准备共享画质设置界面`

02未来源码 / 资产提交建议见新指南末尾。Artifacts证据 / 个人ini、候选与缓存留本地。Stage后继续修改需重新Stage → Review → Commit / Push由用户操作，助手未核验远端。
