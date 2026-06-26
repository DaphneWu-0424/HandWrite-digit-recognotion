from __future__ import annotations

import argparse
import json
import time
from datetime import datetime
from pathlib import Path

import torch
from torch import nn
from torch.utils.data import ConcatDataset, DataLoader, Subset
from torchvision import datasets, transforms

from model_io import infer_model_path, load_npz_model
from models import build_model
from personal_dataset import PersonalJsonlDataset
from train_model import evaluate, export_npz


def infer_base_model_path(base_run: str | None) -> Path | None:
    if not base_run:
        return None
    return infer_model_path(base_run)


def main() -> None:
    parser = argparse.ArgumentParser(description="Fine-tune MLP with personal train split samples.")
    parser.add_argument("--samples", required=True, help="Path to exported *_samples.jsonl")
    parser.add_argument("--base-run", default=None, help="Run directory or model.npz to continue from")
    parser.add_argument("--hidden", type=int, default=64)
    parser.add_argument("--data-dir", default="data")
    parser.add_argument("--runs-dir", default="tools/train/runs")
    parser.add_argument("--epochs", type=int, default=5)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--lr", type=float, default=0.0003)
    parser.add_argument("--repeat-personal", type=int, default=20)
    parser.add_argument("--mnist-limit", type=int, default=10000, help="Limit MNIST samples during fine-tuning")
    parser.add_argument("--device", default="cpu", choices=["cpu", "cuda"])
    args = parser.parse_args()

    device = torch.device(args.device if (args.device == "cpu" or torch.cuda.is_available()) else "cpu")
    base_path = infer_base_model_path(args.base_run)

    base_name = "mlp"
    hidden_size = args.hidden
    model_type = "mlp"
    if base_path:
        model, loaded_model_type, base_name, hidden_size = load_npz_model(base_path, device)
        model_type = loaded_model_type
        if model_type != "mlp":
            raise ValueError("personal fine-tuning currently expects an MLP base model")
    else:
        model, _ = build_model("mlp", hidden_size)
        model = model.to(device)

    personal_train = PersonalJsonlDataset(args.samples, split="train", repeat=args.repeat_personal)
    transform = transforms.Compose([transforms.ToTensor()])
    mnist_train = datasets.MNIST(args.data_dir, train=True, download=True, transform=transform)
    if args.mnist_limit > 0 and args.mnist_limit < len(mnist_train):
        mnist_train = Subset(mnist_train, list(range(args.mnist_limit)))

    train_set = ConcatDataset([mnist_train, personal_train])
    train_loader = DataLoader(train_set, batch_size=args.batch_size, shuffle=True)
    test_set = datasets.MNIST(args.data_dir, train=False, download=True, transform=transform)
    test_loader = DataLoader(test_set, batch_size=args.batch_size)

    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    loss_fn = nn.CrossEntropyLoss()

    first_record = personal_train.records[0]
    safe_person = first_record.person_name.replace(" ", "_")
    run_model_name = f"mlp{hidden_size}-{datetime.now().strftime('%Y%m%d-%H%M%S')}-{safe_person}"
    run_dir = Path(args.runs_dir) / run_model_name
    run_dir.mkdir(parents=True, exist_ok=True)

    config = {
        "model_type": "mlp",
        "model_name": run_model_name,
        "hidden_size": hidden_size,
        "base_model": base_name,
        "base_run": str(base_path) if base_path else None,
        "samples": str(Path(args.samples)),
        "personal_train_samples": len(personal_train.records),
        "repeat_personal": args.repeat_personal,
        "mnist_limit": args.mnist_limit,
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
        total = 0
        for images, labels in train_loader:
            images = images.to(device)
            labels = labels.to(device)
            optimizer.zero_grad()
            loss = loss_fn(model(images), labels)
            loss.backward()
            optimizer.step()
            running_loss += loss.item() * labels.numel()
            total += labels.numel()

        train_loss = running_loss / total if total else 0.0
        test_loss, test_acc = evaluate(model, test_loader, device)
        history.append({"epoch": epoch, "train_loss": train_loss, "test_loss": test_loss, "test_acc": test_acc})
        print(f"epoch={epoch} train_loss={train_loss:.4f} test_loss={test_loss:.4f} test_acc={test_acc:.4f}")

    _, final_acc = evaluate(model, test_loader, device)
    metrics = {
        "test_acc": final_acc,
        "elapsed_sec": time.time() - started,
        "history": history,
    }
    (run_dir / "metrics.json").write_text(json.dumps(metrics, indent=2), encoding="utf-8")
    export_npz(model, model_type, run_model_name, hidden_size, run_dir / "model.npz")
    print(f"saved fine-tuned run to {run_dir}")


if __name__ == "__main__":
    main()
