# -*- coding: utf-8 -*-
"""Import the 5 new Weeping Angel poses (FBX) into UE 5.8.

Run headless with:
  UnrealEditor-Cmd.exe MinecraftDeWish.uproject -run=pythonscript -script="<this file>"
"""
import os
import traceback
import unreal

ROOT = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses"
BASE = "/Game/Characters/WeepingAngel"
MESH_DIR = BASE + "/Meshes"
MAT_DIR = BASE + "/Materials"

REPORT_PATH = os.path.join(ROOT, "import_poses_report.json")
REPORT = {"meshes": [], "errors": []}


def log(msg):
    unreal.log("[WeepingAngelPoses] " + str(msg))


def error(msg):
    unreal.log_error("[WeepingAngelPoses] " + str(msg))
    REPORT["errors"].append(str(msg))


def try_set(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            continue
    return None


def enum_member(enum_class, fragment):
    for name in dir(enum_class):
        if fragment in name.upper():
            try:
                return getattr(enum_class, name)
            except Exception:
                pass
    return None


def save(asset):
    path = asset.get_path_name().split(".")[0]
    ok = unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    log("saved %s -> %s" % (path, ok))
    return ok


MESH_SPECS = [
    ("SM_WeepingAngel_HeadTurned.fbx", "SM_WeepingAngel_HeadTurned"),
    ("SM_WeepingAngel_HandsOnFace.fbx", "SM_WeepingAngel_HandsOnFace"),
    ("SM_WeepingAngel_MidStride.fbx", "SM_WeepingAngel_MidStride"),
    ("SM_WeepingAngel_Crouched.fbx", "SM_WeepingAngel_Crouched"),
    ("SM_WeepingAngel_WingsHalfOpen.fbx", "SM_WeepingAngel_WingsHalfOpen"),
]

SLOT_ORDER = ["M_WeepingAngel_Body", "M_WeepingAngel_Face", "M_WeepingAngel_Wings"]


def import_mesh(fbx_name, dest_name):
    src = os.path.join(ROOT, fbx_name)

    import_ui = unreal.FbxImportUI()
    import_ui.set_editor_property("import_mesh", True)
    import_ui.set_editor_property("import_as_skeletal", False)
    import_ui.set_editor_property("import_materials", False)
    import_ui.set_editor_property("import_textures", False)
    import_ui.set_editor_property("import_animations", False)

    static_mesh_type = enum_member(unreal.FBXImportType, "STATIC")
    if static_mesh_type is not None:
        try_set(import_ui, ["mesh_type_to_import"], static_mesh_type)

    static_data = unreal.FbxStaticMeshImportData()
    try_set(static_data, ["convert_scene"], True)
    try_set(static_data, ["convert_scene_unit"], True)
    try_set(static_data, ["combine_meshes"], False)
    normal_method = enum_member(unreal.FBXNormalImportMethod, "COMPUTE")
    if normal_method is not None:
        try_set(static_data, ["normal_import_method"], normal_method)
    import_ui.set_editor_property("static_mesh_import_data", static_data)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", MESH_DIR)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    task.set_editor_property("factory", unreal.FbxFactory())
    task.set_editor_property("options", import_ui)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = MESH_DIR + "/" + dest_name
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if mesh is None:
        error("mesh import failed: %s" % src)
        return None

    info = {"name": dest_name, "path": asset_path, "import_file": fbx_name, "slots": [], "size_cm": None}
    try:
        box = mesh.get_bounding_box()
        info["size_cm"] = [
            round(box.max.x - box.min.x, 2),
            round(box.max.y - box.min.y, 2),
            round(box.max.z - box.min.z, 2),
        ]
        info["min"] = [round(box.min.x, 2), round(box.min.y, 2), round(box.min.z, 2)]
        info["max"] = [round(box.max.x, 2), round(box.max.y, 2), round(box.max.z, 2)]
    except Exception:
        error("bounding box failed for %s: %s" % (dest_name, traceback.format_exc()))

    REPORT["meshes"].append(info)
    log("mesh %s imported | size=%s cm | slot_count=%d" %
        (dest_name, info["size_cm"], len(mesh.get_editor_property("static_materials"))))
    return mesh


def assign_materials(mesh, dest_name):
    slots = mesh.get_editor_property("static_materials")
    if not slots:
        error("%s has no material slots" % dest_name)
        return

    resolved = []
    for index, slot in enumerate(slots):
        slot_name = ""
        try:
            slot_name = str(slot.get_editor_property("material_slot_name"))
        except Exception:
            pass

        lowered = slot_name.lower()
        chosen = None
        if "body" in lowered:
            chosen = SLOT_ORDER[0]
        elif "face" in lowered:
            chosen = SLOT_ORDER[1]
        elif "wing" in lowered:
            chosen = SLOT_ORDER[2]
        elif index < len(SLOT_ORDER):
            chosen = SLOT_ORDER[index]

        if chosen is None:
            error("%s: cannot resolve slot %d ('%s')" % (dest_name, index, slot_name))
            continue

        material = unreal.EditorAssetLibrary.load_asset(MAT_DIR + "/" + chosen)
        if material is None:
            error("%s: material %s not found" % (dest_name, chosen))
            continue

        new_slot = unreal.StaticMaterial()
        new_slot.set_editor_property("material_interface", material)
        new_slot.set_editor_property("material_slot_name", slot.get_editor_property("material_slot_name"))
        resolved.append(new_slot)

    if resolved:
        mesh.set_editor_property("static_materials", resolved)
        save(mesh)
        for slot in resolved:
            entry = {
                "slot": str(slot.get_editor_property("material_slot_name")),
                "material": slot.get_editor_property("material_interface").get_name(),
            }
            for info in REPORT["meshes"]:
                if info["name"] == dest_name:
                    info["slots"].append(entry)
            log("%s slot '%s' -> %s" % (dest_name, entry["slot"], entry["material"]))


def main():
    log("Unreal version: %s" % unreal.SystemLibrary.get_engine_version())

    if not unreal.EditorAssetLibrary.does_directory_exist(MESH_DIR):
        unreal.EditorAssetLibrary.make_directory(MESH_DIR)
        log("created folder %s" % MESH_DIR)

    for fbx_name, dest_name in MESH_SPECS:
        mesh = import_mesh(fbx_name, dest_name)
        if mesh is not None:
            assign_materials(mesh, dest_name)

    # Enable Nanite on each new mesh (ADR-0004)
    for fbx_name, dest_name in MESH_SPECS:
        asset_path = MESH_DIR + "/" + dest_name
        mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if mesh is None:
            continue
        try:
            # UE 5.x: enable nanite via editor property
            mesh.set_editor_property("nanite_settings", unreal.MeshNaniteSettings())
            log("nanite enabled on %s" % dest_name)
        except Exception as e:
            log("nanite property not set on %s: %s" % (dest_name, str(e)))
        save(mesh)

    import json
    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        json.dump(REPORT, handle, indent=2)

    if REPORT["errors"]:
        error("%d error(s) during import" % len(REPORT["errors"]))
    else:
        log("import finished without errors")


main()
