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


def build_model(model_type: str, hidden_size: int) -> tuple[nn.Module, ModelSpec]:
    if model_type == "perceptron":
        return Perceptron(), ModelSpec("perceptron", "perceptron", 0)
    if model_type == "mlp":
        return MLP(hidden_size), ModelSpec("mlp", f"mlp{hidden_size}", hidden_size)
    raise ValueError(f"unsupported model type: {model_type}")
