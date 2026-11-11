import unreal, json

results = {}
mesh = unreal.EditorAssetLibrary.load_asset("/Game/Characters/WeepingAngel/Meshes/SM_WeepingAngel_Idle")
results["idle_props"] = [p for p in dir(mesh) if 'nanite' in p.lower()]
try:
    ns = mesh.get_editor_property("nanite_settings")
    results["idle_nanite"] = str(ns)
except Exception as e:
    results["idle_nanite_err"] = str(e)

for name in ["SM_WeepingAngel_HeadTurned","SM_WeepingAngel_HandsOnFace","SM_WeepingAngel_MidStride","SM_WeepingAngel_Crouched","SM_WeepingAngel_WingsHalfOpen"]:
    m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/WeepingAngel/Meshes/" + name)
    try:
        ns = m.get_editor_property("nanite_settings")
        results[name] = str(ns)
    except Exception as e:
        results[name + "_err"] = str(e)

with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\nanite_results.json", "w") as f:
    json.dump(results, f, indent=2)
unreal.log("NANITE_CHECK_DONE")
