# M7-05B-2 界面重设计验收

**通过。** 用户于 2026-11-19 确认第 5 节各项完成，总工时 **120 分钟**。两份菜单导航源码已随 `ca16ce7` 补交。[设计与验收矩阵](../M7_05B_2_MenuPauseRelicRedesign.md#5-最终验收与反馈)

| 检查 | 结果 |
| --- | --- |
| 实机画面与焦点 | 用户确认主菜单、暂停、祭坛在 **1920×1080** 与 **1280×720** 的实机图、字体、色彩、窗口尺寸与焦点检查完成。 |
| 主菜单与设置 | 用户确认继续、新试炼、返回及显示预览回滚通过。助手另在 PIE 核对主菜单设置切页与返回。 |
| 暂停与遗物 | 用户确认普通暂停只读、祭坛装卸、长文本及固定操作区通过。助手另在 PIE 核对普通暂停的装卸区隐藏、祭坛比较数据为真实快照及返回流程。 |
| 胜败页 | 用户确认胜败页与 Compile、Save、Validate 通过。 |
| 构建与自动化 | Editor 和 Win64 Development 构建通过。Settings 四项及 Relics 三项共 **7 Success**。8 份 WBP 与 4 份美术资产校验 **12 Valid、0 Invalid、0 Warnings**。 |
| 取景与字体 | 背景取自正式路线，保留 1920×1080 原图。思源宋体标题保留原字体和 OFL 许可证。[来源清单](../AssetManifest.csv) |

用户完成了两种窗口尺寸的实机取图。反馈未填写这些截图的具体路径。另有助手开发复验图 `Artifacts/M7_05B_2/LiveHome.png`、`LivePause.png`、`LiveAltar.png` 与 `LiveSettings.png`，以及 `Smoke.log`、`Verify.log` 和 `RegressionFinal.log`。

## 提交核对

M7-05B 对照基线为 `8784016`。初版重设计提交 `cfd45c3` 包含八份 WBP、四份 UI 美术资产、背景 PNG、字体 TTF 和 OFL 文本。首次验收时，十二份 UE 资产的本机 SHA-256 与该提交里的 Git LFS OID 一致。

两个菜单使用的 `ETMenuPagesWidget.h` 和 `ETMenuPagesWidget.cpp` 已随 `ca16ce7` 提交。后续三份布局资产随 `2baa408` 提交，本机 SHA-256 与其 Git LFS OID 相符。最终布局数值随 `26832f0` 更新到原指南。以上提交均在本地 `origin/main` 的祖先记录中。

## 本次文档 Commit / Push

初版报告已随 `4b333d0` 提交。本次同步补交状态与发布候选资料。[当前提交清单](../M7_07_ShippingCandidate.md#4-commit--push)
