# M2-03：三段轻攻击整章验收

2026-10-16。**A / B / C全部通过，无待补项。**玩家轻攻击已从单段扩展为一个GAS能力拥有的1→2→3，单槽.25世界秒输入缓冲、每段独立命中去重，以及命中结束后的正式方向闪避取消成立。单击 / 长按只一段，第3段不排第4段；玩家每段25、敌人仍单段10，轻攻击不消耗体力，成功闪避只扣一次25。

## 1. 分批结果与范围

| 批次 | 交付与证据 | 用户手动 / 明确等待 |
| --- | --- | ---: |
| [A素材](M2_03A_Verification.md) | 两套独立LS / AS / Montage，姿势、烘焙、Quinn预览、命中通知与短基线 | 50 / 10分钟 |
| [B连段](M2_03B_Verification.md) | 独立玩家AM01，三段 / 有限缓冲 / 每段Serial；Flow与Invulnerability Success、真实三次各25 | 50 / 5分钟 |
| [C恢复取消](M2_03C_Verification.md) | 恢复查询 / 预检后GAS取消交接；三项Success与实机取消 / P / 死亡Retry，编译补修关闭 | 40 / 5分钟 |
| 合计 | M2-03完成 | **140 / 20分钟** |

当前**100Source / 53cpp / 58项来源**；相对M2-02的96 / 50 / 51，本章新增4份Source（Notify h/cpp与两测试cpp），修改8份既有Source，新增7份UE资产（A六素材、B独立AM01）。原伤害GE / 无敌路径、Trace、RMS Task、UI、输入映射与模块依赖复用；没有新的常驻Tick组件或第二套伤害系统。

## 2. 关键设计与复盘入口

- **一个能力拥有整条链。**段间保持Attacking；每段Montage Task / 实例、段代次、Trace Serial独立，结束统一释放，避免三个分别激活的能力间出现意外空闲窗口。
- **输入只存一条且会过期。**活动玩家请求交给同一实例；窗口前输入最多等.25世界秒，窗口内消费一次。连点刷新一条，不排成长队；菜单输入不入队，暂停前合法输入随世界时间冻结。
- **旧回调要有身份。**Notify按Mesh / Montage实例 / Token过滤，Timer按激活 / 段代次过滤；结束Task先完成正常解绑再撤销其旧Ability引用，防同名Montage新激活受到动画队列旧委托影响。SetTimerForNextTick只保证延后调度，不承诺跨一个渲染帧。
- **恢复取消先预检。**从当前唯一命中通知结束读取恢复起点，到自动淡出前截止；第3段也可取消。Dodge成本 / 地面 / 配置等检查通过，GAS才取消原攻击；失败保留原链。交接后底层准备失败不复活旧动作，已付款取消不退款。

源码阅读入口：`ETGA_LightAttack`的TryBufferComboInput / HandleComboWindow / StartStage / ClearStageResources / CanCancelIntoDodge；`ETGA_Dodge`的CanActivateAbility / HasValidRecoveryCancelPolicy / ActivateAbility；`ETANS_ComboWindow`只转发；两个Combo测试呈现真实时序与失败分支。技术说明与所有权边界见[Architecture](../Architecture.md)。

两处C编译问题也保留：double世界时间配float快照导致TestEqual歧义；局部替换遗漏暂停推进声明造成C2065。最终已改double并恢复实际Advance，三测试成功；不删断言凑通过。B旧共享Montage虽重存，导出数据与旧LFS对象相同，只更新包头 / 缩略图区差异，报告没有虚称文件SHA保持。

## 3. 覆盖边界与下一项

运行证据分层：自动化使用保存的真实Montage / Notify / Sweep / GE / RMS，并移动目标靠近手部隔离几何；正式PIE另有三次25与三段恢复取消、Pause / 受击 / 死亡 / Retry / Stop记录，具体输入 / 画面与最终配置结合用户反馈。没有为缺少每个按键独立日志重做旧矩阵，固定步长不当作硬件FPS或性能结论。

动作仍为当前占位制作质量，后续视觉打磨不等于本章代码未完成；死亡冻结、3 / 4诊断旁路、单机与键鼠范围沿既有约定。旧M1 C3 Attempt02未重Cook，不能用旧包演示M2新连段，也不宣称最终投递作品已完成。

M2-01 + 02 + 03登记手动**380分钟（6小时20分钟）** / 等待50；全项目2391 / 明确235。理解 / 动画 / 检查未逐项拆时，助手投入未量化，保留M2整体65h基础与后续预算，不按快速录入速度推算研发成本。下一M2-04有限次数治疗将先定义次数归属、成功生效点、恢复量与取消消费，再分批下发；当前没有新类待建或额外旧功能验收门槛。

## 4. 本次 Commit / Push（固定放在最后）

一新 / 七旧完整源码及C指南 / B报告等已由用户纳入efaae3e，初查的新测试暂存骨架问题已关闭。当前按[C报告末尾清单](M2_03C_Verification.md#5-本次-commit--push固定放在最后)Stage C / 整章报告、来源58项最新说明与路线 / 进度 / 设计文档，不重复制造Source / Content差异；Raw / 生成文件 / 历史包留本地。

最终HEAD / 缓存origin/main均efaae3e，远端本轮未联网验证；用户在CLion检查Staged Diff后Commit / Push最新收尾，助手不操作索引或上传。本章完成不代表整个M2或最终求职作品完成。
