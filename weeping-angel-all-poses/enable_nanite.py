import unreal, json

names = ["SM_WeepingAngel_HeadTurned","SM_WeepingAngel_HandsOnFace","SM_WeepingAngel_MidStride","SM_WeepingAngel_Crouched","SM_WeepingAngel_WingsHalfOpen"]
results = {}
for name in names:
    mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/WeepingAngel/Meshes/" + name)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", ns)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    # verify
    ns2 = mesh.get_editor_property("nanite_settings")
    results[name] = str(ns2.get_editor_property("enabled"))
    unreal.log("Nanite enabled on %s: %s" % (name, results[name]))

with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\nanite_enable_results.json", "w") as f:
    json.dump(results, f, indent=2)
unreal.log("NANITE_ENABLE_DONE")
