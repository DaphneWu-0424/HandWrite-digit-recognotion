import { useEffect, useState } from "react";
import type {
  EvaluationRecord,
  EvaluationReplayResponse,
  EvaluationReplaySample,
  FineTuneResponse,
  PersonalDatasetInfo,
  PersonalEvaluationResponse,
  RunInfo,
} from "../helperClient";
import { getEvaluationReplay } from "../helperClient";

interface ReportPageProps {
  lastTrainResult: FineTuneResponse | null;
  helperStatus: string;
  helperBusy: boolean;
  runs: RunInfo[];
  personalDatasets: PersonalDatasetInfo[];
  reportResult: PersonalEvaluationResponse | null;
  evaluationRecords: EvaluationRecord[];
  onEvaluate(baseRun: string, samplesPath: string): void;
}

function pixelsFromHex(hexText: string): number[] {
  const pixels: number[] = [];
  for (let index = 0; index < hexText.length; index += 2) {
    pixels.push(Number.parseInt(hexText.slice(index, index + 2), 16) || 0);
  }
  return pixels;
}

function ReplaySampleCard({ sample }: { sample: EvaluationReplaySample }) {
  const pixels = pixelsFromHex(sample.pixelsHex);
  return (
    <div className={`replay-sample ${sample.correct ? "correct" : "wrong"}`}>
      <div className="replay-digit-grid" aria-label={`label ${sample.label}, prediction ${sample.prediction}`}>
        {pixels.map((value, index) => (
          <span
            className="replay-pixel"
            key={index}
            style={{ backgroundColor: `rgb(${value}, ${value}, ${value})` }}
          />
        ))}
      </div>
      <div className="replay-caption">
        <strong>
          Label {sample.label} / Pred {sample.prediction}
        </strong>
        <span>{sample.correct ? "correct" : "wrong"}</span>
      </div>
    </div>
  );
}

export function ReportPage({
  lastTrainResult,
  helperStatus,
  helperBusy,
  runs,
  personalDatasets,
  reportResult,
  evaluationRecords,
  onEvaluate,
}: ReportPageProps) {
  const [selectedRunPath, setSelectedRunPath] = useState(lastTrainResult?.runDir ?? "");
  const [selectedSamplesPath, setSelectedSamplesPath] = useState(lastTrainResult?.samplesPath ?? "");
  const selectedRun = runs.find((run) => run.path === selectedRunPath);
  const selectedDataset = personalDatasets.find((dataset) => dataset.samplesPath === selectedSamplesPath);
  const latestRecordId = reportResult?.record.id;
  const [selectedRecordId, setSelectedRecordId] = useState(latestRecordId ?? "");
  const [replay, setReplay] = useState<EvaluationReplayResponse | null>(null);
  const [replayBusy, setReplayBusy] = useState(false);
  const [replayError, setReplayError] = useState("");

  useEffect(() => {
    if (latestRecordId) {
      setSelectedRecordId(latestRecordId);
    }
  }, [latestRecordId]);

  useEffect(() => {
    if (!selectedRecordId) {
      setReplay(null);
      return;
    }

    let cancelled = false;
    setReplayBusy(true);
    setReplayError("");
    void getEvaluationReplay(selectedRecordId)
      .then((data) => {
        if (!cancelled) {
          setReplay(data);
        }
      })
      .catch((error) => {
        if (!cancelled) {
          setReplay(null);
          setReplayError(error instanceof Error ? error.message : String(error));
        }
      })
      .finally(() => {
        if (!cancelled) {
          setReplayBusy(false);
        }
      });

    return () => {
      cancelled = true;
    };
  }, [selectedRecordId]);

  return (
    <section className="panel report-panel">
      <div className="panel-header">
        <div>
          <p className="eyebrow">Report</p>
          <h2>Personal test records</h2>
        </div>
        <div className="report-status">
          <span className="status-pill">{helperStatus}</span>
          <span className="muted">Auto refresh: 3s</span>
        </div>
      </div>

      <div className="helper-select-grid">
        <label>
          Model run
          <select value={selectedRunPath} onChange={(event) => setSelectedRunPath(event.target.value)}>
            <option value="">Select model to evaluate...</option>
            {runs.map((run) => (
              <option value={run.path} key={run.path}>
                {run.name} | {run.modelType}
                {run.quant ? ` | ${run.quant}` : ""}
                {typeof run.testAcc === "number" ? ` | MNIST ${(run.testAcc * 100).toFixed(1)}%` : ""}
              </option>
            ))}
          </select>
        </label>

        <label>
          Personal test set
          <select value={selectedSamplesPath} onChange={(event) => setSelectedSamplesPath(event.target.value)}>
            <option value="">Select personal dataset...</option>
            {personalDatasets.map((dataset) => (
              <option value={dataset.samplesPath} key={dataset.samplesPath}>
                {dataset.name} | {dataset.sampleCount} samples
                {typeof dataset.testAccepted === "number" ? ` | test ${dataset.testAccepted}` : ""}
              </option>
            ))}
          </select>
        </label>
      </div>

      <button
        className="report-action"
        disabled={!selectedRunPath || !selectedSamplesPath || helperBusy}
        onClick={() => onEvaluate(selectedRunPath, selectedSamplesPath)}
      >
        Run Personal Evaluation
      </button>

      {(selectedRun || selectedDataset) && (
        <div className="model-card">
          <div>
            <span>Model</span>
            <strong>{selectedRun?.modelName ?? "-"}</strong>
          </div>
          <div>
            <span>Dataset</span>
            <strong>{selectedDataset?.name ?? "-"}</strong>
          </div>
          <div>
            <span>Test samples</span>
            <strong>{selectedDataset?.testAccepted ?? "-"}</strong>
          </div>
        </div>
      )}

      {evaluationRecords.length > 0 ? (
        <div className="report-table-wrap">
          <table className="report-table">
            <thead>
              <tr>
                <th>Time</th>
                <th>Model</th>
                <th>Type</th>
                <th>Dataset</th>
                <th>Accuracy</th>
                <th>Correct</th>
                <th>Eval file</th>
              </tr>
            </thead>
            <tbody>
              {evaluationRecords.map((record) => (
                <tr
                  className={`${record.id === latestRecordId ? "latest" : ""} ${
                    record.id === selectedRecordId ? "selected" : ""
                  }`}
                  key={record.id}
                  onClick={() => setSelectedRecordId(record.id)}
                >
                  <td>{new Date(record.createdAt).toLocaleString()}</td>
                  <td>
                    <strong>{record.modelName}</strong>
                    <span>{record.runName}</span>
                  </td>
                  <td>
                    {record.modelType}
                    {record.quant ? ` / ${record.quant}` : ""}
                  </td>
                  <td>{record.datasetName}</td>
                  <td>{(record.accuracy * 100).toFixed(1)}%</td>
                  <td>
                    {record.correct}/{record.total}
                  </td>
                  <td>{record.evalPath.split(/[\\/]/).pop()}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      ) : (
        <p className="muted">
          Select a model and a saved personal dataset, then run evaluation. Results will be saved locally and shown here.
        </p>
      )}

      {selectedRecordId && (
        <section className="replay-panel">
          <div className="panel-header">
            <div>
              <p className="eyebrow">Replay</p>
              <h2>{replay ? replay.record.runName : "Loading selected record"}</h2>
            </div>
            {replay && (
              <span className="status-pill">
                {replay.samples.length} samples | {(replay.record.accuracy * 100).toFixed(1)}%
              </span>
            )}
          </div>

          {replayBusy && <p className="muted">Loading replay samples...</p>}
          {replayError && <p className="muted">{replayError}</p>}
          {replay && replay.samples.length === 0 && (
            <p className="muted">This record was created before replay samples were saved. Run evaluation again to replay images.</p>
          )}
          {replay && replay.samples.length > 0 && (
            <div className="replay-grid">
              {replay.samples.map((sample) => (
                <ReplaySampleCard sample={sample} key={sample.index} />
              ))}
            </div>
          )}
        </section>
      )}
    </section>
  );
}
