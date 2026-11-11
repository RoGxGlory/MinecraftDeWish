import unreal, json
out = {}
# find types containing Eval
candidates = [n for n in dir(unreal) if 'eval' in n.lower() or 'pose' in n.lower()]
out['candidates'] = candidates
report = r"F:\Dev\MinecraftDeWish\weeping-angel-all-poses\probe_evalopt_report.json"
with open(report,'w') as f: json.dump(out, f, indent=2, default=str)
unreal.log("PROBE_EVALOPT_DONE")
