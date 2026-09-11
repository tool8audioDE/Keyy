# Projekt-Kontext: Keyy

_Zuletzt aktualisiert: 2026-09-12_

Diese Datei ist für den Einstieg in eine neue Sitzung gedacht. Sie wiederholt
**nicht**, was im `README.md` steht — dort stehen Verfahren, Bauanleitung und
Messwerte. Hier steht, was sonst verloren ginge: Zielsetzung, getroffene
Entscheidungen, verworfene Wege und der offene Stand.

**Zuerst lesen:** `README.md`, danach diese Datei.

---

## Ziel

Keyy bestimmt **Tonart, Paralleltonart, Stimmton und Tempo** einer
Audiodatei — funktional angelehnt an Antares Auto-Key 2, als VST3 für
FL Studio und als Standalone. Zuerst nur der Modus „Use File“: Datei laden,
Ergebnis anzeigen. „Listen“ (Signal der DAW mithören) und „Manual“ kommen
später.

Ausdrücklich ein **Werkzeug für die eigenen Produktionen des Nutzers**, keine
Weitergabe, kein Verkauf — wie Voxx. Daraus folgt: kein AU, kein Installer,
keine Codesignierung, keine JUCE-Lizenzfrage.

Die Infrastruktur folgt **Voxx** (`../Voxx`, Schwesterprojekt des Nutzers):
JUCE 8.0.15, C++17, CMake mit FetchContent, DSP-Kern ohne JUCE, Catch2,
Offline-CLI, CI unter Linux und Windows, Doku auf Deutsch.

---

## Entscheidungen aus dem Interview (2026-09-11)

### Produkt

| Entscheidung | Grund |
|---|---|
| Nur Eigennutzung | wie Voxx |
| Name **Keyy**, Hersteller Off-Music (`Ofmu`), Plugin-Code `Keyy` | analog zu Voxx |
| Ergebnis v1: Tonart + Paralleltonart, Stimmton, BPM | Konfidenz/Zweitkandidat bewusst **nicht** in der Oberfläche (das CLI zeigt ihn zur Diagnose) |
| Ergebnis wird **nur angezeigt** | Übergabe an PitchSnap/Voxx („Send to Auto-Tune“), Zwischenablage, Umbenennen: später |
| Englische Notation: `F Minor`, `Bb`, `F#` | wie Auto-Key, FL Studio und Dateinamen von Sample-Packs |
| Eine globale Tonart je Datei | passt zu Loops und Beats; Verlauf höchstens im CLI |
| Laden per Knopf **und** Ziehen (FL-Browser, Explorer) | alle JUCE-Formate: WAV, AIFF, FLAC, MP3, Ogg |
| BPM: Einsätze + Abgleich mit der Looplänge, Tasten /2 und x2 | Loops aus Sample-Packs sind exakt auf ganze Takte geschnitten |
| Ergebnis + Dateiname im FL-Projekt speichern | kein Neu-Analysieren beim Öffnen; Datei darf verschwunden sein |
| Minimale eigene Oberfläche | generischer Editor kann keine Datei entgegennehmen; Design zweitrangig |
| **Ziel v1: ≥ 75 % exakt** auf den Dateien des Nutzers | MIREX-Wert als Nebenzahl; Fehlgriffe anhören, nicht nur zählen |

### Nachtrag 2026-09-12: der eigentliche Einsatzzweck

Der Nutzer benennt seine Dateien nach der **Auto-Tune-Einstellung**, die er
für die Vocals gewählt hat — nicht nach der Tonart des Instrumentals. Zwei
Folgen:

1. **Diese Dateinamen sind keine verlässliche Referenz.** Sie sagen, worauf
   die Tonhöhenkorrektur stand, was auch danebenliegen kann. Für Messungen
   taugen sie nur mit Vorbehalt; GiantSteps bleibt die Hauptmessung.
2. **Die Paralleltonart-Verwechslung ist für diesen Zweck kein Fehler.**
   a-Moll und C-Dur haben in Auto-Tune dieselben sieben erlaubten Töne.
   Maßgeblich ist deshalb die Trefferquote auf die **Tonleiter** (Tonart
   oder Parallele): 65,8 % statt 54,5 % auf GiantSteps.

Die Oberfläche zeigt beide Tonarten daher gleichrangig
(„F Minor = Ab Major (gleiche Tonleiter)“).

### Material

Hauptsächlich **Samples/Loops** und **fertige Beats/Instrumentals** (Trap,
Hip-Hop). Nicht im Fokus: Acapellas, komplette Songs mit Tonartwechsel.

### Technik

| Entscheidung | Grund |
|---|---|
| Eigenes Chroma + Tonart-Profile | ohne Abhängigkeiten, testbar, erklärbar. libKeyFinder (GPL, FFTW) und neuronale Netze (ONNX) verworfen |
| DSP-Kern ohne JUCE (`source/dsp`) | wie Voxx: derselbe Code in Tests, CLI und Plugin |
| Kern-API blockweise (`KeyAnalyzer::process`) | derselbe Code taugt später für den Listen-Modus |
| Stimmton in einem Durchgang (Cent-Histogramm) | kein zweiter Durchlauf durch die Datei nötig — ebenfalls für Listen |
| Plugin ist Effekt mit Durchreichung, 0 Latenz | Listen-Modus ohne Typwechsel möglich |
| Analyse im Hintergrund-Thread | nie im Audio- oder Message-Thread |
| CLI liest WAV (eigener Leser) + MP3 (minimp3, CC0) | CLI bleibt ohne JUCE; Sample-Packs und GiantSteps sind WAV/MP3 |
| Eigene FFT (Radix 2) | reicht um Größenordnungen, hält den Kern abhängigkeitsfrei |

### Testdaten

* **Eigene Dateien mit Tonart im Namen** legt der Nutzer nach `testdata/`
  (in `.gitignore`, nicht im Repo). Auswertung: `keyy-cli --batch testdata`.
* **GiantSteps Key** (604 EDM-Ausschnitte, Beatport-Vorschauen, CC BY-SA für
  die Beschriftungen): Download ist **freigegeben**, Ziel
  `testdata/giantsteps/`. Auswertung mit `--labels`.
* Synthetische Signale in den Catch2-Tests prüfen die Mechanik, nicht die
  Praxistauglichkeit.

---

## Verworfene Ansätze — nicht erneut vorschlagen

1. **libKeyFinder** — GPLv3, zieht FFTW, bricht mit „Kern ohne
   Abhängigkeiten“.
2. **Neuronales Netz über ONNX Runtime** — schwere Abhängigkeit, schwer
   nachvollziehbar. Könnte später als Alternative hinter `estimateKey`
   gesteckt werden, falls Chroma + Profile das 75-%-Ziel verfehlen.
3. **Stimmton als Mittelwert der Abweichungen** — Obertöne ziehen ihn
   mehrere Cent nach unten (5. Oberton −14 Cent, 7. −31 Cent). Ersetzt durch
   den häufigsten Wert eines geglätteten Cent-Histogramms, noch bevor er
   gebaut wurde; bei Bedarf nachmessen.
4. **Strengere Spitzen-Schwelle gegen Drum-Rauschen** — gemessen das
   Gegenteil: Schwelle 6 → 42 %, 10 → 30 % statt 55 %. Leise Akkordtöne
   gehen verloren, bevor das Rauschen verschwindet.
5. **Oberton-Unterdrückung gegen Dur/Moll- und Quintfehler** — nicht
   gebaut, weil die Diagnose sie widerlegte: Das gemittelte Chromagramm der
   Moll-Stücke hat die kleine Terz klar über der großen. Das Problem war das
   Profil, nicht das Chromagramm (siehe README, „Messung an GiantSteps“).
6. **Frequenzbereich einschränken** (bis 800 Hz, ab 100 Hz) — ± 2 Punkte,
   also wirkungslos.
7. **Grundton aus dem Bass gewichten** (zweites Chromagramm bis 250 Hz, sein
   Grundton geht in die Entscheidung ein). Über Gewichte 0,25 bis 3
   gemessen: 54,5 % gegen 54,8 % — Rauschen.
8. **Direkte Tonleiter-Erkennung** (12 Klassen statt 24 Tonarten, eigenes
   gelerntes Profil je Tonleiter): 65,5 % gegen 65,8 %, die die vorhandene
   24-Klassen-Rechnung ohnehin liefert. Kein Gewinn.

---

## Aktueller Stand (2026-09-11, Ende der ersten Sitzung)

* DSP-Kern, CLI, Tests (18 Fälle, grün), VST3 und Standalone bauen unter
  Windows/MSVC (VS Build Tools 2022, CMake 4.4.1). JUCE und Catch2 wurden
  lokal aus `../Voxx/build/_deps` genommen (`FETCHCONTENT_SOURCE_DIR_*`).
* Standalone per Computer-Use geprüft: Laden per Dialog (MP3), Anzeige,
  Tausch der Paralleltonart funktionieren. **Nicht geprüft:** Ziehen aus dem
  FL-Browser, Laden im FL-Projekt.
* **GiantSteps, 301 Stücke: 54 % exakt, MIREX 64** mit dem gelernten Profil
  `edm` (kreuzvalidiert 54,5 %). Das 75-%-Ziel gilt aber für die Dateien des
  Nutzers — Loops aus Sample-Packs sind kurz und harmonisch eindeutiger als
  EDM-Ausschnitte; die Zahl dort steht aus.
* Die 301 MP3s lagen nur im Scratchpad der Sitzung (nicht in `testdata/`,
  weil der Ordner von Nextcloud synchronisiert wird: ~850 MB für den
  vollen Satz). Neu laden: MD5-Dateien in `testdata/giantsteps/md5`,
  Quelle `https://www.cp.jku.at/datasets/giantsteps/backup/<id>.mp3`.
* Vorgaben jetzt: Schwelle 2, Exponent 0,5, Tonalitätsgewichtung 1, Profil
  `edm`. Das Profil gehört zu genau diesen Einstellungen — wer sie ändert,
  muss es neu lernen (Mittel der Chroma-Spalten aus `--csv`, gedreht auf
  den beschrifteten Grundton).
* Git: lokal initialisiert, Branch `main`, erster Commit `78887a8`
  („Keyy 0.1.0“). **Kein GitHub-Remote** — falls gewünscht, muss das Repo
  dort noch angelegt werden.

## Offene Punkte / Nächste Schritte

- [ ] Nutzer legt **mehr** Testdateien nach `testdata/` (bisher 4, nötig
      wären 30 bis 50) → `keyy-cli --batch testdata --profile all --csv
      ergebnis.csv`. Danach entscheiden: `edm` behalten, ein Profil aus den
      eigenen Dateien lernen, oder Sha'ath. **Vorsicht bei der Wertung:**
      Die Namen stammen aus Auto-Tune-Einstellungen (siehe Nachtrag oben) —
      am aussagekräftigsten ist die Tonleiter-Quote, und Fehlgriffe gehören
      angehört, bevor sie als Fehler zählen.
- [x] **Im Plugin gefunden und behoben (2026-09-12):** JUCEs MP3-Dekoder
      bricht bei LAME-Dateien aus FL Studio kurz vor Ende ab (66,1 von
      66,3 s). Der Prozessor hat das als Fehler gewertet und gar kein
      Ergebnis geliefert, während das CLI (minimp3) dieselbe Datei
      vollständig las. Jetzt wird ausgewertet, was lesbar war; ein Hinweis
      erscheint erst ab 1 s oder 2 % Verlust. Plugin und CLI liefern
      dasselbe Ergebnis (A Minor, 442,6 Hz, 87 BPM).
- [ ] Ergebnis der ersten vier Dateien: 2 von 4 exakt, 3 von 4 auf die
      Tonleiter. `cmin_Düster_4_zig.mp3` ist durchgängig (alle sechs
      20-Sekunden-Abschnitte) E Minor, ein Eb kommt im Bass praktisch nicht
      vor — die Beschriftung war die Auto-Tune-Einstellung.
- [ ] Tempo an echten Beats prüfen (Spitzheits-Schwelle 7,5 stammt nur aus
      synthetischen Signalen; Vorliebe 110 BPM für halb/doppelt).
- [ ] Wo soll der volle GiantSteps-Satz liegen (nicht im Nextcloud-Ordner)?
- [ ] Falls die eigenen Dateien unter 75 % bleiben: nächster Hebel ist nicht
      mehr die Parametrierung (ausgereizt, Plateau bei 55 % auf EDM), sondern
      das Verfahren — z.B. Profil je Genre, Bass-Grundton gesondert
      gewichten, oder ein neuronales Netz hinter `estimateKey`.
- [ ] Plugin in FL Studio testen: Ziehen aus dem FL-Browser, Speichern im
      Projekt.
- [ ] Später: Listen-Modus, Manual-Modus, Klaviatur mit Skalentönen,
      Übergabe an PitchSnap/Voxx, eigene Oberfläche im Off-Music-Design
      (dunkel `#121212`, Akzent `#3B8ED0`).

---

## Technische Details

### Bauen (so lief es auf dem Rechner des Nutzers)

```bat
cmake -B build-dsp -DKEYY_BUILD_PLUGIN=OFF -DFETCHCONTENT_SOURCE_DIR_CATCH2=../Voxx/build/_deps/catch2-src
cmake --build build-dsp --config Release
build-dsp\Release\keyy-tests.exe

cmake -B build -DKEYY_BUILD_TESTS=OFF -DKEYY_BUILD_CLI=OFF -DKEYY_COPY_PLUGIN=OFF -DFETCHCONTENT_SOURCE_DIR_JUCE=../Voxx/build/_deps/juce-src
cmake --build build --config Release
```

Die beiden `FETCHCONTENT_SOURCE_DIR_*`-Angaben sparen den JUCE- und
Catch2-Download, weil Voxx sie schon heruntergeladen hat. `KEYY_COPY_PLUGIN`
muss aus bleiben, solange nicht mit Adminrechten gebaut wird.

### Messungen wiederholen

```bat
keyy-cli --batch <ordner> [--labels <ordner>] --profile all --csv ergebnis.csv
python tools/learn_profile.py ergebnis.csv --profile
```

Zusätzliche Schalter für Versuche: `--threshold`, `--tonality`,
`--exponent`, `--min-hz`, `--max-hz`, `--threads`.

**Wichtig:** Das Profil `edm` gehört zu genau einer Chroma-Einstellung
(Schwelle 2, Exponent 0,5, Tonalitätsgewichtung 1). Wer daran dreht, muss es
mit `tools/learn_profile.py` neu lernen, sonst misst man ein Profil gegen
ein Chromagramm, zu dem es nicht passt.

### Was mit der Sitzung verloren ging

Die 301 GiantSteps-MP3s und alle Zwischen-CSVs lagen im Scratchpad und sind
weg. Wiederherstellbar: MD5-Listen in `testdata/giantsteps/md5`, Quelle
`https://www.cp.jku.at/datasets/giantsteps/backup/<id>.mp3`, Rückfall
`https://geo-samples.beatport.com/lofi/<id>.mp3`. Die Beschriftungen liegen
im Repo-Ordner `testdata/giantsteps/annotations/key` (geklont, nicht
eingecheckt — `testdata/` ist in `.gitignore`, weil der Projektordner von
Nextcloud synchronisiert wird).

## Referenzen

* Schwesterprojekt **Voxx** (`../Voxx`): Vorlage für CMake, CI, Doku-Stil.
  `WavIO.h` stammt von dort (und ursprünglich aus PitchSnap), als Kopie —
  hier um WAVE_FORMAT_EXTENSIBLE erweitert.
* Tonart-Profile: Krumhansl & Kessler (1982), Temperley (2007, Kostka-Payne),
  Sha'ath (2011, KeyFinder; Werte wie in Essentia).
* MIREX-Wertung: exakt 1, Quinte 0,5, Paralleltonart 0,3, gleichnamig 0,2.
