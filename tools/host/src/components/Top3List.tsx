import type { Top3Item } from "../protocol";

interface Top3ListProps {
  items: Top3Item[];
}

export function Top3List({ items }: Top3ListProps) {
  const maxScore = Math.max(...items.map((item) => Math.abs(item.scoreMilli)), 1);

  return (
    <div className="top3">
      {items.map((item, index) => {
        const width = Math.max(8, Math.round((Math.abs(item.scoreMilli) / maxScore) * 100));
        return (
          <div className="top3-row" key={`${item.digit}-${index}`}>
            <div className="rank">#{index + 1}</div>
            <div className="digit-chip">{item.digit}</div>
            <div className="bar-track">
              <div className="bar-fill" style={{ width: `${width}%` }} />
            </div>
            <div className="score">{(item.scoreMilli / 1000).toFixed(3)}</div>
          </div>
        );
      })}
    </div>
  );
}
