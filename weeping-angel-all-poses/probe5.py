import unreal
import json

out = {}
DATA_DIR = "/Game/Data"
DT_PATH = DATA_DIR + "/DT_Tools_Probe"

if unreal.EditorAssetLibrary.does_asset_exist(DT_PATH):
    unreal.EditorAssetLibrary.delete_asset(DT_PATH)

if not unreal.EditorAssetLibrary.does_directory_exist(DATA_DIR):
    unreal.EditorAssetLibrary.make_directory(DATA_DIR)

row_struct = unreal.load_object(None, "/Script/MinecraftDeWish.ToolRow")
factory = unreal.DataTableFactory()
try:
    factory.set_editor_property("struct", row_struct)
except Exception as e:
    out["factory_err"] = str(e)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
dt = asset_tools.create_asset("DT_Tools_Probe", DATA_DIR, unreal.DataTable, factory)
try:
    dt.set_editor_property("row_struct", row_struct)
except Exception as e:
    out["rowstruct_err"] = str(e)

# Build JSON
rows = [
    {
        "Name": "Pickaxe_Wood",
        "ToolType": "Pickaxe",
        "ToolTier": "Wood",
        "BaseMiningForce": 2.0,
        "DurabilityMax": 59,
        "AttackDamage": 1.0,
        "MeshPath": "/Game/Tools/SM_Tool_Pickaxe_Wood.SM_Tool_Pickaxe_Wood",
        "IconPath": None,
        "SwingMontage": None,
        "ImpactCue": "Impact_Pickaxe",
        "MiningForceByBlockCategory": {}
    }
]
json_str = json.dumps(rows)
out["json_str"] = json_str

# Try fill
try:
    res = dt.fill_from_json_string(json_str)
    out["fill_result"] = str(res)
except Exception as e:
    out["fill_err"] = str(e)

try:
    out["row_names"] = [str(n) for n in dt.get_row_names()]
except Exception as e:
    out["row_names_err"] = str(e)

# Export to see format
try:
    out["exported"] = dt.export_to_json_string()
except Exception as e:
    out["exported_err"] = str(e)

unreal.EditorAssetLibrary.save_loaded_asset(dt, False)

report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe5_report.json"
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBE5 DONE")
