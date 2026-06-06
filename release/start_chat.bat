@echo off
chcp 65001 >nul 2>&1
title BoolNet Byte-Stream Chatbot
echo.
echo ╔══════════════════════════════════════════╗
echo ║     BoolNet 纯布尔对话机器人           ║
echo ║     16 Q&A, Byte-Stream 输出           ║
echo ║     全链路 XOR + CMP, 零浮点           ║
echo ╚══════════════════════════════════════════╝
echo.
echo 试试输入: hello, who are you, goodbye, help
echo 命令: list (查看所有问题), quit (退出)
echo.
boolchat.exe
