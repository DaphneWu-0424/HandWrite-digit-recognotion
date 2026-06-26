from __future__ import annotations

import argparse
import json
import time
from datetime import datetime
from pathlib import Path

import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader
from torchvision import datasets, transforms

from models import CLASS_COUNT, INPUT_SIZE, build_model


def evaluate(model: nn.Module, loader: DataLoader, device: torch.device) -> tuple[float, float]:
    model.eval()
    loss_fn = nn.CrossEntropyLoss()
    total_loss = 0.0
    correct = 0
    total = 0

    with torch.no_grad():
        for images, labels in loader:
            images = images.to(device)
            labels = labels.to(device)
            logits = model(images)
            loss = loss_fn(logits, labels)
            pred = logits.argmax(dim=1)
            total_loss += loss.item() * labels.numel()
            correct += (pred == labels).sum().item()
            total += labels.numel()

    return (total_loss / total if total else 0.0, correct / total if total else 0.0)


def export_npz(model: nn.Module, model_type: str, model_name: str, hidden_size: int, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)

    if model_type == "perceptron":
        linear = model.linear  # type: ignore[attr-defined]
        np.savez(
            output,
            model_type=model_type,
            model_name=model_name,
            input_size=np.int32(INPUT_SIZE),
            class_count=np.int32(CLASS_COUNT),
            hidden_size=np.int32(0),
            output_weights=linear.weight.detach().cpu().numpy().astype(np.float32),
            output_bias=linear.bias.detach().cpu().numpy().astype(np.float32),
        )
        return

    if model_type == "mlp":
        layers = model.net  # type: ignore[attr-defined]
        hidden = layers[1]
        output_layer = layers[3]
        np.savez(
            output,
            model_type=model_type,
            model_name=model_name,
            input_size=np.int32(INPUT_SIZE),
            class_count=np.int32(CLASS_COUNT),
            hidden_size=np.int32(hidden_size),
            hidden_weights=hidden.weight.detach().cpu().numpy().astype(np.float32),
            hidden_bias=hidden.bias.detach().cpu().numpy().astype(np.float32),
            output_weights=output_layer.weight.detach().cpu().numpy().astype(np.float32),
            output_bias=output_layer.bias.detach().cpu().numpy().astype(np.float32),
        )
        return

    raise ValueError(f"unsupported model type: {model_type}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Train MNIST perceptron or MLP model.")
    parser.add_argument("--model", choices=["perceptron", "mlp"], default="mlp")
    parser.add_argument("--hidden", type=int, default=64, help="Hidden size for MLP")
    parser.add_argument("--data-dir", default="data")
    parser.add_argument("--runs-dir", default="tools/train/runs")
    parser.add_argument("--epochs", type=int, default=20)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--lr", type=float, default=0.001)
    parser.add_argument("--device", default="cpu", choices=["cpu", "cuda"])
    args = parser.parse_args()

    device = torch.device(args.device if (args.device == "cpu" or torch.cuda.is_available()) else "cpu")
    model, spec = build_model(args.model, args.hidden)
    model = model.to(device)

    transform = transforms.Compose([transforms.ToTensor()])
    train_set = datasets.MNIST(args.data_dir, train=True, download=True, transform=transform)
    test_set = datasets.MNIST(args.data_dir, train=False, download=True, transform=transform)
    train_loader = DataLoader(train_set, batch_size=args.batch_size, shuffle=True)
    test_loader = DataLoader(test_set, batch_size=args.batch_size)

    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    loss_fn = nn.CrossEntropyLoss()

    run_name = f"{spec.model_name}_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    run_dir = Path(args.runs_dir) / run_name
    run_dir.mkdir(parents=True, exist_ok=True)

    config = {
        "model_type": spec.model_type,
        "model_name": spec.model_name,
        "hidden_size": spec.hidden_size,
        "epochs": args.epochs,
        "batch_size": args.batch_size,
        "lr": args.lr,
        "device": str(device),
    }
    (run_dir / "config.json").write_text(json.dumps(config, indent=2), encoding="utf-8")

    started = time.time()
    history: list[dict[str, float | int]] = []
    for epoch in range(1, args.epochs + 1):
        model.train()
        running_loss = 0.0
        for images, labels in train_loader:
            images = images.to(device)
            labels = labels.to(device)
            optimizer.zero_grad()
            loss = loss_fn(model(images), labels)
            loss.backward()
            optimizer.step()
            running_loss += loss.item() * labels.numel()

        train_loss = running_loss / len(train_set)
        test_loss, test_acc = evaluate(model, test_loader, device)
        history.append({"epoch": epoch, "train_loss": train_loss, "test_loss": test_loss, "test_acc": test_acc})
        print(f"epoch={epoch} train_loss={train_loss:.4f} test_loss={test_loss:.4f} test_acc={test_acc:.4f}")

    _, final_acc = evaluate(model, test_loader, device)
    elapsed = time.time() - started
    metrics = {
        "test_acc": final_acc,
        "elapsed_sec": elapsed,
        "history": history,
    }
    (run_dir / "metrics.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    export_npz(model, spec.model_type, spec.model_name, spec.hidden_size, run_dir / "model.npz")
    print(f"saved run to {run_dir}")


if __name__ == "__main__":
    main()
