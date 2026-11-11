import unreal
import json

out = {}
report_path = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\verify_arms_report.json"

skm = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/SK_Arms_FP")
out["skm"] = str(skm)
if skm is not None:
    skel = skm.get_editor_property("skeleton")
    out["skeleton"] = str(skel)
    # bone names
    try:
        bones = skel.get_editor_property("bone_tree") if hasattr(skel, "get_editor_property") else None
        out["bone_tree_type"] = str(type(bones))
    except Exception as e:
        out["bone_tree_err"] = str(e)
    # Try to get bone names via get_reference_skeleton? 
    try:
        ref_skel = skm.get_editor_property("ref_skeleton")  # maybe
        out["ref_skel"] = str(ref_skel)
    except Exception as e:
        out["ref_skel_err"] = str(e)
    out["num_sockets"] = skm.num_sockets()
    # list sockets
    try:
        for i in range(skm.num_sockets()):
            s = skm.get_socket_by_index(i)
            out.setdefault("sockets", []).append(str(s.get_name()) + " -> " + str(s.bone_name if hasattr(s,'bone_name') else '?'))
    except Exception as e:
        out["list_sockets_err"] = str(e)
    # Try adding socket now with correct bone name
    # find hand bone name
    # Use skeleton's bone names via get_editor_property 'bone_info'? 
    try:
        # UE Python: skeleton has 'get_blend_profile'? not. Try 'bone_tree' already.
        pass
    except Exception as e:
        pass

with open(report_path, "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, default=str)
unreal.log("VERIFYARMS DONE")
