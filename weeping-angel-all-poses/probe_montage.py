import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_montage_report.json"

m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out["type"] = str(type(m))
out["methods"] = [mm for mm in dir(m) if "notify" in mm.lower() or "section" in mm.lower() or "slot" in mm.lower() or "track" in mm.lower()]

# Check notifies property
try:
    n = m.get_editor_property("notifies")
    out["notifies_prop"] = str(type(n)) + " len=" + str(len(n) if n is not None else None)
except Exception as e:
    out["notifies_prop_err"] = str(e)

# Check composite sections
try:
    cs = m.get_editor_property("composite_sections")
    out["composite_sections"] = str(type(cs)) + " len=" + str(len(cs) if cs is not None else None)
except Exception as e:
    out["composite_sections_err"] = str(e)

# Check slot anim tracks
try:
    sat = m.slot_anim_tracks
    out["slot_anim_tracks"] = str(type(sat)) + " len=" + str(len(sat) if sat is not None else None)
except Exception as e:
    out["slot_anim_tracks_err"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBEMONTAGE DONE")
