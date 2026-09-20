"""Trainiert einen Tonart-Klassifikator auf Keyys eigenem Chromagramm.

Gedacht als Messwerkzeug, nicht als Teil des Plugins: Das Ergebnis dieser
Sitzung steht im README ("Ein neuronales Netz -- was es bringt").

Ablauf:

    keyy-cli --batch <mtg-audio> --labels <mtg-key> --frames mtg.bin
    keyy-cli --batch <gs-audio>  --labels <gs-key>  --frames gs.bin
    keyy-cli --batch testdata --frames eigene.bin
    python tools/train_net.py <verzeichnis-mit-den-bin-dateien>

Das Netz ist **transpositions-aequivariant** gebaut: Alle Faltungen ueber
die Tonhoehe sind zyklisch, und der Kopf liefert je Grundton einen Wert
fuer Dur und Moll. Dreht man die Eingabe um k Halbtoene, dreht sich die
Ausgabe um genau k mit. Das ist die Symmetrie der Aufgabe; sie geschenkt
zu bekommen ist bei rund 2 000 Stuecken mehr wert als Kapazitaet -- das
Netz kann gar nicht lernen, einfach immer c-Moll zu raten.

Zwei Dinge, die beim Bauen teuer waren und hier festgehalten gehoeren:

1. Die Merkmale muessen dieselben sein wie im C++-Kern. `frames_io` bildet
   ChromaAccumulator::fold nach, samt Zentrierung des Stimmtons. Fehlt die,
   ist das ganze Chromagramm um einen Halbton verdreht -- die Profile
   kamen dann auf 11 statt 53 Prozent. `python tools/frames_io.py *.bin`
   und danach die Profil-Gegenprobe ist die erste Kontrolle, nicht die
   letzte.
2. Die Trainingsstuecke sind 320 Rahmen lang, die Loops des Nutzers im
   Median 22. Ohne gemischte Fensterlaengen faellt das Netz auf kurzen
   Dateien von 52 auf 42 Prozent.
"""
import numpy as np, os, sys, time
import torch, torch.nn as nn, torch.nn.functional as F

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from frames_io import lade, zu_chroma, datensatz


def zyklisch(x, k=1):
    """Ringschluss in der Tonhoehe: Fach 11 ist Nachbar von Fach 0."""
    return F.pad(x, (k, k, 0, 0), mode="circular")


class Block(nn.Module):
    def __init__(self, hinein, heraus):
        super().__init__()
        self.conv = nn.Conv2d(hinein, heraus, (3, 3), padding=(1, 0))
        self.norm = nn.BatchNorm2d(heraus)

    def forward(self, x):
        return F.relu(self.norm(self.conv(zyklisch(x))))


class KeyyNet(nn.Module):
    """Eingabe (B, 1, Rahmen, 12) -> 24 Werte, Grundton + 12 bei Moll."""

    def __init__(self, breite=64):
        super().__init__()
        self.b1, self.b2 = Block(1, breite), Block(breite, breite)
        self.b3, self.b4 = Block(breite, breite), Block(breite, breite)
        self.kopf = nn.Conv1d(breite, 2, 3, padding=0)

    def forward(self, x):
        h = self.b4(self.b3(self.b2(self.b1(x))))
        h = h.mean(dim=2)                                    # ueber die Zeit
        h = self.kopf(F.pad(h, (1, 1), mode="circular"))     # (B, 2, 12)
        return h.reshape(h.size(0), 24)


def zieh(chroma, laenge, rng):
    """Ausschnitt gegebener Laenge; kurze Dateien werden umlaufend gekachelt."""
    if len(chroma) < laenge:
        chroma = np.tile(chroma, (int(np.ceil(laenge / len(chroma))), 1))
    s = rng.integers(0, len(chroma) - laenge + 1)
    return chroma[s:s + laenge]


def bewerte(modell, daten, labels):
    """Je Datei in einem Stueck -- das Profil sieht die Datei auch ganz."""
    modell.eval()
    treffer = tonleiter = 0
    mirex = 0.0
    with torch.no_grad():
        for c, y in zip(daten, labels):
            ist = int(modell(torch.from_numpy(c[None]).float().unsqueeze(1))[0].argmax())
            it, im = ist % 12, ist >= 12
            st, sm = int(y) % 12, int(y) >= 12
            skala = lambda t, m: (t + 3) % 12 if m else t
            if ist == y:
                treffer += 1; mirex += 1.0
            elif im == sm and (it - st) % 12 in (5, 7):
                mirex += 0.5
            elif im != sm and it == (st + (9 if not sm else 3)) % 12:
                mirex += 0.3
            elif im != sm and it == st:
                mirex += 0.2
            if skala(it, im) == skala(st, sm):
                tonleiter += 1
    n = max(1, len(labels))
    return 100 * treffer / n, 100 * tonleiter / n, 100 * mirex / n


def trainiere(x_tr, y_tr, x_va, y_va, epochen=40, pro_datei=4, stapel=32, keim=11):
    rng = np.random.default_rng(keim)
    torch.manual_seed(keim)
    modell = KeyyNet()
    opt = torch.optim.AdamW(modell.parameters(), lr=1e-3, weight_decay=3e-4)
    plan = torch.optim.lr_scheduler.CosineAnnealingLR(opt, epochen)

    # Logarithmisch verteilte Fensterlaengen, damit kurze Loops nicht
    # untergehen -- siehe Punkt 2 im Kopf dieser Datei.
    laengen = np.unique(np.round(np.exp(np.linspace(np.log(4), np.log(320), 16))).astype(int))
    print(f"Parameter {sum(p.numel() for p in modell.parameters()):,}, "
          f"Fensterlaengen {laengen[0]}..{laengen[-1]}")

    bestes, beste = -1.0, None
    for epoche in range(epochen):
        modell.train()
        t0 = time.time()
        paare = [(c, int(v)) for c, v in zip(x_tr, y_tr) for _ in range(pro_datei)]
        rng.shuffle(paare)
        verlust = anzahl = 0.0
        for i in range(0, len(paare), stapel):
            teil = paare[i:i + stapel]
            laenge = int(rng.choice(laengen))
            X, Y = [], []
            for c, v in teil:
                k = int(rng.integers(12))
                X.append(np.roll(zieh(c, laenge, rng), k, axis=1))
                Y.append((v % 12 + k) % 12 + (12 if v >= 12 else 0))
            opt.zero_grad()
            l = F.cross_entropy(modell(torch.from_numpy(np.stack(X)).float().unsqueeze(1)),
                                torch.tensor(Y))
            l.backward(); opt.step()
            verlust += l.item() * len(teil); anzahl += len(teil)
        plan.step()

        if (epoche + 1) % 5 == 0:
            # Auswahl nach ganzen Dateien UND kurzen Ausschnitten: Das Modell
            # soll beides koennen, nicht nur das Trainingsformat.
            lang = bewerte(modell, x_va, y_va)[0]
            kurz = bewerte(modell, [c[:24] for c in x_va], y_va)[0]
            print(f"Epoche {epoche+1:3d}  Verlust {verlust/anzahl:.3f}  "
                  f"Entwicklung ganz {lang:5.1f} % / kurz {kurz:5.1f} %  ({time.time()-t0:.0f} s)")
            if 0.5 * (lang + kurz) > bestes:
                bestes = 0.5 * (lang + kurz)
                beste = {k: v.clone() for k, v in modell.state_dict().items()}

    modell.load_state_dict(beste)
    return modell


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1

    ordner = sys.argv[1]
    _, y_mtg, x_mtg = datensatz(os.path.join(ordner, "mtg.bin"))
    _, y_gs, x_gs = datensatz(os.path.join(ordner, "gs.bin"))
    _, y_ei, x_ei = datensatz(os.path.join(ordner, "eigene.bin"))
    print(f"MTG {len(x_mtg)} zum Trainieren, GiantSteps {len(x_gs)} und "
          f"eigene {len(x_ei)} zum Pruefen")

    misch = np.random.default_rng(11).permutation(len(x_mtg))
    schnitt = int(0.88 * len(misch))
    itr, iva = misch[:schnitt], misch[schnitt:]
    modell = trainiere([x_mtg[i] for i in itr], y_mtg[itr],
                       [x_mtg[i] for i in iva], y_mtg[iva])
    torch.save(modell.state_dict(), os.path.join(ordner, "keyynet.pt"))

    print()
    print(f"{'Satz':<26}{'exakt':>9}{'Tonleiter':>12}{'MIREX':>8}")
    for nm, x, y in (("GiantSteps Key", x_gs, y_gs), ("eigene Dateien", x_ei, y_ei)):
        e, s, m = bewerte(modell, x, y)
        print(f"{nm:<26}{e:8.1f} %{s:11.1f} %{m:8.1f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
