import { TrainingPanel } from "../components/TrainingPanel";
import type { PersonalDatasetInfo, RunInfo } from "../helperClient";

interface TrainingPageProps {
  helperStatus: string;
  helperBusy: boolean;
  runs: RunInfo[];
  personalDatasets: PersonalDatasetInfo[];
  onFineTune(baseRun: string, samplesPath: string, quant: string): void;
}

export function TrainingPage(props: TrainingPageProps) {
  return <TrainingPanel {...props} />;
}
