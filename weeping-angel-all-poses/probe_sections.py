import unreal, json
m = unreal.EditorAssetLibrary.load_asset("/Game/Characters/Arms/Montages/M_Swing_Pickaxe")
out = {}
# try many candidate property names
cands = ["composite_sections", "CompositeSections", "sections", "Sections", "montage_sections",
         "CompositeSection", "anim_sections", "AnimSections", "section_array"]
for c in cands:
    try:
        v = m.get_editor_property(c)
        out[c] = str(type(v)) + " = " + str(v)[:200]
    except Exception as e:
        out[c] = "ERR: " + str(e)[:80]
# Also try get_editor_property on the class default object
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_sections_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_SECTIONS_DONE")
