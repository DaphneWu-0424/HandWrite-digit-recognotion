import type { PredictionFrame } from "../protocol";

interface DigitCanvasProps {
  frame: PredictionFrame | null;
}

export function DigitCanvas({ frame }: DigitCanvasProps) {
  const cells = frame ? Array.from(frame.pixels) : new Array(28 * 28).fill(0);

  return (
    <section className="panel canvas-panel">
      <div className="panel-header">
        <div>
          <p className="eyebrow">Input Canvas</p>
          <h2>28 x 28 digit map</h2>
        </div>
        <span className="seq">#{frame?.seq ?? "mock"}</span>
      </div>

      <div className="digit-grid" aria-label="Digit preview">
        {cells.map((value, index) => (
          <div
            className="pixel"
            key={index}
            style={{ opacity: value > 0 ? 0.2 + (value / 255) * 0.8 : 0.08 }}
          />
        ))}
      </div>
    </section>
  );
}
