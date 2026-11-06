# M3-05C2：实际声音路由验收记录

2026-11-06 19:45完成。**验收通过，无待补，进入[C3共用音量设置卡](../M3_05C3_AudioSettingsUI.md)。** 实际音乐 / 音效独立静音、总音量与持久化恢复已由用户试听确认；目前声音仍为原创技术占位，正式配乐 / 战斗3D音效后续。

## 1. 源码、资产与构建

- 三新 / 两旧Core与DefaultGame.ini均与候选去空白后相同。本地1a2a81b包含生产修改、六UAsset和两源WAV；本轮起点仅C2指南有上次授权编辑器操作记录的未提交修改，已保留。
- 实际**228 Source / 130cpp（Core89 / UI38 / Editor3）、41自动化、308 Content / 来源91**。八声音资源实物SHA全部匹配HEAD LFS OID；来源新增八行，原83行与SHA保持。没有助手新增生产源码 / Config改动。
- Editor / Game receipt时间19:22:03 / 19:23:39；双Build与三索引用户确认通过。EditorAutomation.log（原备份2026.11.06-11.24.56）19:24:46.932，EmberTrial.M3.Settings.AudioRouting Result=Success。
- 上轮助手通过UE设置三Class父子、Mix参数及Wave分流并正常保存；本轮运行RoutingApplied1与ProbeStarted1通过实际配置检查。PIE拒绝、原P / 移动短查按用户反馈通过，本次编辑器日志没有相应命令片段，不补造日志证明。

## 2. 两轮原生日志与听感

原证据存E:/UE_Project/Demo/Artifacts/M3_05C2/Acceptance，含两Runtime日志、专用ini、EditorAutomation / EditorAssetWork.log与Summary.json。表内时间为Asia/Shanghai，UE UTC已加8小时。

| 检查 | 证据与结论 |
| --- | --- |
| 首屏 | 第一轮19:39:43.754，主菜单自动RoutingApplied1、三项1；不是先进试炼才初始化 |
| 三路分流 | 19:41:06音乐0、19:41:20音效0、19:41:34总0；各组后均有两路Probe Started1。实际“只静音乐 / 只静音效 / 两路静音”由用户试听确认 |
| 总音量 | 19:41:44应用0.5 / 1 / 1，19:41:59回1 / 1 / 1；用户确认变小 / 恢复。未采声压，不宣称精确一半感知响度 |
| 非法拒绝 | 19:42:16.596 ApplyAccepted0，快照仍1 / 1 / 1 |
| 最终与暂停 | 19:42:29.136保存0.6 / 0.3 / 0.8，随后两路Started1；暂停试听与继续无卡键按用户反馈通过 |
| 切图 | 第一轮菜单→正式地图→菜单，三个World的ready / released各三条。19:42:47释放正式地图、同秒新菜单应用最终组；未采额外的第二次进入地图，不将其当作独立日志样本 |
| 新进程 | 第二轮19:43:45.599菜单自动恢复最终组；19:43:49.227只读查询0.6 / 0.3 / 0.8、RoutingApplied1，此前没有apply；19:43:57 / 19:44:03两路探针Started1，用户确认实际声音 |
| 结束 | 两轮约19:43:09 / 19:44:05正常退出且各释放最终World；未发现Fatal / Assertion / Ensure / Accessed None |

RoutingApplied / Started表示引擎请求就绪，不代替扬声器听感。本批已结合用户反馈关闭试听待项；未重跑战斗 / 遗物全组或Cook，旧M2_07A/Attempt01不含M3。

## 3. 工时与下一步

本章代码60 + 资产配置30 + 阅读10 + 检查10 = **110分钟（1小时50分）**，构建启动等待**5分钟**，按用户报告记入，助手操作不重复追加。

M3累计**1938分钟（32小时18分）/ 等待126**；项目累计**5349分钟（89小时9分）/ 明确等待450分钟（7小时30分）**。M2仍1400 / 等待139。

C3当前只有指南 / 候选：五新UI与六旧Core+UI、新十一绑定卡与两宿主第八绑定。增加原生路由就绪事件处理首次菜单时序，页面草稿、显式确认、显示预览隔离和关闭解绑；不提前记为生产源码 / 新资产或实际工时。

## 4. Commit / Push（固定放在最后）

C2实现已在本地1a2a81b。当前Stage本报告、C2指南收尾（含上轮编辑器记录）、AssetManifest八新行、C3新指南和实际修改的共享状态文档、Introduction / README。准确清单见C3第10.1节。

- 英文：`docs: verify audio routing and prepare shared audio settings UI`
- 专业中文：`docs: 完成声音路由验收并准备共享音量设置界面`

C3未实施文件不提前Stage。Artifacts原证据 / 候选 / 个人ini与缓存留本地；Stage后继续修改需重新Stage → Review → Commit / Push由用户操作，助手未核验远端。
