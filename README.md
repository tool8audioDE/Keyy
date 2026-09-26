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
| `KeyEstimator` (5 Profile, 24 Tonarten) | fertig, getestet |
| `TempoEstimator` (Einsätze, Looplänge) | fertig, getestet |
| `KeyAnalyzer` (Dezimierung, blockweise) | fertig, getestet |
| Dateinamen-Parser (Tonart, BPM) | fertig, getestet |
| Offline-CLI inkl. Batch-Auswertung | fertig |
| Unit-Tests | 19 Testfälle, grün |
| VST3 + Standalone | baut unter Windows/MSVC |
| Oberfläche | minimal, ohne Gestaltung; Laden, Anzeige, Tauschen im Standalone geprüft |
| Trefferquote GiantSteps (EDM, 301 Stücke) | **54 % exakt**, **66 % Tonleiter**, MIREX 64 — siehe unten |
| Trefferquote an eigenen Loops/Beats (65 Dateien) | **62 % exakt**, **67 % Tonleiter**, MIREX 68 — siehe unten |
| Trefferquote Hip-Hop/Trap-Pack (35 Dateien) | **69 % exakt**, **74 % Tonleiter**, MIREX 77 |
| Tempo an eigenen Loops (61 Dateien) | **80 % exakt**, 90 % mit `/2` oder `x2` |
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
Paralleltonart, gleichnamig, andere, dazu den MIREX-Wert. Das Tempo wird
mitgezählt, wenn es im Namen steht — entweder mit `bpm` (`140bpm`, `140 BPM`)
oder als Zahl hinter `Loop` oder `Tempo`, wie es Sample-Packs schreiben
(`Ghosthack Bass Loop_120_Decay Bass_E Minor.wav`). Eine bloße Zahl reicht
nicht: `149_5.wav` ist kein Tempo. Dateien, die weder Tonart noch Tempo im
Namen tragen, werden übersprungen. Die Fehlgriffe werden einzeln
aufgelistet — die gehören angehört, nicht nur gezählt: Oft ist die
Beschriftung im Sample-Pack selbst falsch oder meint die Paralleltonart.

Für den GiantSteps-Datensatz liegen die Tonarten in eigenen Dateien:
`--labels testdata/giantsteps/annotations/key`.

### 4. Ein eigenes Profil lernen

Die CSV enthält das Chromagramm jeder Datei. Daraus lässt sich ein Profil
mitteln und ehrlich prüfen — gelernt auf der einen Hälfte, getestet auf der
anderen:

```bat
python tools/learn_profile.py ergebnis.csv --profile
```

Das ist der Weg, auf dem das mitgelieferte Profil `edm` entstanden ist. Wer
an Schwelle, Exponent oder Tonalitätsgewichtung dreht, muss es damit neu
lernen: Ein Profil gehört immer zu genau der Chroma-Einstellung, unter der
es gemessen wurde.

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
Halbes und doppeltes Tempo deckt dieses Raster von selbst ab — die Taktzahlen
verdoppeln sich ja. Was es nicht abdeckt, ist die Verzählung um **4/3**:
Liegen die Einsätze auf punktierten Achteln, misst die Autokorrelation 160
statt 120. Findet sich mit der Messung selbst keine Looplänge, sind deshalb
auch die Verhältnisse 3/4 und 4/3 zugelassen. An den eigenen Loops hat das
vier von sechs Tempo-Fehlgriffen behoben, ohne einen einzigen richtigen Wert
zu verschlechtern.

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

## Messung an eigenen Loops

65 Dateien: 61 Loops aus Ghosthack-Packs (Bass und „Musical", 80/100/120 BPM)
und vier eigene Beats. 60 davon tragen eine Tonart im Namen; fünf Bass-Loops
nennen nur einen Grundton ohne Tongeschlecht und werden übersprungen.

| Profil | exakt | Quinte | Parallele | gleichnamig | andere | MIREX | Tonleiter |
|---|---|---|---|---|---|---|---|
| krumhansl | 46,7 % | 6,7 % | 8,3 % | 15,0 % | 23,3 % | 55,5 | 55,0 % |
| temperley | 40,0 % | 16,7 % | 8,3 % | 8,3 % | 26,7 % | 52,5 | 48,3 % |
| shaath | 51,7 % | 8,3 % | 5,0 % | 15,0 % | 20,0 % | 60,3 | 56,7 % |
| edm | 55,0 % | 8,3 % | 6,7 % | 6,7 % | 23,3 % | 62,5 | 61,7 % |
| **mix** | **61,7 %** | 8,3 % | 5,0 % | 3,3 % | 21,7 % | **68,0** | **66,7 %** |

**Das Ziel von 75 % ist verfehlt.** Die Annahme, Sample-Pack-Loops seien
harmonisch eindeutiger als EDM-Ausschnitte, hat sich nicht bestätigt: Keyy
liegt hier praktisch genauso wie an GiantSteps. Nach Herkunft: Bass-Loops
66,7 %, Musical Loops 53,2 %, eigene Beats 2 von 4.

**Ein eigenes Profil bringt nichts.** `learn_profile.py` auf diesen 60
Dateien, kreuzvalidiert: 55,0 % exakt und 58,3 % Tonleiter — gleich bzw.
schlechter als `edm`. Bei acht verschiedenen Beschriftungen und Klumpen von
9 bis 15 Dateien je Tonart ist daraus auch nichts zu lernen.

**Der Bass hilft auch hier nicht.** Der stärkste Ton im Band unter 250 Hz
trifft den beschrifteten Grundton in 46,7 % der Fälle, das volle Band in
61,7 %. Der Bass ist also *schlechter* als das Gesamtchromagramm — dasselbe
Ergebnis wie an GiantSteps, diesmal an eigenem Material.

**Die größte Fehlergruppe ist kein Rechenfehler.** Neun Dateien sind
„F# Minor" beschriftet, sechs davon liegen daneben. Ihr Chromagramm zeigt
durchgehend D am stärksten, dann A, dann E; F# ist schwach bis abwesend, und
*weder G noch G#* kommen vor. Damit ist die Tonleiter aus dem Signal gar
nicht entscheidbar — die vorhandenen Töne F#, A, B, D und E liegen sowohl in
F#-Moll als auch in D-Dur. Es sind Einzelspuren eines Construction Kits: Der
Name nennt die Tonart des Kits, die Datei spielt eine bVI–bIII–bVII-Figur,
ohne den Grundton je zu bringen. Ohne diesen Neunerblock: 60,8 % exakt,
68,6 % Tonleiter. Dieselbe Sorte Problem wie bei den Dateinamen aus
Auto-Tune-Einstellungen — die Beschriftung beschreibt etwas anderes als die
Datei.

### Tempo

56 Dateien tragen das Tempo im Namen (`Loop_120_`), nach der Parser-Erweiterung
sind es 61 gezählte. Auf den 56 gemeinsamen Dateien:

| | vorher | nachher |
|---|---|---|
| exakt (± 0,5 BPM) | 41 | **45** |
| halb / doppelt | 5 | 5 |
| daneben | 6 | **2** |
| kein Tempo erkannt | 4 | 4 |

Die vier gewonnenen Dateien sind genau die, bei denen die Einsatzmessung um
4/3 danebenlag (160 statt 120, 133 statt 100) und der Looplängen-Abgleich sie
deshalb verworfen hat; alle vier sind nachgemessen exakt vier Takte lang.
Keine einzige Datei hat sich dabei verschlechtert. Die zwei verbliebenen
Fehlgriffe (`Anticipate`: 95,9 statt 120, `Visualize`: 68,6 statt 120) sind
echte Aussetzer der Einsatzerkennung, keine Verzählung.

Nachmessen:

```bat
keyy-cli --batch testdata --profile all --csv eigene.csv
```

## Messung an 22 Sample-Packs — warum `mix` das Standardprofil ist

Die Sammlung des Nutzers (22 199 Dateien) enthält 2 753 Dateien mit einer
Tonart im Namen. Davon bleiben nach Abzug von Doppelungen (MP3 neben WAV,
Dry-/Wet-Paare) und von Material ohne eigene Tonart (Schlagzeug, Effekte,
Einzeltöne, Adlibs und Vocal-Chops, deren Name die Tonart des Kits nennt)
**1 263 Dateien aus 22 Packs** übrig.

Zwei Ergebnisse daraus:

**Ein global gelerntes Profil bringt nichts.** Auf allen 1 263 Dateien
gelernt und pack-weise geprüft (lernen auf 21 Packs, prüfen auf dem 22.):
37,4 % exakt gegen 38,3 % für `edm`. Das bestätigt an 1 263 Dateien, was
vorher an 60 gemessen wurde.

**Die Profilwahl hängt aber am Genre.** `edm` wurde an EDM gelernt und ist
dort stark; auf akustischem und Hip-Hop-Material verliert es:

| Pack | n | `edm` | `mix` |
|---|---|---|---|
| Loopmasters Tech House | 17 | **64,7 %** | 35,3 % |
| Loopmasters Techno | 16 | **62,5 %** | 37,5 % |
| Ghosthack Ultimate Techno Essentials | 97 | **55,7 %** | 39,2 % |
| Ghosthack Future House & Bass | 198 | **38,9 %** | 29,8 % |
| Ghosthack Ultimate Melodic Library | 109 | 37,6 % | **41,3 %** |
| Ghosthack Lo-Fi Hip Hop | 46 | 37,0 % | **41,3 %** |
| Ghosthack Ambient Soundscapes (Bass-Loops) | 55 | 56,4 % | **61,8 %** |
| Cymatics Piano and String Loops | 20 | 55,0 % | **65,0 %** |
| Loopmasters Cinematic | 21 | 52,4 % | **66,7 %** |
| Loopmasters Hip Hop & Trap | 30 | 63,3 % | **76,7 %** |

`mix` ist das Mittel aus Sha'ath und `edm`, beide vorher auf gleiche Lage
und Streuung gebracht. Entdeckt wurde es daran, dass die **Summe** der
Korrelationen beider Profile besser trifft als jedes einzelne — und weil
die Korrelation gegen Verschieben und Skalieren unempfindlich ist, tut ein
einziges gemitteltes Profil genau dasselbe, ohne den Schätzer zu ändern.

Weil das Material des Nutzers Trap und Hip-Hop ist, ist `mix` die Vorgabe.
**Für Techno oder House ist `edm` die richtige Wahl** — der Unterschied
beträgt dort 9 bis 29 Punkte in die andere Richtung. Ein Profil, das beides
kann, gibt es nach diesem Stand nicht; auf der durchmischten
Gesamtbibliothek liegen beide fast gleichauf (27,0 % gegen 25,2 %).

Was dabei auffiel und die Zahlen erklärt: **Menge ist nicht Qualität.** Von
den 2 753 beschrifteten Dateien tragen die meisten den Namen eines
Construction Kits. Packs, deren Namen die Tonart der jeweiligen Datei
meinen, erreichen 50 bis 77 %; Packs mit Kit-Beschriftung bleiben bei 9 bis
20 % — etwa „Upfront Drum and Bass" mit 105 Bass-Loops bei 12,4 %.

---

## Warum der Taktanfang nicht hilft

Naheliegend und deshalb gebaut: Loops sind auf ganze Takte geschnitten, und
bei Loops steht der Grundton meist auf der Eins — also sollten Rahmen am
Taktanfang stärker zählen. Der Schalter `--bar-weight <x>` tut genau das
(0 = aus, 1 = die Eins zählt doppelt gegenüber der Taktmitte, weicher
Übergang per Kosinus). Vorgabe ist 0, denn gemessen bringt es nichts:

| `--bar-weight` | exakt | Tonleiter |
|---|---|---|
| 0 | 35,5 % | 46,1 % |
| 1 | 35,7 % | 46,1 % |
| 2 | 35,6 % | 45,9 % |
| 4 | 35,2 % | 45,7 % |

(1 269 kuratierte Dateien, davon 916 mit erkanntem Tempo — nur die können
überhaupt gewichtet werden.)

**Der Grund liegt im Analysefenster.** Es ist 16 384 Punkte lang, bei 11 kHz
also 1,49 s:

| Tempo | Takt | ein Rahmen überdeckt |
|---|---|---|
| 80 BPM | 3,00 s | 50 % |
| 120 BPM | 2,00 s | 74 % |
| 140 BPM | 1,71 s | 87 % |
| 150 BPM | 1,60 s | 93 % |

Bei Trap-Tempo überdeckt ein einzelner Rahmen fast den ganzen Takt. „Der
Rahmen auf der Eins" ist deshalb kaum ein anderer als jeder andere: Bei
Stärke 4 verschiebt sich das Chromagramm im Mittel um 2,1 %, und nur 11,9 %
der Dateien ändern überhaupt ihre Tonart.

Das Fenster ist nicht aus Versehen so lang — bei 50 Hz liegen Halbtöne nur
3 Hz auseinander, und genau dort spielt die 808. **Taktschärfe und
Bassauflösung verlangen entgegengesetzte Fensterlängen.** Ohne zwei
Auflösungen nebeneinander ist dieser Hebel nicht zu haben.

---

## Ein neuronales Netz -- was es bringt

Gebaut, gemessen, und das Ergebnis ist zweischneidig. Das Netz ist **nicht**
Teil des Plugins; die Werkzeuge liegen in `tools/`, damit die Messung
wiederholbar bleibt.

**Aufbau.** Eingabe ist nicht Rohaudio, sondern Keyys eigenes Chromagramm je
Rahmen (`keyy-cli --frames`). Das Netz ist transpositions-aequivariant
gebaut: Faltungen ueber die Tonhoehe sind zyklisch, der Kopf liefert je
Grundton einen Wert fuer Dur und Moll. Dreht man die Eingabe um k Halbtoene,
dreht sich die Ausgabe um genau k mit. Damit ist die Datenvermehrung durch
Transposition exakt und kostenlos -- ein Chromagramm um einen Halbton zu
verschieben heisst, es zu drehen -- und das Netz kann gar nicht lernen,
einfach die haeufigste Tonart zu raten. 112 000 Parameter, Training auf der
CPU in acht Minuten.

**Daten.** Trainiert auf GiantSteps MTG Key (1 158 beschriftete Ausschnitte,
Konfidenz 2), geprueft auf GiantSteps Key (604) und auf Material des
Nutzers. Die Packs, aus denen `testdata/` stammt, sind vom Training
ausgeschlossen.

| Satz (n) | bestes Profil | Netz allein | Netz 0,4 + `mix` 0,6 |
|---|---|---|---|
| GiantSteps Key (604) | 51,8 % | 56,5 % | **56,0 %** |
| Hip-Hop/Trap-Packs (93) | 57,0 % | 50,5 % | **61,3 %** |
| Ghosthack-Loops (164, zurueckgehalten) | 47,6 % | 48,2 % | **50,6 %** |
| eigene `testdata/` (60) | 60,0 % | 55,0 % | 60,0 % |

(Profil-Werte hier mit `mix` und den Python-Merkmalen gerechnet, deshalb
minimal anders als die C++-Zahlen weiter oben.)

**Was man daraus lesen kann:**

* **Allein taugt das Netz nur fuer das, worauf es trainiert wurde.** Nur auf
  EDM trainiert, kam es auf GiantSteps auf 60,3 % -- und auf Hip-Hop auf
  26,9 %, also weit unter jedes Profil. Erst gemischtes Trainingsmaterial
  machte es ausgewogen, und zwar auf Kosten der EDM-Quote (56,5 %).
* **Zusammen mit dem Profil ist es durchgehend etwas besser**, plus 2,5 bis
  4,3 Punkte, ohne irgendwo zu verlieren. Netz und Profil irren sich
  verschieden: Das Netz trifft auf Hip-Hop die Tonleiter zu 60 %, die genaue
  Tonart aber nur zu 50 % -- es verwechselt Grundton und Tongeschlecht, wo
  das Profil richtig liegt.
* **Der Gewinn ist klein gemessen am Aufwand.** Im Plugin braeuchte es den
  Vorwaertspfad von Hand in C++ plus rund 450 kB Gewichte. Das 75-%-Ziel
  rueckt damit nicht in Reichweite.

**Zwei Fallen, die beim Bauen Zeit gekostet haben** und beim naechsten Mal
zuerst geprueft gehoeren:

1. *Die Merkmale muessen dieselben sein wie im Kern.* Der erste Lauf ergab
   38 % und sah plausibel aus. Die Gegenprobe -- reproduzieren die Profile
   auf den Python-Merkmalen ihre bekannten Quoten? -- ergab 11 statt 53 %.
   Ursache war eine fehlende Zeile: Der Kern zentriert den Stimmton
   (`if (result >= 50) result -= 100`), die Nachbildung nicht. Damit war das
   Chromagramm um einen Halbton verdreht.
2. *Trainingsmaterial und Zielmaterial sind verschieden lang.* Die
   GiantSteps-Ausschnitte haben 320 Rahmen, die Loops des Nutzers im Median
   22. Mit festen Fenstern fiel das Netz auf kurzen Dateien auf 41,7 %; mit
   gemischten Laengen (4 bis 320 Rahmen) stieg es auf 51,7 %.

Und eine Zahl, die **nicht** gilt: Auf dem kuratierten Pack-Satz erreichte
das gemischte Netz 83 % -- 996 dieser 1 259 Dateien waren im Training. Das
ist Selbstmessung, kein Ergebnis.

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

## Lizenz

Keyy ist freie Software unter der **GNU Affero General Public License v3**
(siehe `LICENSE`). Grund: Keyy nutzt JUCE 8 unter dessen AGPLv3-Lizenz und wird
als Binary öffentlich weitergegeben (https://tool8.online/keyy); der Quellcode
liegt öffentlich unter https://github.com/tool8audioDE/Keyy.

Fremdcode: JUCE (AGPLv3), minimp3 (CC0, nur CLI), Catch2 (BSL-1.0, nur Tests).

*Keyy is free software licensed under the GNU AGPLv3. Download: https://tool8.online/keyy*
