from __future__ import annotations

import argparse
import json
from pathlib import Path

import torch
from torch import nn
from torch.utils.data import DataLoader

from model_io import load_npz_model
from personal_dataset import PersonalJsonlDataset


def evaluate_personal(model: nn.Module, loader: DataLoader, device: torch.device) -> dict:
    model.eval()
    correct = 0
    total = 0
    per_digit = {digit: {"correct": 0, "total": 0} for digit in range(10)}
    confusion = [[0 for _ in range(10)] for _ in range(10)]

    with torch.no_grad():
        for images, labels in loader:
            images = images.to(device)
            labels = labels.to(device)
            pred = model(images).argmax(dim=1)
            correct_mask = pred == labels
            correct += correct_mask.sum().item()
            total += labels.numel()

            for label, guess in zip(labels.cpu().tolist(), pred.cpu().tolist()):
                per_digit[label]["total"] += 1
                per_digit[label]["correct"] += int(label == guess)
                confusion[label][guess] += 1

    return {
        "accuracy": correct / total if total else 0.0,
        "correct": correct,
        "total": total,
        "per_digit": {
            str(digit): {
                "accuracy": stats["correct"] / stats["total"] if stats["total"] else None,
                **stats,
            }
            for digit, stats in per_digit.items()
        },
        "confusion": confusion,
    }


def eval_model(model_path: str, samples: str, device: torch.device, batch_size: int) -> dict:
    model, model_type, model_name, hidden_size = load_npz_model(model_path, device)
    dataset = PersonalJsonlDataset(samples, split="test")
    loader = DataLoader(dataset, batch_size=batch_size)
    metrics = evaluate_personal(model, loader, device)
    return {
        "model_path": str(Path(model_path)),
        "model_name": model_name,
        "model_type": model_type,
        "hidden_size": hidden_size,
        **metrics,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Evaluate models on personal test split samples.")
    parser.add_argument("--samples", required=True, help="Path to exported *_samples.jsonl")
    parser.add_argument("--model-a", required=True, help="Base run directory or model.npz")
    parser.add_argument("--model-b", default=None, help="Fine-tuned run directory or model.npz")
    parser.add_argument("--output", default=None, help="Optional metrics JSON output path")
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--device", default="cpu", choices=["cpu", "cuda"])
    args = parser.parse_args()

    device = torch.device(args.device if (args.device == "cpu" or torch.cuda.is_available()) else "cpu")
    result = {
        "samples": str(Path(args.samples)),
        "model_a": eval_model(args.model_a, args.samples, device, args.batch_size),
    }

    if args.model_b:
        result["model_b"] = eval_model(args.model_b, args.samples, device, args.batch_size)
        result["delta_accuracy"] = result["model_b"]["accuracy"] - result["model_a"]["accuracy"]

    print(json.dumps(result, indent=2))

    if args.output:
        output = Path(args.output)
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
