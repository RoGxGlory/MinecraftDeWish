import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\inspect_socket_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
sock.set_socket_parent(skm, "hand_r")
out["name"] = sock.get_name()
out["bone_name"] = str(sock.bone_name) if hasattr(sock, "bone_name") else "NA"
# property access
for p in ["bone_name", "socket_name", "relative_location"]:
    try:
        out[p] = str(sock.get_editor_property(p))
    except Exception as e:
        out[p] = "ERR: " + str(e)
# Try initialize_socket_from_location
try:
    sock.initialize_socket_from_location(skm, unreal.Vector(0,0,0), unreal.Rotator(0,0,0))
    out["init_loc"] = "OK"
except Exception as e:
    out["init_loc"] = str(e)

# Add and inspect
skm.add_socket(sock)
out["num_after"] = skm.num_sockets()
out["find"] = str(skm.find_socket("weapon_r"))

# Maybe sockets are on skeleton instead; check skeleton's add_socket via asset
skel = skm.get_editor_property("skeleton")
out["skel_has_add_socket"] = hasattr(skel, "add_socket")

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("INSPECTSOCKET DONE")
