import { useState } from "react";
import { DigitCanvas } from "../components/DigitCanvas";
import { ResultPanel } from "../components/ResultPanel";
import type { RunInfo } from "../helperClient";
import type { PredictionFrame } from "../protocol";
import type { SerialStatus } from "../serial";

interface DemoPageProps {
  frame: PredictionFrame | null;
  status: SerialStatus;
  message: string;
  logs: string[];
  baudRate: number;
  helperStatus: string;
  helperBusy: boolean;
  runs: RunInfo[];
  onExportModel(baseRun: string): void;
}

export function DemoPage({
  frame,
  status,
  message,
  logs,
  baudRate,
  helperStatus,
  helperBusy,
  runs,
  onExportModel,
}: DemoPageProps) {
  const [selectedRunPath, setSelectedRunPath] = useState("");
  const selectedRun = runs.find((run) => run.path === selectedRunPath);

  return (
    <>
      <section className="panel evaluation-panel">
        <div className="panel-header">
          <div>
            <p className="eyebrow">Firmware Model</p>
            <h2>Select model for STM32</h2>
          </div>
          <span className="status-pill">{helperStatus}</span>
        </div>

        <div className="helper-select-grid">
          <label>
            Model name
            <select value={selectedRunPath} onChange={(event) => setSelectedRunPath(event.target.value)}>
              <option value="">Select a model to write into firmware...</option>
              {runs.map((run) => (
                <option value={run.path} key={run.path}>
                  {run.name} | {run.modelType}
                  {run.quant ? ` | ${run.quant}` : ""}
                  {typeof run.testAcc === "number" ? ` | MNIST ${(run.testAcc * 100).toFixed(1)}%` : ""}
                </option>
              ))}
            </select>
          </label>
        </div>

        {selectedRun && (
          <div className="model-card">
            <div>
              <span>Selected</span>
              <strong>{selectedRun.modelName}</strong>
            </div>
            <div>
              <span>Type / label</span>
              <strong>
                {selectedRun.modelType}
                {selectedRun.quant ? ` / ${selectedRun.quant}` : ""}
              </strong>
            </div>
            <div>
              <span>MNIST test</span>
              <strong>{typeof selectedRun.testAcc === "number" ? `${(selectedRun.testAcc * 100).toFixed(1)}%` : "-"}</strong>
            </div>
          </div>
        )}

        <div className="workflow-card">
          <ol>
            <li>Choose the model you want to test on STM32.</li>
            <li>Click export to write parameters into <code>User/model/ModelData.c</code> and <code>ModelData.h</code>.</li>
            <li>Build in VSCode, then flash the board manually.</li>
          </ol>
          <button disabled={!selectedRunPath || helperBusy} onClick={() => onExportModel(selectedRunPath)}>
            Write ModelData.c/.h
          </button>
        </div>
      </section>

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
          <span>{baudRate} baud</span>
        </div>
        <pre>{logs.length ? logs.join("\n") : "No serial frames received yet."}</pre>
      </section>
    </>
  );
}
