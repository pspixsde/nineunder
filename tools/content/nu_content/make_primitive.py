from __future__ import annotations

import json
import struct
import zlib
from pathlib import Path


def _png_rgba(width: int, height: int, rgba: tuple[int, int, int, int]) -> bytes:
    """Minimal uncompressed-ish PNG (filter 0 + zlib)."""
    r, g, b, a = rgba
    raw = bytearray()
    row = bytes([0]) + bytes([r, g, b, a]) * width
    for _ in range(height):
        raw.extend(row)
    compressed = zlib.compress(bytes(raw), 9)

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", compressed) + chunk(b"IEND", b"")


def write_textured_cube_glb(path: Path, *, size: float = 1.0, subdivisions: int = 10) -> None:
    """Write a Y-up textured cube GLB (bottom at y=0).

    ``subdivisions`` per edge (≥10 → ≥600 tris) so the sample clears prop budgets.
    """
    h = size
    div = max(1, subdivisions)
    positions: list[float] = []
    normals: list[float] = []
    uvs: list[float] = []
    tangents: list[float] = []
    indices: list[int] = []

    def add_grid_face(
        origin: tuple[float, float, float],
        axis_u: tuple[float, float, float],
        axis_v: tuple[float, float, float],
        normal: tuple[float, float, float],
        tangent: tuple[float, float, float],
    ) -> None:
        base = len(positions) // 3
        for j in range(div + 1):
            v = j / div
            for i in range(div + 1):
                u = i / div
                x = origin[0] + axis_u[0] * u + axis_v[0] * v
                y = origin[1] + axis_u[1] * u + axis_v[1] * v
                z = origin[2] + axis_u[2] * u + axis_v[2] * v
                positions.extend([x - h * 0.5, y, z - h * 0.5])
                normals.extend(list(normal))
                uvs.extend([u, v])
                tangents.extend([tangent[0], tangent[1], tangent[2], 1.0])
        for j in range(div):
            for i in range(div):
                i0 = base + j * (div + 1) + i
                i1 = i0 + 1
                i2 = i0 + (div + 1)
                i3 = i2 + 1
                indices.extend([i0, i1, i3, i0, i3, i2])

    # Six faces of the cube [0,h]^3 before centering on XZ
    add_grid_face((0, h, 0), (h, 0, 0), (0, 0, h), (0, 1, 0), (1, 0, 0))  # +Y
    add_grid_face((0, 0, h), (h, 0, 0), (0, 0, -h), (0, -1, 0), (1, 0, 0))  # -Y
    add_grid_face((h, 0, 0), (0, 0, h), (0, h, 0), (1, 0, 0), (0, 0, 1))  # +X
    add_grid_face((0, 0, h), (0, 0, -h), (0, h, 0), (-1, 0, 0), (0, 0, -1))  # -X
    add_grid_face((h, 0, h), (-h, 0, 0), (0, h, 0), (0, 0, 1), (-1, 0, 0))  # +Z
    add_grid_face((0, 0, 0), (h, 0, 0), (0, h, 0), (0, 0, -1), (1, 0, 0))  # -Z

    pos_b = struct.pack(f"<{len(positions)}f", *positions)
    nrm_b = struct.pack(f"<{len(normals)}f", *normals)
    uv_b = struct.pack(f"<{len(uvs)}f", *uvs)
    tan_b = struct.pack(f"<{len(tangents)}f", *tangents)
    idx_b = struct.pack(f"<{len(indices)}H", *indices)
    png = _png_rgba(64, 64, (120, 110, 100, 255))

    def align4(b: bytes, pad_byte: bytes = b"\x00") -> bytes:
        pad = (4 - (len(b) % 4)) % 4
        return b + (pad_byte * pad)

    blobs = [align4(pos_b), align4(nrm_b), align4(uv_b), align4(tan_b), align4(idx_b), align4(png)]
    offsets = []
    cursor = 0
    for b in blobs:
        offsets.append(cursor)
        cursor += len(b)
    bin_blob = b"".join(blobs)

    accessors = []
    buffer_views = []

    def add_view(offset: int, length: int, target: int | None = None) -> int:
        view: dict = {"buffer": 0, "byteOffset": offset, "byteLength": length}
        if target is not None:
            view["target"] = target
        buffer_views.append(view)
        return len(buffer_views) - 1

    vcount = len(positions) // 3
    # POSITION
    vmin = [min(positions[i::3]) for i in range(3)]
    vmax = [max(positions[i::3]) for i in range(3)]
    bv = add_view(offsets[0], len(pos_b), 34962)
    accessors.append(
        {
            "bufferView": bv,
            "componentType": 5126,
            "count": vcount,
            "type": "VEC3",
            "max": vmax,
            "min": vmin,
        }
    )
    # NORMAL
    bv = add_view(offsets[1], len(nrm_b), 34962)
    accessors.append({"bufferView": bv, "componentType": 5126, "count": vcount, "type": "VEC3"})
    # TEXCOORD_0
    bv = add_view(offsets[2], len(uv_b), 34962)
    accessors.append({"bufferView": bv, "componentType": 5126, "count": vcount, "type": "VEC2"})
    # TANGENT
    bv = add_view(offsets[3], len(tan_b), 34962)
    accessors.append({"bufferView": bv, "componentType": 5126, "count": vcount, "type": "VEC4"})
    # INDICES
    bv = add_view(offsets[4], len(idx_b), 34963)
    accessors.append({"bufferView": bv, "componentType": 5123, "count": len(indices), "type": "SCALAR"})
    # Image buffer view
    img_view = add_view(offsets[5], len(png))

    doc = {
        "asset": {"version": "2.0", "generator": "nu-content make-primitive"},
        "buffers": [{"byteLength": len(bin_blob)}],
        "bufferViews": buffer_views,
        "accessors": accessors,
        "images": [{"bufferView": img_view, "mimeType": "image/png", "name": "baseColor"}],
        "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
        "textures": [{"sampler": 0, "source": 0}],
        "materials": [
            {
                "name": "ruin_stone",
                "pbrMetallicRoughness": {
                    "baseColorTexture": {"index": 0},
                    "baseColorFactor": [1, 1, 1, 1],
                    "metallicFactor": 0.0,
                    "roughnessFactor": 0.9,
                },
            }
        ],
        "meshes": [
            {
                "name": "prop_mesh_LOD0",
                "primitives": [
                    {
                        "attributes": {
                            "POSITION": 0,
                            "NORMAL": 1,
                            "TEXCOORD_0": 2,
                            "TANGENT": 3,
                        },
                        "indices": 4,
                        "material": 0,
                    }
                ],
            }
        ],
        "nodes": [
            {"name": path.stem, "mesh": 0},
            {
                "name": "COL_box",
                "translation": [0, h * 0.5, 0],
                "scale": [h, h, h],
            },
        ],
        "scenes": [{"name": "Scene", "nodes": [0, 1]}],
        "scene": 0,
    }

    json_bytes = align4(json.dumps(doc, separators=(",", ":")).encode("utf-8"), pad_byte=b" ")
    bin_aligned = align4(bin_blob)
    total = 12 + 8 + len(json_bytes) + 8 + len(bin_aligned)
    header = struct.pack("<III", 0x46546C67, 2, total)
    json_chunk = struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
    bin_chunk = struct.pack("<II", len(bin_aligned), 0x004E4942) + bin_aligned
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + json_chunk + bin_chunk)
