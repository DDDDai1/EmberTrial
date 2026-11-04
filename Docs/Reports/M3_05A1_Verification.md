# M3-05A1：输入偏好与本地保存验收报告

2026-11-04，用户21:50报告完成、无失败或手感问题。**本章验收通过，无待补，进入[M3-05A2：暂停页输入设置与草稿确认](../M3_05A2_InputSettingsUI.md)。** A2当前只有手动指南 / 忽略候选，未修改正式Source / 资产。

## 1. 实物与构建

实际**196 Source / 110cpp（Core78 / UI29 / Editor3）、33自动化、299 Content / 来源80**。三新Core与旧Character.cpp逐份去除空行 / 注释及include顺序差异后与A1候选一致；DefaultEngine.ini只有一处正确GameUserSettingsClassName配置。Look只做轴倍率 / 反转，默认1 / 不反转；锁定和暂停仍走原Controller门控，无新Tick / Timer。

Editor / Game receipt分别**21:43:12 / 21:44:46**，用户确认双Build / 三索引成功。实现已在本地提交`9166313`，审查起点工作区干净；本次未改变Git索引或核验远端。

所有80条来源资产本地SHA保持一致，本章未改Content，不要求重存或改来源清单。个人设置文件在Saved，继续被gitignore覆盖，不复制到共享Config。

## 2. 专项、运行与磁盘读取证据

以下为北京时间2026-11-04；测试与首次应用来自重启前备份日志，恢复 / 默认来自新启动后的EmberTrial.log：

| 时间 | 实际记录 |
| --- | --- |
| **21:45:53.826** | `EmberTrial.M3.Settings.InputPreferences` Result=Success |
| **21:46:09.846** | `EmberTrial.M2.TargetLock.CameraRules` Result=Success |
| 21:46:21.093 | 正确原生Class，初始Sensitivity=1.00 / InvertY=0 |
| 21:46:35.915 | 应用0.50 / 1并Save requested |
| 21:46:50.436 | 非法配置Rejected，当前值不变 |
| **21:48:30.016** | 新启动日志查询仍为0.50 / 1，与用户完整关闭重开UE反馈一致 |
| **21:48:45.409** | 输入defaults后查询回1.00 / 0并请求保存 |

输入专项使用独立对象，不Save / Load个人配置；配置类选择、值副本、真正应用、非法拒绝不改旧值、边界 / NaN / Inf与轴变换通过。CameraRules回归原镜头算法，二者不冒充真实鼠标或磁盘测试。自由镜头实际变慢 / 反转、自由-锁定-解除、P / 祭坛E和最短旧能力检查按用户实机反馈通过。

**路径说明已修正文档**：日志`Ini=GameUserSettings`在本机是逻辑配置名，不是完整物理路径。已直接核对`EmberTrial/Saved/Config/WindowsEditor/GameUserSettings.ini`的`[/Script/EmberTrial.ETGameUserSettings]`，最终`InputPreferences=(MouseSensitivity=1.000000,bInvertMouseY=False)`。这不影响保存功能，无需为日志文字修改源码 / 重新构建。

两份相关日志未发现项目蓝图Error、Ensure或Accessed None。保存副本在`Artifacts/M3_05A1/Acceptance`的BeforeRestart / AfterRestart两文件，摘要同目录；不重复构建 / 自动化 / 重启或新Cook。Settings是机器上的个人偏好，不代表M5试炼磁盘存档已完成；旧M2包不含本章。

## 3. 工时与下一章

代码40 / 阅读配置10 / 检查10，共**60分钟**，构建与重启等待5。M3累计**1005分钟（16小时45分）/ 等待68**；项目**4416分钟（73小时36分）/ 明确等待392分钟（6小时32分）**。助手审查不计用户用时。

A2将输入设置卡嵌入现有Pause左列，草稿 / 应用 / 取消 / 仅输入默认和键盘路径复用同一Flow；五新UI / 两旧Pause、一新Widget与原Pause，Core / Config保持。只跑新InputDraft与原InputPreferences各一次，实机5–10分钟，不重复A1完整重启或旧矩阵。

## 4. Commit / Push（固定放在最后）

A1三新Core / 旧Character.cpp / DefaultEngine.ini已在本地`9166313`，审查起点工作区干净。当前Stage A1验收报告 / 指南收尾、A2新指南与同步状态文档；A2正式Source / Widget尚未实施，不Stage忽略Artifacts内的候选。未变化源码 / 资产不重复Stage；未联网核验远端。

- 英文建议：`docs: verify input preferences and prepare settings UI`
- 中文译文：`docs: 完成输入偏好验收并准备设置界面`

Stage / Commit / Push由用户操作；个人GameUserSettings.ini、Raw / 候选 / 日志 / 缓存 / 包留本地。
