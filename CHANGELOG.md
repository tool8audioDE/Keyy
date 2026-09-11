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
* Dateinamen-Parser für Tonart und Tempo („Loop_Fm_140bpm.wav“).
* Offline-Werkzeug `keyy-cli` mit Batch-Auswertung (Trefferquote je Profil,
  MIREX-Wert, Tempo exakt und mit /2 x2, Chromagramm je Datei in der CSV).
* JUCE-Plugin (VST3 und Standalone): Datei laden per Knopf oder Ziehen,
  Analyse im Hintergrund, Ergebnis im Projekt gespeichert. Tonart und
  Paralleltonart stehen gleichrangig nebeneinander — für die
  Tonhöhenkorrektur sind beide dieselbe Tonleiter.
* Das Plugin wertet auch Dateien aus, deren Dekoder vorzeitig aussteigt
  (MP3 aus FL Studio), und weist einen nennenswerten Verlust aus.
* Catch2-Tests, GitHub-Actions-Workflow.
