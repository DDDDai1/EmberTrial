# M0-05C2-03：闪避参数与位移接入记录

更新：2026-09-28。**C2-03A 验收通过，C2-03B 仅下发代码与操作文档，尚未实现 / Build / 验收。**

## 1. A：用户已完成

用户约 **10 分钟**：在 ETPracticeActionComponent.h 的 private 区域加入两个参数、原代码保留；关闭 UE 后正常 Build 成功；PracticeAction 可见 Dodge Distance / Duration，最终 150 cm / 0.30 s；原 Montage / Auto Activate 正确，蓝图编译通过；1/2/3 原地播放、受击打断与恢复、E/P 和基础输入正常；源码 / 资产已保存。

助手直接核对：

- 源码只增加两组 UPROPERTY 与 float，位置在 AttackMaxHorizontalSpeed 后、CurrentState 前，位于 private 区域。
- 距离 EditDefaultsOnly、范围 1–300、Units=cm、默认 150；持续时间 EditDefaultsOnly、范围 0.05–0.50、Units=s、默认 0.30，与指南一致。
- 最新 UBT 日志末尾为 `Target is up to date`、总耗时 1.57 秒。这是最新增量检查，不将它单独描述为一次全量重编译；正常 Build 成功以用户结果为据。
- 当前 EmberTrial.log 未保留本批的 Component ready / Begin 等 PIE 片段，未匹配到相关蓝图运行错误；不把“无日志片段”当成实机通过证据。具体功能结果采用用户反馈。
- 本轮 Git 中玩家蓝图未新增二进制差异，只有头文件和原有 DefaultEditor.ini 改动；继承新 C++ 默认值不必然产生蓝图磁盘差异，不因此否定用户的保存结果。

## 2. B：明确的实现边界

用户最新要求：**后续所有 C++ 编写 / 修改均输出到文档，由其手动操作，直到明确要求助手直接操作。** 本约定已写入 Introduction v1.2、Progress 与 IssueTemplate。

已下发 [C2-03B 手动接入指南](../M0_05C2_03B_DodgeMovement.md)：四个私有辅助函数、位移源所有权、请求校验、统一退出清理和 Montage 淡出中断。保持 A 的两个配置，目标为自身右侧 150 cm / 0.30 秒，位移窗口与 Montage 状态时长分开。

本轮只读取了本机 5.5.4 的相关引擎声明 / 实现，核对 Apply / Remove / GetByID、部分结束帧、Finished 标记、ClampVelocity 与淡出委托签名；**助手没有修改工程 C++ 或 UE 资产，没有构建新代码，没有运行新位移测试。**

## 3. 待验证

- 用户完成 .h / .cpp 编辑，正常 Build，原组件配置与蓝图 Compile。
- 平地自身右侧距离和收尾、墙壁 / 斜墙 / 可走坡面、打断 / 暂停 / 三次 PIE、不同实际帧率和 C2-02 回归。
- 已有 Lifecycle 自动测试在新实现下重新运行；其范围仍是生命周期，不能代替真实位移测试。
- 位移专项自动测试、C3 四动作组合记录与首次打包另行推进。自动测试的 C++ 同样通过文档交给用户手动实现。

结果未返回前，不把 C2-03 总任务标为完成。
