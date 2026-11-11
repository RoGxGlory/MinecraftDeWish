import unreal, json
asset = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Anims/A_Tool_Swing_Generic")
out = {}
out['type'] = str(type(asset))
out['methods'] = [m for m in dir(asset) if not m.startswith('_')]
# filter animation-related
out['anim_methods'] = [m for m in dir(asset) if 'anim' in m.lower() or 'track' in m.lower() or 'bone' in m.lower() or 'frame' in m.lower() or 'key' in m.lower() or 'length' in m.lower() or 'notify' in m.lower()]
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_animseq_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_ANIMSEQ_DONE")
