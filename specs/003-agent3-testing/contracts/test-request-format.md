# Contract: Test Request Format (Agent 2 → Agent 3)

**Direction**: Agent 2 → Agent 3
**Location**: `./docs/interact/test_request_uid_NNN.md`
**Purpose**: Agent 2 请求 Agent 3 对指定模块执行测试

## Schema

```markdown
# Test Request: {Module Name}

## Target UID

{N}  <!-- 必填。正整数，模块的 Layer UID -->

## Target Module

{name}  <!-- 必填。模块名称，如 bool_router -->

## Test Priority

{high|medium|low}  <!-- 必填。测试优先级 -->

## Test Params

- timeout: {seconds}  <!-- 必填。测试执行超时上限，单位秒，正整数 -->
- cflags: {flags}     <!-- 可选。额外的编译标志，空格分隔 -->
```

## Field Constraints

| 字段 | 约束 | 缺失行为 |
|------|------|----------|
| `## Target UID` | 正整数，匹配 `./src/` 中实际存在的模块 | 拒绝，记录解析错误 |
| `## Target Module` | 非空字符串，≤64 字符 | 拒绝，记录解析错误 |
| `## Test Priority` | 枚举：`high`、`medium`、`low` | 拒绝，记录解析错误 |
| `timeout` | 正整数，≥1，≤86400（24小时） | 拒绝，记录解析错误 |
| `cflags` | 可选，合法 GCC 标志字符串 | 缺失时默认 `-Wall -Wextra` |

## Parsing Rules

1. Agent 3 按 Markdown 标题层级（`## `）分割字段
2. 字段名大小写不敏感（`## target uid` ≡ `## Target UID`）
3. `## Test Params` 下的列表项用 `- key: value` 格式解析
4. 未知标题静默忽略（向前兼容）
5. 解析失败时，Agent 3 在日志中记录原始文档内容和失败原因

## Example

```markdown
# Test Request: Bool Router

## Target UID

1

## Target Module

bool_router

## Test Priority

high

## Test Params

- timeout: 60
- cflags: -Wall -Wextra -O2
```
