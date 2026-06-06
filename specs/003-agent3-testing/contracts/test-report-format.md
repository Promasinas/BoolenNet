# Contract: Test Report Format (Agent 3 Output)

**Producer**: Agent 3
**Location**: `./docs/test/uid_NNN_report.md`
**Consumer**: Agent 1 (查阅完整结果), Agent 2 (获取反馈以修复代码)

## Schema

```markdown
# Test Report: {Module Name}

## Module Info

- **UID**: {N}
- **Module**: {module_name}
- **Source**: {src_path}

## Test Time

- **Start**: {YYYY-MM-DD HH:MM:SS}
- **Duration**: {compile_time_ms + run_time_ms} ms

## Compile Result

- **Status**: {PASS | FAIL | TIMEOUT}
- **Duration**: {compile_time_ms} ms
- **Compiler**: GCC {version}

### Compile Output

```text
{compile stdout/stderr}
```

## Run Result

- **Status**: {PASS | FAIL | TIMEOUT}
- **Duration**: {run_time_ms} ms
- **Exit Code**: {exit_code}

### Run Output

```text
{test run stdout/stderr}
```

## Summary

- **Total Tests**: {total}
- **Passed**: {pass_count}
- **Failed**: {fail_count}
- **Pass Rate**: {percentage}%

## Failed Tests

<!-- 仅在存在失败时出现此节 -->

| Test Case | Input | Expected | Actual | Difference |
|-----------|-------|----------|--------|------------|
| {name} | {input} | {expected} | {actual} | {diff} |

## Errors

<!-- 仅在出现异常时出现此节（编译错误/超时/系统错误） -->

{error_description}
```

## Field Constraints

| 字段 | 何时存在 | 格式 |
|------|----------|------|
| `## Failed Tests` | `fail_count > 0` | Markdown 表格 |
| `## Errors` | 出现编译错误/超时/系统错误 | 纯文本描述 |
| `## Compile Output` | 始终存在 | 代码围栏 ` ```text ` |
| `## Run Result` | 编译通过后 | 含 Exit Code |

## Generation Rules

1. 文件名 = `uid_{NNN}_report.md`（NNN 为零填充 3 位 UID）
2. 所有时间戳使用本地时间（Windows 系统时间）
3. Pass/Fail 统计从测试输出的 `PASS:` / `FAIL:` 行解析（约定测试代码按此格式输出）
4. 编译失败时，`## Run Result` 节不生成，`## Summary` 中 `Total Tests` = 0
