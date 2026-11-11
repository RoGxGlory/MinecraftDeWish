import unreal, json

asset = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Anims/A_Tool_Swing_Generic")
out = {}
# sample pose at frame 0 and frame 12 (windup) for upperarm_r
for frame in [0, 12, 20, 39]:
    try:
        pose = asset.get_anim_pose_at_frame(frame)
        # pose is a list of FTransform? or dict? 
        out[str(frame)] = str(type(pose)) + " len=" + str(len(pose) if hasattr(pose,'__len__') else 'NA')
        # try to get bone transform
        if hasattr(pose, '__len__') and len(pose) > 0:
            out[str(frame) + "_first"] = str(pose[0])
    except Exception as e:
        out[str(frame) + "_err"] = str(e)
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\verify_anim_motion_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("VERIFY_ANIM_MOTION_DONE")
