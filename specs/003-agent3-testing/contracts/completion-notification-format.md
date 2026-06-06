# Contract: Completion Notification Format (Agent 3 → Agents 1 & 2)

**Direction**: Agent 3 → Agent 1, Agent 2
**Location**: `./docs/interact/test_done_uid_NNN.md`
**Purpose**: Agent 3 通知其他 Agent 测试已完成，提供状态摘要

## Schema

```markdown
# Test Complete: {Module Name}

## Status

{completed|error}  <!-- completed = 测试流程完整执行; error = 发生不可恢复错误 -->

## Module

uid_{NNN} ({module_name})

## Result

{pass|fail|compile_error|timeout}

## Summary

- Total: {total} tests
- Passed: {pass_count}
- Failed: {fail_count}
- Duration: {total_duration_ms} ms
- Report: ./docs/test/uid_{NNN}_report.md
```

## Result Values

| 值 | 含义 | 对应报告状态 |
|----|------|-------------|
| `pass` | 编译通过，所有测试通过 | Compile: PASS, Run: PASS |
| `fail` | 编译通过，存在测试失败 | Compile: PASS, Run: FAIL |
| `compile_error` | 编译失败 | Compile: FAIL/ERROR |
| `timeout` | 编译或运行超时 | Compile: TIMEOUT 或 Run: TIMEOUT |

## Consumption Pattern

1. **Agent 2** 轮询 `./docs/interact/` 目录：
   - 查找文件名匹配 `test_done_uid_*.md`
   - 读取 `## Result` 字段
   - 若 `fail`/`compile_error` → 读取对应的详细报告（`## Summary` 中指定的路径）以定位需要修复的代码
   - 若 `pass` → 标记该模块测试完成

2. **Agent 1** 查阅 `./docs/test/` 目录获取完整报告：
   - 读取详细测试报告以汇总项目整体测试状态
   - 向用户反馈各模块测试结果

## Cleanup

Agent 3 不负责清理 `test_done_uid_NNN.md` 文件。下游 Agent（Agent 2）在读取并处理后可自行删除或归档。
