import json
import urllib.request

rev_id = "0655105637b04a25a6fc61a8c5f290cf"
rev = json.load(urllib.request.urlopen(f"http://127.0.0.1:8765/api/library/reviews/{rev_id}"))
voice = json.load(urllib.request.urlopen("http://127.0.0.1:8765/api/library/pages/internal/voices/5"))

print("=== REVIEW manualItems ===")
for it in rev.get("manualItems", []):
    print(it)

print("\n=== KEY PARAMS (API) ===")
keys = {"WOL", "ELVL", "EFSDLV", "EFLN1EL", "EFMODE", "WLLML", "RNDP"}
for p in voice["params"]:
    if p.get("registryId") in keys:
        print(
            f"{p['registryId']} el{p['elementIndex']}: ui={p.get('uiValue')} "
            f"8101={p.get('raw8101')} 0040={p.get('raw0040')} "
            f"label={p.get('valueLabel')!r} auto={p.get('autoCheckStatus')}"
        )
