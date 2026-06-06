<!-- SPECKIT START -->
For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan
at: specs/003-agent3-testing/plan.md
<!-- SPECKIT END -->

# BoolNet — Distilled Project Knowledge

## Identity
Pure Boolean neural network framework. No backprop/gradients/floats. XOR routers + Tsetlin/CoordDescent + cascade trees.

## Architecture (7 modules in src/)
- **bool_router**: `out[i] = in[i] ^ bits[i]`, Tsetlin-trainable
- **conv1d_circular**: Boolean kernel bits, circular wrap, `flip_kernel_bit`
- **upsampling**: M parallel routers, cascade support
- **mem_int**: uint8/16/32/64, trigger→max_value, decay, save/load
- **mem_part**: Float partition storage, LRU, sparse trigger weighted-sum
- **boolnet**: UID-sorted layer registry, forward propagation, TENB save/load
- **tsetlin_train**: Tsetlin (random flip+reward) + CoordDescent (greedy scan) + TrainConfig epochs

## Key Numbers
- 84/84 unit tests pass
- LLM Classify v0.7: 16/16 (100%), 9-node cascade tree
- CoordDescent: 2 sweeps converge, 2500× faster than Tsetlin
- Min model: 62KB .exe, libc only, <1μs/layer inference
- Memory solves signal amp: router bit=1 → cell=max_value(255)

## Build (gcc, no make needed)
```
gcc -std=c99 -Isrc/common -Isrc/bool_router -Isrc/mem_int -Isrc/boolnet \
  -Isrc/tsetlin_train -Isrc/conv1d_circular -Isrc/upsampling \
  -o out.exe src.c src/bool_router/bool_router.c src/mem_int/mem_int_layer.c \
  src/boolnet/boolnet.c src/tsetlin_train/tsetlin_train.c \
  src/conv1d_circular/conv1d_circular.c src/upsampling/upsampling.c -lm
```

## Release versions (release/v0.X-name/)
v0.1(6%)→v0.2(route)→v0.5(100% 64QA)🏆→v0.7(100% classify)🏆→v0.9(200QA)🏆

## Agent Skills (see docs/agent_skills_distilled.md)
- **XOR闭式解**: `rb = input XOR target` — 固定映射最优, O(1) 全局最优
- **级联树**: D=ceil(log₂(N)), 每节点1次CMP, 叶子独立Router
- **Coord Descent**: O(2N)/sweep, 单模式可用, 多模式卡局部最小值
- **Memory触发**: Router学路由(去哪), Memory学放大(值多少)
- **数学限制**: XOR+Manhattan有局部最小值 → 需要同时多比特翻转
- **编码**: N-gram(≥3)去噪, one-hot分类避免XOR噪声
- **测试**: 每模块10-22函数, create/destroy/forward/save/load全覆盖

## Agent 3 Testing (test/uid_001-007/)
Per-function tests for each module. Run: `test/uid_XXX/t.exe`

## Critical Notes
- boolnet_forward buffer: walks layers for max dim (FIXED)
- CoordDescent local minima proven for XOR encoding → use routing trees
- All exes trackable via .gitignore: `!release/*.exe` and `!release/v*/*.exe`
