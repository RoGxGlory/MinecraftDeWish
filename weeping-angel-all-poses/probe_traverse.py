import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_traverse_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
# Try get_bone_children with various starting points
for start in ["", "root", "spine_01"]:
    try:
        children = skm.get_bone_children(start)
        out["children_of_" + (start or "EMPTY")] = [str(c) for c in children]
    except Exception as e:
        out["children_of_" + (start or "EMPTY") + "_err"] = str(e)

# Try get_bone_parent
try:
    out["parent_of_hand_r"] = str(skm.get_bone_parent("hand_r"))
except Exception as e:
    out["parent_of_hand_r_err"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBETRAVERSE DONE")
