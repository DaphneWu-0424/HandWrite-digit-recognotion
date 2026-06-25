from pathlib import Path
import gzip
import struct
import urllib.request

from PIL import Image

TRAIN_IMAGES_URL = "https://ossci-datasets.s3.amazonaws.com/mnist/train-images-idx3-ubyte.gz"
TRAIN_LABELS_URL = "https://ossci-datasets.s3.amazonaws.com/mnist/train-labels-idx1-ubyte.gz"


def download_if_missing(url: str, path: Path) -> None:
    if path.exists():
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    print(f"downloading {url}")
    urllib.request.urlretrieve(url, path)


def read_labels(path: Path) -> bytes:
    with gzip.open(path, "rb") as f:
        magic, count = struct.unpack(">II", f.read(8))
        if magic != 2049:
            raise ValueError(f"unexpected label file magic: {magic}")
        labels = f.read(count)
        if len(labels) != count:
            raise ValueError("label file ended early")
        return labels


def iter_images(path: Path):
    with gzip.open(path, "rb") as f:
        magic, count, rows, cols = struct.unpack(">IIII", f.read(16))
        if magic != 2051:
            raise ValueError(f"unexpected image file magic: {magic}")
        for _ in range(count):
            payload = f.read(rows * cols)
            if len(payload) != rows * cols:
                raise ValueError("image file ended early")
            yield Image.frombytes("L", (cols, rows), payload)

def main() -> None:
    out_dir = Path(__file__).resolve().parent
    raw_dir = out_dir / "raw"
    images_path = raw_dir / "train-images-idx3-ubyte.gz"
    labels_path = raw_dir / "train-labels-idx1-ubyte.gz"

    download_if_missing(TRAIN_IMAGES_URL, images_path)
    download_if_missing(TRAIN_LABELS_URL, labels_path)

    saved = set()
    labels = read_labels(labels_path)
    for image, label in zip(iter_images(images_path), labels):
        if label in saved:
            continue

        image.save(out_dir / f"{label}.png")
        saved.add(label)

        if len(saved) == 10:
            break

    missing = sorted(set(range(10)) - saved)
    if missing:
        raise RuntimeError(f"missing MNIST digits: {missing}")

    print(f"saved 0.png through 9.png to {out_dir}")


if __name__ == "__main__":
    main()
