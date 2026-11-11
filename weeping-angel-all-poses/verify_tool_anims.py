import unreal, json, os

REPORT_PATH = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\verify_tool_anims_report.json"
REPORT = {"anims": []}

ANIMS_DIR = "/Game/Characters/Arms/Anims"
NAMES = ["A_Tool_Swing_Generic", "A_Tool_Swing_Pickaxe", "A_Tool_Swing_Shovel",
         "A_Tool_Swing_Axe", "A_Tool_Swing_Hoe", "A_Tool_Swing_Sword"]

for name in NAMES:
    path = ANIMS_DIR + "/" + name
    asset = unreal.EditorAssetLibrary.load_asset(path)
    info = {"name": name, "path": path, "exists": asset is not None}
    if asset is not None:
        try:
            info["sequence_length"] = asset.get_editor_property("sequence_length")
        except Exception as e:
            info["sequence_length_err"] = str(e)
        try:
            info["num_frames"] = asset.get_editor_property("num_frames")
        except Exception as e:
            info["num_frames_err"] = str(e)
        try:
            info["skeleton"] = str(asset.get_editor_property("skeleton"))
        except Exception as e:
            info["skeleton_err"] = str(e)
        # bone tracks
        try:
            tracks = asset.get_editor_property("raw_animation_data") if False else None
        except Exception:
            pass
        try:
            # Try to get track names via get_animation_track_names? 
            if hasattr(asset, "get_animation_track_names"):
                info["track_names"] = list(asset.get_animation_track_names())
        except Exception as e:
            info["track_names_err"] = str(e)
        # Try raw data track count
        try:
            raw = asset.get_editor_property("raw_animation_data")
            info["raw_tracks"] = len(raw) if raw is not None else None
        except Exception as e:
            info["raw_tracks_err"] = str(e)
    REPORT["anims"].append(info)

with open(REPORT_PATH, "w", encoding="utf-8") as f:
    json.dump(REPORT, f, indent=2, default=str)
unreal.log("VERIFY_TOOL_ANIMS_DONE")
