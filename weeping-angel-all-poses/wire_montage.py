import unreal, json, os

REPORT_PATH = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\wire_montage_report.json"
REPORT = {"wired": [], "errors": []}

MONTAGES = {
    "M_Swing_Pickaxe": "A_Tool_Swing_Pickaxe",
    "M_Swing_Shovel": "A_Tool_Swing_Shovel",
    "M_Swing_Axe": "A_Tool_Swing_Axe",
    "M_Swing_Hoe": "A_Tool_Swing_Hoe",
    "M_Swing_Sword": "A_Tool_Swing_Sword",
}
ANIMS_DIR = "/Game/Characters/Arms/Anims"
MONTAGE_DIR = "/Game/Characters/Arms/Montages"

for mname, aname in MONTAGES.items():
    mpath = MONTAGE_DIR + "/" + mname
    apath = ANIMS_DIR + "/" + aname
    montage = unreal.EditorAssetLibrary.load_asset(mpath)
    anim = unreal.EditorAssetLibrary.load_asset(apath)
    if montage is None or anim is None:
        REPORT["errors"].append("missing asset %s or %s" % (mname, aname)); continue
    try:
        track = montage.slot_anim_tracks[0]
        # clear existing segments to avoid duplicates
        track.anim_track.anim_segments.clear()
        seg = unreal.AnimSegment()
        seg.set_editor_property("anim_reference", anim)
        # anim length
        length = anim.get_play_length() if hasattr(anim,'get_play_length') else anim.sequence_length
        seg.set_editor_property("anim_start_time", 0.0)
        seg.set_editor_property("anim_end_time", length)
        seg.set_editor_property("anim_play_rate", 1.0)
        # append
        track.anim_track.anim_segments.append(seg)
        montage.slot_anim_tracks[0] = track
        # save
        unreal.EditorAssetLibrary.save_loaded_asset(montage, False)
        REPORT["wired"].append({"montage": mname, "anim": aname, "length": length, "segments": len(track.anim_track.anim_segments)})
        unreal.log("wired %s -> %s (segments=%d)" % (mname, aname, len(track.anim_track.anim_segments)))
    except Exception as e:
        REPORT["errors"].append("%s: %s" % (mname, repr(e)))
        unreal.log_error("wire failed %s: %s" % (mname, repr(e)))

with open(REPORT_PATH, "w", encoding="utf-8") as f:
    json.dump(REPORT, f, indent=2, default=str)
unreal.log("WIRE_MONTAGE_DONE")
