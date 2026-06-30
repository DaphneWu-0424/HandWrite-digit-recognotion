from __future__ import annotations

from dataclasses import dataclass

import torch
from torch import nn


INPUT_SIZE = 28 * 28
CLASS_COUNT = 10


@dataclass(frozen=True)
class ModelSpec:
    model_type: str
    model_name: str
    hidden_size: int


class Perceptron(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.linear = nn.Linear(INPUT_SIZE, CLASS_COUNT)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.linear(x.view(x.size(0), -1))


class MLP(nn.Module):
    def __init__(self, hidden_size: int) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Flatten(),
            nn.Linear(INPUT_SIZE, hidden_size),
            nn.ReLU(),
            nn.Linear(hidden_size, CLASS_COUNT),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


class TinyCNN(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.conv1 = nn.Conv2d(1, 8, kernel_size=3, padding=1)
        self.conv2 = nn.Conv2d(8, 16, kernel_size=3, padding=1)
        self.pool = nn.MaxPool2d(2)
        self.fc = nn.Linear(16 * 7 * 7, CLASS_COUNT)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        x = self.pool(torch.relu(self.conv1(x)))
        x = self.pool(torch.relu(self.conv2(x)))
        return self.fc(x.view(x.size(0), -1))


def build_model(model_type: str, hidden_size: int) -> tuple[nn.Module, ModelSpec]:
    if model_type == "perceptron":
        return Perceptron(), ModelSpec("perceptron", "perceptron", 0)
    if model_type == "mlp":
        return MLP(hidden_size), ModelSpec("mlp", f"mlp{hidden_size}", hidden_size)
    if model_type == "cnn":
        return TinyCNN(), ModelSpec("cnn", "cnn8x16", 0)
    raise ValueError(f"unsupported model type: {model_type}")
