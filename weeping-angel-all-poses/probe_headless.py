import unreal, json, os
out = {}
out['version'] = unreal.SystemLibrary.get_engine_version() if hasattr(unreal.SystemLibrary,'get_engine_version') else 'NA'
out['skeleton_exists'] = unreal.EditorAssetLibrary.does_asset_exist('/Game/Characters/Arms/SK_Arms_FP_Skeleton')
out['montage_exists'] = unreal.EditorAssetLibrary.does_asset_exist('/Game/Characters/Arms/Montages/M_Swing_Pickaxe')
report = r'F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_headless_report.json'
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log('PROBE_HEADLESS_DONE')
