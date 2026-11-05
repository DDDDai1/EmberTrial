# M3-05B1：视频模式值契约与原生快照验收报告

2026-11-05，用户15:03报告完成、无失败或不理解处。**本章验收通过，无待补，进入[M3-05B2-01：视频预览确认与回退事务内核](../M3_05B2_01_VideoPreviewTransaction.md)。** 01当前仅手动指南 / 忽略候选，未写正式Source。

## 1. 实物与构建

实际**204 Source / 115cpp（Core80 / UI32 / Editor3）、35自动化、300 Content / 来源81**。三新Core源码逐份归一注释 / 空行与include顺序后与B1候选一致；Build.cs、既有GameUserSettings、UI / Config与Content保持。全部81条来源资产SHA一致，不要求重存或更新清单。

Editor / Game receipt为**14:59:47 / 15:01:16**；用户确认双Build / 三索引成功。本地实现`caf1eec`，审查起点工作区干净；助手未替用户Commit / Push。

## 2. 专项与只读观察

本轮EmberTrial.log，以下为2026-11-05北京时间：

| 时间 | 实际记录 |
| --- | --- |
| **15:02:10.128** | `EmberTrial.M3.Settings.VideoModeRules` Result=Success |
| **15:02:22.639** | Requested=2048×1152，Mode1 / Valid1；Confirmed相同；Desktop=2560×1440 |

Mode1是本项目无边框枚举。该记录说明原生请求 / 上次确认配置与桌面值，**不证明实际Viewport为2048×1152**，也不是B1正规化后已成功切屏；B1没有执行显示修改或保存。

专项覆盖窗口 / 全屏目录资格、无边框使用桌面、非法模式 / 尺寸、失败输出清空、输入输出别名与原生请求 / 确认分离；测试中的分辨率不当作用户平台可用列表。原P输入卡1 / 不反转、移动转向与保存清理按用户反馈通过。本轮日志未见项目蓝图Error、Ensure或Accessed None。

副本在`Artifacts/M3_05B1/Acceptance/EmberTrial_20261105_1503.log`，摘要同目录。不重复Build / 专项、A1重启、旧菜单 / 死亡矩阵或Cook；旧M2包不含M3。

## 3. 工时与下一章

代码30 / 阅读5 / 检查3，共**38分钟**，等待3。M3累计**1198分钟（19小时58分）/ 等待78**；项目**4609分钟（76小时49分）/ 明确等待402分钟（6小时42分）**。助手审查不计用户实耗。

B2拆为01事务内核、02原生窗口 / 实时时钟适配，再由B3接显示UI。01三个普通Core文件处理实际结果观察、期限、Serial隔离、确认 / 恢复与恢复失败，不声明已切屏；02才接真实Viewport、平台目录、暂停下计时与生命周期。每批只新增相关专项及短查，不搭临时验收图。

## 4. Commit / Push（固定放在最后）

B1三份新Core源码已在本地`caf1eec`，审查起点工作区干净。当前Stage B1验收报告 / 指南收尾、B2-01新指南与同步状态文档；01正式Source尚未实施，不Stage忽略Artifacts里的候选。未变化源码 / 资产不重复Stage。

- 英文建议：`docs: verify video modes and prepare preview transactions`
- 中文译文：`docs: 完成视频模式验收并准备预览事务`

Stage / Commit / Push由用户操作；个人设置、Raw / 候选 / 日志 / 缓存 / 包留本地。
