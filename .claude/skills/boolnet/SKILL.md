# BoolNet — Pure Boolean Neural Network Skill

Distilled project knowledge for AI agents working on BoolNet.

## Architecture

BoolNet is a pure Boolean neural network framework. No gradients, no floats. Just XOR routers, Tsetlin automata, and cascaded memory layers.

### Modules (Constitution IV–X)

| Module | Source | Function |
|--------|--------|----------|
| Boolean Router | `src/bool_router/` | Per-bit XOR: `out[i] = in[i] ^ bits[i]`. Tsetlin-trainable bit-flip |
| Circular Conv1D | `src/conv1d_circular/` | Boolean kernel, circular wrap. `kernel_bits` field (uint8) |
| Upsampling | `src/upsampling/` | M parallel routers → M×N concatenated output. Cascade mode |
| Integer Memory | `src/mem_int/` | uint8/16/32/64 cells. Router trigger→reset to max_value. Decay per forward |
| Forward/Topology | `src/boolnet/` | Layer registration by UID. Sequential propagation. Save/load full network |
| Tsetlin Training | `src/tsetlin_train/` | Bit-flip reward learning. Full pipeline (epochs, early-stop, checkpoint). Export/import TENB |
| Partition Manager | `src/mem_part/` | Float partition storage. LRU eviction. Sparse trigger weighted-sum |
| Simple LLM | `src/simple_llm/` | Router→Memory dialogue model |

## Key Design Principles

1. **Router routes, Memory amplifies**: Router decides WHERE (which cell to trigger), Memory decides VALUE (0 or 255). One router bit per memory cell.
2. **Router dimension = input dimension**: `bool_router_create(uid, num_bytes*8, bits)`. Always match byte dimensions.
3. **Memory output for classification**: `mem_int_forward_output` returns raw cell values (not binary mask). Use for output layer.
4. **Memory output for routing**: `mem_int_forward_layer` returns trigger mask. Use for hidden layers.
5. **Hidden Memory decay>0, Output Memory decay=0**: Enables state transitions and stable output.
6. **Pure XOR for text generation**: Memory cells only output 0/255. For arbitrary byte values, skip Memory — use direct XOR.

## Training Methods

### Coordinate Descent (Recommended)
- Deterministic: tests every bit, keeps improvements
- O(2N) forward passes per sweep. Converges in 1-3 sweeps
- For N<10K bits, converges in milliseconds
- Pure integer, no floats
- `coord_descent_train(net, input, target, byte_max, max_sweeps)`

### Tsetlin (Legacy)
- Random single-bit sampling. Probabilistic acceptance
- O(K) steps where K >> N. Slow for large networks
- `tsetlin_train_step(trainer, input, target)`

### Closed-Form XOR
- For fixed Q&A: `router_bits = input XOR target`
- Instant, perfect. Use for dialogue bots with known pairs

## Release Versions

| Version | Q&A | Accuracy | Method |
|---------|-----|----------|--------|
| v0.4 | 16 | 100% | XOR tree |
| v0.5 | 64 | 100% | N-gram + XOR tree |
| v0.7 | 16 tokens | 100% | one-hot + XOR |
| v0.9 | **200** | **100%** | CoordDescent + XOR |

All in `release/` directory. Each has source + exe + README.

## Build Commands

```bash
cd src/
make all              # All 8 modules
make mem_int          # Individual modules
make coord_descent    # Coordinate descent optimizer

# Standalone chatbot (no framework deps)
gcc -std=c99 -O3 -o boolchat.exe test/boolchat_xor.c -lm
```

## File Organization

```
src/           # Framework source (8 modules, ~5000 lines C)
test/          # Test programs (unit + integration)
release/       # Versioned releases (v0.1–v0.9)
weights/       # Model files (.bin, TENB format)
build/         # Build artifacts (.o, .exe) — gitignored
docs/          # Documentation
specs/         # Feature specifications
```

## Common Pitfalls

1. **Router bit count ≠ input byte count**: Router takes `num_bits`, not `num_bytes`. Use `N * 8`.
2. **Memory cell count**: Must match Router output bits for 1:1 mapping
3. **boolnet_forward buffer**: Now auto-sizes to max layer output. Fixed.
4. **memcmp vs memcmp**: Standard is `memcmp` (with 'c'). Check includes.
5. **Tsetlin 7-param stats**: `tsetlin_get_stats(t, &steps, &accepted, &best_r, &epochs, &best_val)`
6. **Memory layer type**: Use `LAYER_MEMORY` for Memory. Import `mem_int_layer.h`.
