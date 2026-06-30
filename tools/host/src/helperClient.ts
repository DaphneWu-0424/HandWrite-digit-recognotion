import type { CollectionSession, SessionMetadata } from "./collection";

const HELPER_URL = "http://127.0.0.1:8765";

export interface SaveDatasetResponse {
  ok: true;
  datasetDir: string;
  samplesPath: string;
  sessionPath: string;
  sampleCount: number;
}

export interface TrainExportResponse {
  ok: true;
  datasetDir: string;
  samplesPath: string;
  runDir: string;
  evalPath: string;
  exportRecordPath: string;
  modelDataC: string;
  modelDataH: string;
  modelName: string;
  displayModelName: string;
  modelType: string;
  quant: string;
  beforeAccuracy: number;
  afterAccuracy: number;
  deltaAccuracy: number;
  logs: string;
}

export interface RunInfo {
  name: string;
  path: string;
  modelPath: string;
  modelName: string;
  modelType: string;
  hiddenSize?: number;
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

export async function saveDataset(session: CollectionSession, metadata: SessionMetadata): Promise<SaveDatasetResponse> {
  return postJson<SaveDatasetResponse>("/api/datasets", {
    session: metadata,
    samples: session.samples,
  });
}

export async function trainExport(samplesPath: string, baseRun: string, quant: string): Promise<TrainExportResponse> {
  return postJson<TrainExportResponse>("/api/train-export", {
    samplesPath,
    baseRun,
    quant,
  });
}
