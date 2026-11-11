import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_traverse2_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
for bone in ["hand_l", "hand_r", "lowerarm_r", "clavicle_r"]:
    try:
        out["children_of_" + bone] = [str(c) for c in skm.get_bone_children(bone)]
    except Exception as e:
        out["children_of_" + bone + "_err"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBETRAVERSE2 DONE")
