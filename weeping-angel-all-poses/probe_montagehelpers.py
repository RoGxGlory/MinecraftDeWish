import unreal, json
out = {}
# search module-level functions containing 'section' or 'montage'
names = [n for n in dir(unreal) if 'montage' in n.lower() or 'section' in n.lower() or 'composite' in n.lower()]
out['helpers'] = names
# also check AnimationLibrary
if hasattr(unreal, 'AnimationLibrary'):
    out['animlib_methods'] = [m for m in dir(unreal.AnimationLibrary) if 'section' in m.lower() or 'montage' in m.lower() or 'segment' in m.lower()]
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_montagehelpers_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_MONTAGEHELPERS_DONE")
