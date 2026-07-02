import { CollectionPanel } from "../components/CollectionPanel";
import type { CollectionConfig, CollectionSession } from "../collection";

interface DatasetPageProps {
  session: CollectionSession | null;
  helperStatus: string;
  helperBusy: boolean;
  onStart(config: CollectionConfig): void;
  onAccept(): void;
  onRetry(): void;
  onSkip(): void;
  onReset(): void;
  onExport(): void;
  onSaveLocal(): void;
}

export function DatasetPage(props: DatasetPageProps) {
  return <CollectionPanel {...props} />;
}
