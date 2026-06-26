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

## STM32 Frame Shape

Example frame sent by the firmware:

```json
{"type":"prediction","seq":1,"w":28,"h":28,"pixels":"00FF...","result":1,"top3":[{"digit":1,"scoreMilli":2210},{"digit":7,"scoreMilli":930},{"digit":8,"scoreMilli":410}]}
```
