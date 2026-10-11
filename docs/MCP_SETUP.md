# MCP setup — ComfyUI + Blender (Cursor)

This project uses Cursor MCP servers so the agent can help with generation and
Blender cleanup. Config lives in your **user** file (not committed):

`C:\Users\emilk\.cursor\mcp.json`

---

## What is configured

| Server | Role | Needs |
|--------|------|--------|
| **comfy-cloud** | Generate via Comfy Cloud (templates, partner models) | `COMFY_API_KEY` user env var |
| **comfy-mcp** | Drive **local** Comfy Desktop on `http://127.0.0.1:8188` | Local Comfy running |
| **blender** | Control Blender via `mcp-for-blender` | Blender open + addon server started |

---

## One-time checklist

### 1. Comfy Cloud API key

1. Create a key at [platform.comfy.org/profile/api-keys](https://platform.comfy.org/profile/api-keys)
   (starts with `comfyui-`).
2. Set a **User** environment variable (Windows search → “Environment Variables”):

   - Name: `COMFY_API_KEY`
   - Value: your key (**no** trailing `}` or quotes)

   Or PowerShell (current user):

   ```powershell
   [System.Environment]::SetEnvironmentVariable('COMFY_API_KEY', 'comfyui-YOUR_KEY', 'User')
   ```

3. **Fully quit and reopen Cursor** so it picks up the env var and MCP config.

Cloud generations use Comfy Cloud credits / subscription. Discovery tools are free;
running jobs may require a paid plan — see [Comfy MCP docs](https://docs.comfy.org/agent-tools/mcp).

### 2. Local Comfy MCP

Already installed via `uv tool`:

- `comfy-mcp` → `C:\Users\emilk\.local\bin\comfy-mcp.exe`
- `comfy-cli` default workspace → Comfy Desktop’s ComfyUI folder

Before asking the agent to run local workflows:

1. Launch **Comfy Desktop → Local** until `http://127.0.0.1:8188/system_stats` is OK.
2. Keep your saved workflows available:
   - `Nine Under - Text to Image v1`
   - `Nine Under - Image to 3D v1`

Local MCP can run API-format workflow JSON and inspect your installed nodes/models.
Saved Desktop UI workflows may need to be exported (API format) if the agent cannot
load them by display name — export from Comfy and keep a copy under
`tools/content/comfyui/workflows/` when you want reproducible agent runs.

### 3. Blender MCP

Addon installed for Blender **4.2**:

`%APPDATA%\Blender Foundation\Blender\4.2\scripts\addons\blender_mcp.py`

Each Blender session:

1. Open Blender 4.2.
2. **Edit → Preferences → Add-ons** → enable **Interface: MCP for Blender**
   (or **MCP for Blender**).
3. In the 3D Viewport press **N** → open the **MCP for Blender** / **BlenderMCP** tab.
4. Click **Start MCP Server** (default `localhost:9876`).
5. In Cursor, confirm the `blender` MCP server is green (Tools & MCP).

Only run **one** MCP client against Blender at a time (don’t also connect Claude Desktop).

---

## Asking the agent

Examples after MCP is green:

- “Using local Comfy, help me refine a prop prompt for an ink-stained ruin block, then queue my text-to-image settings.”
- “Load `raw/selected.glb` in Blender, report triangle count, and walk me through selective Decimate with vertex groups.”
- “Scale the selected object so Z height is 0.9 m and set origin to bottom centre.”

Cloud vs local:

- Prefer **local** (`comfy-mcp`) for Nine Under’s free Desktop models (Pixal3D, your checkpoints).
- Prefer **comfy-cloud** when you intentionally want cloud templates / partner APIs.

---

## Security

- Never commit API keys. Prefer `${env:COMFY_API_KEY}` in `mcp.json`.
- If a key was ever pasted into a shared chat or committed file, rotate it on the Comfy platform.
- Blender’s MCP socket has **no auth** — keep it on localhost.

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `comfy-cloud` unauthorized | Key wrong / trailing `}` / Cursor not restarted after `setx` |
| `comfy-mcp` tools fail | Start Local Comfy; check `:8188/system_stats` |
| `blender` tools fail | Enable addon + **Start MCP Server** in Blender sidebar |
| `uvx` / `comfy-mcp` not found | Ensure `%USERPROFILE%\.local\bin` is on User PATH; restart Cursor |
| Wrong Comfy workspace | `comfy set-default "…\ComfyUI-Installs\ComfyUI\ComfyUI" --where local` |
