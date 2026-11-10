# -*- coding: utf-8 -*-
"""Import the three Weeping Angel poses (FBX) and their textures/materials into UE 5.8.

Run headless with:
  UnrealEditor-Cmd.exe MinecraftDeWish.uproject -run=pythonscript -script="<this file>"

Creates:
  /Game/Characters/WeepingAngel/Meshes/SM_WeepingAngel_{Idle,Attack,Point}
  /Game/Characters/WeepingAngel/Textures/T_WeepingAngel_{Body,Face,Wings}_{D,N}
  /Game/Characters/WeepingAngel/Materials/M_WeepingAngel_{Body,Face,Wings}
and assigns the materials to the mesh slots.
"""

import json
import os
import traceback

import unreal

ROOT = r"D:\Developpement\MinecraftDeWish\weeping-angel-all-poses"
TEX_SRC = os.path.join(ROOT, "textures")

BASE = "/Game/Characters/WeepingAngel"
MESH_DIR = BASE + "/Meshes"
TEX_DIR = BASE + "/Textures"
MAT_DIR = BASE + "/Materials"

REPORT_PATH = os.path.join(ROOT, "import_report.json")
REPORT = {"textures": [], "materials": [], "meshes": [], "errors": []}


def log(msg):
    unreal.log("[WeepingAngel] " + str(msg))


def error(msg):
    unreal.log_error("[WeepingAngel] " + str(msg))
    REPORT["errors"].append(str(msg))


def try_set(obj, names, value):
    """Set the first editor property name that exists, return the name used."""
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            continue
    return None


def enum_member(enum_class, fragment):
    """Return the first enum member whose Python name contains `fragment`."""
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


# ---------------------------------------------------------------------------
# Textures
# ---------------------------------------------------------------------------

TEXTURE_SPECS = [
    ("angelbody.png", "T_WeepingAngel_Body_D", False),
    ("angelface.png", "T_WeepingAngel_Face_D", False),
    ("angelwings.png", "T_WeepingAngel_Wings_D", False),
    ("angelbody_nrm.png", "T_WeepingAngel_Body_N", True),
    ("angelface_nrm.png", "T_WeepingAngel_Face_N", True),
    ("angelwings_nrm.png", "T_WeepingAngel_Wings_N", True),
]


def import_texture(filename, dest_name, is_normal):
    src = os.path.join(TEX_SRC, filename)
    factory = unreal.TextureFactory()
    if is_normal:
        script_name = try_set(
            factory, ["compression_settings"], unreal.TextureCompressionSettings.TC_NORMALMAP
        )
        log("%s: factory compression_settings set via '%s'" % (dest_name, script_name))

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", TEX_DIR)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    task.set_editor_property("factory", factory)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset_path = TEX_DIR + "/" + dest_name
    tex = unreal.EditorAssetLibrary.load_asset(asset_path)
    if tex is None:
        error("texture import failed: %s" % src)
        return None

    if is_normal:
        try_set(tex, ["compression_settings"], unreal.TextureCompressionSettings.TC_NORMALMAP)
        try_set(tex, ["srgb"], False)
        try_set(tex, ["flip_green_channel"], False)
    else:
        try_set(tex, ["compression_settings"], unreal.TextureCompressionSettings.TC_DEFAULT)
        try_set(tex, ["srgb"], True)

    save(tex)

    info = {
        "name": dest_name,
        "path": asset_path,
        "import_file": filename,
        "compression_settings": str(tex.get_editor_property("compression_settings")),
        "srgb": bool(tex.get_editor_property("srgb")),
    }
    REPORT["textures"].append(info)
    log("texture %s | compression=%s srgb=%s" % (dest_name, info["compression_settings"], info["srgb"]))
    return tex


# ---------------------------------------------------------------------------
# Materials
# ---------------------------------------------------------------------------

MATERIAL_SPECS = [
    ("M_WeepingAngel_Body", "T_WeepingAngel_Body_D", "T_WeepingAngel_Body_N"),
    ("M_WeepingAngel_Face", "T_WeepingAngel_Face_D", "T_WeepingAngel_Face_N"),
    ("M_WeepingAngel_Wings", "T_WeepingAngel_Wings_D", "T_WeepingAngel_Wings_N"),
]

MEL = unreal.MaterialEditingLibrary


def make_material(name, base_tex_name, normal_tex_name):
    asset_path = MAT_DIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.EditorAssetLibrary.delete_asset(asset_path)

    base_tex = unreal.EditorAssetLibrary.load_asset(TEX_DIR + "/" + base_tex_name)
    normal_tex = unreal.EditorAssetLibrary.load_asset(TEX_DIR + "/" + normal_tex_name)
    if base_tex is None or normal_tex is None:
        error("missing textures for material %s" % name)
        return None

    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )
    if mat is None:
        error("material creation failed: %s" % name)
        return None

    base_node = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, -100)
    base_node.set_editor_property("texture", base_tex)
    MEL.connect_material_property(base_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)

    normal_node = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -600, 200)
    normal_node.set_editor_property("texture", normal_tex)
    normal_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    MEL.connect_material_property(normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL)

    metallic = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 450)
    metallic.set_editor_property("r", 0.0)
    MEL.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)

    roughness = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -350, 550)
    roughness.set_editor_property("r", 0.8)
    MEL.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    MEL.recompile_material(mat)
    save(mat)

    REPORT["materials"].append({"name": name, "path": asset_path})
    log("material %s created" % name)
    return mat


# ---------------------------------------------------------------------------
# Meshes
# ---------------------------------------------------------------------------

MESH_SPECS = [
    ("SM_WeepingAngel_Idle.fbx", "SM_WeepingAngel_Idle"),
    ("SM_WeepingAngel_Attack.fbx", "SM_WeepingAngel_Attack"),
    ("SM_WeepingAngel_Point.fbx", "SM_WeepingAngel_Point"),
]

# FBX material order inside each pose (Blender slot order).
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


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    log("Unreal version: %s" % unreal.SystemLibrary.get_engine_version())

    for dir_path in (MESH_DIR, TEX_DIR, MAT_DIR):
        if not unreal.EditorAssetLibrary.does_directory_exist(dir_path):
            unreal.EditorAssetLibrary.make_directory(dir_path)
            log("created folder %s" % dir_path)

    for filename, dest_name, is_normal in TEXTURE_SPECS:
        import_texture(filename, dest_name, is_normal)

    for name, base_tex, normal_tex in MATERIAL_SPECS:
        make_material(name, base_tex, normal_tex)

    for fbx_name, dest_name in MESH_SPECS:
        mesh = import_mesh(fbx_name, dest_name)
        if mesh is not None:
            assign_materials(mesh, dest_name)

    with open(REPORT_PATH, "w", encoding="utf-8") as handle:
        json.dump(REPORT, handle, indent=2)

    if REPORT["errors"]:
        error("%d error(s) during import" % len(REPORT["errors"]))
    else:
        log("import finished without errors")


main()
