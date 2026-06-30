# MNIST Model Training

Install dependencies:

```powershell
python -m venv .venv
.\.venv\Scripts\pip install -r tools\train\requirements.txt
```

Train the recommended MLP model:

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model mlp --hidden 64 --epochs 20 --lr 0.001
```

This creates a run directory like:

```text
tools/train/runs/mlp64_YYYYMMDD_HHMMSS/
  config.json
  metrics.json
  model.npz
```

Export one trained model into the firmware:

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\mlp64_YYYYMMDD_HHMMSS\model.npz
```

Export Tiny CNN as INT8:

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\cnn8x16_YYYYMMDD_HHMMSS\model.npz --quant int8
```

You can still train the old single-layer baseline:

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model perceptron --epochs 20 --lr 0.001
```

Train the Tiny CNN extension:

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model cnn --epochs 10 --lr 0.001
```

After export, rebuild the STM32 project. `User/model/ModelData.h` should contain:

```c
#define MODEL_GENERATED 1
```

## Fine-tune with personal samples

Export `*_samples.jsonl` from the React host and place it under `data/personal/`.
Only `split=train` samples are used for fine-tuning.

```powershell
.\.venv\Scripts\python tools\train\finetune_personal.py ^
  --samples data\personal\20260626_150000_yitia_samples.jsonl ^
  --base-run tools\train\runs\mlp64_YYYYMMDD_HHMMSS ^
  --epochs 5 ^
  --lr 0.0003
```

Then export the generated run:

```powershell
.\.venv\Scripts\python tools\train\export_model.py tools\train\runs\mlp64-YYYYMMDD-HHMMSS-yitia\model.npz
```

Evaluate before/after accuracy on the same personal test split:

```powershell
.\.venv\Scripts\python tools\train\eval_personal.py ^
  --samples data\personal\20260626_150000_yitia_samples.jsonl ^
  --model-a tools\train\runs\mlp64_YYYYMMDD_HHMMSS ^
  --model-b tools\train\runs\mlp64-YYYYMMDD-HHMMSS-yitia ^
  --output data\personal\20260626_150000_yitia_eval.json
```
