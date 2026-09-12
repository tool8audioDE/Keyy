# Changelog

## 0.1.0 — in Arbeit

Erste Fassung, nur der Modus „Use File“.

**Neu**

* `ChromaAccumulator` — Chromagramm aus Spektralspitzen, Stimmton aus einem
  Cent-Histogramm, beides in einem Durchgang.
* `KeyEstimator` — Korrelation mit 24 Tonarten, vier Profile (Krumhansl,
  Temperley, Sha'ath und `edm`, aus dem eigenen Chromagramm an GiantSteps
  gelernt — dort 54 % exakt statt 46 % mit Sha'ath).
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
* Catch2-Tests, GitHub-Actions-Workflow.
