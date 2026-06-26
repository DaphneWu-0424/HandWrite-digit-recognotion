import { useState } from "react";
import {
  currentTarget,
  collectionSummary,
  isComplete,
  progressByDigit,
  type CollectionConfig,
  type CollectionSession,
} from "../collection";
import type { PersonalDatasetInfo, RunInfo } from "../helperClient";

interface CollectionPanelProps {
  session: CollectionSession | null;
  onStart(config: CollectionConfig): void;
  onAccept(): void;
  onRetry(): void;
  onSkip(): void;
  onReset(): void;
  onExport(): void;
  onSaveLocal(): void;
  onTrainExport(baseRun: string, samplesPath: string): void;
  helperStatus: string;
  helperBusy: boolean;
  runs: RunInfo[];
  personalDatasets: PersonalDatasetInfo[];
}

export function CollectionPanel({
  session,
  onStart,
  onAccept,
  onRetry,
  onSkip,
  onReset,
  onExport,
  onSaveLocal,
  onTrainExport,
  helperStatus,
  helperBusy,
  runs,
  personalDatasets,
}: CollectionPanelProps) {
  const [personName, setPersonName] = useState("");
  const [trainPerDigit, setTrainPerDigit] = useState(3);
  const [testPerDigit, setTestPerDigit] = useState(2);
  const [shuffle, setShuffle] = useState(true);
  const [baseRun, setBaseRun] = useState("");
  const [personalSamplesPath, setPersonalSamplesPath] = useState("");

  const target = currentTarget(session);
  const complete = isComplete(session);
  const trainCounts = progressByDigit(session, "train");
  const testCounts = progressByDigit(session, "test");
  const summary = collectionSummary(session);

  const submit = () => {
    if (!personName.trim()) {
      return;
    }
    onStart({ personName, trainPerDigit, testPerDigit, shuffle });
  };

  return (
    <section className="panel collection-panel">
      <div className="panel-header">
        <div>
          <p className="eyebrow">Personal Dataset</p>
          <h2>Guided collection</h2>
        </div>
        <span className={`status-pill ${complete ? "connected" : "idle"}`}>
          {complete ? "complete" : session ? "collecting" : "not started"}
        </span>
      </div>

      {!session ? (
        <div className="collection-form">
          <label>
            Person name
            <input
              type="text"
              value={personName}
              onChange={(event) => setPersonName(event.target.value)}
              placeholder="yitia"
            />
          </label>
          <label>
            Train / digit
            <input
              type="number"
              min={1}
              max={20}
              value={trainPerDigit}
              onChange={(event) => setTrainPerDigit(Number(event.target.value))}
            />
          </label>
          <label>
            Test / digit
            <input
              type="number"
              min={1}
              max={20}
              value={testPerDigit}
              onChange={(event) => setTestPerDigit(Number(event.target.value))}
            />
          </label>
          <label className="toggle">
            <input type="checkbox" checked={shuffle} onChange={(event) => setShuffle(event.target.checked)} />
            Shuffle order
          </label>
          <button onClick={submit} disabled={!personName.trim()}>
            Start collection
          </button>
        </div>
      ) : (
        <>
          <div className="target-card">
            <span>Current target</span>
            {target ? (
              <>
                <strong>{target.label}</strong>
                <em>{target.split.toUpperCase()}</em>
              </>
            ) : (
              <>
                <strong>Done</strong>
                <em>DATA COLLECT COMPLETE</em>
              </>
            )}
          </div>

          <div className="summary-grid">
            <div>
              <span>Total</span>
              <strong>
                {summary.totalAccepted}/{summary.totalExpected}
              </strong>
            </div>
            <div>
              <span>Train</span>
              <strong>
                {summary.trainAccepted}/{summary.trainExpected}
              </strong>
            </div>
            <div>
              <span>Test</span>
              <strong>
                {summary.testAccepted}/{summary.testExpected}
              </strong>
            </div>
            <div>
              <span>Data Collect</span>
              <strong>{summary.complete ? "Complete" : "In Progress"}</strong>
            </div>
          </div>

          {session.pendingFrame ? (
            <div className="confirm-card">
              <p>
                Received prediction <strong>{session.pendingFrame.result}</strong> for target{" "}
                <strong>{target?.label}</strong>. Keep this sample?
              </p>
              <div className="confirm-actions">
                <button onClick={onAccept}>Accept</button>
                <button className="secondary" onClick={onRetry}>
                  Retry
                </button>
                <button className="secondary" onClick={onSkip}>
                  Skip
                </button>
              </div>
            </div>
          ) : (
            <p className="muted">Write the target digit on STM32, press OK, then confirm here.</p>
          )}

          <div className="digit-progress">
            {Array.from({ length: 10 }, (_, digit) => (
              <div className="digit-progress-row" key={digit}>
                <span>{digit}</span>
                <b>
                  T {trainCounts[digit]}/{session.config.trainPerDigit}
                </b>
                <b>
                  E {testCounts[digit]}/{session.config.testPerDigit}
                </b>
              </div>
            ))}
          </div>

          <div className="progress-line">
            <span>Base test accuracy</span>
            <strong>{summary.testAccuracy === null ? "-" : `${Math.round(summary.testAccuracy * 1000) / 10}%`}</strong>
          </div>

          <div className="collection-actions">
            <button className="secondary" onClick={onExport} disabled={session.samples.length === 0}>
              Export dataset files
            </button>
            <button className="secondary" onClick={onSaveLocal} disabled={session.samples.length === 0 || helperBusy}>
              Save to local data
            </button>
            <button className="secondary" onClick={onReset}>
              Reset session
            </button>
          </div>
          <div className="helper-box">
            <div className="progress-line">
              <span>Helper</span>
              <strong>{helperStatus}</strong>
            </div>
            <div className="helper-select-grid">
              <label>
                Base MLP run
                {runs.length > 0 ? (
                  <select value={baseRun} onChange={(event) => setBaseRun(event.target.value)}>
                    <option value="">Select a trained base run...</option>
                    {runs.map((run) => (
                      <option value={run.path} key={run.path}>
                        {run.name}
                        {typeof run.testAcc === "number" ? ` | acc ${(run.testAcc * 100).toFixed(1)}%` : ""}
                      </option>
                    ))}
                  </select>
                ) : (
                  <input
                    value={baseRun}
                    onChange={(event) => setBaseRun(event.target.value)}
                    placeholder="tools/train/runs/mlp64_20260626_XXXXXX"
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
            <button
              onClick={() => onTrainExport(baseRun, personalSamplesPath)}
              disabled={!baseRun.trim() || !personalSamplesPath.trim() || helperBusy}
            >
              Train & Export ModelData
            </button>
          </div>
        </>
      )}
    </section>
  );
}
