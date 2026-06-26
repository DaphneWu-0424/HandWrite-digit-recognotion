import { useEffect, useMemo, useRef, useState } from "react";
import { DigitCanvas } from "./components/DigitCanvas";
import { ResultPanel } from "./components/ResultPanel";
import { createMockFrame, type PredictionFrame } from "./protocol";
import { SerialConnection, type SerialStatus } from "./serial";

const BAUD_RATE = 115200;

export function App() {
  const [frame, setFrame] = useState<PredictionFrame | null>(() => createMockFrame());
  const [status, setStatus] = useState<SerialStatus>("idle");
  const [message, setMessage] = useState("Mock frame loaded. Connect to USART1 when ready.");
  const [logs, setLogs] = useState<string[]>([]);
  const [autoReconnect, setAutoReconnect] = useState(true);
  const serialRef = useRef<SerialConnection | null>(null);

  const serial = useMemo(() => {
    const instance = new SerialConnection({
      onStatus: (nextStatus, nextMessage) => {
        setStatus(nextStatus);
        if (nextMessage) {
          setMessage(nextMessage);
        }
      },
      onFrame: (nextFrame) => setFrame(nextFrame),
      onLog: (line) => setLogs((prev) => [line, ...prev].slice(0, 12)),
    });
    serialRef.current = instance;
    return instance;
  }, []);

  useEffect(() => {
    if (autoReconnect) {
      void serial.connectAuthorized(BAUD_RATE);
    }
  }, [autoReconnect, serial]);

  const connect = async () => {
    try {
      await serial.connectWithPrompt(BAUD_RATE);
    } catch (error) {
      setStatus("error");
      setMessage(error instanceof Error ? error.message : String(error));
    }
  };

  const disconnect = async () => {
    await serial.disconnect();
  };

  return (
    <main className="app-shell">
      <header className="hero">
        <div>
          <p className="eyebrow">STM32 HandWrite</p>
          <h1>Serial Digit Monitor</h1>
          <p className="hero-copy">Live 28x28 canvas, prediction result, and Top3 scores from USART1.</p>
        </div>
        <div className="controls">
          <button onClick={connect} disabled={status === "connecting"}>
            Connect
          </button>
          <button className="secondary" onClick={disconnect}>
            Disconnect
          </button>
          <button className="secondary" onClick={() => setFrame(createMockFrame())}>
            Mock frame
          </button>
          <label className="toggle">
            <input
              type="checkbox"
              checked={autoReconnect}
              onChange={(event) => setAutoReconnect(event.target.checked)}
            />
            Auto reconnect
          </label>
        </div>
      </header>

      <div className="dashboard">
        <DigitCanvas frame={frame} />
        <ResultPanel frame={frame} status={status} message={message} />
      </div>

      <section className="panel log-panel">
        <div className="panel-header">
          <div>
            <p className="eyebrow">Serial Log</p>
            <h2>Recent frames</h2>
          </div>
          <span>{BAUD_RATE} baud</span>
        </div>
        <pre>{logs.length ? logs.join("\n") : "No serial frames received yet."}</pre>
      </section>
    </main>
  );
}
