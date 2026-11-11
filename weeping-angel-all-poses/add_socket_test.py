import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\add_socket_test_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
out["before"] = skm.num_sockets()

sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
try:
    sock.set_socket_parent(skm, "hand_r")
    out["set_parent"] = "OK"
except Exception as e:
    out["set_parent"] = str(e)

try:
    skm.add_socket(sock)
    out["add_socket"] = "OK"
except Exception as e:
    out["add_socket"] = str(e)

out["after_add"] = skm.num_sockets()
out["find"] = str(skm.find_socket("weapon_r"))

# Try saving
try:
    unreal.EditorAssetLibrary.save_loaded_asset(skm, False)
    out["saved"] = True
except Exception as e:
    out["saved"] = str(e)

# Reload and check
skm2 = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
out["reloaded_sockets"] = skm2.num_sockets() if skm2 else None

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("ADD_SOCKET_TEST DONE")
