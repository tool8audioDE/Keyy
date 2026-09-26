# Changelog

## 1.0.1 — 2026-09-26

* Visual-C++-Laufzeit statisch eingebunden. Plugin und App laufen jetzt
  auch auf Rechnern ohne installiertes Visual C++ Redistributable
  (vorher fehlten dort MSVCP140.dll / VCRUNTIME140.dll).

## 1.0.0 — 2026-09-26

Erstes öffentliches Release, kostenlos unter AGPLv3 (https://tool8.online/keyy).

## 0.1.0

Erste Fassung, nur der Modus „Use File“.

**Neu**

* `ChromaAccumulator` — Chromagramm aus Spektralspitzen, Stimmton aus einem
  Cent-Histogramm, beides in einem Durchgang.
* `KeyEstimator` — Korrelation mit 24 Tonarten, fünf Profile (Krumhansl,
  Temperley, Sha'ath, `edm`, aus dem eigenen Chromagramm an GiantSteps
  gelernt — dort 54 % exakt statt 46 % mit Sha'ath — und `mix`).
* Profil `mix`, Vorgabe: das Mittel aus Sha'ath und `edm`. An 1263
  beschrifteten Loops aus 22 Sample-Packs gemessen. Auf Hip-Hop, Trap,
  Cinematic und akustischem Material 4 bis 14 Punkte besser als `edm`
  (eigene Dateien 61,7 statt 55,0 % exakt, Loopmasters Hip Hop & Trap 68,6
  statt 57,1 %), auf Techno und House ebenso deutlich schlechter. Wer EDM
  analysiert, nimmt weiter `--profile edm`.
* Tonalitätsgewichtung der Rahmen und Spitzen-Schwelle 2 statt 3, beides an
  GiantSteps gemessen.
* Tempo: keine Angabe, wenn die Einsätze zu weich sind (Schwebung
  gehaltener Töne ist periodisch, aber kein Tempo).
* `TempoEstimator` — Spectral Flux, Autokorrelation als Kamm, Abgleich mit
  der Looplänge.
* `KeyAnalyzer` — alles hinter einer blockweisen Schnittstelle, mit
  Dezimierung auf etwa 11 kHz.
* Dateinamen-Parser für Tonart und Tempo („Loop_Fm_140bpm.wav“). Das Tempo
  wird auch ohne „bpm“ erkannt, wenn es hinter „Loop“ oder „Tempo“ steht —
  so schreiben es Sample-Packs („Ghosthack Bass Loop_120_…“). Eine bloße
  Zahl gilt weiterhin nicht als Tempo.
* Der Looplängen-Abgleich lässt auch die Verzählung um 3/4 und 4/3 zu, wenn
  sich mit der Einsatzmessung selbst keine Looplänge findet: Punktierte
  Achtel führen sonst zu 160 statt 120 BPM, obwohl die Datei nachweislich
  exakt vier Takte lang ist. An 56 eigenen Loops 45 statt 41 exakt, ohne
  einen richtigen Wert zu verschlechtern.
* Offline-Werkzeug `keyy-cli` mit Batch-Auswertung (Trefferquote je Profil,
  MIREX-Wert, Tempo exakt und mit /2 x2, Chromagramm je Datei in der CSV).
* JUCE-Plugin (VST3 und Standalone): Datei laden per Knopf oder Ziehen,
  Analyse im Hintergrund, Ergebnis im Projekt gespeichert. Tonart und
  Paralleltonart stehen gleichrangig nebeneinander — für die
  Tonhöhenkorrektur sind beide dieselbe Tonleiter.
* Das Plugin wertet auch Dateien aus, deren Dekoder vorzeitig aussteigt
  (MP3 aus FL Studio), und weist einen nennenswerten Verlust aus.
* Schalter `--bar-weight` (Vorgabe 0 = aus): gewichtet Rahmen am Taktanfang
  stärker. Gebaut, an 1 269 Dateien gemessen und für wirkungslos befunden —
  das Analysefenster ist mit 1,49 s länger als ein Takt bei Trap-Tempo. Der
  Schalter bleibt, damit die Messung wiederholbar ist, falls die Analyse
  einmal eine zweite, kürzere Auflösung bekommt. Siehe README.
* Bei bestätigter Looplänge liegt der erste Taktanfang jetzt auf 0 statt aus
  der Einsatz-Hüllkurve gemessen zu werden; die Messung fand an Loops oft
  die Snare auf der Zwei.
* Werkzeuge fuer einen neuronalen Klassifikator in `tools/` (`frames_io.py`,
  `train_net.py`). Gemessen, aber **nicht** Teil des Plugins: allein
  schlechter als die Profile auf Hip-Hop, im Verbund mit `mix` 2,5 bis 4,3
  Punkte besser. Siehe README.
* Oberfläche durchgängig auf Englisch: Knöpfe, Ergebnisanzeige, Hinweise und
  Fehlermeldungen. Kommentare und Doku bleiben deutsch.
* Hersteller **tooL8** (Code `TooL`) statt Off-Music (`Ofmu`). Das ändert die
  eindeutige VST3-Kennung — FL Studio muss neu suchen, und Projekte mit dem
  alten Keyy finden es nicht wieder. Siehe CONTEXT, „FL Studio: Plugin-Kennung
  und Datenbank".
* Catch2-Tests, GitHub-Actions-Workflow.
