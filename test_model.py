# test_model.py
import sys
import json
import os
import cv2
import numpy as np
import torch
from pathlib import Path
from torch.utils.data import DataLoader
from anomalib.models import EfficientAd
from anomalib.engine import Engine
from anomalib.visualization import visualize_anomaly_map
from anomalib.data.dataclasses.torch import ImageItem
from anomalib.data.dataclasses.torch import ImageBatch

wip_dataset_dir = Path("~/eagle_nest/wip/dataset").expanduser()
resize_dims = (256, 256)

def load_images(resize_dims=None):
    json_path = wip_dataset_dir / "dataset.json"

    if not json_path.exists():
        raise FileNotFoundError(f"JSON file not found at {json_path}")

    images = []

    with open(json_path, "r") as f:
        data = json.load(f)

    for entry in data:
        if entry.get("inclusion") != "Included" or entry.get("dataset_type") != "test":
            continue

        img_path = entry.get("dest_img_path")
        category = entry.get("category")

        if not img_path or not os.path.exists(img_path):
            print(f"Warning: Image path not found or invalid: {img_path}")
            continue

        img = cv2.imread(img_path)
        if img is None:
            print(f"Warning: Unable to load image at {img_path}")
            continue

        if resize_dims:
            img = cv2.resize(img, resize_dims)

        images.append((img, category, img_path))

    print(f"Loaded {len(images)} images for testing")
    return images

class InferenceDataset(torch.utils.data.Dataset):
    def __init__(self, images):
        self.images = images

    def __len__(self):
        return len(self.images)

    def __getitem__(self, idx):
        image, cls, path = self.images[idx]
        image = image.astype(np.float32) / 255.0
        image_tensor = torch.from_numpy(image).permute(2, 0, 1)
        label = 1 if cls == "abnormal" else 0
        return ImageItem(image=image_tensor, gt_label=label, image_path=path)

def build_dataloader(images, batch_size=32):
    dataset = InferenceDataset(images)
    return DataLoader(dataset, batch_size=batch_size, collate_fn=ImageBatch.collate)

def main(model_name: str, resume_from_ckpt: str = ""):
    print(f"Testing {model_name} with saved checkpoint")
    test_dir = wip_dataset_dir / "test"

    if not model_name:
        raise ValueError("[Error] Model name is required.")

    if resume_from_ckpt:
        print(f"[Info] Loading model checkpoint...")
        model = EfficientAd.load_from_checkpoint(resume_from_ckpt)
    else:
        raise ValueError("[Error] Model checkpoint is required.")

    print("[Info] Initializing dataset...")
    images = load_images(resize_dims=resize_dims)
    if not images:
        raise ValueError("[Error] No images found in the test directory.")
    dataloader = build_dataloader(images, batch_size=32)
    
    print("[Info] Initializing engine...")
    engine = Engine()

    print("[Info] Start testing...")
    results = engine.test(model=model, dataloaders=dataloader)
    # Extract the first result (assuming only one dictionary)
    metrics = results[0]

    # Print in separate lines for C++ parsing
    print(f"[Info] AUROC: {metrics['image_AUROC']:.4f}")
    print(f"[Info] F1 Score: {metrics['image_F1Score']:.4f}")

    # Predict on the test set
    predictions = engine.predict(model=model, dataloaders=dataloader)

    # Get the number of batches
    num_batches = len(predictions)
    if num_batches == 0:
        raise ValueError("[Error] No predictions found.")

    heatmap_dir = test_dir / "heatmap"
    heatmap_dir.mkdir(parents=True, exist_ok=True)
    results = []

    for i in range(num_batches):
        batch_pred = predictions[i]
        if not batch_pred:
            raise ValueError(f"[Error] No predictions found for batch {i}.")
        
        num_patches = batch_pred.pred_mask.shape[0]
        print(f"[Batch {i}] Num patches: {num_patches}")

        for j in range(num_patches):
            image_path = Path(batch_pred.image_path[j])

            # Visualize anomaly map
            anomaly_map = batch_pred.anomaly_map[j].squeeze()
            anomaly_map_vis = visualize_anomaly_map(
                anomaly_map,
                colormap=True,      # Apply colormap
                normalize=True      # Normalize values to [0, 255]
            )
            
            # Save anomaly heatmap
            heatmap_path = heatmap_dir / f"{image_path.stem}.png"
            anomaly_map_vis.save(heatmap_path)

            # Extract scalar values; note that these fields should have batch dimension too
            anomaly_score = batch_pred.pred_score[i].item() if batch_pred.pred_score.dim() > 0 else batch_pred.pred_score.item()

            # Record result
            results.append({
                "input_image": str(image_path),
                "anomaly_heatmap": str(heatmap_path),
                "anomaly_score": round(anomaly_score, 3)
            })

    # Save JSON results
    with open(test_dir / "pred_results.json", "w") as f:
        json.dump(results, f, indent=4)

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