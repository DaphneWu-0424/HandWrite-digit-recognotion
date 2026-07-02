import type { CollectionSession, SessionMetadata } from "./collection";

const HELPER_URL = "http://127.0.0.1:8765";

export interface SaveDatasetResponse {
  ok: true;
  datasetDir: string;
  samplesPath: string;
  sessionPath: string;
  sampleCount: number;
}

export interface FineTuneResponse {
  ok: true;
  datasetDir: string;
  samplesPath: string;
  runDir: string;
  evalPath: string;
  modelName: string;
  displayModelName: string;
  modelType: string;
  quant: string;
  beforeAccuracy: number;
  afterAccuracy: number;
  deltaAccuracy: number;
  logs: string;
}

export interface FirmwareExportResponse {
  ok: true;
  runDir: string;
  exportRecordPath: string;
  modelDataC: string;
  modelDataH: string;
  modelName: string;
  displayModelName: string;
  modelType: string;
  quant: string;
  logs: string;
}

export interface PersonalModelMetrics {
  modelPath: string;
  modelName: string;
  modelType: string;
  hiddenSize: number;
  accuracy: number;
  correct: number;
  total: number;
  perDigit: Record<string, { accuracy: number | null; correct: number; total: number }>;
  confusion: number[][];
}

export interface PersonalEvaluationResponse {
  ok: true;
  samplesPath: string;
  runDir: string;
  evalPath: string;
  model: PersonalModelMetrics;
  record: EvaluationRecord;
  logs: string;
}

export interface EvaluationRecord {
  id: string;
  createdAt: string;
  runName: string;
  runDir: string;
  modelName: string;
  modelType: string;
  quant?: string | null;
  datasetName: string;
  samplesPath: string;
  evalPath: string;
  accuracy: number;
  correct: number;
  total: number;
}

export interface EvaluationReplaySample {
  index: number;
  label: number;
  prediction: number;
  correct: boolean;
  pixelsHex: string;
  createdAt: string;
}

export interface RunInfo {
  name: string;
  path: string;
  modelPath: string;
  modelName: string;
  modelType: string;
  hiddenSize?: number;
  quant?: string | null;
  testAcc?: number;
}

export interface RunsResponse {
  ok: true;
  runsDir: string;
  runs: RunInfo[];
}

export interface PersonalDatasetInfo {
  name: string;
  path: string;
  samplesPath: string;
  sessionPath?: string | null;
  personName?: string;
  sampleCount: number;
  trainAccepted?: number;
  testAccepted?: number;
  complete?: boolean;
  baseModel?: string | null;
}

export interface PersonalDatasetsResponse {
  ok: true;
  personalDir: string;
  datasets: PersonalDatasetInfo[];
}

export interface EvaluationRecordsResponse {
  ok: true;
  recordsPath: string;
  records: EvaluationRecord[];
}

export interface EvaluationReplayResponse {
  ok: true;
  record: EvaluationRecord;
  samples: EvaluationReplaySample[];
}

interface ErrorResponse {
  ok: false;
  error: string;
}

function isErrorResponse(value: unknown): value is ErrorResponse {
  return typeof value === "object" && value !== null && "ok" in value && (value as { ok: unknown }).ok === false;
}

async function postJson<T>(path: string, payload: unknown): Promise<T> {
  const response = await fetch(`${HELPER_URL}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const data = (await response.json()) as unknown;
  if (!response.ok || isErrorResponse(data)) {
    throw new Error(isErrorResponse(data) ? data.error : `request failed: ${response.status}`);
  }
  return data as T;
}

export async function checkHelper(): Promise<string> {
  const response = await fetch(`${HELPER_URL}/api/health`);
  const data = (await response.json()) as { ok: boolean; repoRoot: string };
  if (!response.ok || !data.ok) {
    throw new Error("helper health check failed");
  }
  return data.repoRoot;
}

export async function listRuns(): Promise<RunsResponse> {
  const response = await fetch(`${HELPER_URL}/api/runs`);
  const data = (await response.json()) as RunsResponse | ErrorResponse;
  if (!response.ok || isErrorResponse(data)) {
    throw new Error(isErrorResponse(data) ? data.error : "failed to list runs");
  }
  return data;
}

export async function listPersonalDatasets(): Promise<PersonalDatasetsResponse> {
  const response = await fetch(`${HELPER_URL}/api/personal-datasets`);
  const data = (await response.json()) as PersonalDatasetsResponse | ErrorResponse;
  if (!response.ok || isErrorResponse(data)) {
    throw new Error(isErrorResponse(data) ? data.error : "failed to list personal datasets");
  }
  return data;
}

export async function listEvaluationRecords(): Promise<EvaluationRecordsResponse> {
  const response = await fetch(`${HELPER_URL}/api/evaluation-records`);
  const data = (await response.json()) as EvaluationRecordsResponse | ErrorResponse;
  if (!response.ok || isErrorResponse(data)) {
    throw new Error(isErrorResponse(data) ? data.error : "failed to list evaluation records");
  }
  return data;
}

export async function getEvaluationReplay(recordId: string): Promise<EvaluationReplayResponse> {
  const response = await fetch(`${HELPER_URL}/api/evaluation-replay?id=${encodeURIComponent(recordId)}`);
  const data = (await response.json()) as EvaluationReplayResponse | ErrorResponse;
  if (!response.ok || isErrorResponse(data)) {
    throw new Error(isErrorResponse(data) ? data.error : "failed to load evaluation replay");
  }
  return data;
}

export async function saveDataset(session: CollectionSession, metadata: SessionMetadata): Promise<SaveDatasetResponse> {
  return postJson<SaveDatasetResponse>("/api/datasets", {
    session: metadata,
    samples: session.samples,
  });
}

export async function fineTunePersonal(samplesPath: string, baseRun: string, quant: string): Promise<FineTuneResponse> {
  return postJson<FineTuneResponse>("/api/fine-tune", {
    samplesPath,
    baseRun,
    quant,
  });
}

export async function exportFirmwareModel(baseRun: string): Promise<FirmwareExportResponse> {
  return postJson<FirmwareExportResponse>("/api/export-model", {
    baseRun,
  });
}

export async function evaluatePersonalModel(
  baseRun: string,
  samplesPath: string,
): Promise<PersonalEvaluationResponse> {
  return postJson<PersonalEvaluationResponse>("/api/evaluate-personal", {
    baseRun,
    samplesPath,
  });
}
