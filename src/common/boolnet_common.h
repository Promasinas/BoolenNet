/*
 * boolnet_common.h — BoolNet 框架公共定义
 *
 * 定义全框架共享的类型、枚举和错误码。
 * 所有 BoolNet 模块均应包含此头文件。
 */
#ifndef BOOLNET_COMMON_H
#define BOOLNET_COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- Layer UID ---- */

/** 全局唯一层标识符 */
typedef uint32_t LayerUID;

/* ---- 精度类型 ---- */

/** Memory 层精度枚举 */
typedef enum {
    ML_PRECISION_UINT8  = 0,
    ML_PRECISION_UINT16 = 1,
    ML_PRECISION_UINT32 = 2,
    ML_PRECISION_UINT64 = 3
} MemLayerPrecision;

/** 获取精度对应的 C 类型大小（字节） */
#define ML_PRECISION_SIZE(p) \
    ((p) == ML_PRECISION_UINT8  ? 1 : \
     (p) == ML_PRECISION_UINT16 ? 2 : \
     (p) == ML_PRECISION_UINT32 ? 4 : \
     (p) == ML_PRECISION_UINT64 ? 8 : 0)

/** 获取精度对应的最大值 */
#define ML_PRECISION_MAX(p) \
    ((p) == ML_PRECISION_UINT8  ? UINT8_MAX  : \
     (p) == ML_PRECISION_UINT16 ? UINT16_MAX : \
     (p) == ML_PRECISION_UINT32 ? UINT32_MAX : \
     (p) == ML_PRECISION_UINT64 ? UINT64_MAX : 0)

/* ---- 错误码 ---- */

#define ML_OK                    0   /**< 成功 */
#define ML_ERR_NULL_PARAM       -1   /**< 空指针参数 */
#define ML_ERR_INVALID_LENGTH   -2   /**< 长度为 0 */
#define ML_ERR_INVALID_PRECISION -3  /**< 无效精度类型 */
#define ML_ERR_ALLOC_FAILED     -4   /**< 内存分配失败 */
#define ML_ERR_FILE_OPEN        -5   /**< 文件打开失败 */
#define ML_ERR_FILE_READ        -6   /**< 文件读取失败 */
#define ML_ERR_FILE_WRITE       -7   /**< 文件写入失败 */
#define ML_ERR_CORRUPTED_DATA   -8   /**< 数据格式损坏 */

#endif /* BOOLNET_COMMON_H */
