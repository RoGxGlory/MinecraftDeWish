import unreal, json
m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out = {}
out['type'] = str(type(m))
out['slot_anim_tracks'] = str(m.slot_anim_tracks)
# inspect first track
if m.slot_anim_tracks and len(m.slot_anim_tracks) > 0:
    t = m.slot_anim_tracks[0]
    out['track_type'] = str(type(t))
    out['track_props'] = [p for p in dir(t) if not p.startswith('_')]
    out['track'] = str(t)
    # anim track?
    try:
        out['anim_track'] = str(t.get_editor_property('anim_track'))
    except Exception as e:
        out['anim_track_err'] = str(e)
    try:
        # slot name
        out['slot_name'] = str(t.slot_name) if hasattr(t,'slot_name') else 'NA'
    except Exception as e:
        out['slot_name_err'] = str(e)
# sections
out['num_sections'] = m.get_num_sections()
for i in range(m.get_num_sections()):
    out.setdefault('sections',[]).append(m.get_section_name(i))
# skeleton
out['skeleton'] = str(m.get_skeleton())
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_montage2_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_MONTAGE2_DONE")
