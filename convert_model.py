# train_model.py
import sys
from anomalib.engine import Engine
from anomalib.models import EfficientAd
from anomalib.deploy import ExportType
from pathlib import Path

def main(model_name: str):
    print(f"[Info] Converting {model_name} model to ONNX format...")

    if not model_name:
        raise ValueError("Model name is required.")
    
    ckpt_path = Path(f"~/eagle_nest/wip/model/EfficientAd/{model_name}/latest/weights/lightning/model.ckpt").expanduser()
    model = EfficientAd.load_from_checkpoint(str(ckpt_path))
    engine = Engine()

    onnx_path = Path(f"~/eagle_nest/wip/model/EfficientAd/{model_name}/latest/").expanduser()
    engine.export(
        model=model,
        export_root=onnx_path,
        export_type=ExportType.ONNX,
    )

    print("[Info] Converting completed.")

if __name__ == "__main__":
    try:
        model_name = sys.argv[1] if len(sys.argv) > 1 else ""
        main(model_name)
        sys.exit(0)  # Success
    except Exception as e:
        print(f"[Error] Error during converting: {e}", file=sys.stderr)
        sys.exit(1)  # Failure