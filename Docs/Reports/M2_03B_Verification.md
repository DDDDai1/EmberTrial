# M2-03B：三段轻攻击与有限输入缓冲验收

2026-10-16，UE5.5.4。用户13:26全项完成、无问题，手动50 / 构建等待5分钟。**本批验收通过，无待补项；直接进入M2-03C恢复阶段闪避取消。**助手本轮只读源码、哈希、构建 / 索引与日志，没有执行UE Build / 测试或修改工程Source / 资产。

## 1. 实现与构建

三新 / 六旧Source均与[B指南](../M2_03B_LightCombo.md)一致；Character.h与Editor Validator只有缩进差异，其余90份旧Source保持A哈希。实际**99Source / 52cpp**，三CodeModel为36 + 13 + 3；Editor receipt 12:52:20、Game receipt 12:59:58均覆盖本批源码，最新UBT记录Editor up-to-date。两个目标Build和CLion导航结合用户反馈接受。

原生命 / 阵营 / 无敌伤害路径保留；一个玩家能力拥有整条链，激活 / 段代次、Montage实例与窗口Token隔离，段间不释放Attacking，每段独立Trace Serial。结束清输入 / Timer / Trace、解绑Task并在正常EndTask后撤销已结束Task的旧Ability指针。敌人仍关闭Combo，正式2在本批仍拒绝攻击中闪避。

## 2. 自动化与真实运行

- **13:14:11.506 Combo.Flow Success**：真实保存Montage / Notify / Sweep / GE，1000→925、三独立Serial；单次 / 过早过期 / 单槽连点 / 无第四段、旧窗口 / Serial / Task迟到回调、受击 / 暂停 / 死亡 / EndPlay均由实际测试驱动。
- **13:14:37.971 Dodge.Invulnerability Success**：复用旧真实敌人单段与无敌门控专项，不重跑未变资源 / Practice / 移动矩阵。
- **13:24:32–34正式PIE**：玩家三段Serial1 / 2 / 3，目标100→75→50→25，Current / Base同步；整链末尾AttackingTag0。13:25:07 / 10受击取消后可重新从1开始；暂停显示 / 恢复与缓冲到期有记录。
- **13:25:49–53正式敌人**：Stage1/1、原共用Montage、连续每次10，玩家HUD100→90→80→70；随后Stop清理。单点 / 长按、具体焦点键鼠、2拒绝与最终恢复结合用户全项反馈，不要求额外旧矩阵。

本轮日志没有玩法Error或Accessed None。引擎编辑器图标 / EOS权限与退出时HTTP Warning属于工具 / 联网背景，未作为玩法失败或删除历史。测试夹具移动目标靠近手部只用于隔离几何；自然站位真实三次命中另由上述PIE补证，不宣称AI覆盖或性能改善。

## 3. 资产实际差异

新增独立`Combo/Montages/AM_ET_Light_01`，来源清单**57→58**；既有玩家GA / 第2段AM是计划变更，敌人GA为正常Compile / Save。GA玩家13:12:55 / 13:13:00、敌人13:13:39均有本项目Valid / Issues0；两段连段通知、第三段只命中、采样 / Slot / Blend与最终数值按用户配置反馈，助手未打开UE读取完整属性。

旧`AM_M0_Attack_01_Montage`13:07:05发生重存，文件SHA与A不同，不能记作“字节未改”。对照本地LFS保存的A旧对象：旧 / 新头区9396 / 9398字节，后面的序列化导出数据SHA完全相同；差异限定在包头 / 缩略图区，均未出现ETANS_ComboWindow名称。它的动作导出数据保持，真实旧测试和敌人单段10也通过，**不是待补验缺口**。新SHA如实更新清单，不回滚用户资产。

其余53份旧清单资产、Config / uproject / 冻结B1指南保持A哈希。没有新模块 / Build依赖 / UI / 输入映射 / 地图或Cook；历史独立包仍M1 C3 Attempt02。

## 4. 工时与交接

B手动50 / 等待5；M2-03当前100 / 15，M2当前340 / 45分钟。项目累计**2351分钟（39小时11分钟）** / 已明确等待**230分钟（3小时50分钟）**。独立检查耗时未拆报，助手设计 / 排障不计用户手动；不将录入速度作为缩减后续预算的依据。

下一[M2-03C](../M2_03C_RecoveryDodgeCancel.md)只在当前段命中结束后的恢复区间允许正式2取消；先通过Dodge合法性 / 成本等预检，再交接，失败不能吞掉原攻击。B无需重做，C尚未实施；本批没有把整个M2-03标为完成。Raw本地保留Acceptance、源码 / 资产指纹、RuntimeExcerpt与旧包导出区对照。

## 5. 本次 Commit / Push（固定放在最后）

核对期间用户已提交`9a0cf14`，本地HEAD / 缓存origin/main一致，B实现与资产已纳入；助手没有Stage / Commit / Push或联网核验。当前重新Stage本报告、来源清单58项、B已验收状态和路线 / 进度 / 设计更新；C新指南可一起提交。没有新实现差异的Source / uasset不人为重存；旧共用AM的真实重存已纳入B提交，清单记录新SHA。

Raw / 日志 / 生成CMake / IDE缓存 / Binaries / Intermediate / Saved与历史包留本地。CLion查看最新Changes与Staged Diff后由用户Commit / Push，可用`docs: verify light combo and prepare recovery dodge cancel`；C代码实施完成后另按C末尾清单提交。
