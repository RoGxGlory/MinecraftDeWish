import unreal, json
m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out = {}
out['methods'] = [mm for mm in dir(m) if 'section' in mm.lower() or 'segment' in mm.lower() or 'track' in mm.lower() or 'notify' in mm.lower() or 'slot' in mm.lower() or 'blend' in mm.lower()]
# Also AnimComposite? 
# Try to find add section method
for mm in dir(m):
    if 'add' in mm.lower() or 'create' in mm.lower() or 'insert' in mm.lower():
        out.setdefault('add_methods',[]).append(mm)
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_montage3_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_MONTAGE3_DONE")
