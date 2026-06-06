# Contract: Generated Makefile Contract (Agent 3 → Build System)

**Producer**: Agent 3
**Location**: `./test/uid_NNN/Makefile`
**Consumer**: GNU Make (MinGW/MSYS2 on Windows)

## Required Targets

### `all` (default)

编译测试可执行文件。

```makefile
.PHONY: all
all: $(TARGET)
```

**行为**:
- 编译所有测试源代码和被测模块源代码
- 链接生成测试可执行文件 `test_uid_NNN.exe`
- 返回值：0 = 成功, 非0 = 编译失败

### `clean`

清理编译产物。

```makefile
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
```

**行为**:
- 删除所有 .o 文件
- 删除测试可执行文件
- 不应删除源代码或文档

## Required Variables

| 变量 | 说明 | 来源 |
|------|------|------|
| `CC` | C 编译器 | `gcc`（项目约定） |
| `CFLAGS` | 编译标志 | 默认 `-Wall -Wextra -std=c11`，可由 Agent 2 通过 `cflags` 覆盖 |
| `TARGET` | 输出可执行文件 | `test_uid_NNN.exe` |
| `SRCS` | 源文件列表 | 测试 .c 文件 + 被测模块 .c 文件 |
| `OBJS` | 目标文件列表 | `$(SRCS:.c=.o)` |

## Invocation Contract

Agent 3 将通过以下方式调用 Makefile：

```c
// 编译
CreateProcess("make", "-C ./test/uid_NNN/ all", ...);

// 清理
CreateProcess("make", "-C ./test/uid_NNN/ clean", ...);
```

- 工作目录: `./test/uid_NNN/`（通过 `-C` 或 `CreateProcess` 的 `lpCurrentDirectory` 指定）
- 标准输出/错误: 重定向到管道，由 Agent 3 捕获
- 超时: 编译默认 300 秒，由 `## Test Params` 的 `cflags_timeout` 覆盖（若提供）

## Test Output Convention (Advisory)

测试代码（由 Agent 2 或开发者编写）SHOULD 按以下格式输出，以便 Agent 3 解析统计信息：

```text
[TEST] Running {test_name}...
[PASS] {test_name}
[FAIL] {test_name} - expected: {expected}, actual: {actual}
[TEST] Running {test_name}...
[PASS] {test_name}
...
=== TEST SUMMARY ===
Total: {N} | Passed: {P} | Failed: {F}
```

Agent 3 解析规则：
- `[PASS]` → 增加 pass_count
- `[FAIL]` → 增加 fail_count，记录期望值和实际值
- `=== TEST SUMMARY ===` → 可选，若存在则直接提取总数
