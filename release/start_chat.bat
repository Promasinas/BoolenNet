@echo off
chcp 65001 >nul 2>&1
title BoolNet Chatbot
echo.
echo ╔══════════════════════════════════════════╗
echo ║     BoolNet 对话机器人                  ║
echo ║  1. v0.5 - 64 Q&A Byte流 (推荐)        ║
echo ║  2. v0.7 - 16 Token 分类               ║
echo ║  3. v0.4 - 16 Q&A Byte流               ║
echo ╚══════════════════════════════════════════╝
echo.
set /p choice="选择版本 (1/2/3): "
if "%choice%"=="1" v0.5-boolchat-v2\boolchat_v2.exe
if "%choice%"=="2" v0.7-llm-classify\llm_classify.exe
if "%choice%"=="3" v0.4-boolchat\boolchat.exe
