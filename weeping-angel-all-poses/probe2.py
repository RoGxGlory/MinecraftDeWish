import unreal
import json

out = {}
# Search for tool-related attributes
tool_attrs = [a for a in dir(unreal) if "tool" in a.lower() or "tier" in a.lower()]
out["tool_attrs"] = tool_attrs

# Search for enum-like with Pickaxe
pickaxe_attrs = [a for a in dir(unreal) if "pickaxe" in a.lower()]
out["pickaxe_attrs"] = pickaxe_attrs

# List all enums
enums = [a for a in dir(unreal) if a.startswith("E") and a[1].isupper()]
out["E_enums_count"] = len(enums)
out["E_enums_sample"] = enums[:50]

# Try loading enum class
for path in ["/Script/MinecraftDeWish.EToolType", "/Script/MinecraftDeWish.EToolTier", "/Script/MinecraftDeWish.ToolType"]:
    try:
        o = unreal.load_object(None, path)
        out[path] = str(o)
    except Exception as e:
        out[path] = "ERR: " + str(e)

# Try load class for ToolDefinition
td = unreal.load_class(None, "/Script/MinecraftDeWish.ToolDefinition")
out["ToolDefinition_class"] = str(td)

report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe2_report.json"
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBE2 DONE")
