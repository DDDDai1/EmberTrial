# M3-01：遗物静态定义、ID与资源注册验收

2026-10-28，UE **5.5.4**。用户17:21报告全项完成、无失败或不理解处，代码 / 配置40分钟、检查2分钟、等待5分钟。**M3-01通过，进入[M3-02A两槽方案与纯预览](../M3_02A_LoadoutRules.md)。**本次沿用用户刚完成的Build / 日志，不重复启动UE、测试或打包。

## 1. 源码、配置与实物

四新Source与指南代码归一换行后相同；旧Runtime除Core Build.cs的Editor条件AssetRegistry依赖和DefaultGame.ini的ETRelic扫描条目外保持前基线。没有修改Character / GAS / 旧HUD / 输入。当前**161 Source / 88cpp，Core62 / UI23 / Editor3、24项自动化、296份Content / 来源77**。

本地HEAD81a32c7已包含完整实现与两新资产，核对起点工作区干净；两资产LFS OID与本机SHA匹配，未联网核验远端。原独立包M2_07A/Attempt01仍仅代表M2，不包含新定义，AlwaysCook配置不等于本轮已验证Cook。

| 材料 | 本次核对 |
| --- | --- |
| Editor / 索引 | 正常Editor receipt为17:11:22，三模块索引已生成；全部最终Source早于receipt，用户确认Build / 导航通过 |
| Game | Development / Win64 / UE5.5.4 receipt为17:15:41，晚于最终Source；用户确认正常Game Build通过 |
| DA_ET_Relic_LifeEmber | `/Game/EmberTrial/Items/Relics`，原生ETRelicDefinition，2831字节；17:19:36正常保存 |
| DA_ET_Relic_KeenEdge | 同目录 / 原生类型，2835字节；17:20:06正常保存 |
| 数据契约 | 用户确认life_ember / keen_edge、两槽、20 / 5、FText与空图标；两份Valid及实物ID加载由日志 / 专项支持，未逐一独立反序列化所有Details字段 |

## 2. 当次有效证据

下列时间由UTC日志转为北京时间，来源`EmberTrial/Saved/Logs/EmberTrial.log`：

- **17:20:20.345**：两份资产均有`contains valid data`，走原生IsDataValid；不要求旧Blueprint Validator前缀。
- **17:20:51.999**：`EmberTrial.M3.Inventory.Definitions`明确Success，事件区间无Error / Warning。检查真实目录、两件预期ID、各ID自动注册路径和目录唯一性，不在测试里补注册来掩盖配置。
- **17:21:01–14**：正式地图PIE，生命100/100；攻击请求Stage1 / Damage25、闪避150.05cm / SourceTime .3001与旧HUD清理有日志。真实命中25、E / P和具体输入体验按用户反馈接受，不把请求Damage字段冒充本轮目标实际扣血证据。
- 保存 / 重开按用户反馈登记；本轮无需为收尾重启或重跑旧23项。编辑器引擎图标、EOS / 退出HTTP警告与本项目玩法 / 自动化结果分开，不声称整份编辑器日志零Warning。

本章没有装备入口，正式数值仍100 / 原25。无新GE效果、库存、跨World装备恢复或构筑UI，不因DataAsset字段已存在就声称这些行为已实现。首次PIE等待需求保持关闭。

## 3. 工时与下一步

本批手动42 / 等待5分钟：M3已量化42 / 5，全项目3453分钟（57小时33分）/ 明确等待329分钟（5小时29分）。M2仍1400 / 139，06C与07C阅读未登记；助手审查和下发估计不计用户实耗。

M3-02拆为A纯两槽方案 / 预览、B轻击加成属性与装备GE、C库存 / 身体事务及会话集成；C视规模再拆，不把所有生命周期与页面同批录入。A无新资产 / Gameplay状态，复用一个规则专项与现有Definitions，不重搭验收Graph。

## 4. Commit / Push（固定放在最后）

81a32c7已包含本批实现 / 两资产，不重复Stage无差异Source或Content。当前提交本报告、来源两行 / 状态收尾和新A指南，精确清单见[A末尾](../M3_02A_LoadoutRules.md#11-本批-commit--push固定放在最后)。建议英文`docs: verify relic definitions and prepare loadout rules`，中文译文`docs: 记录遗物定义验收并准备装备方案规则`。

A实施后建议英文`feat: add relic loadout planning and stat previews`，中文译文`feat: 新增遗物装备方案与属性预览`。Raw / 日志 / 包留本地，Stage / Commit / Push由用户操作，助手未改索引或联网核验远端。
