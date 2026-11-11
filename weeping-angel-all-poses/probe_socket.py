import unreal
import json
import traceback

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_socket_report.json"

try:
    skm_path = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
    exists = unreal.EditorAssetLibrary.does_asset_exist(skm_path)
    out["exists"] = exists
    skm = unreal.EditorAssetLibrary.load_asset(skm_path)
    out["skm"] = str(skm) + " " + str(type(skm))
    if skm is not None:
        skel = skm.get_editor_property("skeleton")
        out["skeleton"] = str(skel)
        out["socket_methods"] = [m for m in dir(skel) if "socket" in m.lower()]
        try:
            sockets = skel.sockets
            out["sockets_attr"] = [str(s) for s in (sockets or [])]
        except Exception as e:
            out["sockets_attr_err"] = str(e)
        try:
            out["sockets_prop"] = str(skel.get_editor_property("sockets"))
        except Exception as e:
            out["sockets_prop_err"] = str(e)
        # Try add socket
        try:
            sock = unreal.SkeletalMeshSocket()
            sock.set_editor_property("socket_name", "weapon_r")
            sock.set_editor_property("bone_name", "hand_r")
            skel.add_socket(sock)
            out["add_socket_ok"] = True
        except Exception as e:
            out["add_socket_err"] = str(e)
except Exception as e:
    out["exception"] = traceback.format_exc()

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBESOCKET DONE")
