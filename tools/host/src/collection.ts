import type { PredictionFrame, Top3Item } from "./protocol";

export type DatasetSplit = "train" | "test";

export const PERSONAL_DATASET_SCHEMA_VERSION = 1;

export interface CollectionConfig {
  personName: string;
  trainPerDigit: number;
  testPerDigit: number;
  shuffle: boolean;
}

export interface CollectionTarget {
  split: DatasetSplit;
  label: number;
  index: number;
}

export interface PersonalSample {
  schemaVersion: number;
  sessionId: string;
  personName: string;
  split: DatasetSplit;
  label: number;
  prediction: number;
  model: string;
  modelType: string;
  pixels: string;
  top3: Top3Item[];
  createdAt: string;
}

export interface CollectionSummary {
  trainAccepted: number;
  trainExpected: number;
  testAccepted: number;
  testExpected: number;
  totalAccepted: number;
  totalExpected: number;
  testAccuracy: number | null;
  complete: boolean;
}

export interface SessionMetadata {
  schemaVersion: number;
  sessionId: string;
  personName: string;
  trainPerDigit: number;
  testPerDigit: number;
  digitRange: number[];
  shuffle: boolean;
  startedAt: string;
  exportedAt: string;
  baseModel: string | null;
  baseModelType: string | null;
  summary: CollectionSummary;
}

export interface CollectionSession {
  sessionId: string;
  personName: string;
  config: CollectionConfig;
  targets: CollectionTarget[];
  cursor: number;
  samples: PersonalSample[];
  pendingFrame: PredictionFrame | null;
  startedAt: string;
}

function pad(value: number): string {
  return String(value).padStart(2, "0");
}

function makeSessionId(personName: string): string {
  const now = new Date();
  const stamp = [
    now.getFullYear(),
    pad(now.getMonth() + 1),
    pad(now.getDate()),
    "_",
    pad(now.getHours()),
    pad(now.getMinutes()),
    pad(now.getSeconds()),
  ].join("");
  const safeName = personName.trim().replace(/[^a-zA-Z0-9_-]+/g, "_") || "anonymous";
  return `${stamp}_${safeName}`;
}

function shuffleTargets(targets: CollectionTarget[]): CollectionTarget[] {
  const copy = [...targets];
  for (let i = copy.length - 1; i > 0; i -= 1) {
    const j = Math.floor(Math.random() * (i + 1));
    [copy[i], copy[j]] = [copy[j], copy[i]];
  }
  return copy.map((target, index) => ({ ...target, index }));
}

export function createTargets(config: CollectionConfig): CollectionTarget[] {
  const targets: CollectionTarget[] = [];
  for (let digit = 0; digit <= 9; digit += 1) {
    for (let i = 0; i < config.trainPerDigit; i += 1) {
      targets.push({ split: "train", label: digit, index: targets.length });
    }
    for (let i = 0; i < config.testPerDigit; i += 1) {
      targets.push({ split: "test", label: digit, index: targets.length });
    }
  }

  return config.shuffle ? shuffleTargets(targets) : targets;
}

export function createSession(config: CollectionConfig): CollectionSession {
  const targets = createTargets(config);
  return {
    sessionId: makeSessionId(config.personName),
    personName: config.personName.trim(),
    config,
    targets,
    cursor: 0,
    samples: [],
    pendingFrame: null,
    startedAt: new Date().toISOString(),
  };
}

export function currentTarget(session: CollectionSession | null): CollectionTarget | null {
  if (!session || session.cursor >= session.targets.length) {
    return null;
  }
  return session.targets[session.cursor];
}

export function isComplete(session: CollectionSession | null): boolean {
  return !!session && session.cursor >= session.targets.length;
}

export function frameToHex(frame: PredictionFrame): string {
  return Array.from(frame.pixels)
    .map((value) => value.toString(16).padStart(2, "0").toUpperCase())
    .join("");
}

export function acceptPendingSample(session: CollectionSession): CollectionSession {
  const target = currentTarget(session);
  const frame = session.pendingFrame;
  if (!target || !frame) {
    return session;
  }

  const sample: PersonalSample = {
    schemaVersion: PERSONAL_DATASET_SCHEMA_VERSION,
    sessionId: session.sessionId,
    personName: session.personName,
    split: target.split,
    label: target.label,
    prediction: frame.result,
    model: frame.model,
    modelType: frame.modelType,
    pixels: frameToHex(frame),
    top3: frame.top3,
    createdAt: new Date().toISOString(),
  };

  return {
    ...session,
    samples: [...session.samples, sample],
    pendingFrame: null,
    cursor: session.cursor + 1,
  };
}

export function retryPendingSample(session: CollectionSession): CollectionSession {
  return { ...session, pendingFrame: null };
}

export function skipCurrentTarget(session: CollectionSession): CollectionSession {
  return { ...session, pendingFrame: null, cursor: session.cursor + 1 };
}

export function progressByDigit(session: CollectionSession | null, split: DatasetSplit): number[] {
  const counts = new Array(10).fill(0) as number[];
  if (!session) {
    return counts;
  }

  for (const sample of session.samples) {
    if (sample.split === split) {
      counts[sample.label] += 1;
    }
  }
  return counts;
}

export function splitAccuracy(session: CollectionSession | null, split: DatasetSplit): number | null {
  if (!session) {
    return null;
  }
  const samples = session.samples.filter((sample) => sample.split === split);
  if (samples.length === 0) {
    return null;
  }
  const correct = samples.filter((sample) => sample.label === sample.prediction).length;
  return correct / samples.length;
}

export function collectionSummary(session: CollectionSession | null): CollectionSummary {
  if (!session) {
    return {
      trainAccepted: 0,
      trainExpected: 0,
      testAccepted: 0,
      testExpected: 0,
      totalAccepted: 0,
      totalExpected: 0,
      testAccuracy: null,
      complete: false,
    };
  }

  const trainExpected = 10 * session.config.trainPerDigit;
  const testExpected = 10 * session.config.testPerDigit;
  const trainAccepted = session.samples.filter((sample) => sample.split === "train").length;
  const testAccepted = session.samples.filter((sample) => sample.split === "test").length;

  return {
    trainAccepted,
    trainExpected,
    testAccepted,
    testExpected,
    totalAccepted: session.samples.length,
    totalExpected: session.targets.length,
    testAccuracy: splitAccuracy(session, "test"),
    complete: isComplete(session),
  };
}

export function sessionMetadata(session: CollectionSession): SessionMetadata {
  const firstSample = session.samples[0] ?? null;
  return {
    schemaVersion: PERSONAL_DATASET_SCHEMA_VERSION,
    sessionId: session.sessionId,
    personName: session.personName,
    trainPerDigit: session.config.trainPerDigit,
    testPerDigit: session.config.testPerDigit,
    digitRange: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9],
    shuffle: session.config.shuffle,
    startedAt: session.startedAt,
    exportedAt: new Date().toISOString(),
    baseModel: firstSample?.model ?? null,
    baseModelType: firstSample?.modelType ?? null,
    summary: collectionSummary(session),
  };
}

export function sessionToJsonl(session: CollectionSession): string {
  return session.samples.map((sample) => JSON.stringify(sample)).join("\n") + "\n";
}

export function datasetFilename(session: CollectionSession): string {
  return `${session.sessionId}_samples.jsonl`;
}

export function sessionFilename(session: CollectionSession): string {
  return `${session.sessionId}_session.json`;
}
