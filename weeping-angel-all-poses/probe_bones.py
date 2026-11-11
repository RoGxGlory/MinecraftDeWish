import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_bones_report.json"

skel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP_Skeleton")
out["methods"] = [m for m in dir(skel) if "bone" in m.lower() or "name" in m.lower() or "ref" in m.lower()]
# Try get_reference_skeleton
try:
    rs = skel.get_reference_skeleton()
    out["ref_skel"] = str(rs)
    out["ref_skel_methods"] = [m for m in dir(rs) if not m.startswith("_")]
except Exception as e:
    out["ref_skel_err"] = str(e)

# Try find bone
for cand in ["hand_r", "Hand_R", "lowerarm_r"]:
    try:
        idx = skel.get_bone_index_from_name(cand) if hasattr(skel, "get_bone_index_from_name") else None
        out["idx_" + cand] = str(idx)
    except Exception as e:
        out["idx_" + cand + "_err"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBEBONES DONE")
