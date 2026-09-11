# Keyy

Tonart-, Stimmton- und Tempoerkennung als VST3-Plugin und Standalone-App,
gebaut mit JUCE 8. Funktional angelehnt an Antares Auto-Key 2, zunächst nur
der Modus „Use File“: Audiodatei laden oder aufs Plugin ziehen, Ergebnis
ablesen.

Einstieg für eine neue Arbeitssitzung: erst dieses README, dann
`CONTEXT.md` — dort stehen Zielsetzung, getroffene und verworfene
Entscheidungen sowie der offene Stand.

---

## Stand

| Baustein | Zustand |
|---|---|
| `ChromaAccumulator` (Spektralspitzen, Stimmton) | fertig, getestet |
| `KeyEstimator` (4 Profile, 24 Tonarten) | fertig, getestet |
| `TempoEstimator` (Einsätze, Looplänge) | fertig, getestet |
| `KeyAnalyzer` (Dezimierung, blockweise) | fertig, getestet |
| Dateinamen-Parser (Tonart, BPM) | fertig, getestet |
| Offline-CLI inkl. Batch-Auswertung | fertig |
| Unit-Tests | 18 Testfälle, grün |
| VST3 + Standalone | baut unter Windows/MSVC |
| Oberfläche | minimal, ohne Gestaltung; Laden, Anzeige, Tauschen im Standalone geprüft |
| Trefferquote GiantSteps (EDM, 301 Stücke) | **54 % exakt**, **66 % Tonleiter**, MIREX 64 — siehe unten |
| Trefferquote an eigenen Loops/Beats | **offen** — bisher nur 4 Dateien, deren Namen aus Auto-Tune-Einstellungen stammen und nicht das Instrumental beschreiben |
| Listen- und Manual-Modus | später |

---

## Bauen (Windows, Visual Studio)

Voraussetzung: Visual Studio (oder Build Tools) mit C++-Workload, CMake, Git.

```bat
cmake -B build -DKEYY_COPY_PLUGIN=OFF
cmake --build build --config Release
```

Der erste Aufruf lädt JUCE 8.0.15, Catch2 und minimp3 herunter. Wer Voxx
schon gebaut hat, spart sich den JUCE-Download:

```bat
cmake -B build -DKEYY_COPY_PLUGIN=OFF -DFETCHCONTENT_SOURCE_DIR_JUCE=../Voxx/build/_deps/juce-src
```

Ergebnisse unter `build/Keyy_artefacts/Release/`:

* `VST3/Keyy.vst3` — nach `C:\Program Files\Common Files\VST3` kopieren
  (oder einen eigenen Ordner in FL Studio unter *Options → Manage plugins*
  eintragen) und in FL neu scannen
* `Standalone/Keyy.exe` — zum Testen ohne DAW

`KEYY_COPY_PLUGIN=ON` (Vorgabe) kopiert das VST3 automatisch, braucht dafür
aber Adminrechte — ohne sie scheitert der Bau im letzten Schritt.

Zwei Eigenheiten unter Windows, beide wie bei Voxx: Der Generator „Visual
Studio“ ist Multi-Config, `-DCMAKE_BUILD_TYPE` wirkt dort nicht — es zählt
`--config Release`. Und **nur am DSP arbeiten** geht mit
`-DKEYY_BUILD_PLUGIN=OFF` ohne JUCE in Sekunden.

---

## Benutzen

Keyy als Effekt auf eine beliebige Mixer-Spur legen; das Audiosignal läuft
unverändert durch. Datei per Knopf laden oder aus dem FL-Browser bzw.
Explorer aufs Fenster ziehen. Gelesen werden WAV, AIFF, FLAC, MP3 und Ogg.

Angezeigt werden:

* **Tonart** und **Paralleltonart** — der Knopf ⇅ tauscht beide. Beide
  bestehen aus denselben sieben Tönen: Für Auto-Tune ist die Wahl also
  gleichgültig, für die Beschriftung eines Samples nicht. Wer weiß, dass
  der Loop in As-Dur gedacht ist, tauscht.
* **Stimmton**, z.B. `A = 437.0 Hz (-11.9 Cent)`. Wichtig bei Samples aus
  alten Platten, die nicht auf 440 Hz stehen.
* **Tempo** mit `/2` und `x2`. „aus Looplänge“ heißt: Die Datei ist exakt
  eine ganze Zahl von Takten lang, das Tempo ist daraus berechnet und
  genauer als jede Messung.

Das Ergebnis wird im FL-Projekt gespeichert und steht beim Öffnen sofort
wieder da, auch ohne die Datei.

**MP3 aus FL Studio.** JUCEs MP3-Dekoder steigt bei LAME-Dateien aus FL
Studio kurz vor dem Ende aus — gemessen 66,1 von 66,3 Sekunden. Keyy
wertet deshalb aus, was es lesen konnte, statt abzubrechen, und meldet es
nur, wenn mehr als eine Sekunde oder 2 % der Datei fehlen. Das CLI benutzt
einen anderen Dekoder (minimp3) und liest dieselben Dateien vollständig;
beide liefern dasselbe Ergebnis.

---

## Selbst prüfen

### 1. Unit-Tests

```bat
cmake -B build-dsp -DKEYY_BUILD_PLUGIN=OFF
cmake --build build-dsp --config Release
build-dsp\Release\keyy-tests
```

Geprüft wird unter anderem: dass alle 24 Tonarten an einer Kadenz mit jedem
Profil erkannt werden, dass der Stimmton bei 432, 437, 446 und 450 Hz auf
3 Cent genau stimmt, dass eine Kadenz unter einem Drum-Loop noch erkannt
wird, dass ein exakt geschnittener Loop sein Tempo aus der Länge bekommt
und ein ungenau geschnittener nicht, dass ein gehaltener Akkord kein Tempo
vortäuscht und dass das Ergebnis nicht von der Blockgröße abhängt.

### 2. Eine Datei

```bat
build-dsp\Release\keyy-cli loop.wav --profile all
```

Zeigt die Tonart je Profil samt Zweitkandidat, Stimmton und Tempo.
`--report chroma.csv` schreibt das Chromagramm und alle 24 Korrelationen
mit — daran ist ablesbar, *warum* eine Tonart gewählt wurde.

### 3. Trefferquote über einen Ordner

```bat
build-dsp\Release\keyy-cli --batch testdata --profile all --csv ergebnis.csv
```

Durchläuft alle WAV/MP3-Dateien, liest die richtige Tonart aus dem
Dateinamen (`Loop_Fm_140bpm.wav`) und zählt je Profil: exakt, Quinte,
Paralleltonart, gleichnamig, andere, dazu den MIREX-Wert. Dateien ohne
Tonart im Namen werden übersprungen. Die Fehlgriffe werden einzeln
aufgelistet — die gehören angehört, nicht nur gezählt: Oft ist die
Beschriftung im Sample-Pack selbst falsch oder meint die Paralleltonart.

Für den GiantSteps-Datensatz liegen die Tonarten in eigenen Dateien:
`--labels testdata/giantsteps/annotations/key`.

---

## Verfahren

**Dezimierung auf etwa 11 kHz.** Tonart und Tempo stecken unter 5 kHz. Bei
einem Viertel der Abtastrate braucht dieselbe Frequenzauflösung ein Viertel
so große FFTs. Ganzzahliger Faktor ohne Interpolation, davor ein
Butterworth-Tiefpass achter Ordnung.

**Chromagramm aus Spektralspitzen.** FFT über 16384 Punkte (1,5 s, 0,67 Hz
Auflösung — um 50 Hz, wo die 808 spielt, liegen Halbtöne nur 3 Hz
auseinander). Gezählt werden nur Spitzen, die doppelt so hoch wie das
Mittel ihrer Umgebung sind: Rauschen von Hi-Hats und Snares ragt kaum
heraus, Töne schon. Die Höhe jeder Spitze geht mit der Quadratwurzel ein,
damit laute Bässe die Akkordtöne nicht überdecken. Jeder Rahmen zählt nach
seinem tonalen Anteil (Spitzen gegen ganzes Spektrum) — nicht nach
Lautstärke, sonst bestimmten die Kick-Einschläge die Tonart.

**Stimmton aus dem häufigsten Wert.** Jede Spitze trägt ihre Abweichung vom
440-Hz-Raster in ein Cent-Histogramm ein. Der Stimmton ist dessen Maximum,
nicht der Mittelwert: Obertöne liegen nicht im gleichstufigen Raster (der
fünfte 14 Cent zu tief, der siebte 31 Cent) und würden einen Mittelwert
mitziehen. Erst am Ende wird das feine Tonklassen-Histogramm um den
Stimmton verschoben und auf 12 Tonklassen gefaltet — ein Durchgang genügt.

**Tonart aus der Korrelation mit Profilen.** Das Chromagramm wird mit allen
24 Tonarten verglichen (Pearson-Korrelation mit dem gedrehten Profil). Vier
Profile stehen zur Wahl: Krumhansl (Hörversuche), Temperley (Notenzählung),
Sha'ath (aus Pop- und elektronischer Musik) und `edm` — aus Keyys eigenem
Chromagramm gelernt, gemittelt über GiantSteps. Die Lehrbuch-Profile
beschreiben, welche Töne eine Tonart ausmachen; `edm` beschreibt, wie das
Chromagramm dieser Analyse bei echter Musik tatsächlich aussieht, mit
Obertönen und Rauschboden. Vorgabe ist vorläufig `edm`.

**Tempo aus Einsätzen.** Spectral Flux (46-ms-Rahmen alle 6 ms),
Autokorrelation, ausgewertet als Kamm über vier Schlagabstände, dann mit
acht verfeinert. Eine schwache Vorliebe für 110 BPM entscheidet zwischen
halbem und doppeltem Tempo; der Rest ist Sache von `/2` und `x2`.

**Tempo aus der Looplänge.** Ist die Datei höchstens 64 s lang und ergibt
ihre Länge bei 1, 2, 4, 8 … Takten ein Tempo, das bis auf 0,02 BPM
ganzzahlig ist und höchstens 4 % neben der Messung liegt, gilt dieses.

**Kein Tempo bei weichen Einsätzen.** Gehaltene Töne schweben gegeneinander,
und die Schwebung ist periodisch — die Autokorrelation fände darin ein
Tempo. Schläge sind aber spitz, Schwebungen weich. Unterhalb einer
Mindest-Spitzheit meldet Keyy „Tempo nicht erkannt“.

---

## Messung an GiantSteps

301 der 604 Stücke (jedes zweite), 2-Minuten-Ausschnitte elektronischer
Tanzmusik von Beatport, 85 % davon in Moll. Ein schwerer Datensatz. Zur
Einordnung, aus der Literatur übernommen und nicht selbst nachgemessen:
Klassische Chroma-Verfahren liegen dort grob zwischen 45 und 60 %, erst
neuronale Netze kommen in die Nähe von 75 %.

| Profil | exakt | Quinte | Parallele | gleichnamig | andere | MIREX |
|---|---|---|---|---|---|---|
| krumhansl | 39,2 % | 12,6 % | 7,6 % | 17,6 % | 22,9 % | 51,3 |
| temperley | 37,2 % | 15,9 % | 13,6 % | 10,6 % | 22,6 % | 51,4 |
| shaath | 46,5 % | 12,6 % | 7,3 % | 14,3 % | 19,3 % | 57,9 |
| **edm** | **54,2 %** | 11,6 % | 11,3 % | 4,7 % | 18,3 % | **64,3** |

`edm` ist auf demselben Datensatz gelernt. Damit das kein Auswendiglernen
misst, wurde kreuzvalidiert — gelernt auf der einen Hälfte, geprüft auf der
anderen und umgekehrt: 54,5 %. Der Wert hält also.

**Die strenge Zahl ist für den Einsatzzweck zu streng.** Keyy wird benutzt,
um eine Tonhöhenkorrektur einzustellen, und dort haben a-Moll und C-Dur
dieselben sieben erlaubten Töne. Zählt man die Paralleltonart als richtig —
also „stimmt die Tonleiter?“ —, liegt Keyy bei **65,8 %** (kreuzvalidiert).
Eine Erkennung, die von vornherein nur die zwölf Tonleitern unterscheidet
statt der 24 Tonarten, bringt nichts zusätzlich (65,5 %): Die vorhandene
Rechnung liefert die Tonleiter bereits so gut, wie es geht.

Was die Stellschrauben bewirken (kreuzvalidiert, exakt):

| Stellschraube | Werte → Trefferquote |
|---|---|
| Spitzen-Schwelle | 1 → 25 %, 1,5 → 45 %, **2 → 55 %**, 3 → 54 %, 6 → 42 %, 10 → 30 % |
| Exponent | 0 → 35 %, 0,25 → 52 %, **0,5 → 55 %**, 1 → 49 % |
| Tonalitätsgewichtung | 0 → 53 %, **1 → 55 %**, 2 → 54 % |
| Frequenzbereich | 45–800 Hz bis 100–3500 Hz: kaum Unterschied (± 2 Punkte) |

**Der Bass hilft nicht.** Naheliegend wäre, den Grundton aus dem Bass zu
holen: Ein zweites Chromagramm bis 250 Hz, dessen Grundton in die
Entscheidung eingeht. Gemessen über alle Gewichtungen von 0,25 bis 3: 54,5 %
gegen 54,8 % — Rauschen. Bei den beiden Fehlgriffen an eigenen Dateien
widersprach der Bass sogar der Beschriftung.

**Eine naheliegende Vermutung war falsch.** Die Fehler sahen nach Obertönen
aus: Moll wurde oft als Dur erkannt (der 5. Oberton eines Tons ist seine
große Terz) und die Tonart eine Quinte zu hoch (der 3. Oberton ist die
Quinte). Das über alle Moll-Stücke gemittelte Chromagramm zeigte aber die
kleine Terz deutlich über der großen — das Chromagramm stimmt, es passt nur
nicht zu den Lehrbuch-Profilen. Deshalb das gelernte Profil statt einer
Oberton-Unterdrückung.

Nachmessen:

```bat
keyy-cli --batch <giantsteps-audio> --labels testdata/giantsteps/annotations/key --profile all --csv gs.csv
```

---

## Aufbau

```
Key/
├── source/dsp/             Analyse-Kern, reines C++17 ohne JUCE
│   ├── DspCommon.h              Umrechnungen, Biquad
│   ├── Fft.*                    Radix-2-FFT
│   ├── Decimator.h              Tiefpass + ganzzahlige Dezimierung
│   ├── ChromaAccumulator.*      Chromagramm und Stimmton
│   ├── KeyProfiles.*            Krumhansl, Temperley, Sha'ath, edm (gelernt)
│   ├── KeyEstimator.*           Korrelation mit 24 Tonarten
│   ├── TempoEstimator.*         Einsätze, Autokorrelation, Looplänge
│   ├── KeyAnalyzer.*            alles hinter einer blockweisen Schnittstelle
│   └── Key.*                    Tonart, Namen, Parser, MIREX-Wertung
├── source/plugin/          JUCE-Anbindung: Analyse-Thread, Oberfläche, Zustand
├── tools/                  Offline-CLI, WAV-Leser, MP3 über minimp3
└── tests/                  Catch2-Tests mit synthetischen Signalen
```

## Lizenzhinweis

JUCE ist kostenlos nur unter AGPLv3 oder mit persönlicher Lizenz nutzbar.
Keyy ist ein Werkzeug für die eigenen Produktionen; eine binäre Weitergabe
ohne kommerzielle JUCE-Lizenz verpflichtet zur Offenlegung des Quellcodes.
minimp3 ist gemeinfrei (CC0).
