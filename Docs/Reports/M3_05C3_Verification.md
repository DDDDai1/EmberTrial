# M3-05C3：共用音量设置界面验收记录

2026-11-06 20:55完成。**验收通过，无待补，进入[M3-06A当前闭环独立包](../M3_06A_IntegratedPackage.md)。** 已实施的输入 / 显示 / 画质 / 音量四类设置均有实际界面与持久化证据；镜头震动等随反馈系统后续接入，不宣称全部计划设置或M3已完工。

## 1. 源码、构建与资产

- 五新UI / 六旧Core+UI与候选去空白后全部一致，本地f363841包含十一源码 / 三Widget。本轮起点工作区干净；助手只收尾文档、来源与启动工具，没有修改生产源码 / Content / Config，也未Stage / Commit / Push或核验远端。
- 实际**233 Source / 133cpp（Core89 / UI41 / Editor3）、42自动化、309 Content / 来源92**。Editor / Game receipt时间20:20:54 / 20:22:54；双Build / 三索引用户确认通过。
- Editor日志20:24:55.163，EmberTrial.UI.Settings.AudioDraft Result=Success。Pause20:49:29.098、MainMenu20:49:31.729、新Audio WBP20:49:37.124均有最终Valid；用户确认十一 / 八绑定、无Binding / Never和正常保存。
- 三Widget实物SHA匹配HEAD的LFS OID；来源新增真实音量卡、更新两宿主，原其他89行保持，92行SHA全部匹配。

## 2. 两轮原生窗口与证据边界

原证据位于E:/UE_Project/Demo/Artifacts/M3_05C3/Acceptance，含Editor.log、AudioUI01 / 02.log、AudioUIUser.ini、Summary.json，不上传个人配置或原日志。表内为Asia/Shanghai，UE原UTC已加8小时。

| 检查 | 实际结果 |
| --- | --- |
| 首次主菜单 | 第一轮20:50:24.772声音路由就绪；用户确认首次音量卡自动可用，无须先进试炼 |
| 浏览 / 取消 | 20:50:57.105查询仍1 / 1 / 1；用户确认浏览无声音副作用、取消回已应用值 |
| 控件应用 | 20:51:14.818应用1 / 0 / 1；用户确认音乐静音、音效仍有。Core / View两条为同一次操作，非保存两次 |
| 默认与最终组 | 用户确认默认仅草稿；20:52:03.507真正应用回1 / 1 / 1；20:52:17.524控件应用最终0.5 / 0.5 / 1 |
| 显示预览隔离 | 20:53:13.767启动1280×800、Paused1；20:53:22.843–.846取消回1280×720，期间无额外音量应用；用户确认卡禁用保留草稿 / 取消恢复 |
| 焦点与页面 | 三滑条5%步进、Tab / ShiftTab / 方向键 / Enter / Space、Esc本卡取消、自动滚动、无误开始、Pause离开丢弃与菜单新卡，按用户实机反馈通过 |
| 新进程只读 | 第二轮20:54:08.260菜单RoutingApplied1，20:54:26.213只查询0.5 / 0.5 / 1；之前无apply。20:54:31 / 35两路探针Started1，用户确认真实试听 |
| 清理与结束 | 两轮约20:53:45 / 20:54:36正常退出，最终World路由释放；无Fatal / Assertion / Ensure / Accessed None。PIE禁用与P / 移动短查按用户反馈通过 |

未额外录制键鼠 / 采集扬声器输出，日志不替代视觉和听感。没有新Cook或42项阶段全组成功结论；旧M2包不含M3，留下一章专门验证资源 / 包内流程。

## 3. 工时与下一步

代码70 + Designer35 + 阅读10 + 检查10 = **125分钟（2小时5分）**，构建启动等待**5分钟**。M3累计**2063分钟（34小时23分）/ 等待131**；项目累计**5474分钟（91小时14分）/ 明确等待455分钟（7小时35分）**。助手工作不计用户实耗，M2仍1400 / 等待139。

下一章M3-06A先把现有两遗物 / 祭坛 / 设置闭环放入新Development包，做一次42项阶段回归及两轮包内集成，不新增测试蓝图。启动脚本已增加M3归档 / 专用个人ini与显式目录保护，后续由用户执行实际Build / Cook / 运行。完整六件与列表压力仍后续，本章通过不关闭整个M3。

启动工具 / 新指南三段PowerShell语法、M3参数及缺省目录保护、原M2参数兼容已通过；`-WhatIf`没有启动进程或创建Runtime文件，证据在Artifacts/M3_06A/PreparationChecks.json。配置静态检查PASS / Errors0 / Warnings0。以上均为准备检查，不计为42项或新包验收。

## 4. Commit / Push（固定放在最后）

当前Stage本报告、C3指南收尾、AssetManifest新卡 / 两宿主、M3-06A新指南、Run-StageBuild.ps1和实际更新的状态 / 路线文档、Introduction / README。准确清单见新指南第9节。

- 英文：`docs: verify audio settings UI and prepare M3 package integration`
- 专业中文：`docs: 完成音量设置界面验收并准备M3开发包集成`

本地f363841中的实现不重复Stage。Artifacts证据 / 包 / 候选 / 个人ini与缓存留本地，Stage后继续修改需重新Stage → Review → Commit / Push由用户操作；助手未核验远端。
