import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_bonenames_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
# List methods containing bone
out["skm_bone_methods"] = [m for m in dir(skm) if "bone" in m.lower()]
# Try get bone name by index
for method in ["get_bone_name", "get_bone_name_by_index", "find_bone_index"]:
    if hasattr(skm, method):
        out["has_" + method] = True
        try:
            out[method + "_0"] = str(getattr(skm, method)(0))
        except Exception as e:
            out[method + "_err"] = str(e)

# Skeleton
skel = skm.get_editor_property("skeleton")
out["skel_bone_methods"] = [m for m in dir(skel) if "bone" in m.lower()]

# Try get reference skeleton via mesh's skeleton asset? 
# Check if there is a 'bone_info' readable
try:
    bi = skel.get_editor_property("bone_info")
    out["bone_info_len"] = len(bi)
    out["bone_info_sample"] = str(bi[0]) if len(bi) else None
except Exception as e:
    out["bone_info_err"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBEBONENAMES DONE")
