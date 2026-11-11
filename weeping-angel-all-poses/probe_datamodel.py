import unreal, json
m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out = {}
dm = m.get_editor_property("data_model")
out['dm_type'] = str(type(dm))
out['dm_props'] = [p for p in dir(dm) if not p.startswith('_') and not callable(getattr(dm,p,None))]
# composite sections?
for n in ['composite_sections','sections','anim_segments','get_sections','get_num_sections']:
    try:
        v = dm.get_editor_property(n) if hasattr(dm,'get_editor_property') else getattr(dm,n)
        out['dm_'+n] = str(type(v)) + " = " + str(v)[:300]
    except Exception as e:
        out['dm_'+n+'_err'] = str(e)
# methods with section/segment
out['dm_methods'] = [mm for mm in dir(dm) if 'section' in mm.lower() or 'segment' in mm.lower() or 'track' in mm.lower() or 'notify' in mm.lower() or 'add' in mm.lower()]
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_datamodel_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_DATAMODEL_DONE")
with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_datamodel_props.txt",'w') as f:
    f.write('\n'.join(out['dm_props']))
with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_datamodel_methods.txt",'w') as f:
    f.write('\n'.join(out['dm_methods']))
unreal.log("done")
