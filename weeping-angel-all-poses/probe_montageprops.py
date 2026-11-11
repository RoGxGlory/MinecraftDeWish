import unreal, json
m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out = {}
# list props via dir filtered
props = [p for p in dir(m) if not p.startswith('_') and not callable(getattr(m,p,None))]
out['props'] = props
# try common property names
for n in ['composite_sections','anim_segments','slot_anim_tracks','blend_in','blend_out','sequence_length','rate_scale']:
    try:
        v = m.get_editor_property(n)
        out['prop_'+n] = str(type(v)) + " = " + str(v)[:200]
    except Exception as e:
        out['prop_'+n+'_err'] = str(e)
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_montageprops_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_MONTAGEPROPS_DONE")
with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_montageprops_props.txt",'w') as f:
    f.write('\n'.join(props))
unreal.log("done")
