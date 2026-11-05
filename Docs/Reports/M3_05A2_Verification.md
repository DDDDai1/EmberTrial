# M3-05A2：暂停输入设置与草稿确认验收报告

2026-11-05，用户本次反馈完成时分14:08、全项通过、无布局或失败问题。**本章验收通过，无待补，进入[M3-05A3：主菜单复用输入设置卡](../M3_05A3_MainMenuInputSettings.md)。** A3只有手动指南 / 忽略候选，未改正式Source / 资产。

## 1. 实物与构建

实际**201 Source / 113cpp（Core78 / UI32 / Editor3）、34自动化、300 Content / 来源81**。五新UI / 两旧Pause与候选逐份比较，除空行 / 注释及include顺序外一致；七绑定、主Pause第五绑定和统一busy传递已落地。原Core、Controller / Flow及配置未改。

值Draft保留应用基线和候选，编辑 / 默认不写运行偏好；确认时重验外部当前值，页面移除后禁止迟到确认。View的程序同步有重入保护，明确应用才调用A1接口 / SaveSettings；无Tick / 属性Binding、ASC / 身体观察或额外暂停所有者。Enter / Space交给焦点控件，Esc只取消卡内草稿，P仍走原Pause释放路径。

**证据日期分开记录**：用户本次反馈为11月5日14:08；当前磁盘构建 / 保存 / 运行证据来自**11月4日晚**。不将旧日志时间改写成14:08，也不为日期差要求重复测试。Editor / Game receipt分别11月4日**22:22:42 / 22:24:29**；用户确认双Build与三索引成功。

WBP_ETInputSettings / WBP_PauseMenu实物保存分别11月4日**22:42:15 / 22:44:49**，两份最终均有本项目Valid；七 / 五绑定、无Binding / Never与键鼠按用户实机反馈通过，本次未重新操作Designer。两资产SHA与本地HEAD的LFS OID吻合，来源清单新增输入卡并更新Pause至81行。实现已在本地`df3a1cd`，审查起点工作区干净；未操作索引或核验远端。

## 2. 专项与运行证据

以下均为日志中的**2026-11-04北京时间**：

| 时间 | 实际记录 |
| --- | --- |
| **22:45:10.603** | `EmberTrial.UI.Settings.InputDraft` Result=Success |
| **22:45:26.399** | `EmberTrial.M3.Settings.InputPreferences` Result=Success |
| **22:49:02.062** | 实际卡片应用Sensitivity=.50 / InvertY=1，SaveRequested=1 |
| 22:49:28.310 / 22:49:35.160 | .25 / 3.00边界值实际应用 |
| 22:49:46.294、22:50:10.952、**22:50:46.586** | 多次恢复1.00 / 不反转；最后一条应用为默认值 |

当前Saved/Config/WindowsEditor/GameUserSettings.ini也已核对为1.000000 / False。未应用P丢弃、应用后实际手感、取消恢复已应用、默认为草稿、Tab / 方向键 / Enter-Space / Esc、P继续焦点、祭坛确认 / 普通P只读、Stop新卡与短基线按用户完整反馈通过。纯草稿专项不冒充真实UMG / 键盘或磁盘读取；SaveRequested也不被单独写成落盘成功证明。

**历史装配错误已关闭**：补第五绑定前日志出现InputSettingsPanel缺失，最后一条22:43:29；22:44:49最终保存 / Valid之后，专项与实机没有再出现项目蓝图Error、Ensure或Accessed None。不会把整份日志写成从未有错，也不要求重做已通过检查。

原日志副本保留在`Artifacts/M3_05A2/Acceptance/EmberTrial_SourceLog_20261104_2251.log`，验收摘要同目录。此次不重复Build、专项、A1重启、完整34项或Cook；旧M2包不含M3。

## 3. 工时与下一章

代码60 / Designer30 / 阅读配置5 / 检查5，共**100分钟**，等待5。M3累计**1105分钟（18小时25分）/ 等待73**；项目**4516分钟（75小时16分）/ 明确等待397分钟（6小时37分）**。助手审查不计用户实耗。

A3只在主菜单复用同一输入卡与busy，四旧UI文件 / 两旧Widget，数量保持；一项原InputDraft和5–10分钟菜单 / 游戏切换与键盘检查，无新测试或临时验收蓝图。显示实际应用 / 确认回退下一批继续，个人偏好与M5试炼存档仍分开。

## 4. Commit / Push（固定放在最后）

A2五新UI / 两旧Pause与两资产已在本地`df3a1cd`，审查起点工作区干净。当前Stage A2验收报告 / 指南收尾、来源清单、A3新指南与同步状态文档；A3正式Source / 资产尚未实施，不Stage忽略Artifacts内的候选。未变化源码 / 资产不重复Stage；未联网核验远端。

- 英文建议：`docs: verify pause input settings and prepare main menu reuse`
- 中文译文：`docs: 完成暂停输入设置验收并准备主菜单复用`

Stage / Commit / Push由用户操作；个人GameUserSettings.ini、Raw / 候选 / 日志 / 缓存 / 包留本地。
