# test_model.py
import sys
from pathlib import Path
from anomalib.models import EfficientAd
from anomalib.data import Folder
from anomalib.engine import Engine

def main(model_name: str, resume_from_ckpt: str = ""):
    print(f"Testing {model_name} with saved checkpoint")
    
    if not model_name:
        raise ValueError("[Error] Model name is required.")

    if resume_from_ckpt:
        print(f"[Info] Loading model checkpoint...")
        model = EfficientAd.load_from_checkpoint(resume_from_ckpt)
    else:
        raise ValueError("[Error] Model checkpoint is required.")

    print("[Info] Initializing dataset...")
    datamodule = Folder(
        name=model_name,
        root=Path("~/eagle_nest/wip/dataset/test").expanduser(),
        normal_dir="",
        normal_test_dir="normal",  # Subfolder containing normal test images
        abnormal_dir="abnormal", # Subfolder containing anomaly images
    )

    print("[Info] Initializing engine...")
    engine = Engine()

    print("[Info] Start testing...")
    results = engine.test(model=model, datamodule=datamodule)
    # Extract the first result (assuming only one dictionary)
    metrics = results[0]

    # Print in separate lines for C++ parsing
    print(f"[Info] AUROC: {metrics['image_AUROC']:.4f}")
    print(f"[Info] F1 Score: {metrics['image_F1Score']:.4f}")
    print("[Info] Testing completed.")

if __name__ == "__main__":
    try:
        model_name = sys.argv[1] if len(sys.argv) > 1 else ""
        resume_from_ckpt = sys.argv[2] if len(sys.argv) > 2 else ""
        main(model_name, resume_from_ckpt)
        sys.exit(0)  # Success
    except Exception as e:
        print(f"[Error] Error during testing: {e}", file=sys.stderr)
        sys.exit(1)  # Failure