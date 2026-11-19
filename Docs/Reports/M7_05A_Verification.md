# M7-05A 战斗 HUD 验收

**通过。** 用户于 2026-11-17 23:48 完成验收，手动 **30 分钟**，构建等待 **5 分钟**。[操作指南](../M7_05A_CombatHUDPolish.md)

| 检查 | 结果 |
| --- | --- |
| 构建 | 用户确认 Editor Build、Game Development Build 和三模块索引完成。两份装配源码已随 `2c55ea0` 提交。 |
| HUD 配置 | 回读四份 Designer 和编译后 WidgetTree。左下锚点、四卡坐标、背景、字号、三条高 8 的进度条与原绑定正确。资产校验 **4 Valid、0 Invalid、0 Warnings**。 |
| 诊断开关 | 回读 Controller，只有一个 UIBootstrap，`Show Lifecycle Status=False`，五个 Widget Class 配置齐全。用户确认临时打开后面板可见，恢复关闭并重新 Play 正常。日志记录 23:45:48 的诊断面板创建。 |
| 普通与大视口 | 此前重叠已定位到旧会话持有旧模板。重启后由助手在用户会话中复验普通内嵌和 F11 大视口，四卡间隔与文字正常。用户本轮确认常用窗口及小窗均无裁剪或遮挡。 |
| 实战与输入 | 用户确认生命、闪避体力、Q 次数、F 冷却、P 冻结恢复、E 提示和 Tab 标记正常，Stop 后新一轮正常。 |
| 自动化 | `EmberTrial.UI.ResourceHUD.Formatting` 于 **23:46:21** 返回 **Success**。`EmberTrial.UI.BreakPulse.Lifecycle` 于 **23:46:36** 返回 **Success**。 |

本轮反馈未填写小窗的具体像素尺寸和新增截图路径，尺寸检查按用户确认记录。已有 `Artifacts/M7_05A/CombatHUD_PIE.png` 为 1280 × 720 的编辑器窗口截图，现场 F11 大视口图由 computer-use 返回。证据还包括 `M7_05A_Acceptance.log`、`BootstrapFinal.json`、`VerifyFinalHUDAndMenuTrees.log` 与 `UserSessionRecheck.log`。

四个 HUD WBP、Controller 和两份源码已随 `2c55ea0` 提交。五份二进制文件的本机 SHA-256 与该提交的 Git LFS OID 一致，来源清单已同步。

## 本次文档 Commit / Push

暂存本报告、`Docs/M7_05A_CombatHUDPolish.md`、`Docs/AssetManifest.csv`、`Docs/Progress.md`、`Docs/Backlog.md` 和 `Introduction.md`。下一章指南可以一并暂存。

- 英文 `docs(ui): verify combat HUD and plan menu polish`
- 中文 `docs(ui): 记录战斗 HUD 验收与菜单界面打磨方案`
