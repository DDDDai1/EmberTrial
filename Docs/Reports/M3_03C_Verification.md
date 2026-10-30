# M3-03C：遗物查询控件验收报告

2026-10-30，用户16:44报告全项完成、无布局或功能问题。**本批通过，无待补；进入[M3-04A祭坛发现与使用资格](../M3_04A_RelicAltarDiscovery.md)，下一批仅指南准备。**

## 1. 实物、构建与范围

实际186 Source / 104cpp（Core72 / UI29 / Editor3）、31自动化、298 Content / 来源79。五旧Source均已落地：UI Build.cs、Presenter.h、Panel.h / cpp、Presenter测试。既有Presenter增加只读Query查询，Panel的11个必需绑定含SearchBox / FilterCombo / SortCombo；无新增业务Tick / Timer或Core反向依赖UI。

本地HEAD `abfd9ba`已含完整实现、Panel及操作记录；核对起点工作区干净，未联网核验远端。Panel保存于16:36:34，Pause于10月29日20:48:23，原Pause字节未变，不强制重存制造差异；两者本机SHA与HEAD LFS一致。当前代码与指南关键完整代码块一致，三项文本焦点Setter已落地。

Editor / Game receipt分别为16:05:01 / 16:07:33；三索引、头文件导航与实际正常双Build按用户反馈通过。本轮没有重新执行Build或测试。

## 2. 自动化与实际运行证据

引擎日志UTC加8小时为北京时间，本轮`EmberTrial/Saved/Logs/EmberTrial.log`：

| 2026-10-30北京时间 | 核对内容 |
| --- | --- |
| 16:38:44.580 | EmberTrial.UI.Relics.ViewModels：Result=Success |
| 16:39:02.306 | EmberTrial.UI.Relics.Presenter：Result=Success；包含查询、空结果、稳定Item与库存无副作用新断言 |
| 16:39:09.623–16:40:09.867 | 第一轮Panel订阅 / 解绑；Items在2 / 1 / 0间变化并恢复，空结果Selected=None、Preview=0 |
| 16:40:18.304–16:40:41.954 | 第二轮新Panel从两项默认选择开始，订阅 / 解绑闭合 |
| 16:40:59.967 | 真实ET_Relic装备生命遗物，Result=0、Revision4、Health100/120、Bonus0 |
| 16:41:10.462–16:42:28.946 | 已装备Preview=2，筛选后Items=1；最终面板解绑，后续Stop / 退出清理 |

实际三次订阅 / 三次解绑。中文与大小写、四筛选、排序保留选择、P字符 / Esc / Enter及输入法确认、浏览不治疗、最后键鼠短查由用户确认通过；日志不独立证明每个画面或输入法行为。本次自动化也不代替Slate真实焦点检查。

## 3. 历史报错已经关闭

装配过程中16:16–16:21曾有缺少三个绑定的编译错误，16:21:44出现ET-UI-001。这些发生在7.1控件尚未补齐 / 成功编译前。16:22:26及后续多次本项目Valid，最后16:36:35.129再次Valid；7.4.1已记录旧Message Log保留机制及本次Compile / Save核对。

最后Valid之后的两项自动化与实机段没有新增Error或Ensure；不能把装配中的旧红行写成尚未解决，也不把整份历史日志称作从未报错。ListEntry Auto、Panel / Pause Never保持，未放宽校验器或删除断言来消除错误。

## 4. 工时、产物与后续

用户手动代码90 / Designer30 / 检查5 = **125分钟**，构建等待5分钟。M3累计636分钟（10小时36分）/ 等待49；项目4047分钟（67小时27分）/ 明确等待373分钟（6小时13分）。助手操作不额外计入用户实耗，未单独登记的操作不推算追加。

来源清单更新Panel真实SHA与本章阶段；Pause保留旧SHA并标注未变。原始日志留本地`Artifacts/M3_03C/Acceptance/EmberTrial_20261030.log`。两件列表验证不构成500条压力、图标异步或新包Cook证明；最新包仍M2_07A/Attempt01。

下一批建立当前World祭坛弱索引、按需发现与距离 / 资格检查及灰盒标记；E / 页面 / 最终换装许可分批接入。继续手动C++、按影响范围验收与用户Git操作约定。

## 5. Commit / Push（固定放在最后）

实现已在本地abfd9ba；本轮只Stage本报告、03C收尾、04A指南、AssetManifest与同步状态文档。未变化Source / 资产不重复Stage；日志 / 候选 / 包留本地。Stage / Commit / Push由用户操作。

- 英文建议：`docs: verify relic query controls and prepare altar discovery`
- 中文译文：`docs: 完成遗物查询控件验收并准备祭坛发现功能`
