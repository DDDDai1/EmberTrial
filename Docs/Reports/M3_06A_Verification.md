# M3-06A：当前遗物与设置闭环独立包验收

2026-11-06 23:35完成。**验收通过，无待补项。** 当前两件遗物 / 两槽祭坛、四类设置及新身体恢复已在UE5.5.4 Win64 Development包内成立。下一步进入[M3-06B1第三件代价型遗物](../M3_06B1_TradeoffRelic.md)；完整六件、列表压力和最终美术仍后续，M3尚未总收尾。

## 1. 构建与工程状态

- 实际包为Artifacts/Builds/M3_06A/Attempt01；入口Windows/EmberTrial.exe，实际Game二进制在Windows/EmberTrial/Binaries/Win64/EmberTrial.exe。UAT完成Build / Cook / Stage / Package / Archive，BUILD SUCCESSFUL / ExitCode0，BuildCookRun记录78.22秒。
- 归档Game二进制SHA与当前工程Game二进制相同。Game receipt为Win64 / Game / Development / UE5.5.4，无EmberTrialEditor项目构建产物。根启动器SHA未被当作Gameplay代码证据。
- **233 Source / 133cpp（Core89 / UI41 / Editor3）、42自动化、309 Content / 来源92**；92份来源实物SHA均保持。起点工作区干净，HEAD为c03948c，前一文档提交9c0f9d5；三项受伤测试修复已包含在本地历史，未核验远端。
- 本轮仅收尾报告 / 状态与准备B1指南 / 本地候选，未写正式Source / Content / Config、未Stage / Commit / Push。

| 归档文件 | 字节 | SHA256 |
| --- | ---: | --- |
| EmberTrial.exe | 304640000 | 82EE5E4B88BB0E80D589FDC335A50E96701713FE4F980F817450DB9B7CFEF8D4 |
| EmberTrial-Windows.pak | 10358596 | C86878D4686ADE1D79695E1DE45F5CE61B8C809D489DE63ABFB82B82951254A5 |
| EmberTrial-Windows.ucas | 232015152 | E42ED3F1A3763223A83B32A3BB9CAC5C010DFE7F700C106377F903218A363D39 |
| EmberTrial-Windows.utoc | 212936 | 590EFE0FFEC8CB739F7D80BF0728ABC83B28641B23CA2E54854F6711438CC9E7 |

## 2. 自动化修复与完整回归

首次全组39成功 / 3失败。BreakPulseAbility、StaggerAbility和Healing.Ability在初始化前设生命40，M3新身体装备恢复随后按最终上限补满100，破坏了旧场景的受伤前提。已将受伤准备移到初始化 / 落地之后，新增Current / Base均40的前置断言，保留全部原行为断言和正式新身体规则。

正常Editor Build约42秒通过，21:21命令行完整EmberTrial **42 Success / 0 Fail / 0 Warning / 0 NotRun**，退出码0。证据：Artifacts/M3_06A/HealthFixtureFix下的Before_Automation.log、EditorBuild.log、Automation.log与AutomationReport/index.json。没有把删除断言或只改预期为100当作修复。

## 3. Run01与包内流程

Run01_20261106_212728_128使用上述新包，UE编辑器关闭、专用M3PackageUser.ini。以下为日志与实际UI / 用户反馈结合的结果，时间均Asia/Shanghai：

| 样本 | 结果 |
| --- | --- |
| 首屏 / 设置 | Continue禁用、中文四卡与首次音量可用；输入1 / 不反转。显示预览1365×767→取消1280×720，画质应用中 / 60 / VSync0 |
| 音量 / 草稿 | UI改音乐0而不应用时当前组不变，确认后生效；最终0.5 / 0.5 / 1。用户确认音乐探针静音、音效有声，两路Started1 |
| 祭坛 / 权限 | 真实两行；槽A生息、槽B砺锋，装备后100/120不治疗，普通P仍只读，未绕过祭坛Access |
| 真实攻击 | 21:33:53，左键对训练目标生命70→40，Requested / Actual=30，装备加成真实参与结算 |
| 资源 / 敌人 | 2体力100→75并恢复；F真实使目标韧性100→40；AI追击并每次10伤害；Q在90/120成功至120/120，次数3→2 |
| 检查点 / 新身体 | Checkpoint_A激活；敌人真实致死→21:35:10 Retry_A新120/120与默认资源；21:35:33菜单Continue_A，两槽均保留，新120/120 |
| 正常结束 | 音量最终恢复0.5 / 0.5 / 1，21:38:01菜单正常退出 |

助手完成菜单 / 设置 / 装备UI、一次真实轻击、Retry / Continue与退出；用户协助持续移动、部分战斗与实际听感。未采集扬声器输出或逐键录屏，未虚构为全程助手键鼠独立完成。

Run01遗漏Tab，另用LockSupplement_20261106_232951_035独立进程补测：23:30:24日志锁定真实敌人 / 投影Active1；用户与助手均观察单标记、镜头朝向敌人，23:31:10正常退出。21:39那次进程只到菜单后退出，另一次仅有启动JSON，没有冒充有效锁定或第二轮证据。

## 4. Run02新进程

Run02_20261106_233205_616与Run01明确指定同一Attempt01和同一M3PackageUser.ini：

- 23:32:34只查询音量Valid1 / 0.5-0.5-1 / RoutingApplied1；23:32:41只查询画质Valid1 / 1-60-0，之前无UI Apply，证明跨进程读取。
- 音效探针Started1；用户反馈实际试听正常。首屏Continue禁用 / 新卡自行就绪由用户确认。
- 23:32:54新试炼Requested=Entry / Actual=Entry，库存Owned2、两槽None，生命100/100与资源默认；个人设置保留而本进程试炼装备清空，符合当前语义。
- 最短输入 / P继续按用户反馈通过，23:33:11从菜单正常退出。Run01 / Run02 / 有效锁定补测均未发现Fatal / Assertion / Ensure / Accessed None。

两轮Runtime原日志、启动JSON、个人ini与摘要保留在Artifacts/M3_06A，本地Summary.json列包指纹和证据。功能检查未采集性能Trace，不能据此宣称60 FPS性能或Shipping完成。

## 5. 工时与后续

用户汇总准备30 / 检查40 / 记录10，共**80分钟（1小时20分）**；Build-Cook等待**10分钟**。UAT内部78.22秒与用户汇总等待口径不同，分别保留。按用户本章汇总只记一次，不再叠加助手操作、自动化或补测时长。

M3累计**2143分钟（35小时43分）/ 等待141分钟**；项目累计**5554分钟（92小时34分）/ 明确等待465分钟（7小时45分）**。M2仍1400 / 等待139。

B1用现有生命 / 攻击消费者加入代价型第三件，解除相关测试固定两件的前提，不扩建新测试框架；随后补体力恢复 / 破势效率 / 治疗效果与列表压力。新包当前仍两件，未来配置变化不自动写回旧包。

## 6. Commit / Push（固定放在最后）

当前提交本报告、06A指南收尾、B1新指南及实际变化的共享状态文档；准确十二文件清单见B1指南末尾。三份已提交测试修复、Source / Content / Config与来源92本轮不重Stage。

- 英文：`docs: verify M3 package and prepare tradeoff relic`
- 专业中文：`docs: 完成M3开发包验收并准备代价型遗物`

包 / UAT / Runtime / JSON / 专用个人ini / 候选保持本地。Stage后继续编辑需重新Stage → Review → Commit / Push由用户操作，助手未核验远端。
