from __future__ import annotations

import argparse
import json
import numpy as np
import subprocess
import sys
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import parse_qs, urlparse


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8765
REPORT_RECORDS_PATH = REPO_ROOT / "data" / "reports" / "evaluation_records.jsonl"


def json_response(handler: BaseHTTPRequestHandler, status: int, payload: dict[str, Any]) -> None:
    body = json.dumps(payload, ensure_ascii=False, indent=2).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Access-Control-Allow-Origin", "*")
    handler.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
    handler.send_header("Access-Control-Allow-Headers", "Content-Type")
    handler.end_headers()
    handler.wfile.write(body)


def read_json(handler: BaseHTTPRequestHandler) -> dict[str, Any]:
    length = int(handler.headers.get("Content-Length", "0"))
    if length <= 0:
        return {}
    return json.loads(handler.rfile.read(length).decode("utf-8"))


def run_command(command: list[str]) -> tuple[int, str]:
    proc = subprocess.run(
        command,
        cwd=REPO_ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return proc.returncode, proc.stdout


def safe_session_id(value: str) -> str:
    return "".join(ch if ch.isalnum() or ch in "_-" else "_" for ch in value) or "session"


def parse_saved_run_dir(output: str) -> Path | None:
    marker = "saved fine-tuned run to "
    for line in output.splitlines():
        if marker not in line:
            continue

        run_text = line.split(marker, 1)[1].strip()
        if " (" in run_text:
            run_text = run_text.split(" (", 1)[0].strip()

        run_dir = Path(run_text)
        if not run_dir.is_absolute():
            run_dir = REPO_ROOT / run_dir
        return run_dir

    return None


def resolve_existing_path(value: Any, label: str) -> Path:
    path = Path(str(value or ""))
    if not path.is_absolute():
        path = REPO_ROOT / path
    if not path.exists():
        raise FileNotFoundError(f"{label} not found: {path}")
    return path


def resolve_run_and_model(value: Any) -> tuple[Path, Path]:
    run_or_model = resolve_existing_path(value, "model run")
    model_path = run_or_model / "model.npz" if run_or_model.is_dir() else run_or_model
    if not model_path.exists():
        raise FileNotFoundError(f"model.npz not found: {model_path}")
    return model_path.parent, model_path


def read_npz_metadata(model_path: Path) -> dict[str, Any]:
    data = np.load(model_path)
    return {
        "modelName": str(data["model_name"].tolist()),
        "modelType": str(data["model_type"].tolist()),
        "hiddenSize": int(data["hidden_size"].tolist()),
    }


def normalize_personal_metrics(metrics: dict[str, Any]) -> dict[str, Any]:
    return {
        "modelPath": metrics["model_path"],
        "modelName": metrics["model_name"],
        "modelType": metrics["model_type"],
        "hiddenSize": metrics["hidden_size"],
        "accuracy": metrics["accuracy"],
        "correct": metrics["correct"],
        "total": metrics["total"],
        "perDigit": metrics["per_digit"],
        "confusion": metrics["confusion"],
    }


def read_run_config(run_dir: Path) -> dict[str, Any]:
    config_path = run_dir / "config.json"
    if not config_path.exists():
        return {}
    return json.loads(config_path.read_text(encoding="utf-8"))


def append_evaluation_record(record: dict[str, Any]) -> None:
    REPORT_RECORDS_PATH.parent.mkdir(parents=True, exist_ok=True)
    with REPORT_RECORDS_PATH.open("a", encoding="utf-8") as f:
        f.write(json.dumps(record, ensure_ascii=False) + "\n")


def list_evaluation_records() -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    if REPORT_RECORDS_PATH.exists():
        with REPORT_RECORDS_PATH.open("r", encoding="utf-8") as f:
            for line in f:
                if line.strip():
                    records.append(json.loads(line))
    records.sort(key=lambda item: str(item.get("createdAt", "")), reverse=True)
    return {"recordsPath": str(REPORT_RECORDS_PATH), "records": records}


def get_evaluation_replay(record_id: str) -> dict[str, Any]:
    records = list_evaluation_records()["records"]
    record = next((item for item in records if item.get("id") == record_id), None)
    if record is None:
        raise FileNotFoundError(f"evaluation record not found: {record_id}")

    eval_path = resolve_existing_path(record.get("evalPath"), "evaluation file")
    eval_data = json.loads(eval_path.read_text(encoding="utf-8"))
    model_data = eval_data.get("model_a", {})
    samples = model_data.get("samples", [])
    if not isinstance(samples, list):
        samples = []

    return {
        "record": record,
        "samples": [
            {
                "index": sample.get("index"),
                "label": sample.get("label"),
                "prediction": sample.get("prediction"),
                "correct": sample.get("correct"),
                "pixelsHex": sample.get("pixels_hex", ""),
                "createdAt": sample.get("created_at", ""),
            }
            for sample in samples
            if isinstance(sample, dict)
        ],
    }


def save_dataset(payload: dict[str, Any]) -> dict[str, Any]:
    session = payload.get("session")
    samples = payload.get("samples")
    if not isinstance(session, dict):
        raise ValueError("session must be an object")
    if not isinstance(samples, list) or not samples:
        raise ValueError("samples must be a non-empty array")

    session_id = safe_session_id(str(session.get("sessionId", "session")))
    dataset_dir = REPO_ROOT / "data" / "personal" / session_id
    dataset_dir.mkdir(parents=True, exist_ok=True)

    samples_path = dataset_dir / "samples.jsonl"
    session_path = dataset_dir / "session.json"
    samples_path.write_text("".join(json.dumps(sample, ensure_ascii=False) + "\n" for sample in samples), encoding="utf-8")
    session_path.write_text(json.dumps(session, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    return {
        "datasetDir": str(dataset_dir),
        "samplesPath": str(samples_path),
        "sessionPath": str(session_path),
        "sampleCount": len(samples),
    }


def list_runs() -> dict[str, Any]:
    runs_dir = REPO_ROOT / "tools" / "train" / "runs"
    runs: list[dict[str, Any]] = []
    if runs_dir.exists():
        for run_dir in sorted((p for p in runs_dir.iterdir() if p.is_dir()), key=lambda p: p.name, reverse=True):
            model_path = run_dir / "model.npz"
            if not model_path.exists():
                continue

            config_path = run_dir / "config.json"
            metrics_path = run_dir / "metrics.json"
            config: dict[str, Any] = {}
            metrics: dict[str, Any] = {}
            if config_path.exists():
                config = json.loads(config_path.read_text(encoding="utf-8"))
            if metrics_path.exists():
                metrics = json.loads(metrics_path.read_text(encoding="utf-8"))

            runs.append(
                {
                    "name": run_dir.name,
                    "path": str(run_dir),
                    "modelPath": str(model_path),
                    "modelName": config.get("model_name", run_dir.name),
                    "modelType": config.get("model_type", "unknown"),
                    "hiddenSize": config.get("hidden_size"),
                    "quant": config.get("quant"),
                    "testAcc": metrics.get("test_acc"),
                }
            )

    return {"runsDir": str(runs_dir), "runs": runs}


def list_personal_datasets() -> dict[str, Any]:
    personal_dir = REPO_ROOT / "data" / "personal"
    datasets: list[dict[str, Any]] = []
    if personal_dir.exists():
        for dataset_dir in sorted((p for p in personal_dir.iterdir() if p.is_dir()), key=lambda p: p.name, reverse=True):
            samples_path = dataset_dir / "samples.jsonl"
            session_path = dataset_dir / "session.json"
            if not samples_path.exists():
                continue

            session: dict[str, Any] = {}
            if session_path.exists():
                session = json.loads(session_path.read_text(encoding="utf-8"))

            sample_count = 0
            with samples_path.open("r", encoding="utf-8") as f:
                sample_count = sum(1 for line in f if line.strip())

            summary = session.get("summary", {}) if isinstance(session.get("summary", {}), dict) else {}
            datasets.append(
                {
                    "name": dataset_dir.name,
                    "path": str(dataset_dir),
                    "samplesPath": str(samples_path),
                    "sessionPath": str(session_path) if session_path.exists() else None,
                    "personName": session.get("personName", ""),
                    "sampleCount": sample_count,
                    "trainAccepted": summary.get("trainAccepted"),
                    "testAccepted": summary.get("testAccepted"),
                    "complete": summary.get("complete"),
                    "baseModel": session.get("baseModel"),
                }
            )

    return {"personalDir": str(personal_dir), "datasets": datasets}


def export_model(payload: dict[str, Any]) -> dict[str, Any]:
    run_dir, model_path = resolve_run_and_model(payload.get("baseRun"))
    config_quant = None
    config = read_run_config(run_dir)
    config_quant = config.get("quant")

    metadata = read_npz_metadata(model_path)
    quant = str(payload.get("quant") or config_quant or ("int8" if metadata["modelType"] == "cnn" else "fp32"))
    if quant not in {"fp32", "int8"}:
        raise ValueError("quant must be fp32 or int8")

    if quant == "int8" and metadata["modelType"] != "cnn":
        quant = "fp32"

    py = sys.executable
    export_cmd = [py, "tools/train/export_model.py", str(model_path), "--quant", quant]
    code, output = run_command(export_cmd)
    if code != 0:
        raise RuntimeError("export failed\n" + output)

    display_model_name = f"{metadata['modelName']}-{quant}"
    export_record = {
        "modelName": metadata["modelName"],
        "displayModelName": display_model_name,
        "modelType": metadata["modelType"],
        "quant": quant,
        "modelDataC": str(REPO_ROOT / "User" / "model" / "ModelData.c"),
        "modelDataH": str(REPO_ROOT / "User" / "model" / "ModelData.h"),
    }
    export_record_path = run_dir / f"export_{quant}.json"
    export_record_path.write_text(json.dumps(export_record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    return {
        "runDir": str(run_dir),
        "exportRecordPath": str(export_record_path),
        "modelDataC": str(REPO_ROOT / "User" / "model" / "ModelData.c"),
        "modelDataH": str(REPO_ROOT / "User" / "model" / "ModelData.h"),
        "modelName": metadata["modelName"],
        "displayModelName": display_model_name,
        "modelType": metadata["modelType"],
        "quant": quant,
        "logs": output,
    }


def evaluate_personal(payload: dict[str, Any]) -> dict[str, Any]:
    samples_path = resolve_existing_path(payload.get("samplesPath"), "personal samples")
    run_dir, model_path = resolve_run_and_model(payload.get("baseRun"))
    eval_path = run_dir / f"personal_eval_{samples_path.parent.name}.json"
    run_config = read_run_config(run_dir)

    py = sys.executable
    eval_cmd = [
        py,
        "tools/train/eval_personal.py",
        "--samples",
        str(samples_path),
        "--model-a",
        str(model_path),
        "--output",
        str(eval_path),
    ]
    code, output = run_command(eval_cmd)
    if code != 0:
        raise RuntimeError("evaluation failed\n" + output)

    eval_data = json.loads(eval_path.read_text(encoding="utf-8"))
    model_metrics = normalize_personal_metrics(eval_data["model_a"])
    record = {
        "id": f"{datetime.now(timezone.utc).strftime('%Y%m%d%H%M%S%f')}-{run_dir.name}-{samples_path.parent.name}",
        "createdAt": datetime.now(timezone.utc).isoformat(),
        "runName": run_dir.name,
        "runDir": str(run_dir),
        "modelName": model_metrics["modelName"],
        "modelType": model_metrics["modelType"],
        "quant": run_config.get("quant"),
        "datasetName": samples_path.parent.name,
        "samplesPath": str(samples_path),
        "evalPath": str(eval_path),
        "accuracy": model_metrics["accuracy"],
        "correct": model_metrics["correct"],
        "total": model_metrics["total"],
    }
    append_evaluation_record(record)
    return {
        "samplesPath": str(samples_path),
        "runDir": str(run_dir),
        "evalPath": str(eval_path),
        "model": model_metrics,
        "record": record,
        "logs": output,
    }


def fine_tune(payload: dict[str, Any]) -> dict[str, Any]:
    samples_path = Path(str(payload.get("samplesPath", "")))
    if not samples_path.is_absolute():
        samples_path = REPO_ROOT / samples_path
    if not samples_path.exists():
        raise FileNotFoundError(samples_path)

    base_run = payload.get("baseRun")
    if not base_run:
        raise ValueError("baseRun is required for fine-tuning")

    base_path = Path(str(base_run))
    if not base_path.is_absolute():
        base_path = REPO_ROOT / base_path
    base_model_path = base_path / "model.npz" if base_path.is_dir() else base_path
    if not base_model_path.exists():
        raise FileNotFoundError(base_model_path)

    epochs = int(payload.get("epochs", 5))
    lr = float(payload.get("lr", 0.0003))
    repeat_personal = int(payload.get("repeatPersonal", 20))
    mnist_limit = int(payload.get("mnistLimit", 10000))
    quant = str(payload.get("quant", "fp32"))
    if quant not in {"fp32", "int8"}:
        raise ValueError("quant must be fp32 or int8")
    base_npz = np.load(base_model_path)
    base_model_type = str(base_npz["model_type"].tolist())
    if quant == "int8" and base_model_type != "cnn":
        quant = "fp32"

    py = sys.executable
    logs: list[str] = []

    fine_cmd = [
        py,
        "tools/train/finetune_personal.py",
        "--samples",
        str(samples_path),
        "--base-run",
        str(base_path),
        "--epochs",
        str(epochs),
        "--lr",
        str(lr),
        "--repeat-personal",
        str(repeat_personal),
        "--mnist-limit",
        str(mnist_limit),
        "--quant-label",
        quant,
    ]
    code, output = run_command(fine_cmd)
    logs.append(output)
    if code != 0:
        raise RuntimeError("fine-tune failed\n" + output)

    run_dir = parse_saved_run_dir(output)
    if run_dir is None or not run_dir.exists():
        raise RuntimeError("could not determine fine-tuned run directory")

    eval_path = run_dir / "personal_eval.json"
    eval_cmd = [
        py,
        "tools/train/eval_personal.py",
        "--samples",
        str(samples_path),
        "--model-a",
        str(base_path),
        "--model-b",
        str(run_dir),
        "--output",
        str(eval_path),
    ]
    code, output = run_command(eval_cmd)
    logs.append(output)
    if code != 0:
        raise RuntimeError("evaluation failed\n" + output)

    eval_data = json.loads(eval_path.read_text(encoding="utf-8"))
    model_name = eval_data["model_b"]["model_name"]

    return {
        "datasetDir": str(samples_path.parent),
        "samplesPath": str(samples_path),
        "runDir": str(run_dir),
        "evalPath": str(eval_path),
        "modelName": model_name,
        "displayModelName": model_name,
        "modelType": eval_data["model_b"]["model_type"],
        "quant": quant,
        "beforeAccuracy": eval_data["model_a"]["accuracy"],
        "afterAccuracy": eval_data["model_b"]["accuracy"],
        "deltaAccuracy": eval_data.get("delta_accuracy"),
        "logs": "\n".join(logs),
    }


class Handler(BaseHTTPRequestHandler):
    def do_OPTIONS(self) -> None:
        json_response(self, 200, {"ok": True})

    def do_GET(self) -> None:
        try:
            parsed = urlparse(self.path)
            path = parsed.path
            query = parse_qs(parsed.query)
            if path == "/api/health":
                json_response(self, 200, {"ok": True, "repoRoot": str(REPO_ROOT)})
                return
            if path == "/api/runs":
                json_response(self, 200, {"ok": True, **list_runs()})
                return
            if path == "/api/personal-datasets":
                json_response(self, 200, {"ok": True, **list_personal_datasets()})
                return
            if path == "/api/evaluation-records":
                json_response(self, 200, {"ok": True, **list_evaluation_records()})
                return
            if path == "/api/evaluation-replay":
                record_id = query.get("id", [""])[0]
                if not record_id:
                    raise ValueError("id is required")
                json_response(self, 200, {"ok": True, **get_evaluation_replay(record_id)})
                return
            json_response(self, 404, {"ok": False, "error": "not found"})
        except Exception as exc:
            json_response(self, 500, {"ok": False, "error": str(exc)})

    def do_POST(self) -> None:
        try:
            payload = read_json(self)
            if self.path == "/api/datasets":
                json_response(self, 200, {"ok": True, **save_dataset(payload)})
                return
            if self.path in {"/api/fine-tune", "/api/train-export"}:
                json_response(self, 200, {"ok": True, **fine_tune(payload)})
                return
            if self.path == "/api/export-model":
                json_response(self, 200, {"ok": True, **export_model(payload)})
                return
            if self.path == "/api/evaluate-personal":
                json_response(self, 200, {"ok": True, **evaluate_personal(payload)})
                return
            json_response(self, 404, {"ok": False, "error": "not found"})
        except Exception as exc:
            json_response(self, 500, {"ok": False, "error": str(exc)})

    def log_message(self, format: str, *args: Any) -> None:
        return


def main() -> None:
    parser = argparse.ArgumentParser(description="HandWrite local helper")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), Handler)
    print(f"HandWrite helper running at http://{args.host}:{args.port}")
    print(f"Repository root: {REPO_ROOT}")
    server.serve_forever()


if __name__ == "__main__":
    main()
