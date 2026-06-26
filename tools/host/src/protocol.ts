export interface Top3Item {
  digit: number;
  scoreMilli: number;
}

export interface PredictionFrame {
  type: "prediction";
  seq: number;
  w: number;
  h: number;
  pixels: Uint8Array;
  result: number;
  top3: Top3Item[];
  model: string;
  modelType: string;
  receivedAt: number;
  raw: string;
}

interface RawPredictionFrame {
  type: string;
  seq: number;
  w: number;
  h: number;
  pixels: string;
  result: number;
  top3: Top3Item[];
  model?: string;
  modelType?: string;
}

function decodeHexPixels(hex: string, expectedLength: number): Uint8Array {
  if (hex.length !== expectedLength * 2) {
    throw new Error(`pixel payload length ${hex.length}, expected ${expectedLength * 2}`);
  }

  const pixels = new Uint8Array(expectedLength);
  for (let i = 0; i < expectedLength; i += 1) {
    const byte = Number.parseInt(hex.slice(i * 2, i * 2 + 2), 16);
    if (Number.isNaN(byte)) {
      throw new Error(`invalid hex byte at index ${i}`);
    }
    pixels[i] = byte;
  }
  return pixels;
}

export function parsePredictionLine(line: string): PredictionFrame | null {
  const trimmed = line.trim();
  if (!trimmed) {
    return null;
  }

  const raw = JSON.parse(trimmed) as RawPredictionFrame;
  if (raw.type !== "prediction") {
    return null;
  }
  if (raw.w !== 28 || raw.h !== 28) {
    throw new Error(`unsupported frame size ${raw.w}x${raw.h}`);
  }
  if (!Array.isArray(raw.top3) || raw.top3.length === 0) {
    throw new Error("missing top3 result list");
  }

  return {
    type: "prediction",
    seq: raw.seq,
    w: raw.w,
    h: raw.h,
    pixels: decodeHexPixels(raw.pixels, raw.w * raw.h),
    result: raw.result,
    top3: raw.top3.slice(0, 3),
    model: raw.model ?? "unknown",
    modelType: raw.modelType ?? "unknown",
    receivedAt: Date.now(),
    raw: trimmed,
  };
}

export function createMockFrame(): PredictionFrame {
  const pixels = new Uint8Array(28 * 28);
  for (let y = 5; y < 23; y += 1) {
    const x = 14 + Math.floor((y - 5) / 8);
    pixels[y * 28 + x] = 255;
    pixels[y * 28 + x - 1] = 180;
  }

  return {
    type: "prediction",
    seq: 0,
    w: 28,
    h: 28,
    pixels,
    result: 1,
    model: "mlp64",
    modelType: "mlp",
    top3: [
      { digit: 1, scoreMilli: 2210 },
      { digit: 7, scoreMilli: 930 },
      { digit: 8, scoreMilli: 410 },
    ],
    receivedAt: Date.now(),
    raw: "mock",
  };
}
