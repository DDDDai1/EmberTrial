# M3-03B：暂停页遗物浏览验收报告

2026-10-30归档。主体反馈为2026-10-29 21:08，Tick窄补修反馈为同日21:24。**主体功能与补修均通过，本章验收关闭，进入[M3-03C查询控件](../M3_03C_RelicQueryControls.md)。**

## 1. 实物与主体结果

当前186 Source / 104cpp（Core72 / UI29 / Editor3）、31自动化、298 Content / 来源79。Panel、Pause分别保存于10月29日20:42:23、20:48:23；ListEntry补修后保存于21:22:05。本地HEAD `b1c9579`已包含主体和Auto配置，三资产LFS OID与本机SHA一致；未联网核验远端。

主体Editor / Game receipt为20:58:00 / 21:00:56，三索引由用户确认。换控补修已在角色Restart阶段恢复装备，旧身体不重复回血。21:03:56.013 Presenter、21:04:10.871 EquipmentTransactions实际Success；两项不创建屏幕条目，不代替Slate显示检查。配置补修不涉及C++，没有要求重复Build或自动化。

原主体日志证据（引擎UTC加8小时为北京时间）：

| 北京时间，2026-10-29 | 核对结果 |
| --- | --- |
| 21:04:16.518 | 遗物面板订阅，Ready / 两Item，Pause shown |
| 21:04:25–21:05:14 | 两件选择与槽A / B切换，Presenter持续Ready |
| 21:05:27.706 | ET_Relic实际装备生命遗物，100/120、Revision2 |
| 21:05:32.400 | 已装备状态Preview=2，列表保持两项 |
| 21:06:37.020 | 最后一份面板解绑；四次订阅 / 四次解绑闭合 |
| 21:06:53.907 | Stop释放玩家装备GE与库存 / 会话 |

中文、布局、浏览无属性副作用、重复遗物拒绝、P焦点和菜单Continue按用户实机反馈通过。短日志不独立证明每个键鼠画面或真实Continue切图；无500条池化或图标异步压力结论。

## 2. Tick问题与关闭证据

主体日志21:04:19.025曾出现Handled Ensure：

```text
TickFrequency != EWidgetTickFrequency::Never
SObjectWidget and UUserWidget have mismatching tick states
```

调用栈经过SObjectWidget::Tick及STableViewBase。依据本机UE5.5.4的Slate列表包装与UserWidget Tick路径，优先调整列表条目的Tick兼容设置；没有通过调试器取得出错实例名，故不把初步推断写成完整引擎故障归因。

用户仅将**WBP_ETRelicListEntry Tick Frequency改Auto**并Compile / Save；原生DisableNativeTick保留，无业务Event Tick / Timer，Panel / Pause保持Never。新编辑器进程的`EmberTrial/Saved/Logs/EmberTrial.log`保存至21:22:57，核对结果：

| 北京时间，2026-10-29 | 核对结果 |
| --- | --- |
| 21:22:14.187 → 21:22:16.622 | 首次Panel订阅与解绑，期间真实选择变化 |
| 21:22:25.523 → 21:22:25.937 | 第二轮订阅与解绑 |
| 21:22:26.389 → 21:22:36.054 | 第三轮订阅与解绑 |
| 整个新进程日志 | 无TickFrequency / mismatching tick / Handled Ensure或Error记录 |

两行、槽位切换、最后移动与Stop由用户确认。共三订阅 / 三解绑，未让用户重复装备、Continue或旧能力矩阵。新ListEntry SHA256为`D7C4275EA91EB6F4CA440228FEA97765E801CA7078C3169841D7BE735859F41F`，与本地HEAD的LFS OID一致。此兼容问题关闭，无待补验收项。

新进程原日志已留存本地`Artifacts/M3_03B/TickFix/EmberTrial_20261029_2122.log`。来源清单79项实际SHA全部匹配，三份本章UMG均与HEAD的LFS一致；本轮未修改正式Source / Content。03C只做文档和接口静态核对，不作为其编译 / 运行成功证据。

## 3. 工时与后续边界

主体人工135分钟（代码90 / Designer40 / 检查5），补修4分钟，共**139分钟**；构建等待5分钟。M3累计511 / 等待44分钟，项目人工3922分钟（65小时22分）/ 明确等待368分钟（6小时08分）。助手审查不计用户实耗。

下一批03C只把既有查询能力接到搜索 / 筛选 / 排序控件，检查输入焦点、空结果与选择保留；真实确认换装、安全祭坛、完整六件、磁盘存档和最终美术仍后续。下一批文档不等于代码已落地；最新独立包仍M2_07A/Attempt01。

## 4. Commit / Push（固定放在最后）

主体和Auto资产配置已在本地`b1c9579`。当前提交本报告、03B补修收尾、03C指南、AssetManifest及同步状态文档；无变化Source / 资产无需重复Stage。用户自行刷新Changes、Stage最新差异、Review、Commit / Push；日志、Raw和包留本地。

- 英文建议：`docs: close relic panel tick fix and prepare inventory query controls`
- 中文译文：`docs: 完成遗物面板 Tick 补修验收并准备库存查询控件`
