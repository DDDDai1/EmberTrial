# M1-07C2：战斗引用、输入与配置校验验收

更新2026-10-14，UE **5.5.4**。用户14:09报告全项完成、无问题，手动20 / 构建等待5分钟。**C2验收通过、无待补项。**下一步[C3 Attempt02与M1收口](../M1_07C3_UpdatedPackage.md)，新的包尚未生成；本轮不提前标记M1整章收口。

## 1. 源码、构建与配置

两份新Validator文件与C2指南全文一致；Editor Build.cs仅增加四项私有依赖（规范化空白后与示例一致）。相对C1原73Source仅Editor Build.cs改变，其他72份全部保持，Runtime Source无新变化；现 **75Source / 37cpp**。CMake仍三个CodeModel，两Target / uproject和Runtime依赖边界保持C1。

Editor模块DLL / receipt **14:00:41**晚于源码，UE5.5.4 / Win64 / Development；Game receipt / exe保持C1 **13:18:06 / 05**，Game产品无Editor模块。此次Game构建成功依据用户反馈登记；Runtime Source未变，保持旧有效Game产物是正常的，**不虚构本轮新增Game链接**或把Editor时间写成Game时间。

共享DefaultEngine.ini未变，DefaultGame.ini新增 `[/Script/GameplayAbilities.AbilitySystemGlobals]` 下的 `/Game/EmberTrial/Abilities` Cue扫描路径。助手再次运行只读脚本直接得到 **Config PASS Errors=0 Warnings=0**。脚本检查共享配置 / 文件存在，不合并平台 / Saved覆写，不反序列化umap；Cue回退Warning是否在真正包进程消失留C3，不用本轮Editor没有Warning冒充Runtime验证。

## 2. 直接校验与自动化证据

日志时间换算为北京时间：

| 时段 | 直接结果 |
| --- | --- |
| 14:03:29–14:05:11 | 九个正式资产全部M1-07C2 Valid / Issues0：玩家、敌人、训练目标、AI Controller、三GA、伤害GE、IMC_Default |
| 14:06:20.326 / 40.373 | 同一GA Probe两次Invalid / Issues1，均为精确ET-GA-001 / AttackMontage None |
| 14:06:52.250以后 | 同一Probe恢复Valid，最后14:07:51.490仍Valid / Issues0 |
| 14:08:45.469 | DodgeMovement一次Success；30 / 60 / 120三步长均Travel2D150.05cm / SourceTime0.3001s |
| 14:08:45.577 | Lifecycle一次Success |

最终GA Probe **14:07:45**正常保存，随后同路径14:07:51的Valid支持最终配置已恢复。未授予正式身体按用户确认登记，没有把仅日志阅读表述为新的原生资产引用遍历。

本轮匹配Error / Fatal / Accessed None / Ensure仅有**两条故意清空Montage产生的ET-GA-001**；多次校验发生在同一负例期间，随后恢复，不是两项未关闭错误。Native规则 / 已有自动化执行有真实项目日志；不声称每条规则均有独立负例，C2错误样例实测仅ET-GA-001。现有动作测试不证明本批GA实战、所有Runtime UI门控或配置生效，C3承担新增包相关短流程。

原47项清单资产与C1逐份哈希一致，新增合法GA Probe后共 **48项**；原BT Probe不变。A原包七项指纹均不变，仍含历史Radius150；当前Source / Config更新不会自动修改Attempt01。

## 3. 工时与交接

C2实际 **20 / 等待5分钟**，累计已报告手动 **2001分钟（33小时21分钟）** / 已明确等待 **175分钟**。M1-07 A+B+C1+C2手动165 / 已明确等待35；B最后短函数替换未报告的新耗时仍不推算。C1+C2工具初建实际60 / 等待10，不据两批样本声称稳定节省率。

本地HEAD仍 `e3ee554`，本轮未联网核验远端。C2源码 / 私有依赖、Cue配置、合法GA Probe、工具 / 指南与本报告 / AssetManifest48和状态文字，按C2最后清单由用户重新Stage / Commit / Push。旧包 / Raw / 测试日志 / 生成索引留本地。

原件在 `Raw/M1_07C2_2026-10-14/Evidence.json`、`ValidationAndAutomation.txt`，含75Source / 48资产指纹与receipts。冻结B1文档哈希保持 `FBA8C03A1656C619EDF219864535D80AB577F3588A0F611E5746638EF1133E09`。助手只读核查 / 配置脚本，更新文档 / 来源清单与C3启动工具；未改工程Source / 资产 / 共享Config、构建或运行PIE、操作Git索引 / 远端。
