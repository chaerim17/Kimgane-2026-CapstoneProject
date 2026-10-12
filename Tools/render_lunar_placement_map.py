"""Draw a terrain/placement inspection map from the exported engine coordinates.

Requires numpy and Pillow; no Blender session is needed. +Z is north on this map.
"""
import json
from collections import Counter
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Assets/LunarKit"
FONT = "C:/Windows/Fonts/malgun.ttf"


def main():
    config = json.loads((OUT / "settlements.json").read_text())
    layout = json.loads((OUT / "layout.json").read_text())
    h = np.fromfile(ROOT / "Assets/Terrain/lunar_craters_r16.raw", dtype="<u2").reshape(513, 513) / 65535 * 48 - 16
    dz, dx = np.gradient(h, .75)
    normals = np.stack((-dx, -dz, np.ones_like(h)), -1)
    normals /= np.linalg.norm(normals, axis=2)[..., None]
    shade = np.maximum(0, normals @ np.array([-.45, -.35, .82]))
    tone = np.clip(.23 + shade * .45 + (h + 16) / 48 * .13, 0, 1)
    rgb = np.stack((tone * .77, tone * .85, tone), -1)
    levels = np.floor(h / 2)
    contour = (levels != np.roll(levels, 1, 0)) | (levels != np.roll(levels, 1, 1))
    rgb[contour] *= .74
    terrain = Image.fromarray((rgb[::-1] * 255).astype("uint8")).resize((1280, 1280), Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", (1960, 1480), "#111c27")
    canvas.paste(terrain, (60, 135))
    draw = ImageDraw.Draw(canvas)
    font = lambda size: ImageFont.truetype(FONT, size)
    draw.text((60, 32), "L02  달 기지 · 거점 배치도", font=font(38), fill="#e8f3fa")
    draw.text((62, 87), "384 × 384 m  |  RAW 원본 지형 유지  |  +Z 북쪽  |  등고선 2m", font=font(21), fill="#a8bfce")
    def point(x, z):
        return (60 + (x+192) / 384 * 1280, 135 + (192-z) / 384 * 1280)
    for value in range(-192, 193, 48):
        x, _ = point(value, 0); _, y = point(0, value)
        draw.line((x, 135, x, 1415), fill="#566c7a", width=1)
        draw.line((60, y, 1340, y), fill="#566c7a", width=1)
    draw.rectangle((60, 135, 1340, 1415), outline="#71899a", width=2)
    draw.text((76, 150), "+Z / N", font=font(24), fill="#e7f8ff")
    colors = {"CargoDepot_Ruined": "#f6bb58", "CargoDepot_Intact": "#f5dd91",
              "PurificationStation": "#5ce3dd", "RelayPlatform": "#b5a5ff",
              "WreckWorkshop": "#e79170", "ServiceCorridor": "#a3bbcd", "CoverCheckpoint": "#c9d3db"}
    catalog = json.loads((OUT / "catalog.json").read_text())
    bounds = {}
    for entry in layout["instances"]:
        name, kind = entry["asset"], entry["type"]
        if name not in bounds:
            mesh = OUT / catalog[kind][name]["lodMeshes"][0]
            pts = np.array([list(map(float, l.split()[1:4])) for l in mesh.read_text().splitlines() if l.startswith("v ")])
            bounds[name] = (max(abs(pts[:, 0])), max(abs(pts[:, 2])))
        hx, hz = bounds[name]
        if round(entry["yawDeg"] / 90) % 2: hx, hz = hz, hx
        x, _, z = entry["positionM"]
        color = colors.get(name, "#75aec0")
        if kind == "prefabs":
            draw.rectangle((*point(x-hx, z+hz), *point(x+hx, z-hz)), fill=color, outline="#101820", width=2)
        else:
            px, py = point(x, z)
            draw.ellipse((px-3, py-3, px+3, py+3), fill=color)
    labels = {"A": "중앙 착륙 기지", "B": "북서 화물 회수", "C": "동부 잔해·통신", "D": "남부 잔해 야드",
              "E": "서부 정비 거점", "F": "남동 정화 전초", "G": "북동 원격 창고", "H": "북부 통신 연결"}
    totals = Counter(e["type"] for e in layout["instances"])
    draw.text((1390, 135), f"{len(config['sites'])}개 거점 / {len(layout['instances'])}개 인스턴스", font=font(28), fill="#e8f3fa")
    draw.text((1390, 180), f"조합 프리팹 {totals['prefabs']} · 개별 설비 {totals['parts']}", font=font(22), fill="#aec4d2")
    for i, site in enumerate(config["sites"]):
        key = site["id"]
        entries = [e for e in layout["instances"] if e.get("site") == key]
        positions = np.array([[e["positionM"][0], e["positionM"][2]] for e in entries])
        x, z = positions.mean(0)
        px, py = point(x, z)
        draw.ellipse((px-62, py-62, px+62, py+62), outline="#e1eff7", width=2)
        draw.rounded_rectangle((px-70, py-82, px-28, py-42), radius=6, fill="#142a39", outline="#c9e7f8", width=2)
        draw.text((px-61, py-82), key, font=font(29), fill="#edfaff")
        top = 246 + i * 103
        draw.text((1390, top), f"{key}   {labels[key]}", font=font(26), fill="#edf4f8")
        count = Counter(e["type"] for e in entries)
        cx, cz = site["centerXZ"]
        draw.text((1390, top+38), f"X {cx:>4} / Z {cz:>4} m    프리팹 {count['prefabs']} · 설비 {count['parts']}", font=font(19), fill="#aac1d0")
    draw.line((1390, 1103, 1900, 1103), fill="#456174", width=2)
    legend = [("CargoDepot_Ruined", "파손 화물기지"), ("PurificationStation", "정화시설"),
              ("RelayPlatform", "통신 플랫폼"), ("WreckWorkshop", "폐허 작업장")]
    for i, (name, label) in enumerate(legend):
        yy = 1125 + i * 39
        draw.rectangle((1390, yy+5, 1412, yy+27), fill=colors[name])
        draw.text((1430, yy), label, font=font(22), fill="#c4d6e0")
    draw.text((1390, 1310), f"공유 Box 콜라이더 {len(layout['boxes']):,}개", font=font(23), fill="#c4d6e0")
    draw.text((1390, 1351), "각 시설의 LOD0–3 / 동일 메시 재사용", font=font(19), fill="#94afc0")
    draw.text((60, 1430), "실제 layout.json 기준 · 윤곽은 배치 범위 · 거점 간 이동 동선은 현장 플레이테스트 대상", font=font(19), fill="#a6bdcc")
    path = OUT / "previews/L02_PlacementMap.png"
    canvas.save(path)
    print(path)


if __name__ == "__main__":
    main()
