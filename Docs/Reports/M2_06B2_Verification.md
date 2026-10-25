# M2-06B2：镜头跟随与右肩补修验收

2026-10-24，UE5.5.4。**B2基础跟随及右侧越肩补修已完成。**原用户14:24报告手动80+检查10 / 等待5；后续用户明确授权“右侧的越肩视角，你直接修改操作”，并指定可参考《黑神话：悟空》等动作游戏。助手本次直接改代码、正常构建 / 刷索引、跑受影响专项并操作UE核对取景；未进行Git Stage / Commit / Push，未进入C的目标切换业务。

## 1. 变更与所有权

基础B2三新 / 两旧由用户提交63abd20，原CameraRules14:18:10.595 Success；随后报告近景同轴遮挡。用户已经手动写入未提交的YawOffset25 / Pitch-25补修，助手先在忽略Raw保留三文件原样，再改为真正右肩偏移。

本轮仍只修改**ETTargetLockCameraComponent.h / .cpp及ETTargetLockCameraTests.cpp**三个既有Core文件。实际156 Source / 85cpp，Core59 / UI23 / Editor3，22项自动化，来源73 / 本地292份UE资产；其他Source / Config保持。BP Controller中继承的参数已通过computer-use逐项核对并执行Compile / Save，该资产实际变化并同步清单；其余291份旧UE资产与本轮开始一致，不制造额外重存。

右肩通过SpringArm SocketOffset局部(0,75,55)cm；目标中心上方45cm为注视点，Pitch / Yaw附加偏角均0、速度8、期望Pitch -45至15、原臂长400不变。解析补偿侧移 / 抬高视线，不用上一帧相机位置反馈，不改角色 / GA方向。保存取得时的原Socket值，逐帧趋近原值+配置，不反复叠加；主动解除归还鼠标并平滑回正，失活 / 换身体 / EndPlay精确归还并清弱引用。Pause冻结镜头 / 回正，无新增Tick / Timer，Pawn观察成对解绑。

这些参数为本项目当前设定，风格参考用户指定动作游戏，不是取得或复刻商业游戏内部参数。本轮没有导入第三方图像 / 模型或新增资产授权。

## 2. 正常构建与专项

反射 / Editor Build成功，正常Refresh-CLion已刷新三模块元数据；Game Development成功链接。工具记录Editor总55.48秒、Game总61.50秒，两次均ExitCode0；运行与构建来自本机UE5.5，未使用Live Coding。首次普通沙箱无法执行引擎批文件，随后正常权限执行成功，未绕过自动审批或删缓存。

15:01:15.763 CameraRules、15:01:15.833 Selection、15:01:15.880 MarkerRules均**Success**；本地Automation/index.json为succeeded3 / warnings0 / failed0 / notRun0，进程ExitCode0。其余19项沿用，不宣称22项当次全组执行。

CameraRules保留原角度跨界 / 有限数值 / 30-60-120 / Pitch与拒绝规则，追加右肩下玩家左置 / 目标居中补偿、极近有限值 / 非法Socket，以及真实测试World中不累加、平滑归还、Deactivate、Possess旧臂恢复、新身体隔离、Pause冻结与EndPlay精确还原。不创建Viewport，不把单位测试当作画面结果。

## 3. computer-use与真实画面

使用已安装computer-use技能操作UE5.5，现有Controller中观察到唯一继承Camera，8 / -45 / 15 / 0 / 0、Shoulder0 / 75 / 55、Focus45 / AutoActivate正确；Compile显示Good to go并执行Save。实际模式为**Selected Viewport**。

出生点距敌人略超原获取 / Sight范围，先用只读阵营 / 胶囊位置查询确认，再在PIE临时Teleport和设ControlRotation取得无遮挡观察位，没有修改地图、碰撞或阵营。电脑工具短Tab未形成可靠Started样本，镜头检查改用已有ke正式ToggleTargetLock入口；不把它记作新的物理Tab验收，输入沿用原用户反馈且源码 / 映射未改。

15:11:11.425真实锁定BP_ETMeleeEnemy_C_1，看到玩家偏左、敌人身体 / 头部与动作空间清晰、菱形正常。P显示暂停，Enter继续后保持同一右肩取景；15:14:06.332组件正式请求解除，Timer0、标记停止，画面肩位回正 / 鼠标归还。场景临时slomo .05只为便于观察，15:14:06.333已恢复slomo1，随后Shift-F1 / Stop结束PIE，临时位置不保存。

原生HighResShot截图15:12:55生成，**1359×754**，已保存在`Artifacts/M2_06B2_RightShoulder/RightShoulder_PIE.png`；它是引擎场景截图，不含UMG / 锁定标记，UI是否存在另由computer-use画面与日志支持。未对截图编辑。本次未重新搭墙边 / 伤害 / Retry全矩阵，SpringArm原碰撞路径保留，原用户B2相关检查沿用；碰撞优先时仍可能出现短暂遮挡，不宣传所有场景完美。

## 4. 工时与收尾

原用户B2手动90 / 等待5已经计入：M2-06为270 / 15，M2为1340 / 119，项目3351分钟（55小时51分）/ 明确等待304分钟（5小时4分）。本次助手操作和工具构建时间独立记录，不冒充用户新增手动工时，不重复原90。主观远近 / 偏移仍可由后续试玩微调；当前请求的右肩取景、恢复与验证已完成，未新Cook / 采性能或生成C。

## 5. 当前 Commit / Push（固定放在最后）

用户明确授权后，助手已直接完成右肩锁定镜头：实际修改三个既有Core文件（ETTargetLockCameraComponent.h / .cpp、ETTargetLockCameraTests.cpp），Editor / Game Build与三项相关自动化通过，UE中核对组件 / Compile-Save和取景 / Pause / 解除检查完成。HEAD仍63abd20，尚未提交本轮修改；Stage这三份Source、[B2指南](../M2_06B2_LockCamera.md) / 本报告及最新状态 / 架构 / 路线 / 清单。同时Stage本次Compile-Save实际改变的BP_ETPlayerController.uasset；其余未变资产不重复Stage。建议Commit（英文）：`fix: add a right-shoulder lock-on camera`。中文译文：`fix: 新增右肩锁定镜头与偏移恢复`。Raw / 日志 / 截图 / Saved / 索引 / 包体留本地；Git由用户操作，助手未Stage、Commit / Push或联网核验远端。
