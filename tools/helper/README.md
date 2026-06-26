# HandWrite Local Helper

Local HTTP helper for saving personal datasets and running fine-tune/export.

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
- `POST /api/datasets`
- `POST /api/train-export`

The helper does not compile or flash firmware. After export, build with the VSCode STM32 plugin and flash with STM32Programmer.
