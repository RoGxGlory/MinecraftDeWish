import unreal
import json

out = {}
# Inspect ToolType / ToolTier / ToolInstance / ToolRow
for name in ["ToolType", "ToolTier", "ToolInstance", "ToolRow", "ToolDefinition"]:
    obj = getattr(unreal, name, None)
    out[name] = str(type(obj))
    if obj is not None:
        out[name + "_repr"] = str(obj)
        try:
            out[name + "_members"] = [m for m in dir(obj) if not m.startswith("_")][:40]
        except Exception as e:
            out[name + "_members_err"] = str(e)

# Enum values via load_object
et = unreal.load_object(None, "/Script/MinecraftDeWish.EToolType")
out["EToolType_obj"] = str(et)
try:
    out["EToolType_values"] = [ (m, getattr(et, m)) for m in dir(et) if m[0].isupper() and not m.startswith("Static")][:20]
except Exception as e:
    out["EToolType_values_err"] = str(e)

# Try get enum members via get_name_string? 
try:
    # unreal has helper: get_default_object? Use Enum
    names = et.get_editor_property("enum_names") if hasattr(et, "get_editor_property") else None
    out["enum_names_prop"] = str(names)
except Exception as e:
    out["enum_names_prop_err"] = str(e)

report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe3_report.json"
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBE3 DONE")
