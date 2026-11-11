import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_anim_report.json"

# Check classes
for name in ["AnimBlueprint", "AnimBlueprintFactory", "AnimMontage", "AnimMontageFactory", "AnimSequence", "AnimSequenceFactory", "Skeleton", "AnimInstance"]:
    out[name] = hasattr(unreal, name)

# Methods on AnimMontage
if hasattr(unreal, "AnimMontage"):
    out["AnimMontage_methods"] = [m for m in dir(unreal.AnimMontage) if "skeleton" in m.lower() or "section" in m.lower() or "notify" in m.lower() or "slot" in m.lower()]

# factories
for name in ["AnimBlueprintFactory", "AnimMontageFactory", "AnimSequenceFactory", "FbxFactory", "FbxImportUI"]:
    if hasattr(unreal, name):
        f = getattr(unreal, name)
        out[name + "_props"] = [p for p in dir(f) if not p.startswith("_") and ("skeleton" in p.lower() or "target" in p.lower() or "preview" in p.lower())]

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBEANIM DONE")
