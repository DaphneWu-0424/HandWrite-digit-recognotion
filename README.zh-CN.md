# HandWrite

[English](README.md)

HandWrite 是一个基于 STM32 的手写数字识别嵌入式系统。项目由 STM32F103 固件、ILI9341 LCD、XPT2046 电阻触摸屏、轻量神经网络推理模块、React Web Serial 上位机以及 Python 训练/导出工具组成。

这个仓库不是单一的固件工程，而是一套完整的嵌入式 AI 闭环：在触摸屏上书写数字，MCU 将笔画预处理为 `28 x 28` 输入，在 STM32 端完成推理，通过串口把预测帧发送给浏览器，上位机采集个人样本并触发微调，再把训练后的模型导出为 C 代码，重新编译烧录到开发板上验证。

## 功能特性

- STM32F103 固件基于 HAL，使用 FSMC 驱动 LCD，USART1 输出结果，GPIO 软件时序采样触摸。
- 触摸手写画布支持 OK/Clear 按钮、笔画缓存、线段补点和 LCD 预览。
- 面向 MCU 的图像预处理：包围盒裁剪、等比例缩放、居中和笔画膨胀，输出 `28 x 28` 像素。
- 板端识别模块支持感知机、MLP 和 Tiny CNN。
- 当前导出的模型为 INT8 量化 Tiny CNN，卷积通道配置为 `8 x 16`，输出 10 类数字。
- USART1 使用 JSON Lines 协议输出预测结果、Top3 分数、推理耗时和输入像素。
- React + TypeScript 上位机基于 Web Serial，实现实时演示、样本采集、训练流程、评估报告和模型导出。
- Python 工具支持 MNIST 训练、个人样本微调、个人测试集评估和固件 `ModelData.c/.h` 生成。

## 目录结构

```text
Core/                 STM32CubeMX 生成的主程序入口与 HAL 初始化代码
Drivers/              STM32 HAL/CMSIS 驱动
cmake/                STM32CubeMX CMake 集成文件
User/
  app/                手写画布、预处理、识别器、串口协议
  font/               LCD 字库数据
  lcd/                ILI9341 LCD 与 XPT2046 触摸驱动
  model/              导出的模型参数和元信息
tools/
  host/               React Web Serial 上位机
  helper/             本地 HTTP 辅助服务：数据集、训练、评估、导出
  train/              Python 模型训练、微调、评估、导出脚本
report/               课程设计报告源码与图片
HandWrite.ioc         STM32CubeMX 工程
CMakeLists.txt        固件顶层构建文件
```

## 硬件组成

- 主控：STM32F103 系列，HSE + PLL 配置为 72 MHz SYSCLK。
- 显示：ILI9341 彩色 LCD，通过 16 位 FSMC 并行总线访问。
- 触摸：XPT2046 电阻触摸控制器，通过 GPIO 软件时序读写。
- 上位机通信：USART1 通过 USB-TTL 模块连接 PC。
- 串口参数：`115200` baud，8 数据位，无校验，1 停止位。

## 固件运行流程

固件上电后依次初始化 HAL、系统时钟、GPIO、FSMC、USART1、LCD、触摸屏、串口协议和手写画布。主循环持续处理触摸输入。当用户点击 OK 且存在有效笔画时，固件会：

1. 将画布 bit-map 转换为 `28 x 28` 数字输入。
2. 调用 `DigitRecognizer_PredictTop3()` 完成推理。
3. 在 LCD 上显示最佳识别结果。
4. 通过 USART1 发送一行 JSON Lines 预测帧。

串口帧示例：

```json
{"type":"prediction","seq":1,"model":"cnn8x16-int8-20260702-140745-wuyueying-int8","modelType":"cnn","quant":"int8","inferMs":12,"w":28,"h":28,"pixels":"00FF...","result":1,"top3":[{"digit":1,"scoreMilli":2210},{"digit":7,"scoreMilli":930},{"digit":8,"scoreMilli":410}]}
```

## 构建与烧录固件

可以使用 VSCode STM32 插件、STM32CubeIDE/CubeCLT，或兼容的 CMake + Arm GNU 工具链构建。

典型流程：

1. 使用或保留 `HandWrite.ioc` 中的 CubeMX 外设配置。
2. 构建 `HandWrite` 固件目标。
3. 通过 ST-Link、STM32Programmer 或常用 STM32 烧录流程下载固件。
4. 使用 USB-TTL 将 USART1 连接到 PC。
5. 打开 Web 上位机并连接串口。

导出新模型后，需要重新构建并烧录固件。本地 Helper 只负责写入 `User/model/ModelData.c` 和 `User/model/ModelData.h`，不会自动编译或烧录开发板。

## 运行 Web 上位机

上位机使用 Web Serial API，因此需要 Chrome 或 Edge。

```powershell
cd tools\host
npm install
npm run dev
```

打开终端中打印的本地地址，点击 `Connect`，选择连接 USART1 的 USB-TTL 串口。没有硬件时可以使用 `Mock frame` 预览界面。

上位机主要功能包括：

- 实时串口连接与预测结果展示。
- `28 x 28` 像素预览、模型信息、Top3 分数和最近串口帧。
- 按 train/test 划分采集个人手写样本。
- 通过本地 Helper 启动个人样本微调。
- 保存评估记录、按数字统计准确率并支持样本回放。
- 将选定模型导出到 `User/model/ModelData.c/.h`。

## 运行本地 Helper

创建 Python 虚拟环境并安装训练依赖：

```powershell
python -m venv .venv
.\.venv\Scripts\pip install -r tools\train\requirements.txt
```

在仓库根目录启动 Helper：

```powershell
.\.venv\Scripts\python tools\helper\server.py
```

默认地址：

```text
http://127.0.0.1:8765
```

Helper 负责保存个人样本、启动微调、评估个人测试集、列出模型 run、导出固件模型数据，并把报告评估记录保存到 `data/reports/evaluation_records.jsonl`。

## 训练、微调与模型导出

训练推荐的 MLP 基线模型：

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model mlp --hidden 64 --epochs 20 --lr 0.001
```

训练 Tiny CNN：

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model cnn --epochs 10 --lr 0.001
```

使用个人样本微调：

```powershell
.\.venv\Scripts\python tools\train\finetune_personal.py ^
  --samples data\personal\YOUR_SAMPLES.jsonl ^
  --base-run tools\train\runs\YOUR_BASE_RUN ^
  --epochs 5 ^
  --lr 0.0003
```

评估微调前后准确率：

```powershell
.\.venv\Scripts\python tools\train\eval_personal.py ^
  --samples data\personal\YOUR_SAMPLES.jsonl ^
  --model-a tools\train\runs\YOUR_BASE_RUN ^
  --model-b tools\train\runs\YOUR_FINE_TUNED_RUN ^
  --output data\personal\personal_eval.json
```

导出模型到固件：

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\YOUR_RUN\model.npz
```

将 Tiny CNN 以 INT8 方式导出：

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\YOUR_CNN_RUN\model.npz --quant int8
```

导出后确认 `User/model/ModelData.h` 中包含：

```c
#define MODEL_GENERATED 1
```

然后重新构建并烧录 STM32 固件。

## 测试说明

课程报告中记录的基础测试覆盖：

- LCD 上电界面、书写区域、预览区域、结果文本、OK 按钮和 Clear 按钮。
- 触摸书写连续显示与线段补点。
- 点击 OK 后完成预处理和识别。
- 点击 Clear 后清空画布。
- USART1 输出合法 JSON Lines，包含 `result`、`top3`、`pixels` 和 `inferMs`。
- 固件模型状态与上位机显示的模型元信息一致。

报告中的个人手写评估使用 30 个测试样本。预训练模型在该个人测试集上为 `24/30`，准确率 `80.0%`；个人数据微调后的模型为 `30/30`，准确率 `100.0%`。该结果适合说明个人化闭环有效，但样本量较小，不应视为通用基准。

## 后续改进方向

- 增加可配置的触摸屏校准流程，适配不同 LCD/触摸模块。
- 采集更大规模的个人和多人测试集。
- 对比 MLP、CNN、FP32、INT8 模型在准确率、Flash、SRAM 和推理耗时上的差异。
- 将 Tiny CNN 中剩余的浮点尺度恢复进一步改为定点计算，提高 STM32F103 上的性能。
- 在上位机中扩展混淆矩阵和错误样本回放分析。
