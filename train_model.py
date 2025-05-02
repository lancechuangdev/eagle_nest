# train_model.py
import sys
from pathlib import Path
from anomalib.data import Folder
from anomalib.models import EfficientAd
from anomalib.engine import Engine

def main(model_name: str, model_size: str = "medium", max_epochs: int = 100):
    print(f"Training {model_name} with model size: {model_size}, max epochs: {max_epochs}")
    
    print("Initializing model...")
    model = EfficientAd(model_size=model_size, pad_maps=False)

    print("Initializing dataset...")
    datamodule = Folder(
        name=model_name,
        root=Path("~/eagle_nest/wip/dataset").expanduser(),
        normal_dir="normal",
        abnormal_dir="abnormal",
        train_batch_size=1,
        eval_batch_size=1,
    )

    print("Initializing engine...")
    engine = Engine(
        max_epochs=max_epochs,
        accelerator="auto",
        devices=1,
        default_root_dir=Path("~/eagle_nest/wip").expanduser(),
    )

    print("Start training...")
    engine.fit(model=model, datamodule=datamodule)
    print("Training completed.")

if __name__ == "__main__":
    model_name = sys.argv[1] if len(sys.argv) > 1 else "eagle_nest_model"
    model_size = sys.argv[2] if len(sys.argv) > 2 else "medium"
    max_epochs = int(sys.argv[3]) if len(sys.argv) > 3 else 100
    main(model_name, model_size, max_epochs)