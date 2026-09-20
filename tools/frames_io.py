"""Tonart-Klassifikator auf Keyys eigenem Chromagramm.

Eingabe sind die Rahmen aus `keyy-cli --frames`: je Rahmen 120 Faecher
(10 je Halbton). Daraus wird der Stimmton bestimmt, korrigiert und auf 12
Tonklassen gefaltet -- genau wie im C++-Kern, nur je Rahmen statt summiert.

Das Netz ist **transpositions-aequivariant** gebaut: Alle Faltungen ueber
die Tonhoehe sind zyklisch, und der Kopf liefert je Grundton einen Wert
fuer Dur und Moll. Dreht man die Eingabe um k Halbtoene, drehen sich die
Ausgaben um genau k mit. Das ist die Symmetrie der Aufgabe, und sie
geschenkt zu bekommen ist bei rund 1 100 Stuecken mehr wert als Kapazitaet:
Das Netz kann gar nicht lernen, einfach immer c-Moll zu raten.
"""
import numpy as np, struct, sys

BINS, SEMI = 120, 10          # Faecher gesamt, Faecher je Halbton

def lade(pfad):
    """Liest das KEYYFRM1-Format. Liefert (name, label, frames[T,120])."""
    with open(pfad, "rb") as f:
        magie = f.read(8)
        if magie != b"KEYYFRM1":
            raise ValueError(f"{pfad}: kein KEYYFRM1 (sondern {magie!r})")
        bins, anzahl = struct.unpack("<ii", f.read(8))
        assert bins == BINS, bins
        out = []
        for _ in range(anzahl):
            (nlen,) = struct.unpack("<i", f.read(4))
            name = f.read(nlen).decode("utf-8", "replace")
            label, rahmen = struct.unpack("<ii", f.read(8))
            roh = np.frombuffer(f.read(rahmen * bins * 4), dtype="<f4")
            out.append((name, label, roh.reshape(rahmen, bins).astype(np.float32)))
    return out

def stimmton_versatz(frames):
    """Versatz des Stimmtons in Faechern, -5..+5 -- wie getTuningCents.

    Das feine Histogramm modulo einem Halbton sagt, wo die Toene gegenueber
    dem 440-Hz-Raster liegen. Entscheidend ist das Zentrieren am Ende: Ein
    Gipfel bei Fach 8 heisst -2 Faecher, nicht +8. Ohne das ist das ganze
    Chromagramm um einen Halbton verdreht.
    """
    summe = frames.sum(axis=0).reshape(12, SEMI).sum(axis=0)     # 10 Faecher
    if summe.sum() <= 0:
        return 0.0
    glatt = np.roll(summe, -1) + 2.0 * summe + np.roll(summe, 1)
    i = int(np.argmax(glatt))
    a, b, c = glatt[(i - 1) % SEMI], glatt[i], glatt[(i + 1) % SEMI]
    nenner = a - 2 * b + c
    fein = 0.5 * (a - c) / nenner if nenner < 0 else 0.0
    versatz = (i + fein) % SEMI
    return versatz - SEMI if versatz >= SEMI / 2 else versatz

def faltmatrix(versatz):
    """(120, 12): verteilt jedes feine Fach auf die zwei nachbarlichen
    Halbtoene -- Zeile fuer Zeile dasselbe wie ChromaAccumulator::fold."""
    M = np.zeros((BINS, 12), dtype=np.float32)
    lage = np.arange(BINS) / SEMI - versatz / SEMI
    unten = np.floor(lage)
    anteil = lage - unten
    halbton = unten.astype(int)
    M[np.arange(BINS), halbton % 12] += 1.0 - anteil
    M[np.arange(BINS), (halbton + 1) % 12] += anteil
    return M

def zu_chroma(frames):
    """120 Faecher je Rahmen -> 12 Tonklassen, stimmton-korrigiert.

    Nicht je Rahmen normieren: Die Werte tragen schon die
    Tonalitaetsgewichtung des Kerns, und die soll erhalten bleiben.
    Normiert wird die Datei als Ganzes.
    """
    chroma = frames @ faltmatrix(stimmton_versatz(frames))
    mittel = chroma.sum() / max(1, len(chroma))
    return chroma / mittel if mittel > 0 else chroma

def datensatz(pfad, nur_mit_label=True, min_rahmen=4):
    namen, labels, daten = [], [], []
    for name, label, frames in lade(pfad):
        if nur_mit_label and label < 0: continue
        if len(frames) < min_rahmen: continue
        c = zu_chroma(frames)
        if c.sum() <= 0: continue
        namen.append(name); labels.append(label); daten.append(c)
    return namen, np.array(labels), daten

if __name__ == "__main__":
    for p in sys.argv[1:]:
        n, y, x = datensatz(p)
        laengen = np.array([len(a) for a in x])
        dur = int((y < 12).sum())
        print(f"{p}: {len(n)} Dateien, Rahmen {laengen.min()}..{laengen.max()} "
              f"(median {int(np.median(laengen))}), {dur} Dur / {len(n)-dur} Moll")
