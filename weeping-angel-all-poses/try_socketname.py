import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\try_socketname_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
sock.set_socket_parent(skm, "hand_r")

# Try direct attribute assignment
try:
    sock.socket_name = "weapon_r"
    out["direct_assign"] = "OK"
except Exception as e:
    out["direct_assign"] = str(e)

# Try rename the uobject to set socket name? Already named weapon_r.
out["uobject_name"] = sock.get_name()
out["socket_name_prop"] = str(sock.get_editor_property("socket_name"))

# Try add_socket again and check
skm.add_socket(sock)
out["num"] = skm.num_sockets()
out["find"] = str(skm.find_socket("weapon_r"))

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("TRY_SOCKETNAME DONE")
