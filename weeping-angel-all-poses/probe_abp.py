import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_abp_report.json"

skel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP_Skeleton")
out["skel"] = str(skel)

# Inspect factory instance
fac = unreal.AnimBlueprintFactory()
out["fac_props"] = [p for p in dir(fac) if not p.startswith("_")]
# Try set target_skeleton
for name in ["target_skeleton", "TargetSkeleton", "skeleton", "Skeleton"]:
    try:
        fac.set_editor_property(name, skel)
        out["set_" + name] = "OK"
    except Exception as e:
        out["set_" + name] = str(e)[:120]

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBEABP DONE")
