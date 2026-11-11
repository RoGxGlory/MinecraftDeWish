import unreal, json
out = {}
data = unreal.FbxAnimSequenceImportData()
out['props'] = [p for p in dir(data) if not p.startswith('_')]
# print property identifiers via rna
try:
    for p in data.get_class().get_editor_property if False else []:
        pass
except Exception as e:
    out['err'] = str(e)
# Try get_editor_property list by iterating known
import inspect
# Use unreal reflection: get all properties
try:
    cls = data.get_class()
    # property names from dir filtered
    out['all'] = [x for x in dir(data)]
except Exception as e:
    out['err2'] = str(e)
report = r'F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_fbxanim2_report.json'
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
# Also print property identifiers via get_class property names
unreal.log('PROBE_FBXANIM2_DONE')
# Dump rna properties
props = []
try:
    for p in data.get_class().get_properties():
        props.append(p.get_name())
except Exception as e:
    props.append('ERR ' + str(e))
with open(r'F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_fbxanim2_props.txt','w') as f:
    f.write('\n'.join(props))
unreal.log('props written')
