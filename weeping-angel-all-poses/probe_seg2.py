import unreal, json
seg = unreal.AnimSegment()
out = {}
out['props'] = [p for p in dir(seg) if not p.startswith('_')]
# rna properties
try:
    for p in seg.get_class().get_properties() if hasattr(seg.get_class(),'get_properties') else []:
        out.setdefault('rna',[]).append(p.get_name())
except Exception as e:
    out['rna_err'] = str(e)
# try get_editor_property for common names
for n in ['anim_reference','anim','start_pos','end_pos','anim_start_time','anim_end_time','anim_play_rate','looping','slot_index']:
    try:
        v = seg.get_editor_property(n)
        out['prop_'+n] = str(v)
    except Exception as e:
        out['prop_'+n+'_err'] = str(e)
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_seg2_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_SEG2_DONE")
# write props to file
with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_seg2_props.txt",'w') as f:
    f.write('\n'.join([p for p in dir(seg) if not p.startswith('_')]))
unreal.log("done")
