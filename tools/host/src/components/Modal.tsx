interface ModalProps {
  title: string;
  children: React.ReactNode;
  onClose(): void;
}

export function Modal({ title, children, onClose }: ModalProps) {
  return (
    <div className="modal-backdrop" role="presentation">
      <div className="modal" role="dialog" aria-modal="true" aria-label={title}>
        <div className="panel-header">
          <div>
            <p className="eyebrow">Notification</p>
            <h2>{title}</h2>
          </div>
          <button className="secondary" onClick={onClose}>
            Close
          </button>
        </div>
        <div className="modal-content">{children}</div>
      </div>
    </div>
  );
}
