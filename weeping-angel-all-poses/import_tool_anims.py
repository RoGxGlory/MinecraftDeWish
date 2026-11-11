import unreal, json, os

PROJECT = r"F:\Dev\MinecraftDeWish"
REPORT_PATH = os.path.join(PROJECT, "weeping-angel-all-poses", "import_tool_anims_report.json")
REPORT = {"imported": [], "errors": []}

def log(m): unreal.log("[ToolAnims] " + str(m))
def err(m):
    unreal.log_error("[ToolAnims] " + str(m)); REPORT["errors"].append(str(m))

def try_set(o, names, v):
    for n in names:
        try:
            o.set_editor_property(n, v); return n
        except Exception:
            continue
    return None

ANIMS_DIR = "/Game/Characters/Arms/Anims"
IMPORT_DIR = os.path.join(PROJECT, "Content", "Import")

skel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP_Skeleton")
if skel is None:
    err("skeleton not found"); 
    with open(REPORT_PATH,"w") as f: json.dump(REPORT,f,indent=2); 
    raise SystemExit

if not unreal.EditorAssetLibrary.does_directory_exist(ANIMS_DIR):
    unreal.EditorAssetLibrary.make_directory(ANIMS_DIR)

NAMES = ["A_Tool_Swing_Generic", "A_Tool_Swing_Pickaxe", "A_Tool_Swing_Shovel",
         "A_Tool_Swing_Axe", "A_Tool_Swing_Hoe", "A_Tool_Swing_Sword"]

for name in NAMES:
    src = os.path.join(IMPORT_DIR, name + ".fbx")
    if not os.path.exists(src):
        err("missing fbx: %s" % src); continue

    import_ui = unreal.FbxImportUI()
    import_ui.set_editor_property("import_mesh", False)
    import_ui.set_editor_property("import_as_skeletal", True)
    import_ui.set_editor_property("import_animations", True)
    import_ui.set_editor_property("import_materials", False)
    import_ui.set_editor_property("import_textures", False)
    try_set(import_ui, ["skeleton"], skel)
    try_set(import_ui, ["override_animation_name"], name)

    # anim sequence import data tweaks
    try:
        ad = import_ui.get_editor_property("anim_sequence_import_data")
        # try common props
        for prop, val in [("import_translation", False), ("import_rotation", True),
                          ("import_uniform_scale", 1.0), ("convert_scene", True),
                          ("convert_scene_unit", True)]:
            try_set(ad, [prop], val)
    except Exception as e:
        log("anim data access: %s" % e)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", ANIMS_DIR)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    task.set_editor_property("factory", unreal.FbxFactory())
    task.set_editor_property("options", import_ui)

    try:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        asset_path = ANIMS_DIR + "/" + name
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset is None:
            err("import failed (asset None): %s" % name)
        else:
            log("imported %s -> %s" % (name, asset.get_path_name()))
            # set skeleton if anim sequence has skeleton property
            try:
                asset.set_editor_property("skeleton", skel)
            except Exception as e:
                log("set skeleton on anim (may not be needed): %s" % e)
            try:
                # length info
                seq_len = asset.get_editor_property("sequence_length") if hasattr(asset,'get_editor_property') else None
                log("  sequence_length=%s" % seq_len)
            except Exception as e:
                log("  len err: %s" % e)
            unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
            REPORT["imported"].append({"name": name, "path": asset_path})
    except Exception as e:
        err("exception importing %s: %s" % (name, repr(e)))

with open(REPORT_PATH, "w", encoding="utf-8") as f:
    json.dump(REPORT, f, indent=2, default=str)
log("done: %d imported, %d errors" % (len(REPORT["imported"]), len(REPORT["errors"])))
