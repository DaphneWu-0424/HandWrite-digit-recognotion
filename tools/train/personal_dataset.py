from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Literal

import torch
from torch.utils.data import Dataset

from models import INPUT_SIZE

Split = Literal["train", "test"]


@dataclass(frozen=True)
class PersonalSampleRecord:
    session_id: str
    person_name: str
    split: Split
    label: int
    prediction: int
    model: str
    model_type: str
    pixels_hex: str
    created_at: str


def _decode_pixels(hex_text: str) -> torch.Tensor:
    if len(hex_text) != INPUT_SIZE * 2:
        raise ValueError(f"pixel hex length {len(hex_text)}, expected {INPUT_SIZE * 2}")
    values = [int(hex_text[i : i + 2], 16) / 255.0 for i in range(0, len(hex_text), 2)]
    return torch.tensor(values, dtype=torch.float32).view(1, 28, 28)


def load_personal_records(samples_path: Path, split: Split | None = None) -> list[PersonalSampleRecord]:
    records: list[PersonalSampleRecord] = []
    with samples_path.open("r", encoding="utf-8") as f:
        for line_no, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            raw = json.loads(line)
            if split is not None and raw.get("split") != split:
                continue
            try:
                records.append(
                    PersonalSampleRecord(
                        session_id=str(raw["sessionId"]),
                        person_name=str(raw["personName"]),
                        split=raw["split"],
                        label=int(raw["label"]),
                        prediction=int(raw["prediction"]),
                        model=str(raw["model"]),
                        model_type=str(raw["modelType"]),
                        pixels_hex=str(raw["pixels"]),
                        created_at=str(raw["createdAt"]),
                    )
                )
            except KeyError as exc:
                raise ValueError(f"missing key {exc} at {samples_path}:{line_no}") from exc
    return records


class PersonalJsonlDataset(Dataset[tuple[torch.Tensor, int]]):
    def __init__(self, samples_path: str | Path, split: Split, repeat: int = 1) -> None:
        self.samples_path = Path(samples_path)
        self.records = load_personal_records(self.samples_path, split=split)
        self.repeat = max(1, repeat)
        if not self.records:
            raise ValueError(f"no {split} samples found in {self.samples_path}")

    def __len__(self) -> int:
        return len(self.records) * self.repeat

    def __getitem__(self, index: int) -> tuple[torch.Tensor, int]:
        record = self.records[index % len(self.records)]
        image = _decode_pixels(record.pixels_hex)
        return image, record.label
