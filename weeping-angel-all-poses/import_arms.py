import unreal
import json
import os

PROJECT = r"F:\Dev\MinecraftDeWish"
REPORT_PATH = os.path.join(PROJECT, "weeping-angel-all-poses", "import_arms_report.json")
REPORT = {"errors": []}

MESH_DIR = "/Game/Characters/Arms"
FBX = os.path.join(PROJECT, "Content", "Import", "SK_Arms_FP.fbx")

def log(msg): unreal.log("[Arms] " + str(msg))
def error(msg):
    unreal.log_error("[Arms] " + str(msg)); REPORT["errors"].append(str(msg))

def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(MESH_DIR):
        unreal.EditorAssetLibrary.make_directory(MESH_DIR)

    import_ui = unreal.FbxImportUI()
    import_ui.set_editor_property("import_mesh", True)
    import_ui.set_editor_property("import_as_skeletal", True)
    import_ui.set_editor_property("import_materials", False)
    import_ui.set_editor_property("import_textures", False)
    import_ui.set_editor_property("import_animations", False)

    skel_data = unreal.FbxSkeletalMeshImportData()
    try_set = lambda o, n, v: [o.set_editor_property(x, v) for x in n if _try(o, x, v)]
    def _try(o, x, v):
        try:
            o.set_editor_property(x, v); return True
        except Exception:
            return False
    try_set(skel_data, ["convert_scene", "convert_scene_unit"], True)

    import_ui.set_editor_property("skeletal_mesh_import_data", skel_data)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", FBX)
    task.set_editor_property("destination_path", MESH_DIR)
    task.set_editor_property("destination_name", "SK_Arms_FP")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    task.set_editor_property("factory", unreal.FbxFactory())
    task.set_editor_property("options", import_ui)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    skm_path = MESH_DIR + "/SK_Arms_FP"
    skm = unreal.EditorAssetLibrary.load_asset(skm_path)
    if skm is None:
        error("import failed: %s" % skm_path)
    else:
        log("imported %s" % skm_path)
        # Add socket weapon_r to skeleton
        skel = skm.get_editor_property("skeleton")
        sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
        try:
            sock.set_socket_parent(skm, "hand_r")
            skm.add_socket(sock)
            log("added socket weapon_r -> hand_r")
        except Exception as e:
            error("socket add failed: %s" % e)
        unreal.EditorAssetLibrary.save_loaded_asset(skm, False)
        unreal.EditorAssetLibrary.save_loaded_asset(skel, False)
        REPORT["skm"] = skm_path
        REPORT["sockets"] = skm.num_sockets()

    with open(REPORT_PATH, "w", encoding="utf-8") as f:
        json.dump(REPORT, f, indent=2, default=str)
    log("done")

main()
