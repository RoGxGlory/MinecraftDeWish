import unreal
import json
import inspect

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_socket3_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
# Inspect add_socket
try:
    out["add_socket_sig"] = str(inspect.signature(skm.add_socket))
except Exception as e:
    out["add_socket_sig_err"] = str(e)
# Try add_socket with name and bone
try:
    skm.add_socket("weapon_r", "hand_r")
    out["add_socket_call1"] = "OK"
except Exception as e:
    out["add_socket_call1"] = str(e)
try:
    out["num_sockets"] = skm.num_sockets()
    out["find_socket"] = str(skm.find_socket("weapon_r"))
except Exception as e:
    out["socket_query_err"] = str(e)

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBESOCKET3 DONE")
