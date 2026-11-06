# M3-05B4-02：共享画质设置界面验收记录

2026-11-05 23:16完成，2026-11-06复核。**验收通过，无待补，进入[M3-05C1音量偏好](../M3_05C1_AudioPreferences.md)。** 画质卡已实际接入主菜单 / Pause；不把后续音频路由、完整设置或M3打包记为完成。

## 1. 源码、构建与资产

- 五新 / 四旧UI与本批候选去空白后全部一致。本地`1150a6c`已含九源码及三Widget；复核起点Git工作区干净。助手本轮只写文档 / 本地候选和证据，不Stage / Commit / Push，未核验远端。
- 实际**222 Source / 126cpp（Core85 / UI38 / Editor3）、39自动化、302 Content / 来源83**。Editor / Game receipt时间22:44:28 / 22:46:51；双Build与三索引按用户反馈通过。
- Editor日志23:09:27.314，`EmberTrial.UI.Settings.GraphicsDraft` Result=Success。新WBP 23:09:11.196、主菜单23:09:05.816、Pause23:09:02.965有最终Valid记录；用户确认八绑定 / 宿主七绑定、无Binding / Never、三资产正常保存。
- 三资产实物SHA与当前HEAD的Git LFS OID一致。来源新增真实新卡一行、只更新两个宿主，其他80行不改，83项SHA均与实物匹配。源码 / Config / 非本章资产没有助手改动。

## 2. 独立窗口与证据边界

本地原证据保存在`E:/UE_Project/Demo/Artifacts/M3_05B4_02/Acceptance/`，含Editor.log、GraphicsUI01 / 02.log、专用ini与Summary.json；不上传原日志 / 个人配置。以下时间为Asia/Shanghai，UE日志UTC时间已加8小时。

| 项目 | 核对结果 |
| --- | --- |
| 两轮启动 | 均为-game，使用同一GraphicsUIUser.ini、独立abslog；不是旧M2包 |
| 真实控件应用 | 第一轮23:11:23.158，Core应用Quality0 / Cap30 / VSync1；23:11:46.313改为Quality1 / Cap60 / VSync0。Core / View各一条是同一调用的分层记录，不计两次保存 |
| 浏览、默认与取消 | 用户确认浏览无副作用、取消恢复、默认只改草稿；最终中 / 60 / 同步关 |
| 显示预览隔离 | 23:13:34.636开始Serial1、1280×800，World暂停；23:13:46.562–.565取消回1280×720。期间无额外画质应用；用户确认画质控件禁用且保留草稿，取消后恢复 |
| 页面与焦点 | 两ScrollBox滚动、Tab / ShiftTab / 方向键 / Space / Enter、展开Esc与草稿Esc、无误开始、Pause离开丢弃与新卡按用户实机反馈通过；本轮未额外录屏或重放键鼠，不把日志称作完整视觉证明 |
| 新进程只读 | 第二轮23:15:05.681，在此前无apply情况下输出Valid1 / Quality1 / Cap60 / VSync0，EffectiveCap60 / EffectiveVSync0；证明上一轮保存并重新读取 |
| 退出与短基线 | 两轮均有正常退出，最终引擎清理约23:14:19 / 23:15:12；PIE禁用、原P / 移动短查用户确认通过。两轮未发现Fatal / Assertion / Ensure / Accessed None |

ini实物支持最终1 / 60 / 0，但重启结论以第二轮只读查询为证。GPU实际FPS、完整旧能力 / 装备矩阵和当前M3独立包未在本批重测；旧M2_07A/Attempt01不含M3，留M3阶段集成。

## 3. 工时与下一步

本章代码70 + Designer40 + 阅读5 + 检查10 = **125分钟（2小时5分）**，构建启动等待**5分钟**。M3累计**1768分钟（29小时28分）/ 等待116**；项目累计**5179分钟（86小时19分）/ 明确等待440分钟（7小时20分）**。助手核对 / 文档工作不计用户实耗，M2仍1400 / 等待139。

下一批C1只完成总音量 / 音乐 / 音效三项合法值及本机保存，不冒充实际声音生效。C2接真实声音路由，C3做共享音量界面；三新 / 两旧Core当前只有指南 / 候选，未正式实施，不提前计入源码或测试数量。

## 4. Commit / Push（固定放在最后）

本地1150a6c已包含B4-02实现。当前Stage本报告、原指南收尾、Docs/AssetManifest.csv、新C1指南以及实际更新的Progress / Backlog / Architecture / 路线 / 验收与版本文档、Introduction / README；以CLion实际差异为准。没有新的生产源码或资产修改。

- 英文：`docs: verify graphics settings UI and prepare audio preferences`
- 专业中文：`docs: 完成画质设置界面验收并准备音量偏好功能`

Artifacts候选 / 证据 / 个人ini、生成缓存留本地。C1实施后的三新 / 两旧Core清单与Commit见新指南末尾。Stage后继续修改需重新Stage，Review → Commit / Push由用户操作，助手未核验远端。
