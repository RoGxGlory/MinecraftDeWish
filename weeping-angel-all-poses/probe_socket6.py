import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_socket6_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
sock = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r")

attempts = []
# attempt: (component?, bone)
for args in [("hand_r",), ("hand_r", "hand_r")]:
    try:
        sock.set_socket_parent(*args)
        attempts.append((args, "OK"))
    except Exception as e:
        attempts.append((args, str(e)[:200]))

# Try with skm as first
try:
    sock2 = unreal.new_object(unreal.SkeletalMeshSocket, None, "weapon_r2")
    sock2.set_socket_parent(skm, "hand_r")
    attempts.append(("skm,hand_r", "OK"))
except Exception as e:
    attempts.append(("skm,hand_r", str(e)[:200]))

out["attempts"] = attempts

# Now try add_socket with the socket where bone set via set_socket_parent (if succeeded)
for a in attempts:
    if a[1] == "OK":
        try:
            skm.add_socket(sock)
            out["add_socket"] = "OK"
        except Exception as e:
            out["add_socket"] = str(e)[:200]

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("PROBESOCKET6 DONE")
