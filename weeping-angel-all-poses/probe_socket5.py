import unreal
import json
import inspect

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_socket5_report.json"

sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
out["sock_name"] = str(sock.get_name())
# methods
out["methods"] = [m for m in dir(sock) if "parent" in m.lower() or "bone" in m.lower() or "init" in m.lower() or "transform" in m.lower()]

# Try set_socket_parent with bone name string
try:
    sock.set_socket_parent("hand_r")
    out["set_socket_parent_str"] = "OK"
except Exception as e:
    out["set_socket_parent_str"] = str(e)

# Try set_socket_parent with None? Check signature
try:
    out["set_socket_parent_sig"] = str(inspect.signature(sock.set_socket_parent))
except Exception as e:
    out["set_socket_parent_sig_err"] = str(e)

# Check bone_name property maybe set via set_editor_property after construction? already read-only. Try set_editor_property anyway on this instance
try:
    sock.set_editor_property("bone_name", "hand_r")
    out["set_bone_name"] = "OK"
except Exception as e:
    out["set_bone_name"] = str(e)

# relative_location etc writable?
try:
    sock.set_editor_property("relative_location", unreal.Vector(0,0,0))
    out["set_rel_loc"] = "OK"
except Exception as e:
    out["set_rel_loc"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBESOCKET5 DONE")
