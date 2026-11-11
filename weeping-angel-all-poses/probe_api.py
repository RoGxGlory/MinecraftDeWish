import unreal
import json

out = {}
# Check classes
for name in ["FToolRow", "EToolType", "EToolTier", "ToolDefinition", "DataTable", "DataTableFactory", "DataAssetFactory"]:
    out[name] = hasattr(unreal, name)

# Check EToolType members
if hasattr(unreal, "EToolType"):
    out["EToolType_members"] = [m for m in dir(unreal.EToolType) if not m.startswith("_")]
if hasattr(unreal, "EToolTier"):
    out["EToolTier_members"] = [m for m in dir(unreal.EToolTier) if not m.startswith("_")]

# Check DataTable methods
if hasattr(unreal, "DataTable"):
    out["DataTable_methods"] = [m for m in dir(unreal.DataTable) if "row" in m.lower() or "add" in m.lower()]

# Check struct lookup
s = unreal.load_object(None, "/Script/MinecraftDeWish.FToolRow")
out["struct_load_FToolRow"] = str(s)
s2 = unreal.load_object(None, "/Script/MinecraftDeWish.ToolRow")
out["struct_load_ToolRow"] = str(s2)

# Try find struct
try:
    out["find_struct"] = str(unreal.find_struct("FToolRow"))
except Exception as e:
    out["find_struct_err"] = str(e)

report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_report.json"
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBE DONE")
