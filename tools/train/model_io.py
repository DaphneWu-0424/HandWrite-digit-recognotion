from __future__ import annotations

from pathlib import Path

import numpy as np
import torch
from torch import nn

from models import build_model


def infer_model_path(model_or_run: str | Path) -> Path:
    path = Path(model_or_run)
    if path.is_dir():
        path = path / "model.npz"
    if not path.exists():
        raise FileNotFoundError(path)
    return path


def load_npz_model(model_or_run: str | Path, device: torch.device) -> tuple[nn.Module, str, str, int]:
    path = infer_model_path(model_or_run)
    data = np.load(path)
    model_type = str(data["model_type"].tolist())
    model_name = str(data["model_name"].tolist())
    hidden_size = int(data["hidden_size"].tolist())
    model, _ = build_model(model_type, hidden_size if hidden_size > 0 else 64)

    with torch.no_grad():
        if model_type == "perceptron":
            linear = model.linear  # type: ignore[attr-defined]
            linear.weight.copy_(torch.from_numpy(data["output_weights"].astype(np.float32)))
            linear.bias.copy_(torch.from_numpy(data["output_bias"].astype(np.float32)))
        elif model_type == "mlp":
            layers = model.net  # type: ignore[attr-defined]
            hidden = layers[1]
            output_layer = layers[3]
            hidden.weight.copy_(torch.from_numpy(data["hidden_weights"].astype(np.float32)))
            hidden.bias.copy_(torch.from_numpy(data["hidden_bias"].astype(np.float32)))
            output_layer.weight.copy_(torch.from_numpy(data["output_weights"].astype(np.float32)))
            output_layer.bias.copy_(torch.from_numpy(data["output_bias"].astype(np.float32)))
        elif model_type == "cnn":
            model.conv1.weight.copy_(torch.from_numpy(data["conv1_weights"].astype(np.float32)))  # type: ignore[attr-defined]
            model.conv1.bias.copy_(torch.from_numpy(data["conv1_bias"].astype(np.float32)))  # type: ignore[attr-defined]
            model.conv2.weight.copy_(torch.from_numpy(data["conv2_weights"].astype(np.float32)))  # type: ignore[attr-defined]
            model.conv2.bias.copy_(torch.from_numpy(data["conv2_bias"].astype(np.float32)))  # type: ignore[attr-defined]
            model.fc.weight.copy_(torch.from_numpy(data["fc_weights"].astype(np.float32)))  # type: ignore[attr-defined]
            model.fc.bias.copy_(torch.from_numpy(data["fc_bias"].astype(np.float32)))  # type: ignore[attr-defined]
        else:
            raise ValueError(f"unsupported model type: {model_type}")

    model = model.to(device)
    model.eval()
    return model, model_type, model_name, hidden_size
