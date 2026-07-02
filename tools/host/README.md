# HandWrite Host

React Web Serial host for the STM32 handwritten digit recognizer.

## Run

Use Chrome or Edge because Web Serial API is required.

```powershell
cd tools\host
npm install
npm run dev
```

Open the printed local URL, then click `Connect` and select the USART1 USB-TTL serial port.

You can click `Mock frame` before connecting hardware to preview the layout.

## Serial Settings

The STM32 firmware sends JSON Lines through USART1:

- Baud rate: `115200`
- Data bits: `8`
- Parity: `None`
- Stop bits: `1`
- Frame ending: `\n`

The browser cannot scan every system COM port without permission. After the first `Connect`, authorized ports are available through auto reconnect.

## Local helper

For `Save to local data`, `Start Fine Tuning`, report evaluation, and firmware model export, run the helper from repository root:

```powershell
.\.venv\Scripts\python tools\helper\server.py
```

The helper writes datasets to `data/personal/<sessionId>/`, runs personal fine-tuning, evaluates before/after accuracy, and saves report rows to `data/reports/evaluation_records.jsonl`. Fine-tuning does not update firmware parameters; `User/model/ModelData.c/h` is written only from the Evaluation page after selecting a model. It does not build or flash firmware.

## STM32 Frame Shape

Example frame sent by the firmware:

```json
{"type":"prediction","seq":1,"model":"mlp64","modelType":"mlp","w":28,"h":28,"pixels":"00FF...","result":1,"top3":[{"digit":1,"scoreMilli":2210},{"digit":7,"scoreMilli":930},{"digit":8,"scoreMilli":410}]}
```
