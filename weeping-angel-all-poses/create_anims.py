import unreal
import json
import os

PROJECT = r"F:\Dev\MinecraftDeWish"
REPORT_PATH = os.path.join(PROJECT, "weeping-angel-all-poses", "create_anims_report.json")
REPORT = {"created": [], "errors": []}

ARMS_DIR = "/Game/Characters/Arms"
MONTAGE_DIR = ARMS_DIR + "/Montages"

def log(msg): unreal.log("[Anims] " + str(msg))
def error(msg):
    unreal.log_error("[Anims] " + str(msg)); REPORT["errors"].append(str(msg))

def main():
    for d in (ARMS_DIR, MONTAGE_DIR):
        if not unreal.EditorAssetLibrary.does_directory_exist(d):
            unreal.EditorAssetLibrary.make_directory(d)

    skel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP_Skeleton")
    if skel is None:
        error("skeleton not found"); 
        with open(REPORT_PATH,"w") as f: json.dump(REPORT,f,indent=2); return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    # AnimBlueprint
    abp_path = ARMS_DIR + "/ABP_FP_Arms"
    if unreal.EditorAssetLibrary.does_asset_exist(abp_path):
        unreal.EditorAssetLibrary.delete_asset(abp_path)
    fac = unreal.AnimBlueprintFactory()
    try:
        fac.set_editor_property("target_skeleton", skel)
    except Exception as e:
        error("ABP factory target_skeleton: %s" % e)
    abp = asset_tools.create_asset("ABP_FP_Arms", ARMS_DIR, unreal.AnimBlueprint, fac)
    if abp is None:
        error("ABP creation failed")
    else:
        log("created ABP_FP_Arms")
        REPORT["created"].append(abp_path)
        unreal.EditorAssetLibrary.save_loaded_asset(abp, False)

    # Montages
    montage_names = ["M_Swing_Pickaxe", "M_Swing_Shovel", "M_Swing_Axe", "M_Swing_Hoe", "M_Swing_Sword"]
    for name in montage_names:
        path = MONTAGE_DIR + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            unreal.EditorAssetLibrary.delete_asset(path)
        mfac = unreal.AnimMontageFactory()
        try:
            mfac.set_editor_property("target_skeleton", skel)
        except Exception as e:
            error("montage factory target_skeleton (%s): %s" % (name, e))
        montage = asset_tools.create_asset(name, MONTAGE_DIR, unreal.AnimMontage, mfac)
        if montage is None:
            error("montage creation failed: %s" % name)
        else:
            log("created %s" % name)
            REPORT["created"].append(path)
            unreal.EditorAssetLibrary.save_loaded_asset(montage, False)

    with open(REPORT_PATH, "w", encoding="utf-8") as f:
        json.dump(REPORT, f, indent=2)
    log("done, %d created, %d errors" % (len(REPORT["created"]), len(REPORT["errors"])))

main()
