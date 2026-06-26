import type { PredictionFrame } from "../protocol";
import { Top3List } from "./Top3List";

interface ResultPanelProps {
  frame: PredictionFrame | null;
  status: string;
  message: string;
}

export function ResultPanel({ frame, status, message }: ResultPanelProps) {
  return (
    <section className="panel result-panel">
      <div className="panel-header">
        <div>
          <p className="eyebrow">Prediction</p>
          <h2>Recognition result</h2>
        </div>
        <span className={`status-pill ${status}`}>{status}</span>
      </div>

      <div className="result-number">{frame?.result ?? "-"}</div>
      <p className="muted">{message}</p>

      <div className="subsection-title">Top 3</div>
      <Top3List items={frame?.top3 ?? []} />

      <div className="meta">
        <div>
          <span>Model</span>
          <strong>{frame?.model ?? "-"}</strong>
        </div>
        <div>
          <span>Type</span>
          <strong>{frame?.modelType ?? "-"}</strong>
        </div>
        <div>
          <span>Frame</span>
          <strong>{frame?.seq ?? "-"}</strong>
        </div>
        <div>
          <span>Received</span>
          <strong>{frame ? new Date(frame.receivedAt).toLocaleTimeString() : "-"}</strong>
        </div>
      </div>
    </section>
  );
}
