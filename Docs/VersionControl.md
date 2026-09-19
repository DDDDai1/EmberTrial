# 本项目的 Git / LFS 基线

Git 仓库根目录：`E:\UE_Project\Demo`。UE 工程位于其中的 `EmberTrial` 子目录。

## 1. 已配置内容

- `.gitignore` 排除编译产物、Intermediate、Saved、缓存和 IDE 临时文件；这些文件可重新生成。
- `.gitattributes` 将 `.uasset` / `.umap` 交给 Git LFS，并为后续 Blender / FBX 等源资产预留规则。
- 源码、Config、`.uproject`、`.vsconfig`、文档和实际 Content 进入版本管理。
- World Partition 的 `__ExternalActors__` / `__ExternalObjects__` 也是地图内容的一部分，已保留。
- 当前没有远端，没有上传。这里只是本地历史，不是异地备份。

## 2. 第一个可恢复检查点

`3562f61`：用户创建并验证的 UE 5.4.4 模板工程，作为迁移前检查点。

初始提交包含 237 个文件，其中 212 个 UE 资产由 LFS 管理；LFS 完整性检查通过。模板原有的个别尾随空白予以保留，没有为了格式重写用户的模板代码。

迁移后的编译通过检查点为 `861be2b`，保留版本变化的清晰差异。提交号和验证结果记录到 Progress 与迁移报告。

## 3. 你需要知道的日常规则

1. UE 中修改资产后先保存，Git 才能记录磁盘上的结果；未保存的蓝图不在提交中。
2. 不手工改二进制资产内容；同一个地图或蓝图避免双方同时修改。
3. 不把 `.git` 当作可清理的生成目录；只复制源码而漏掉 LFS 对象，不能保证保留完整历史资产。
4. 大型 Trace、视频、dump 与试玩包另行归档；小型证据摘要放在 Docs/Reports。
5. 需要恢复某个版本时告诉我提交号和目标；先保留当前改动，再选择独立副本或其他合适方式验证，不盲目 reset 或覆盖当前工作区。

## 4. 可选的只读查看命令

在工作区打开终端后，可用：

```powershell
git status --short
git log --oneline -5
git lfs ls-files
```

`git status` 看哪些文件变化，`git log` 看提交历史，`git lfs ls-files` 看哪些二进制文件由 LFS 管理。你当前不需要额外执行这些命令才能继续练习。
