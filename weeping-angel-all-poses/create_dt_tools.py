# -*- coding: utf-8 -*-
"""Create DT_Tools DataTable and DA_ToolDefinition DataAsset for MinecraftDeWish (ADR-0007)."""

import json
import os

import unreal

PROJECT = r"F:\Dev\MinecraftDeWish"
REPORT_PATH = os.path.join(PROJECT, "weeping-angel-all-poses", "dt_tools_report.json")
REPORT = {"rows": [], "errors": []}

DATA_DIR = "/Game/Data"
DT_PATH = DATA_DIR + "/DT_Tools"
DA_PATH = DATA_DIR + "/DA_ToolDefinition"
PROBE_PATH = DATA_DIR + "/DT_Tools_Probe"

TIER_DURABILITY = {"Wood": 59, "Stone": 131, "Iron": 250, "Diamond": 1561, "Obsidian": 2031}
TIER_MINING_FORCE = {"Wood": 2.0, "Stone": 4.0, "Iron": 6.0, "Diamond": 10.0, "Obsidian": 15.0}
SWORD_DAMAGE = {"Wood": 4.0, "Stone": 5.0, "Iron": 6.0, "Diamond": 7.0, "Obsidian": 8.0}

TOOL_TYPES = ["Pickaxe", "Shovel", "Axe", "Hoe", "Sword"]
TIERS = ["Wood", "Stone", "Iron", "Diamond", "Obsidian"]

MESH_PATHS = {
    ("Pickaxe", "Wood"): "/Game/Tools/SM_Tool_Pickaxe_Wood.SM_Tool_Pickaxe_Wood",
}

IMPACT_CUES = {
    "Pickaxe": "Impact_Pickaxe",
    "Shovel": "Impact_Shovel",
    "Axe": "Impact_Axe",
    "Hoe": "Impact_Hoe",
    "Sword": "Impact_Sword",
}


def log(msg):
    unreal.log("[DT_Tools] " + str(msg))


def error(msg):
    unreal.log_error("[DT_Tools] " + str(msg))
    REPORT["errors"].append(str(msg))


def main():
    log("Unreal version: %s" % unreal.SystemLibrary.get_engine_version())

    if not unreal.EditorAssetLibrary.does_directory_exist(DATA_DIR):
        unreal.EditorAssetLibrary.make_directory(DATA_DIR)
        log("created folder %s" % DATA_DIR)

    # Clean up probe asset
    if unreal.EditorAssetLibrary.does_asset_exist(PROBE_PATH):
        unreal.EditorAssetLibrary.delete_asset(PROBE_PATH)
        log("deleted probe asset")

    # Delete existing real assets
    for p in (DT_PATH, DA_PATH):
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            unreal.EditorAssetLibrary.delete_asset(p)
            log("deleted existing %s" % p)

    # Build JSON rows
    rows = []
    for tool_type in TOOL_TYPES:
        for tier in TIERS:
            row_name = "%s_%s" % (tool_type, tier)
            mesh_path = MESH_PATHS.get((tool_type, tier), "None")
            attack_damage = SWORD_DAMAGE[tier] if tool_type == "Sword" else 1.0
            rows.append({
                "Name": row_name,
                "ToolType": tool_type,
                "ToolTier": tier,
                "BaseMiningForce": TIER_MINING_FORCE[tier],
                "DurabilityMax": TIER_DURABILITY[tier],
                "AttackDamage": attack_damage,
                "MeshPath": mesh_path,
                "IconPath": "None",
                "SwingMontage": "None",
                "ImpactCue": IMPACT_CUES[tool_type],
                "MiningForceByBlockCategory": {}
            })
            REPORT["rows"].append(row_name)

    json_str = json.dumps(rows)

    # Create DataTable
    row_struct = unreal.load_object(None, "/Script/MinecraftDeWish.ToolRow")
    factory = unreal.DataTableFactory()
    try:
        factory.set_editor_property("struct", row_struct)
    except Exception as e:
        log("factory struct set failed: %s" % e)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    dt = asset_tools.create_asset("DT_Tools", DATA_DIR, unreal.DataTable, factory)
    if dt is None:
        error("DataTable creation failed")
        with open(REPORT_PATH, "w", encoding="utf-8") as f:
            json.dump(REPORT, f, indent=2)
        return

    ok = dt.fill_from_json_string(json_str)
    log("fill_from_json_string -> %s" % ok)

    names = [str(n) for n in dt.get_row_names()]
    log("row count = %d" % len(names))
    if len(names) != 25:
        error("expected 25 rows, got %d" % len(names))

    unreal.EditorAssetLibrary.save_loaded_asset(dt, False)
    log("saved %s" % DT_PATH)

    # Create DA_ToolDefinition
    da = None
    # Approach 1: create_asset with ToolDefinition class and a factory
    try:
        fac = unreal.DataAssetFactory()
        try:
            fac.set_editor_property("data_asset_class", unreal.ToolDefinition.static_class() if hasattr(unreal.ToolDefinition, "static_class") else None)
        except Exception as e:
            log("fac data_asset_class set failed: %s" % e)
        da = asset_tools.create_asset("DA_ToolDefinition", DATA_DIR, unreal.DataAsset, fac)
    except Exception as e:
        log("approach1 failed: %s" % e)
        da = None

    if da is None:
        # Approach 2: create with ToolDefinition as asset class
        try:
            da = asset_tools.create_asset("DA_ToolDefinition", DATA_DIR, unreal.ToolDefinition, None)
        except Exception as e:
            log("approach2 failed: %s" % e)
            da = None

    if da is not None:
        try:
            da.set_editor_property("tools_table", dt)
            log("set tools_table ok")
        except Exception as e:
            error("setting tools_table failed: %s" % e)
        unreal.EditorAssetLibrary.save_loaded_asset(da, False)
        log("created %s" % DA_PATH)
    else:
        error("could not create DA_ToolDefinition")

    with open(REPORT_PATH, "w", encoding="utf-8") as f:
        json.dump(REPORT, f, indent=2)
    log("done, %d rows, %d errors" % (len(REPORT["rows"]), len(REPORT["errors"])))


main()