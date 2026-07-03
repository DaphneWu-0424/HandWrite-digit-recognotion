# HandWrite

[中文文档](README.zh-CN.md)

HandWrite is an STM32-based handwritten digit recognition system. It combines an STM32F103 firmware application, an ILI9341 LCD, an XPT2046 resistive touch screen, a lightweight neural-network inference module, a React Web Serial host, and Python training/export tools.

The project is designed as a full embedded AI workflow: write a digit on the touch screen, preprocess it into a `28 x 28` input on the MCU, run inference on the STM32, stream prediction frames to the browser, collect personal samples, fine-tune a model, export C model data, rebuild the firmware, and test the updated model on the board.

## Features

- STM32F103 firmware with HAL, FSMC LCD access, USART1 output, and GPIO-based touch sampling.
- Touch handwriting canvas with OK/Clear buttons, stroke buffering, line interpolation, and LCD preview.
- Integer-friendly preprocessing: bounding-box crop, aspect-ratio preserving resize, centering, and stroke dilation into `28 x 28` pixels.
- On-device digit recognition with perceptron, MLP, and Tiny CNN support.
- Current generated model: INT8 Tiny CNN, `8 x 16` convolution channels, 10 output classes.
- JSON Lines serial protocol over USART1 for prediction results, Top3 scores, inference time, and input pixels.
- React + TypeScript host using Web Serial for real-time visualization, sample collection, training workflow, evaluation reports, and model export.
- Python tools for MNIST training, personal fine-tuning, personal test evaluation, and firmware `ModelData.c/.h` generation.

## Repository Layout

```text
Core/                 STM32CubeMX generated application entry and HAL init code
Drivers/              STM32 HAL/CMSIS drivers
cmake/                STM32CubeMX CMake integration
User/
  app/                Handwriting canvas, preprocessing, recognizer, serial protocol
  font/               LCD font data
  lcd/                ILI9341 LCD and XPT2046 touch drivers
  model/              Exported model parameters and metadata
tools/
  host/               React Web Serial host
  helper/             Local HTTP helper for dataset, training, evaluation, export
  train/              Python model training, fine-tuning, evaluation, export scripts
report/               Course report sources and figures
HandWrite.ioc         STM32CubeMX project
CMakeLists.txt        Top-level firmware build file
```

## Hardware

- MCU: STM32F103 series, configured for 72 MHz SYSCLK from HSE + PLL.
- Display: ILI9341 color LCD through 16-bit FSMC parallel bus.
- Touch: XPT2046 resistive touch controller using GPIO software timing.
- Host link: USART1 through a USB-TTL adapter.
- Serial settings: `115200` baud, 8 data bits, no parity, 1 stop bit.

## Firmware Flow

On startup, the firmware initializes HAL, system clock, GPIO, FSMC, USART1, LCD, touch, serial protocol, and the handwriting canvas. The main loop continuously processes touch input. When the user taps OK and a valid stroke exists, the firmware:

1. Converts the canvas bitmap into a `28 x 28` digit input.
2. Runs `DigitRecognizer_PredictTop3()`.
3. Displays the best result on the LCD.
4. Sends one JSON Lines prediction frame through USART1.

Example frame:

```json
{"type":"prediction","seq":1,"model":"cnn8x16-int8-20260702-140745-wuyueying-int8","modelType":"cnn","quant":"int8","inferMs":12,"w":28,"h":28,"pixels":"00FF...","result":1,"top3":[{"digit":1,"scoreMilli":2210},{"digit":7,"scoreMilli":930},{"digit":8,"scoreMilli":410}]}
```

## Build And Flash Firmware

Use the VSCode STM32 extension, STM32CubeIDE/CubeCLT, or a compatible CMake + Arm GNU toolchain setup.

Typical workflow:

1. Generate or keep the CubeMX configuration from `HandWrite.ioc`.
2. Build the `HandWrite` firmware target.
3. Flash the generated firmware with ST-Link, STM32Programmer, or your normal STM32 flashing workflow.
4. Connect USART1 to the PC through USB-TTL.
5. Open the web host and connect to the serial port.

After exporting a new model, rebuild and flash the firmware again. The helper service only writes `User/model/ModelData.c` and `User/model/ModelData.h`; it does not compile or flash the board.

## Run The Web Host

The host requires Chrome or Edge because it uses the Web Serial API.

```powershell
cd tools\host
npm install
npm run dev
```

Open the printed local URL, click `Connect`, and select the USB-TTL port connected to USART1. Use `Mock frame` to preview the UI without hardware.

The host provides:

- Real-time serial connection and prediction display.
- `28 x 28` pixel preview, model metadata, Top3 scores, and recent frames.
- Personal dataset collection with train/test split labels.
- Personal fine-tuning workflow through the local helper.
- Evaluation records, per-digit statistics, and replay data.
- Firmware model export into `User/model/ModelData.c/.h`.

## Run The Local Helper

Create the Python environment and install training dependencies:

```powershell
python -m venv .venv
.\.venv\Scripts\pip install -r tools\train\requirements.txt
```

Start the helper from the repository root:

```powershell
.\.venv\Scripts\python tools\helper\server.py
```

Default helper URL:

```text
http://127.0.0.1:8765
```

The helper supports saving personal samples, launching fine-tuning, evaluating personal test sets, listing runs, exporting firmware model data, and storing report records under `data/reports/evaluation_records.jsonl`.

## Train, Fine-Tune, And Export Models

Train the recommended MLP baseline:

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model mlp --hidden 64 --epochs 20 --lr 0.001
```

Train the Tiny CNN:

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model cnn --epochs 10 --lr 0.001
```

Fine-tune with personal samples:

```powershell
.\.venv\Scripts\python tools\train\finetune_personal.py ^
  --samples data\personal\YOUR_SAMPLES.jsonl ^
  --base-run tools\train\runs\YOUR_BASE_RUN ^
  --epochs 5 ^
  --lr 0.0003
```

Evaluate before/after accuracy:

```powershell
.\.venv\Scripts\python tools\train\eval_personal.py ^
  --samples data\personal\YOUR_SAMPLES.jsonl ^
  --model-a tools\train\runs\YOUR_BASE_RUN ^
  --model-b tools\train\runs\YOUR_FINE_TUNED_RUN ^
  --output data\personal\personal_eval.json
```

Export a model into the firmware:

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\YOUR_RUN\model.npz
```

Export Tiny CNN as INT8:

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\YOUR_CNN_RUN\model.npz --quant int8
```

After export, confirm `User/model/ModelData.h` contains:

```c
#define MODEL_GENERATED 1
```

Then rebuild and flash the STM32 firmware.

## Test Notes

The course report records the following validation coverage:

- LCD startup UI, writing area, preview area, result text, OK button, and Clear button.
- Continuous touch writing and line rendering.
- OK-triggered preprocessing and recognition.
- Clear-triggered canvas reset.
- Valid USART1 JSON Lines output with `result`, `top3`, `pixels`, and `inferMs`.
- Model metadata consistency between firmware and host.

The personal handwriting evaluation in the report used 30 test samples. The pretrained model reached `24/30` (`80.0%`) on that personal test set, while the personal fine-tuned model reached `30/30` (`100.0%`). This is a small personal test set, so it is useful for workflow validation rather than as a broad benchmark.

## Further Work

- Add a configurable touch calibration flow for different LCD/touch modules.
- Collect larger personal and multi-user test sets.
- Compare MLP, CNN, FP32, and INT8 models by accuracy, flash usage, SRAM usage, and inference time.
- Convert the remaining Tiny CNN scale recovery path to fixed-point arithmetic for better STM32F103 performance.
- Expand host-side confusion matrix and error replay analysis.
