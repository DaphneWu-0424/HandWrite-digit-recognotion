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
import { Modal } from "./components/Modal";
import {
  checkHelper,
  evaluatePersonalModel,
  exportFirmwareModel,
  fineTunePersonal,
  listEvaluationRecords,
  listPersonalDatasets,
  listRuns,
  saveDataset,
  type EvaluationRecord,
  type FineTuneResponse,
  type PersonalEvaluationResponse,
  type PersonalDatasetInfo,
  type RunInfo,
  type SaveDatasetResponse,
} from "./helperClient";
import { DatasetPage } from "./pages/DatasetPage";
import { DemoPage } from "./pages/DemoPage";
import { ReportPage } from "./pages/ReportPage";
import { TrainingPage } from "./pages/TrainingPage";
import { createMockFrame, type PredictionFrame } from "./protocol";
import { SerialConnection, type SerialStatus } from "./serial";

const BAUD_RATE = 115200;
type Page = "demo" | "dataset" | "training" | "report";

const pageCopy: Record<Page, { title: string; subtitle: string }> = {
  demo: {
    title: "Evaluation",
    subtitle: "Choose the firmware model, write ModelData, then build, flash, and test over USART1.",
  },
  dataset: {
    title: "Personal Dataset",
    subtitle: "Collect guided train/test samples for personal fine-tuning.",
  },
  training: {
    title: "Training",
    subtitle: "Fine-tune a model with personal data without changing firmware parameters.",
  },
  report: {
    title: "Report",
    subtitle: "Evaluate any model run on a saved personal test split.",
  },
};

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
  const [trainResult, setTrainResult] = useState<FineTuneResponse | null>(null);
  const [reportResult, setReportResult] = useState<PersonalEvaluationResponse | null>(null);
  const [runs, setRuns] = useState<RunInfo[]>([]);
  const [personalDatasets, setPersonalDatasets] = useState<PersonalDatasetInfo[]>([]);
  const [evaluationRecords, setEvaluationRecords] = useState<EvaluationRecord[]>([]);
  const [page, setPage] = useState<Page>("demo");
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
        return Promise.all([listRuns(), listPersonalDatasets(), listEvaluationRecords()]);
      })
      .then(([runsData, datasetsData, recordsData]) => {
        setRuns(runsData.runs);
        setPersonalDatasets(datasetsData.datasets);
        setEvaluationRecords(recordsData.records);
      })
      .catch(() => setHelperStatus("offline"));
  }, []);

  useEffect(() => {
    if (page !== "report") {
      return;
    }

    let cancelled = false;
    const refreshRecords = async () => {
      try {
        const recordsData = await listEvaluationRecords();
        if (!cancelled) {
          setEvaluationRecords(recordsData.records);
        }
      } catch {
        if (!cancelled) {
          setHelperStatus("offline");
        }
      }
    };

    void refreshRecords();
    const timer = window.setInterval(() => void refreshRecords(), 3000);
    return () => {
      cancelled = true;
      window.clearInterval(timer);
    };
  }, [page]);

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

  const fineTuneModel = async (baseRun: string, samplesPath: string, quant: string) => {
    if (!samplesPath) {
      setModal({ title: "Dataset not selected", body: <p>Select a personal dataset before training.</p> });
      return;
    }
    setHelperBusy(true);
    try {
      const result = await fineTunePersonal(samplesPath, baseRun, quant);
      setTrainResult(result);
      setHelperStatus("online");
      const [runsResponse, datasetsResponse] = await Promise.all([listRuns(), listPersonalDatasets()]);
      setRuns(runsResponse.runs);
      setPersonalDatasets(datasetsResponse.datasets);
      setModal({
        title: "Fine-tune complete",
        body: (
          <>
            <p>Model: {result.displayModelName}</p>
            <p>Run label: {result.quant}</p>
            <p>
              Before: {(result.beforeAccuracy * 100).toFixed(1)}% | After: {(result.afterAccuracy * 100).toFixed(1)}%
            </p>
            <pre>{`run:\n${result.runDir}\n\npersonal eval:\n${result.evalPath}\n\nNext:\nUse the Evaluation page when you want to write this run into ModelData.c/.h.`}</pre>
          </>
        ),
      });
    } catch (error) {
      setHelperStatus("error");
      setModal({ title: "Fine-tune failed", body: <pre>{error instanceof Error ? error.message : String(error)}</pre> });
    } finally {
      setHelperBusy(false);
    }
  };

  const exportSelectedModel = async (baseRun: string) => {
    if (!baseRun) {
      setModal({ title: "Model not selected", body: <p>Select a model before writing firmware parameters.</p> });
      return;
    }
    setHelperBusy(true);
    try {
      const result = await exportFirmwareModel(baseRun);
      setHelperStatus("online");
      const runsResponse = await listRuns();
      setRuns(runsResponse.runs);
      setModal({
        title: "Model parameters written",
        body: (
          <>
            <p>
              {result.displayModelName} has been written into <code>ModelData.c</code> and <code>ModelData.h</code>.
            </p>
            <pre>{`ModelData.c:\n${result.modelDataC}\n\nModelData.h:\n${result.modelDataH}\n\nNext:\nBuild in VSCode, then flash the STM32 manually.`}</pre>
          </>
        ),
      });
    } catch (error) {
      setHelperStatus("error");
      setModal({ title: "Export failed", body: <pre>{error instanceof Error ? error.message : String(error)}</pre> });
    } finally {
      setHelperBusy(false);
    }
  };

  const evaluateReport = async (baseRun: string, samplesPath: string) => {
    if (!baseRun || !samplesPath) {
      setModal({ title: "Evaluation input missing", body: <p>Select both a model and a personal dataset first.</p> });
      return;
    }
    setHelperBusy(true);
    try {
      const result = await evaluatePersonalModel(baseRun, samplesPath);
      setReportResult(result);
      setHelperStatus("online");
      const recordsData = await listEvaluationRecords();
      setEvaluationRecords(recordsData.records);
    } catch (error) {
      setHelperStatus("error");
      setModal({ title: "Evaluation failed", body: <pre>{error instanceof Error ? error.message : String(error)}</pre> });
    } finally {
      setHelperBusy(false);
    }
  };

  return (
    <main className="app-shell">
      <header className="hero">
        <div>
          <p className="eyebrow">STM32 HandWrite</p>
          <h1>{pageCopy[page].title}</h1>
          <p className="hero-copy">{pageCopy[page].subtitle}</p>
        </div>
        <div className="controls">
          <button className={page === "demo" ? "" : "secondary"} onClick={() => setPage("demo")}>
            Evaluation
          </button>
          <button className={page === "dataset" ? "" : "secondary"} onClick={() => setPage("dataset")}>
            Dataset
          </button>
          <button className={page === "training" ? "" : "secondary"} onClick={() => setPage("training")}>
            Training
          </button>
          <button className={page === "report" ? "" : "secondary"} onClick={() => setPage("report")}>
            Report
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

      {page === "demo" && (
        <DemoPage
          frame={frame}
          status={status}
          message={message}
          logs={logs}
          baudRate={BAUD_RATE}
          helperStatus={helperStatus}
          helperBusy={helperBusy}
          runs={runs}
          onExportModel={exportSelectedModel}
        />
      )}

      {page === "dataset" && (
        <DatasetPage
          session={collectionSession}
          helperStatus={helperStatus}
          helperBusy={helperBusy}
          onStart={startCollection}
          onAccept={() => setCollectionSession((current) => (current ? acceptPendingSample(current) : current))}
          onRetry={() => setCollectionSession((current) => (current ? retryPendingSample(current) : current))}
          onSkip={() => setCollectionSession((current) => (current ? skipCurrentTarget(current) : current))}
          onReset={() => setCollectionSession(null)}
          onExport={exportDataset}
          onSaveLocal={saveDatasetToLocal}
        />
      )}

      {page === "training" && (
        <TrainingPage
          helperStatus={helperStatus}
          helperBusy={helperBusy}
          onFineTune={fineTuneModel}
          runs={runs}
          personalDatasets={personalDatasets}
        />
      )}

      {page === "report" && (
        <ReportPage
          lastTrainResult={trainResult}
          helperStatus={helperStatus}
          helperBusy={helperBusy}
          runs={runs}
          personalDatasets={personalDatasets}
          reportResult={reportResult}
          evaluationRecords={evaluationRecords}
          onEvaluate={evaluateReport}
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
