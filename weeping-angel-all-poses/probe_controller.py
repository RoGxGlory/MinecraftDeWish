import unreal, json
m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out = {}
ctrl = m.get_editor_property("controller")
out['ctrl_type'] = str(type(ctrl))
out['ctrl_props'] = [p for p in dir(ctrl) if not p.startswith('_') and not callable(getattr(ctrl,p,None))]
out['ctrl_methods'] = [mm for mm in dir(ctrl) if 'section' in mm.lower() or 'segment' in mm.lower() or 'track' in mm.lower() or 'notify' in mm.lower() or 'add' in mm.lower() or 'anim' in mm.lower()]
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_controller_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_CONTROLLER_DONE")
with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_controller_props.txt",'w') as f:
    f.write('\n'.join(out['ctrl_props']))
with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_controller_methods.txt",'w') as f:
    f.write('\n'.join(out['ctrl_methods']))
unreal.log("done")
