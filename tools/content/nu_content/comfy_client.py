from __future__ import annotations

import json
import os
import time
import uuid
from pathlib import Path
from typing import Any

import httpx

from nu_content.paths import workflows_dir


def comfy_url() -> str:
    return os.environ.get("NU_COMFYUI_URL", "http://127.0.0.1:8188").rstrip("/")


class ComfyClient:
    def __init__(self, base_url: str | None = None, timeout: float = 600.0) -> None:
        self.base = base_url or comfy_url()
        self.client = httpx.Client(base_url=self.base, timeout=timeout)
        self.client_id = str(uuid.uuid4())

    def close(self) -> None:
        self.client.close()

    def __enter__(self) -> ComfyClient:
        return self

    def __exit__(self, *args: object) -> None:
        self.close()

    def ping(self) -> bool:
        try:
            r = self.client.get("/system_stats")
            return r.status_code == 200
        except httpx.HTTPError:
            return False

    def load_workflow(self, name: str) -> dict[str, Any]:
        path = workflows_dir() / name
        if not path.suffix:
            path = workflows_dir() / f"{name}.json"
        with path.open(encoding="utf-8") as f:
            return json.load(f)

    def queue_prompt(self, workflow: dict[str, Any]) -> str:
        payload = {"prompt": workflow, "client_id": self.client_id}
        r = self.client.post("/prompt", json=payload)
        r.raise_for_status()
        data = r.json()
        return str(data["prompt_id"])

    def wait_prompt(self, prompt_id: str, poll_s: float = 1.0) -> dict[str, Any]:
        while True:
            r = self.client.get(f"/history/{prompt_id}")
            r.raise_for_status()
            hist = r.json()
            if prompt_id in hist:
                return hist[prompt_id]
            time.sleep(poll_s)

    def upload_image(self, path: Path, overwrite: bool = True) -> str:
        with path.open("rb") as f:
            files = {"image": (path.name, f, "application/octet-stream")}
            data = {"overwrite": "true" if overwrite else "false"}
            r = self.client.post("/upload/image", files=files, data=data)
        r.raise_for_status()
        return str(r.json().get("name", path.name))

    def download_view(self, filename: str, subfolder: str = "", folder_type: str = "output") -> bytes:
        r = self.client.get(
            "/view",
            params={"filename": filename, "subfolder": subfolder, "type": folder_type},
        )
        r.raise_for_status()
        return r.content

    def collect_output_files(self, history: dict[str, Any]) -> list[dict[str, str]]:
        files: list[dict[str, str]] = []
        outputs = history.get("outputs", {})
        for node_out in outputs.values():
            for key in ("images", "gifs", "meshes", "files"):
                for item in node_out.get(key, []):
                    if isinstance(item, dict) and "filename" in item:
                        files.append(
                            {
                                "filename": item["filename"],
                                "subfolder": item.get("subfolder", ""),
                                "type": item.get("type", "output"),
                            }
                        )
        return files


def patch_workflow(workflow: dict[str, Any], replacements: dict[str, Any]) -> dict[str, Any]:
    """Replace placeholders like {{PROMPT}} / {{SEED}} in the workflow JSON.

    Numeric values replace the quoted token (`\"{{SEED}}\"` → `42`) so ComfyUI
    receives ints/floats instead of strings.
    """
    text = json.dumps(workflow)
    for key, value in replacements.items():
        token = "{{" + key + "}}"
        quoted = f'"{token}"'
        if isinstance(value, bool):
            text = text.replace(quoted, "true" if value else "false")
        elif isinstance(value, (int, float)) and not isinstance(value, bool):
            text = text.replace(quoted, json.dumps(value))
        else:
            text = text.replace(token, str(value))
    return json.loads(text)
