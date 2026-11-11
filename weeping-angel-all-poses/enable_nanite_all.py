import unreal, json

names = [
    "SM_WeepingAngel_Point",
    "SM_WeepingAngel_Idle",
    "SM_WeepingAngel_Attack",
    "SM_WeepingAngel_Point_Body",
    "SM_WeepingAngel_Point_RightArm",
    "SM_WeepingAngel_HeadTurned",
    "SM_WeepingAngel_HandsOnFace",
    "SM_WeepingAngel_MidStride",
    "SM_WeepingAngel_Crouched",
    "SM_WeepingAngel_WingsHalfOpen",
]
results = {}
for name in names:
    path = "/Game/Characters/WeepingAngel/Meshes/" + name
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if mesh is None:
        results[name] = "MISSING"
        continue
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", True)
    mesh.set_editor_property("nanite_settings", ns)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
    ns2 = mesh.get_editor_property("nanite_settings")
    results[name] = str(ns2.get_editor_property("enabled"))

with open(r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\nanite_all_results.json", "w") as f:
    json.dump(results, f, indent=2)
unreal.log("NANITE_ALL_DONE")
