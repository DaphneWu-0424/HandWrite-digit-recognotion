import { useEffect, useMemo, useRef, useState } from "react";
import {
  acceptPendingSample,
  createSession,
  datasetFilename,
  retryPendingSample,
  sessionFilename,
  sessionMetadata,
  sessionToJsonl,
  skipCurrentTarget,
  type CollectionConfig,
  type CollectionSession,
} from "./collection";
import { CollectionPanel } from "./components/CollectionPanel";
import { DigitCanvas } from "./components/DigitCanvas";
import { Modal } from "./components/Modal";
import { ResultPanel } from "./components/ResultPanel";
import {
  checkHelper,
  listPersonalDatasets,
  listRuns,
  saveDataset,
  trainExport,
  type PersonalDatasetInfo,
  type RunInfo,
  type SaveDatasetResponse,
  type TrainExportResponse,
} from "./helperClient";
import { createMockFrame, type PredictionFrame } from "./protocol";
import { SerialConnection, type SerialStatus } from "./serial";

const BAUD_RATE = 115200;
type Page = "evaluation" | "training";

export function App() {
  const [frame, setFrame] = useState<PredictionFrame | null>(() => createMockFrame());
  const [status, setStatus] = useState<SerialStatus>("idle");
  const [message, setMessage] = useState("Mock frame loaded. Connect to USART1 when ready.");
  const [logs, setLogs] = useState<string[]>([]);
  const [autoReconnect, setAutoReconnect] = useState(true);
  const [collectionSession, setCollectionSession] = useState<CollectionSession | null>(null);
  const [helperStatus, setHelperStatus] = useState("unknown");
  const [helperBusy, setHelperBusy] = useState(false);
  const [savedDataset, setSavedDataset] = useState<SaveDatasetResponse | null>(null);
  const [trainResult, setTrainResult] = useState<TrainExportResponse | null>(null);
  const [runs, setRuns] = useState<RunInfo[]>([]);
  const [personalDatasets, setPersonalDatasets] = useState<PersonalDatasetInfo[]>([]);
  const [page, setPage] = useState<Page>("evaluation");
  const [modal, setModal] = useState<{ title: string; body: React.ReactNode } | null>(null);
  const serialRef = useRef<SerialConnection | null>(null);

  const serial = useMemo(() => {
    const instance = new SerialConnection({
      onStatus: (nextStatus, nextMessage) => {
        setStatus(nextStatus);
        if (nextMessage) {
          setMessage(nextMessage);
        }
      },
      onFrame: (nextFrame) => {
        setFrame(nextFrame);
        setCollectionSession((current) => {
          if (!current || current.cursor >= current.targets.length || current.pendingFrame) {
            return current;
          }
          return { ...current, pendingFrame: nextFrame };
        });
      },
      onLog: (line) => setLogs((prev) => [line, ...prev].slice(0, 12)),
    });
    serialRef.current = instance;
    return instance;
  }, []);

  useEffect(() => {
    if (autoReconnect) {
      void serial.connectAuthorized(BAUD_RATE).catch((error) => {
        setStatus("error");
        setMessage(error instanceof Error ? error.message : String(error));
      });
    }
  }, [autoReconnect, serial]);

  useEffect(() => {
    void checkHelper()
      .then((root) => {
        setHelperStatus(`online: ${root}`);
        return Promise.all([listRuns(), listPersonalDatasets()]);
      })
      .then(([runsData, datasetsData]) => {
        setRuns(runsData.runs);
        setPersonalDatasets(datasetsData.datasets);
      })
      .catch(() => setHelperStatus("offline"));
  }, []);

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

  const startCollection = (config: CollectionConfig) => {
    setCollectionSession(createSession(config));
  };

  const downloadText = (filename: string, content: string, type: string) => {
    const blob = new Blob([content], { type });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);
  };

  const exportDataset = () => {
    if (!collectionSession || collectionSession.samples.length === 0) {
      return;
    }

    downloadText(datasetFilename(collectionSession), sessionToJsonl(collectionSession), "application/jsonl;charset=utf-8");
    downloadText(
      sessionFilename(collectionSession),
      `${JSON.stringify(sessionMetadata(collectionSession), null, 2)}\n`,
      "application/json;charset=utf-8",
    );
  };

  const saveDatasetToLocal = async () => {
    if (!collectionSession || collectionSession.samples.length === 0) {
      return;
    }
    setHelperBusy(true);
    try {
      const metadata = sessionMetadata(collectionSession);
      const result = await saveDataset(collectionSession, metadata);
      setSavedDataset(result);
      setHelperStatus("online");
      const [runsResponse, datasetsResponse] = await Promise.all([listRuns(), listPersonalDatasets()]);
      setRuns(runsResponse.runs);
      setPersonalDatasets(datasetsResponse.datasets);
      setModal({
        title: "Dataset saved",
        body: (
          <>
            <p>Samples: {result.sampleCount}</p>
            <pre>{`dataset:\n${result.datasetDir}\n\nsamples:\n${result.samplesPath}\n\nsession:\n${result.sessionPath}`}</pre>
          </>
        ),
      });
    } catch (error) {
      setHelperStatus("error");
      setModal({ title: "Save failed", body: <pre>{error instanceof Error ? error.message : String(error)}</pre> });
    } finally {
      setHelperBusy(false);
    }
  };

  const trainAndExport = async (baseRun: string, samplesPath: string, quant: string) => {
    if (!samplesPath) {
      setModal({ title: "Dataset not selected", body: <p>Select a personal dataset before training.</p> });
      return;
    }
    setHelperBusy(true);
    try {
      const result = await trainExport(samplesPath, baseRun, quant);
      setTrainResult(result);
      setHelperStatus("online");
      const [runsResponse, datasetsResponse] = await Promise.all([listRuns(), listPersonalDatasets()]);
      setRuns(runsResponse.runs);
      setPersonalDatasets(datasetsResponse.datasets);
      setModal({
        title: "Fine-tune and export complete",
        body: (
          <>
            <p>Model: {result.displayModelName}</p>
            <p>Quant: {result.quant}</p>
            <p>
              Before: {(result.beforeAccuracy * 100).toFixed(1)}% | After: {(result.afterAccuracy * 100).toFixed(1)}%
            </p>
            <pre>{`run:\n${result.runDir}\n\nexport record:\n${result.exportRecordPath}\n\nModelData.c:\n${result.modelDataC}\n\nModelData.h:\n${result.modelDataH}\n\nNext:\nBuild with VSCode STM32 plugin, then flash with STM32Programmer.`}</pre>
          </>
        ),
      });
    } catch (error) {
      setHelperStatus("error");
      setModal({ title: "Train/export failed", body: <pre>{error instanceof Error ? error.message : String(error)}</pre> });
    } finally {
      setHelperBusy(false);
    }
  };

  return (
    <main className="app-shell">
      <header className="hero">
        <div>
          <p className="eyebrow">STM32 HandWrite</p>
          <h1>{page === "evaluation" ? "Manual Evaluation" : "Personal Training"}</h1>
          <p className="hero-copy">
            {page === "evaluation"
              ? "Write on the LCD, press OK, and review the live prediction from USART1."
              : "Collect personal train/test samples, fine-tune MLP, and export ModelData."}
          </p>
        </div>
        <div className="controls">
          <button className={page === "evaluation" ? "" : "secondary"} onClick={() => setPage("evaluation")}>
            Evaluation
          </button>
          <button className={page === "training" ? "" : "secondary"} onClick={() => setPage("training")}>
            Personal Training
          </button>
          <button onClick={connect} disabled={status === "connecting" || status === "connected"}>
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

      {page === "evaluation" ? (
        <>
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
        </>
      ) : (
        <CollectionPanel
          session={collectionSession}
          onStart={startCollection}
          onAccept={() => setCollectionSession((current) => (current ? acceptPendingSample(current) : current))}
          onRetry={() => setCollectionSession((current) => (current ? retryPendingSample(current) : current))}
          onSkip={() => setCollectionSession((current) => (current ? skipCurrentTarget(current) : current))}
          onReset={() => setCollectionSession(null)}
          onExport={exportDataset}
          onSaveLocal={saveDatasetToLocal}
          onTrainExport={trainAndExport}
          helperStatus={helperStatus}
          helperBusy={helperBusy}
          runs={runs}
          personalDatasets={personalDatasets}
        />
      )}
      {modal && (
        <Modal title={modal.title} onClose={() => setModal(null)}>
          {modal.body}
        </Modal>
      )}
    </main>
  );
}
