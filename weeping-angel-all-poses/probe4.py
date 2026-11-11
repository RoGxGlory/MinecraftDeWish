import unreal
import json

out = {}
dt_methods = [m for m in dir(unreal.DataTable) if not m.startswith("_")]
out["DataTable_methods_all"] = dt_methods

# Check if there is a function to add row via EditorAssetLibrary or DataTableFunctionLibrary
for mod in ["EditorAssetLibrary", "DataTableFunctionLibrary", "DataTableEditorUtils"]:
    if hasattr(unreal, mod):
        m = getattr(unreal, mod)
        methods = [x for x in dir(m) if "row" in x.lower() or "table" in x.lower()]
        out[mod] = methods

# Check TSoftObjectPtr handling: try setting mesh_path on a ToolRow instance
row = unreal.ToolRow()
out["row_type"] = str(type(row))
try:
    row.set_editor_property("mesh_path", "/Game/Tools/SM_Tool_Pickaxe_Wood")
    out["mesh_path_set_string"] = str(row.get_editor_property("mesh_path"))
except Exception as e:
    out["mesh_path_set_string_err"] = str(e)

# Try soft object ptr via load_asset
try:
    mesh = unreal.EditorAssetLibrary.load_asset("/Game/Tools/SM_Tool_Pickaxe_Wood")
    row.set_editor_property("mesh_path", mesh)
    out["mesh_path_set_asset"] = str(row.get_editor_property("mesh_path"))
except Exception as e:
    out["mesh_path_set_asset_err"] = str(e)

# Check import_text / export_text
try:
    out["export_text"] = row.export_text()
except Exception as e:
    out["export_text_err"] = str(e)

report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe4_report.json"
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBE4 DONE")
