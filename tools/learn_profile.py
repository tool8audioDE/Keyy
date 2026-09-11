"""Lernt ein Tonart-Profil aus Keyys eigenem Chromagramm und prüft es.

Warum das hier liegt: Das Profil `edm` in source/dsp/KeyProfiles.cpp ist
nicht aus der Literatur, sondern aus Messdaten. Ohne dieses Skript wäre
nicht nachvollziehbar, wie es entstanden ist — und es müsste neu gelernt
werden, sobald sich an der Chroma-Berechnung etwas ändert (Schwelle,
Exponent, Tonalitätsgewichtung), weil es genau zu diesen Einstellungen
gehört.

Ablauf:

    keyy-cli --batch <ordner> [--labels <ordner>] --csv ergebnis.csv
    python tools/learn_profile.py ergebnis.csv --profile

Das Profil ist schlicht das über alle Dateien gemittelte Chromagramm,
gedreht auf den jeweils beschrifteten Grundton — getrennt nach Dur und
Moll. Geprüft wird kreuzvalidiert: gelernt auf jeder zweiten Datei,
getestet auf den übrigen und umgekehrt. Ohne diese Trennung misst man nur,
wie gut sich das Profil an die eigenen Daten erinnert.

Gemessen an GiantSteps (301 Stücke, Schwelle 2, Exponent 0,5,
Tonalitätsgewichtung 1): Sha'ath 46,5 %, gelernt 54,5 % exakt.
"""

import csv
import math
import sys

NAMES = ["C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]


def parse_key(text):
    """'Fm' -> (5, True), 'Ab' -> (8, False)"""
    minor = text.endswith("m")
    return NAMES.index(text[:-1] if minor else text), minor


def correlation(a, b):
    mean_a, mean_b = sum(a) / 12, sum(b) / 12
    cov = sum((x - mean_a) * (y - mean_b) for x, y in zip(a, b))
    var_a = sum((x - mean_a) ** 2 for x in a)
    var_b = sum((y - mean_b) ** 2 for y in b)
    return cov / math.sqrt(var_a * var_b) if var_a > 0 and var_b > 0 else 0.0


def learn(subset):
    """Mittleres Chromagramm je Tongeschlecht, gedreht auf den Grundton."""
    acc = {False: [0.0] * 12, True: [0.0] * 12}
    for (tonic, minor), chroma in subset:
        for i in range(12):
            acc[minor][i] += chroma[(tonic + i) % 12]
    return acc[False], acc[True]


def estimate(chroma, major, minor_profile):
    best = None
    for is_minor, profile in ((False, major), (True, minor_profile)):
        for tonic in range(12):
            score = correlation(chroma, [profile[(pc - tonic) % 12] for pc in range(12)])
            if best is None or score > best[0]:
                best = (score, tonic, is_minor)
    return best[1], best[2]


def score(subset, major, minor_profile):
    """Liefert (exakt, Tonleiter, MIREX) in Prozent.

    Tonleiter zählt die Paralleltonart mit: Beide haben dieselben sieben
    Töne, und für eine Tonhöhenkorrektur ist genau das die Frage.
    """
    exact = scale_ok = 0
    mirex = 0.0
    for (tonic, minor), chroma in subset:
        et, em = estimate(chroma, major, minor_profile)
        scale = lambda t, m: (t + 3) % 12 if m else t
        if (et, em) == (tonic, minor):
            exact += 1
            mirex += 1.0
        elif em == minor and (et - tonic) % 12 in (5, 7):
            mirex += 0.5
        elif em != minor and et == (tonic + (9 if not minor else 3)) % 12:
            mirex += 0.3
        elif em != minor and et == tonic:
            mirex += 0.2
        if scale(et, em) == scale(tonic, minor):
            scale_ok += 1
    n = len(subset)
    return 100 * exact / n, 100 * scale_ok / n, 100 * mirex / n


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    rows = [r for r in csv.DictReader(open(sys.argv[1], encoding="utf-8")) if r["erwartet"]]
    data = [(parse_key(r["erwartet"]), [float(r["chroma_" + n]) for n in NAMES]) for r in rows]

    if not data:
        print("Keine Zeilen mit Beschriftung und Chromagramm gefunden.")
        return 1

    # Ein Profil aus einer Handvoll Dateien ist keines. Die Zahlen sehen
    # trotzdem nach Messung aus — deshalb hier deutlich sagen, wenn sie
    # nichts bedeuten.
    if len(data) < 30:
        print(f"ACHTUNG: nur {len(data)} Dateien. Gelernt wird auf der Haelfte davon;")
        print("das reicht weder fuer ein Profil noch fuer eine belastbare Quote.")
        print("Sinnvoll ab etwa 50 Dateien, besser mehr.\n")

    a, b = data[0::2], data[1::2]
    r1, r2 = score(b, *learn(a)), score(a, *learn(b))
    print(f"{len(data)} Dateien, kreuzvalidiert:")
    print(f"  exakt      {(r1[0] + r2[0]) / 2:5.1f} %")
    print(f"  Tonleiter  {(r1[1] + r2[1]) / 2:5.1f} %")
    print(f"  MIREX      {(r1[2] + r2[2]) / 2:5.1f}")

    if "--profile" in sys.argv:
        major, minor_profile = learn(data)
        norm = lambda v: ", ".join(f"{100 * x / sum(v):.2f}" for x in v)
        print("\nFuer source/dsp/KeyProfiles.cpp (Reihenfolge ab Grundton):")
        print(f"  Dur : {{ {norm(major)} }}")
        print(f"  Moll: {{ {norm(minor_profile)} }}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
