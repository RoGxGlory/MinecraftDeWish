import unreal
import json

out = {}
base = "/Game/Data"
out["assets"] = [str(p) for p in unreal.EditorAssetLibrary.list_assets(base, recursive=False)]

# Load DA and DT
dt = unreal.EditorAssetLibrary.load_asset(base + "/DT_Tools")
da = unreal.EditorAssetLibrary.load_asset(base + "/DA_ToolDefinition")
out["DT_loaded"] = str(dt)
out["DA_loaded"] = str(da)
if dt is not None:
    out["DT_rows"] = [str(n) for n in dt.get_row_names()]
if da is not None:
    try:
        out["DA_tools_table"] = str(da.get_editor_property("tools_table"))
    except Exception as e:
        out["DA_tools_table_err"] = str(e)

report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\verify_report.json"
with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("VERIFY DONE")
