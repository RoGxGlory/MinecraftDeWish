import unreal, json
out = {}
ui = unreal.FbxImportUI()
out['props'] = [p for p in dir(ui) if not p.startswith('_')]
# filter interesting
out['interesting'] = [p for p in dir(ui) if 'anim' in p.lower() or 'skel' in p.lower() or 'mesh' in p.lower() or 'import' in p.lower()]
# check anim_sequence_import_data attribute
for attr in ['anim_sequence_import_data','animation_import_data','skeletal_mesh_import_data']:
    try:
        v = getattr(ui, attr, 'MISSING')
        out[attr] = str(type(v))
    except Exception as e:
        out[attr] = 'ERR ' + str(e)
report = r'F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_fbxanim_report.json'
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log('PROBE_FBXANIM_DONE')
