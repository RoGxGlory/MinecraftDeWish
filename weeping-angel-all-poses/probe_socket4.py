import unreal
import json
import inspect

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_socket4_report.json"

try:
    out["init_sig"] = str(inspect.signature(unreal.SkeletalMeshSocket.__init__))
except Exception as e:
    out["init_sig_err"] = str(e)

# Try construct with keyword args
try:
    s = unreal.SkeletalMeshSocket(socket_name="weapon_r", bone_name="hand_r")
    out["ctor_kwargs"] = "OK: " + str(s)
except Exception as e:
    out["ctor_kwargs"] = str(e)

# Try positional
try:
    s = unreal.SkeletalMeshSocket("weapon_r", "hand_r")
    out["ctor_pos"] = "OK: " + str(s)
except Exception as e:
    out["ctor_pos"] = str(e)

# Try new_object
try:
    s = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")
    out["new_object"] = "OK: " + str(s)
except Exception as e:
    out["new_object"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBESOCKET4 DONE")
