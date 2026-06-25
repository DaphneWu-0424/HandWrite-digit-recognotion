# MNIST Perceptron Training

Install dependencies:

```powershell
python -m venv .venv
.\.venv\Scripts\pip install -r tools\train\requirements.txt
```

Train the single-layer model:

```powershell
.\.venv\Scripts\python tools\train\train_perceptron.py --output tools\train\perceptron_mnist.npz
```

Export weights into the firmware:

```powershell
.\.venv\Scripts\python tools\train\export_perceptron.py tools\train\perceptron_mnist.npz
```

After export, rebuild the STM32 project. `User/model/PerceptronData.h` should contain:

```c
#define PERCEPTRON_MODEL_GENERATED 1
```
