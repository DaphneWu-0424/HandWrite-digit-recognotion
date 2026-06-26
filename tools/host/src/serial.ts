import { parsePredictionLine, type PredictionFrame } from "./protocol";

export type SerialStatus = "unsupported" | "idle" | "connecting" | "connected" | "error";

export interface SerialCallbacks {
  onStatus(status: SerialStatus, message?: string): void;
  onFrame(frame: PredictionFrame): void;
  onLog(line: string): void;
}

export class SerialConnection {
  private port: SerialPort | null = null;
  private keepReading = false;
  private readonly decoder = new TextDecoder();
  private buffer = "";

  constructor(private readonly callbacks: SerialCallbacks) {}

  isSupported(): boolean {
    return typeof navigator !== "undefined" && !!navigator.serial;
  }

  async connectWithPrompt(baudRate: number): Promise<void> {
    if (!navigator.serial) {
      this.callbacks.onStatus("unsupported", "Web Serial API is not available.");
      return;
    }

    this.callbacks.onStatus("connecting", "Waiting for port selection...");
    const port = await navigator.serial.requestPort();
    await this.openPort(port, baudRate);
  }

  async connectAuthorized(baudRate: number): Promise<boolean> {
    if (!navigator.serial) {
      this.callbacks.onStatus("unsupported", "Web Serial API is not available.");
      return false;
    }

    const ports = await navigator.serial.getPorts();
    if (ports.length === 0) {
      this.callbacks.onStatus("idle", "No authorized serial ports yet.");
      return false;
    }

    this.callbacks.onStatus("connecting", "Opening authorized serial port...");
    await this.openPort(ports[0], baudRate);
    return true;
  }

  async disconnect(): Promise<void> {
    this.keepReading = false;
    const current = this.port;
    this.port = null;
    if (current) {
      try {
        await current.close();
      } catch {
        // Some browsers reject close while a reader is unwinding.
      }
    }
    this.callbacks.onStatus("idle", "Disconnected.");
  }

  private async openPort(port: SerialPort, baudRate: number): Promise<void> {
    this.port = port;
    await port.open({ baudRate });
    this.keepReading = true;
    this.callbacks.onStatus("connected", this.describePort(port));
    void this.readLoop(port);
  }

  private async readLoop(port: SerialPort): Promise<void> {
    while (this.keepReading && port.readable) {
      const reader = port.readable.getReader();
      try {
        while (this.keepReading) {
          const { value, done } = await reader.read();
          if (done) {
            break;
          }
          if (value) {
            this.consumeText(this.decoder.decode(value, { stream: true }));
          }
        }
      } catch (error) {
        this.callbacks.onStatus("error", error instanceof Error ? error.message : String(error));
      } finally {
        reader.releaseLock();
      }
    }

    if (this.keepReading) {
      this.callbacks.onStatus("idle", "Serial stream ended.");
    }
  }

  private consumeText(chunk: string): void {
    this.buffer += chunk;
    let newline = this.buffer.indexOf("\n");
    while (newline >= 0) {
      const line = this.buffer.slice(0, newline);
      this.buffer = this.buffer.slice(newline + 1);
      this.handleLine(line);
      newline = this.buffer.indexOf("\n");
    }
  }

  private handleLine(line: string): void {
    const trimmed = line.trim();
    if (!trimmed) {
      return;
    }
    this.callbacks.onLog(trimmed);

    try {
      const frame = parsePredictionLine(trimmed);
      if (frame) {
        this.callbacks.onFrame(frame);
      }
    } catch (error) {
      this.callbacks.onStatus("error", error instanceof Error ? error.message : String(error));
    }
  }

  private describePort(port: SerialPort): string {
    const info = port.getInfo();
    const vendor = info.usbVendorId?.toString(16).padStart(4, "0") ?? "unknown";
    const product = info.usbProductId?.toString(16).padStart(4, "0") ?? "unknown";
    return `Connected: VID ${vendor}, PID ${product}`;
  }
}
