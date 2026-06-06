# BoolNet 模型权重目录

## 目录约定

所有训练产出（模型权重、Memory 状态、检查点）统一存放在此目录。

## 文件命名规范

| 类型 | 命名格式 | 示例 |
|------|---------|------|
| 最终模型 | `{network_name}_model.bin` | `deep128_model.bin` |
| 最佳模型 | `best_model.bin` | `best_model.bin` |
| 检查点 | `ckpt_{epoch:04d}.bin` | `ckpt_0050.bin` |
| Memory 状态 | `memory_{uid}.bin` | `memory_005.bin` |
| 训练器状态 | `trainer_{name}.bin` | `trainer_xor.bin` |

## 二进制格式

### 模型文件 (magic: `TENB` = 0x424E4554)

```
[magic:4B] [version:4B] [num_layers:4B] [input_dim:4B] [output_dim:4B]
每层:
  [type:4B] [uid:4B]
  Router: [num_bits:4B] [bits: ceil(num_bits/8) bytes]
  Conv1D: [input_length:4B] [kernel_size:4B] [stride:4B] [dilation:4B] [kernel_bits: ceil(K/8) bytes]
  Memory: [precision:1B] [length:4B] [max_value:8B] [decay:8B] [cells: length*sizeof(type) bytes]
```

### Memory 状态文件

```
[uid:4B] [precision:1B] [length:4B] [max_value:8B] [decay:8B] [cells: length*sizeof(type) bytes]
```

## 读取模型

```c
// 使用 tsetlin_train.h 中的导出/加载函数
#include "../src/tsetlin_train.h"

// 保存
tsetlin_export_model(net, "weights/my_model.bin");

// Memory 状态
mem_int_save(memory_layer, "weights/memory_001.bin");
MemIntLayer *loaded = mem_int_load("weights/memory_001.bin");
```

## Agent 使用说明

- **Agent 1 (设计)**: 指定网络名称和权重文件路径
- **Agent 2 (实现)**: 调用 `tsetlin_export_model()` 将训练好的权重保存到此目录
- **Agent 3 (测试)**: 从此目录加载权重进行推理验证
