import { useState } from "react";
import type { PersonalDatasetInfo, RunInfo } from "../helperClient";

interface TrainingPanelProps {
  helperStatus: string;
  helperBusy: boolean;
  runs: RunInfo[];
  personalDatasets: PersonalDatasetInfo[];
  onFineTune(baseRun: string, samplesPath: string, quant: string): void;
}

export function TrainingPanel({
  helperStatus,
  helperBusy,
  runs,
  personalDatasets,
  onFineTune,
}: TrainingPanelProps) {
  const [baseRun, setBaseRun] = useState("");
  const [personalSamplesPath, setPersonalSamplesPath] = useState("");
  const [quant, setQuant] = useState("int8");
  const selectedRun = runs.find((run) => run.path === baseRun);
  const quantOptions =
    selectedRun?.modelType === "cnn"
      ? [
          { value: "int8", label: "INT8 (smaller/faster CNN)" },
          { value: "fp32", label: "FP32 (baseline)" },
        ]
      : [{ value: "fp32", label: "FP32 (MLP/Perceptron only)" }];

  const updateBaseRun = (value: string) => {
    setBaseRun(value);
    const run = runs.find((item) => item.path === value);
    if (run && run.modelType !== "cnn") {
      setQuant("fp32");
    }
  };

  return (
    <section className="panel training-panel">
      <div className="panel-header">
        <div>
          <p className="eyebrow">Fine Tune</p>
          <h2>Train personal model</h2>
        </div>
        <span className="status-pill">{helperStatus}</span>
      </div>

      <div className="helper-select-grid">
        <label>
          Base model run
          {runs.length > 0 ? (
            <select value={baseRun} onChange={(event) => updateBaseRun(event.target.value)}>
              <option value="">Select a trained base run...</option>
              {runs.map((run) => (
                <option value={run.path} key={run.path}>
                  {run.name}
                  {run.quant ? ` | ${run.quant}` : ""}
                  {typeof run.testAcc === "number" ? ` | acc ${(run.testAcc * 100).toFixed(1)}%` : ""}
                </option>
              ))}
            </select>
          ) : (
            <input
              value={baseRun}
              onChange={(event) => setBaseRun(event.target.value)}
              placeholder="tools/train/runs/cnn8x16_YYYYMMDD_HHMMSS"
            />
          )}
        </label>

        <label>
          Personal dataset
          {personalDatasets.length > 0 ? (
            <select value={personalSamplesPath} onChange={(event) => setPersonalSamplesPath(event.target.value)}>
              <option value="">Select saved personal data...</option>
              {personalDatasets.map((dataset) => (
                <option value={dataset.samplesPath} key={dataset.samplesPath}>
                  {dataset.name} | {dataset.sampleCount} samples
                  {dataset.complete ? " | complete" : ""}
                </option>
              ))}
            </select>
          ) : (
            <input
              value={personalSamplesPath}
              onChange={(event) => setPersonalSamplesPath(event.target.value)}
              placeholder="data/personal/session/samples.jsonl"
            />
          )}
        </label>
      </div>

      <label className="training-field">
        Run quant label
        <select value={quant} onChange={(event) => setQuant(event.target.value)}>
          {quantOptions.map((option) => (
            <option value={option.value} key={option.value}>
              {option.label}
            </option>
          ))}
        </select>
      </label>

      <button
        onClick={() => onFineTune(baseRun, personalSamplesPath, quant)}
        disabled={!baseRun.trim() || !personalSamplesPath.trim() || helperBusy}
      >
        Start Fine Tuning
      </button>

      <p className="muted">
        This step only creates a new run under <code>tools/train/runs</code>. Use the Evaluation page later to write a
        selected run into <code>ModelData.c</code> and <code>ModelData.h</code>.
      </p>
    </section>
  );
}
