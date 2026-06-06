# Quickstart: Agent 3 — 自动化测试

## 前置条件

1. **Windows** 操作系统（项目目标平台）
2. **MinGW** 或 **MSYS2** 已安装，`gcc` 和 `make` 在 PATH 中
3. **Git** 已安装并初始化（项目仓库已就绪）
4. `./src/` 目录下存在至少一个 .c 源文件
5. Agent 2 可通过 `./docs/interact/` 发送测试请求

## 编译 Agent 3

```bash
# 在项目根目录下
cd ./src/
make agent3

# 或手动编译
gcc -Wall -Wextra -std=c11 -o ../agent3.exe \
    agent3/main.c \
    agent3/scanner.c \
    agent3/pipeline.c \
    agent3/makefile_gen.c \
    agent3/process_mgr.c \
    agent3/git_mgr.c \
    agent3/reporter.c \
    agent3/config.c \
    agent3/utils.c
```

## 启动 Agent 3

```bash
# 在项目根目录下，以后台进程方式运行
./agent3.exe

# 或指定轮询周期（秒）
./agent3.exe --poll-interval 30

# 或指定并发上限
./agent3.exe --max-concurrent 4
```

## 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--poll-interval N` | 60 | 轮询周期（秒），最小 10 |
| `--max-concurrent N` | CPU 核心数 | 并行测试流水线上限，最小 1 |
| `--once` | - | 执行一次检测-测试循环后退出（调试用） |
| `--verbose` | - | 输出详细日志 |

> **注意**: 测试执行超时无默认值——Agent 2 MUST 在 `## Test Params` 的 `timeout` 字段中指定超时秒数。Agent 3 拒绝缺少此字段的指令。

## Agent 2 如何发送测试请求

在 `./docs/interact/` 目录下创建文件 `test_request_uid_001.md`：

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
```

Agent 3 在下一轮轮询（≤60 秒）时检测到此文件并启动测试。

## 查看测试结果

测试完成后：

1. **通知文件**: `./docs/interact/test_done_uid_001.md` — 快速状态摘要
2. **详细报告**: `./docs/test/uid_001_report.md` — 完整测试结果

```bash
# 查看通知
cat ./docs/interact/test_done_uid_001.md

# 查看详细报告
cat ./docs/test/uid_001_report.md
```

## 停止 Agent 3

```bash
# 优雅停止（发送 SIGTERM）
# Agent 3 等待当前活跃流水线完成后退出
taskkill /PID <agent3_pid>

# 强制停止
taskkill /F /PID <agent3_pid>
```

## 目录初始化

Agent 3 首次运行时自动创建所需的目录结构：

```text
./test/              # 由 Agent 3 创建
./docs/test/         # 由 Agent 3 创建
./docs/interact/     # 应已存在（由 Agent 1 创建）
```

## 故障排查

| 问题 | 检查项 |
|------|--------|
| Agent 3 不检测新文件 | 确认 `./src/` 路径正确；检查文件扩展名为 `.c` 或 `.h` |
| 编译失败 | 检查 MinGW/GCC 在 PATH 中；确认 Makefile 模板正确 |
| Git 提交失败 | 确认 Git 已配置 user.name/user.email；检查无合并冲突 |
| 并行测试资源不足 | 减小 `--max-concurrent` 参数 |
