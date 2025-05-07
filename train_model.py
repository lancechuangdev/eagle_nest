# train_model.py
import sys
from pathlib import Path
from anomalib.data import Folder
from anomalib.models import EfficientAd
from anomalib.engine import Engine

def main(model_name: str, model_size: str = "medium", max_epochs: int = 100, resume_from_ckpt: str = ""):
    print(f"Training {model_name} with model size: {model_size}, max epochs: {max_epochs}")
    
    if not model_name:
        raise ValueError("[Error] Model name is required.")

    if resume_from_ckpt:
        print(f"[Info] Loading model checkpoint...")
        model = EfficientAd.load_from_checkpoint(resume_from_ckpt)
    else:
        print("[Info] Initializing model...")
        model = EfficientAd(model_size=model_size, pad_maps=False)

    print("[Info] Initializing dataset...")
    datamodule = Folder(
        name=model_name,
        root=Path("~/eagle_nest/wip/dataset").expanduser(),
        normal_dir="normal",
        abnormal_dir="abnormal",
        train_batch_size=1,
        eval_batch_size=1,
    )

    print("[Info] Initializing engine...")
    engine = Engine(
        max_epochs=max_epochs,
        accelerator="auto",
        devices=1,
        default_root_dir=Path("~/eagle_nest/wip/model").expanduser(),
    )

    print("[Info] Start training...")
    engine.fit(model=model, datamodule=datamodule)
    print("[Info] Training completed.")

if __name__ == "__main__":
    try:
        model_name = sys.argv[1] if len(sys.argv) > 1 else ""
        model_size = sys.argv[2] if len(sys.argv) > 2 else "medium"
        max_epochs = int(sys.argv[3]) if len(sys.argv) > 3 else 100
        resume_from_ckpt = sys.argv[4] if len(sys.argv) > 4 else ""
        main(model_name, model_size, max_epochs, resume_from_ckpt)
        sys.exit(0)  # Success
    except Exception as e:
        print(f"[Error] Error during training: {e}", file=sys.stderr)
        sys.exit(1)  # Failure