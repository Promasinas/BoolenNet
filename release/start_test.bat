@echo off
chcp 65001 >nul 2>&1
title BoolNet Test Suite
echo.
echo ╔══════════════════════════════════════════╗
echo ║     BoolNet 测试套件                    ║
echo ╚══════════════════════════════════════════╝
echo.
echo 编译全部测试模块...
echo.

cd /d "%~dp0..\test"

REM 编译训练测试
echo [1/4] 编译训练测试...
gcc -std=c99 -O2 -I../src train_test.c ../src/bool_router/bool_router.c ../src/boolnet/boolnet.c ../src/mem_int/mem_int_layer.c ../src/tsetlin_train/tsetlin_train.c -o train_test.exe -lm 2>&1
if %ERRORLEVEL% EQU 0 (echo   train_test.exe OK) else (echo   train_test.exe FAILED)

echo [2/4] 编译全流程测试...
gcc -std=c99 -O2 -I../src full_train_save.c ../src/bool_router/bool_router.c ../src/boolnet/boolnet.c ../src/mem_int/mem_int_layer.c ../src/tsetlin_train/tsetlin_train.c ../src/conv1d_circular/conv1d_circular.c ../src/upsampling/upsampling.c -o full_train_save.exe -lm 2>&1
if %ERRORLEVEL% EQU 0 (echo   full_train_save.exe OK) else (echo   full_train_save.exe FAILED)

echo [3/4] 编译级联测试...
gcc -std=c99 -O2 -I../src learned_cascade.c ../src/bool_router/bool_router.c ../src/boolnet/boolnet.c ../src/mem_int/mem_int_layer.c ../src/tsetlin_train/tsetlin_train.c -o learned_cascade.exe -lm 2>&1
if %ERRORLEVEL% EQU 0 (echo   learned_cascade.exe OK) else (echo   learned_cascade.exe FAILED)

echo [4/4] 编译深度级联测试...
gcc -std=c99 -O2 -I../src deep_cascade.c ../src/bool_router/bool_router.c ../src/boolnet/boolnet.c ../src/mem_int/mem_int_layer.c ../src/tsetlin_train/tsetlin_train.c ../src/conv1d_circular/conv1d_circular.c -o deep_cascade.exe -lm 2>&1
if %ERRORLEVEL% EQU 0 (echo   deep_cascade.exe OK) else (echo   deep_cascade.exe FAILED)

echo.
echo === 运行测试 ===
echo.
echo [测试 1] 最小训练 (2 层, 单模式)
train_test.exe
echo.
echo [测试 2] 全流程 (5 层, Train-Save-Load-Infer)
full_train_save.exe
echo.
echo [测试 3] 级联树路由 (16 节点, 逐层训练)
learned_cascade.exe
echo.
echo === 全部完成 ===
pause
