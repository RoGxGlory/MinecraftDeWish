# -*- coding: utf-8 -*-
"""Reload the imported Weeping Angel assets and dump their effective content as JSON."""

import json
import os

import unreal

BASE = "/Game/Characters/WeepingAngel"
REPORT_PATH = r"D:\Developpement\MinecraftDeWish\weeping-angel-all-poses\validation_report.json"
MEL = unreal.MaterialEditingLibrary

report = {"materials": [], "meshes": [], "textures": [], "errors": []}


def log(msg):
    unreal.log("[WAValidate] " + str(msg))


def error(msg):
    unreal.log_error("[WAValidate] " + str(msg))
    report["errors"].append(str(msg))


def material_report(name):
    path = BASE + "/Materials/" + name
    mat = unreal.EditorAssetLibrary.load_asset(path)
    if mat is None:
        error("missing material %s" % path)
        return
    entry = {"name": name, "expressions": 0, "inputs": {}}
    entry["expressions"] = len(MEL.get_material_expressions(mat))
    pins = [
        (unreal.MaterialProperty.MP_BASE_COLOR, "base_color"),
        (unreal.MaterialProperty.MP_NORMAL, "normal"),
        (unreal.MaterialProperty.MP_METALLIC, "metallic"),
        (unreal.MaterialProperty.MP_ROUGHNESS, "roughness"),
    ]
    for prop, key in pins:
        node = MEL.get_material_property_input_node(mat, prop)
        if node is None:
            entry["inputs"][key] = None
            continue
        info = {"class": node.get_class().get_name()}
        try:
            tex = node.get_editor_property("texture")
            if tex is not None:
                info["texture"] = tex.get_path_name()
                info["sampler_type"] = str(node.get_editor_property("sampler_type"))
        except Exception:
            pass
        try:
            info["constant"] = round(float(node.get_editor_property("r")), 3)
        except Exception:
            pass
        entry["inputs"][key] = info
    report["materials"].append(entry)
    log("material %s -> %s" % (name, entry["inputs"]))


def mesh_report(name):
    path = BASE + "/Meshes/" + name
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if mesh is None:
        error("missing mesh %s" % path)
        return
    box = mesh.get_bounding_box()
    entry = {
        "name": name,
        "size_cm": [round(box.max.x - box.min.x, 2), round(box.max.y - box.min.y, 2), round(box.max.z - box.min.z, 2)],
        "min_z_cm": round(box.min.z, 2),
        "slots": [],
        "lod_count": mesh.get_num_lods(),
        "nanite": bool(mesh.get_editor_property("nanite_settings").get_editor_property("enabled")),
    }
    for slot in mesh.get_editor_property("static_materials"):
        material = slot.get_editor_property("material_interface")
        entry["slots"].append({
            "slot": str(slot.get_editor_property("material_slot_name")),
            "material": material.get_name() if material else None,
            "material_path": material.get_path_name() if material else None,
        })
    report["meshes"].append(entry)
    log("mesh %s -> %s cm | %s" % (name, entry["size_cm"], entry["slots"]))


def texture_report():
    paths = unreal.EditorAssetLibrary.list_assets(BASE + "/Textures", recursive=False)
    for path in sorted(paths):
        tex = unreal.EditorAssetLibrary.load_asset(path)
        if tex is None:
            error("missing texture %s" % path)
            continue
        entry = {
            "name": tex.get_name(),
            "compression": str(tex.get_editor_property("compression_settings")),
            "srgb": bool(tex.get_editor_property("srgb")),
            "size": [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()],
        }
        try:
            entry["flip_green"] = bool(tex.get_editor_property("flip_green_channel"))
        except Exception:
            entry["flip_green"] = "n/a"
        report["textures"].append(entry)
        log("texture %s -> %s srgb=%s flip=%s" % (entry["name"], entry["compression"], entry["srgb"], entry["flip_green"]))


material_report("M_WeepingAngel_Body")
material_report("M_WeepingAngel_Face")
material_report("M_WeepingAngel_Wings")

mesh_report("SM_WeepingAngel_Idle")
mesh_report("SM_WeepingAngel_Attack")
mesh_report("SM_WeepingAngel_Point")

texture_report()

with open(REPORT_PATH, "w", encoding="utf-8") as handle:
    json.dump(report, handle, indent=2)

log("validation done, %d error(s)" % len(report["errors"]))
