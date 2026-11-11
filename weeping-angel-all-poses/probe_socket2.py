import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_socket2_report.json"
sock = unreal.SkeletalMeshSocket()
out["sock_type"] = str(type(sock))
out["props"] = [p for p in dir(sock) if not p.startswith("_")]
# Try set_editor_property with various names
for name in ["socket_name", "SocketName", "bone_name", "BoneName"]:
    try:
        sock.set_editor_property(name, "weapon_r")
        out["set_" + name] = "OK"
    except Exception as e:
        out["set_" + name] = str(e)
# Check readable props
for name in ["socket_name", "bone_name"]:
    try:
        out["get_" + name] = str(sock.get_editor_property(name))
    except Exception as e:
        out["get_" + name] = str(e)
# Check Skeleton methods for socket
skel = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
out["skel_methods_socket"] = [m for m in dir(skel) if "sock" in m.lower() or "add" in m.lower()]
# Check USkeletalMesh has sockets
skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
out["skm_socket_methods"] = [m for m in dir(skm) if "sock" in m.lower()]
out["skm_props_socket"] = [p for p in dir(skm) if "sock" in p.lower()]

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBESOCKET2 DONE")
