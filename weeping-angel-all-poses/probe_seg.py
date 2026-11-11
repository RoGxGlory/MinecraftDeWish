import unreal, json
out = {}
for name in ['AnimSegment','FAnimSegment','AnimTrack','SlotAnimationTrack']:
    out[name] = hasattr(unreal, name)
# find classes with 'segment' in name
out['seg_classes'] = [n for n in dir(unreal) if 'segment' in n.lower()]
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_seg_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_SEG_DONE")
