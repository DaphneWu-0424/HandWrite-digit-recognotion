from __future__ import annotations

import argparse
import json
import subprocess
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8765


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


def train_export(payload: dict[str, Any]) -> dict[str, Any]:
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

    epochs = int(payload.get("epochs", 5))
    lr = float(payload.get("lr", 0.0003))
    repeat_personal = int(payload.get("repeatPersonal", 20))
    mnist_limit = int(payload.get("mnistLimit", 10000))
    quant = str(payload.get("quant", "fp32"))
    if quant not in {"fp32", "int8"}:
        raise ValueError("quant must be fp32 or int8")

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

    export_cmd = [py, "tools/train/export_model.py", str(run_dir / "model.npz"), "--quant", quant]
    code, output = run_command(export_cmd)
    logs.append(output)
    if code != 0:
        raise RuntimeError("export failed\n" + output)

    eval_data = json.loads(eval_path.read_text(encoding="utf-8"))
    model_name = eval_data["model_b"]["model_name"]
    display_model_name = f"{model_name}-{quant}"
    export_record = {
        "modelName": model_name,
        "displayModelName": display_model_name,
        "modelType": eval_data["model_b"]["model_type"],
        "quant": quant,
        "modelDataC": str(REPO_ROOT / "User" / "model" / "ModelData.c"),
        "modelDataH": str(REPO_ROOT / "User" / "model" / "ModelData.h"),
        "evalPath": str(eval_path),
    }
    export_record_path = run_dir / f"export_{quant}.json"
    export_record_path.write_text(json.dumps(export_record, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    return {
        "datasetDir": str(samples_path.parent),
        "samplesPath": str(samples_path),
        "runDir": str(run_dir),
        "evalPath": str(eval_path),
        "exportRecordPath": str(export_record_path),
        "modelDataC": str(REPO_ROOT / "User" / "model" / "ModelData.c"),
        "modelDataH": str(REPO_ROOT / "User" / "model" / "ModelData.h"),
        "modelName": model_name,
        "displayModelName": display_model_name,
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
        if self.path == "/api/health":
            json_response(self, 200, {"ok": True, "repoRoot": str(REPO_ROOT)})
            return
        if self.path == "/api/runs":
            json_response(self, 200, {"ok": True, **list_runs()})
            return
        if self.path == "/api/personal-datasets":
            json_response(self, 200, {"ok": True, **list_personal_datasets()})
            return
        json_response(self, 404, {"ok": False, "error": "not found"})

    def do_POST(self) -> None:
        try:
            payload = read_json(self)
            if self.path == "/api/datasets":
                json_response(self, 200, {"ok": True, **save_dataset(payload)})
                return
            if self.path == "/api/train-export":
                json_response(self, 200, {"ok": True, **train_export(payload)})
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
