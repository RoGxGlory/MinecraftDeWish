import unreal, json, os

REPORT_PATH = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\final_verify_report.json"
REPORT = {"montages": [], "anims": [], "errors": []}

MONTAGE_DIR = "/Game/Characters/Arms/Montages"
ANIMS_DIR = "/Game/Characters/Arms/Anims"
MONTAGE_NAMES = ["M_Swing_Pickaxe", "M_Swing_Shovel", "M_Swing_Axe", "M_Swing_Hoe", "M_Swing_Sword"]
ANIM_NAMES = ["A_Tool_Swing_Generic", "A_Tool_Swing_Pickaxe", "A_Tool_Swing_Shovel",
              "A_Tool_Swing_Axe", "A_Tool_Swing_Hoe", "A_Tool_Swing_Sword"]

skel_path = "/Game/Characters/Arms/SK_Arms_FP_Skeleton"
skel = unreal.EditorAssetLibrary.load_asset(skel_path)

for name in ANIM_NAMES:
    path = ANIMS_DIR + "/" + name
    asset = unreal.EditorAssetLibrary.load_asset(path)
    info = {"name": name, "path": path, "exists": asset is not None}
    if asset is not None:
        info["skeleton_match"] = (asset.get_editor_property("skeleton") == skel)
        try:
            info["play_length"] = asset.get_play_length()
        except Exception as e:
            info["play_length_err"] = str(e)
        try:
            info["sequence_length"] = asset.get_editor_property("sequence_length")
        except Exception as e:
            info["sequence_length_err"] = str(e)
    else:
        REPORT["errors"].append("missing anim: " + name)
    REPORT["anims"].append(info)

for name in MONTAGE_NAMES:
    path = MONTAGE_DIR + "/" + name
    asset = unreal.EditorAssetLibrary.load_asset(path)
    info = {"name": name, "path": path, "exists": asset is not None}
    if asset is not None:
        info["skeleton_match"] = (asset.get_skeleton() == skel)
        info["num_sections"] = asset.get_num_sections()
        info["sections"] = [asset.get_section_name(i) for i in range(asset.get_num_sections())]
        # segments
        try:
            track = asset.slot_anim_tracks[0]
            info["segments"] = len(track.anim_track.anim_segments)
            if track.anim_track.anim_segments:
                seg = track.anim_track.anim_segments[0]
                info["segment_anim"] = str(seg.get_editor_property("anim_reference").get_name()) if seg.get_editor_property("anim_reference") else None
        except Exception as e:
            info["segment_err"] = str(e)
    else:
        REPORT["errors"].append("missing montage: " + name)
    REPORT["montages"].append(info)

with open(REPORT_PATH, "w", encoding="utf-8") as f:
    json.dump(REPORT, f, indent=2, default=str)
unreal.log("FINAL_VERIFY_DONE")
