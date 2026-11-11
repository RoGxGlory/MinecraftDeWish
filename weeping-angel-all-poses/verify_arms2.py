import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\verify_arms2_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
skel = skm.get_editor_property("skeleton")
bone_tree = skel.get_editor_property("bone_tree")
out["bone_count"] = len(bone_tree)
names = []
for b in bone_tree:
    # b is a BoneNode? try name
    try:
        names.append(str(b.get_editor_property("name")) if hasattr(b, "get_editor_property") else str(b))
    except Exception as e:
        names.append("ERR: " + str(e))
out["bone_names"] = names

# Try add socket with bone name from list (find hand_r)
hand_name = None
for n in names:
    if "hand" in n.lower() and "_r" in n.lower():
        hand_name = n
out["hand_name"] = hand_name

if hand_name:
    sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
    try:
        sock.set_socket_parent(skm, hand_name)
        skm.add_socket(sock)
        out["add_socket"] = "OK"
    except Exception as e:
        out["add_socket"] = str(e)
    out["num_sockets"] = skm.num_sockets()
    unreal.EditorAssetLibrary.save_loaded_asset(skm, False)
    unreal.EditorAssetLibrary.save_loaded_asset(skel, False)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("VERIFYARMS2 DONE")
