# 权重文件约定 — 全 Agent 通知

## Target: Agent 1, Agent 2, Agent 3

## 变更内容

所有模型权重、Memory 状态、训练检查点统一迁移至 `weights/` 目录。

## 各 Agent 职责

### Agent 2 — 代码生成

- 训练完成后调用 `tsetlin_export_model()` 输出模型到 `weights/` 目录
- 命名规范: `{module_name}_model.bin`
- 检查点自动保存至 `weights/ckpt_{epoch:04d}.bin`
- Memory 状态保存至 `weights/memory_{uid}.bin`

### Agent 3 — 自动化测试

- 从 `weights/` 目录读取模型文件进行推理测试
- 验证模型文件格式完整性 (magic: TENB)
- 输出测试报告到 `docs/test/`

### Agent 1 — 设计主控

- 指定网络架构对应的权重文件路径
- 汇总训练结果

## 目录结构

```text
weights/
├── README.md              # 目录说明与格式文档
├── best_model.bin         # 最佳模型（自动保存）
├── {name}_model.bin       # 命名模型
├── ckpt_{epoch}.bin       # 训练检查点
└── memory_{uid}.bin       # Memory 层状态
```

## 文件格式

参见 `weights/README.md` 中的二进制格式定义。

## 加载示例

```c
// Agent 3 推理加载
BoolNet *net = /* build same architecture */;
// 然后使用 boolnet_load() 或逐层加载权重
MemIntLayer *mem = mem_int_load("weights/memory_005.bin");
```
