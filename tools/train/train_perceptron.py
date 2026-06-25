import argparse
from pathlib import Path

import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader
from torchvision import datasets, transforms


class Perceptron(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.linear = nn.Linear(28 * 28, 10)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.linear(x.view(x.size(0), -1))


def evaluate(model: nn.Module, loader: DataLoader, device: torch.device) -> float:
    model.eval()
    correct = 0
    total = 0
    with torch.no_grad():
        for images, labels in loader:
            images = images.to(device)
            labels = labels.to(device)
            pred = model(images).argmax(dim=1)
            correct += (pred == labels).sum().item()
            total += labels.numel()
    return correct / total if total else 0.0


def main() -> None:
    parser = argparse.ArgumentParser(description="Train a single-layer MNIST perceptron.")
    parser.add_argument("--data-dir", default="data", help="MNIST download/cache directory")
    parser.add_argument("--output", default="perceptron_mnist.npz", help="Output .npz weights file")
    parser.add_argument("--epochs", type=int, default=8)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--lr", type=float, default=0.1)
    parser.add_argument("--device", default="cpu", choices=["cpu", "cuda"])
    args = parser.parse_args()

    device = torch.device(args.device if (args.device == "cpu" or torch.cuda.is_available()) else "cpu")
    transform = transforms.Compose([transforms.ToTensor()])

    train_set = datasets.MNIST(args.data_dir, train=True, download=True, transform=transform)
    test_set = datasets.MNIST(args.data_dir, train=False, download=True, transform=transform)
    train_loader = DataLoader(train_set, batch_size=args.batch_size, shuffle=True)
    test_loader = DataLoader(test_set, batch_size=args.batch_size)

    model = Perceptron().to(device)
    optimizer = torch.optim.SGD(model.parameters(), lr=args.lr)
    loss_fn = nn.CrossEntropyLoss()

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

        accuracy = evaluate(model, test_loader, device)
        avg_loss = running_loss / len(train_set)
        print(f"epoch={epoch} loss={avg_loss:.4f} test_acc={accuracy:.4f}")

    weights = model.linear.weight.detach().cpu().numpy().astype(np.float32)
    bias = model.linear.bias.detach().cpu().numpy().astype(np.float32)
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    np.savez(output, weights=weights, bias=bias)
    print(f"saved {output}")


if __name__ == "__main__":
    main()
