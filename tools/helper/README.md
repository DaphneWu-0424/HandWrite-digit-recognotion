# HandWrite Local Helper

Local HTTP helper for saving personal datasets, running personal fine-tuning, exporting firmware model data, and evaluating reports.

Run from repository root:

```powershell
.\.venv\Scripts\python tools\helper\server.py
```

Default URL:

```text
http://127.0.0.1:8765
```

Supported endpoints:

- `GET /api/health`
- `GET /api/evaluation-records`
- `GET /api/evaluation-replay?id=<recordId>`
- `POST /api/datasets`
- `POST /api/fine-tune`
- `POST /api/export-model`
- `POST /api/evaluate-personal`

Fine-tuning only creates a new run under `tools/train/runs` and evaluates before/after accuracy. Report evaluations are appended to `data/reports/evaluation_records.jsonl`, and replay samples are stored inside each evaluation JSON. Firmware parameters are written only by `POST /api/export-model`. The helper does not compile or flash firmware. After export, build with the VSCode STM32 plugin and flash with STM32Programmer.
