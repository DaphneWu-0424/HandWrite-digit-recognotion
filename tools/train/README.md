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

You can still train the old single-layer baseline:

```powershell
.\.venv\Scripts\python tools\train\train_model.py --model perceptron --epochs 20 --lr 0.001
```

After export, rebuild the STM32 project. `User/model/ModelData.h` should contain:

```c
#define MODEL_GENERATED 1
```
