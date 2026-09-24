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
| Name **Keyy**, Hersteller tooL8 (`TooL`), Plugin-Code `Keyy` | Hersteller 2026-09-24 von Off-Music umbenannt |
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

* **Eigene Dateien mit Tonart im Namen** liegen in `testdata/`
  (in `.gitignore`, nicht im Repo). Stand 2026-09-12: 65 Dateien — 61 Loops
  aus Ghosthack-Packs (Bass und „Musical“, 80/100/120 BPM) und vier eigene
  Beats. Auswertung: `keyy-cli --batch testdata`. **Die Ghosthack-Namen sind
  brauchbare Beschriftungen**, anders als die eigenen Beats (deren Namen
  nennen die Auto-Tune-Einstellung) — mit einer Ausnahme: Bei Einzelspuren
  aus Construction Kits nennt der Name die Tonart des Kits, nicht die der
  Datei (siehe README, der F#-Moll-Block).
* **GiantSteps Key** (604 EDM-Ausschnitte, Beatport-Vorschauen, CC BY-SA für
  die Beschriftungen): Download ist **freigegeben**, Ziel
  `testdata/giantsteps/`. Auswertung mit `--labels`.
* **Die Sample-Bibliothek des Nutzers** unter `FL Sample Packs/fl sample
  packs` auf der 5TB-Platte (22 199 Dateien, davon 2 753 mit Tonart im
  Namen) ist als Messgrundlage erschlossen. Der ganze Ordner läuft in 23
  Sekunden durch (`keyy-cli --batch`). **Achtung, zwei Fallen:**
  `testdata/` ist eine Teilmenge dieser Bibliothek (56 Dateien) — ohne
  Dedup trainiert man auf den eigenen Testdaten. Und die Menge trügt: Nur
  Packs, deren Dateinamen die Tonart der jeweiligen Datei meinen, sind
  brauchbar (50 bis 77 %); Packs mit Construction-Kit-Beschriftung liegen
  bei 9 bis 20 %. Nach Abzug von Doppelungen (MP3 neben WAV, Dry-/Wet-
  Paare) und untauglichem Material (Schlagzeug, Effekte, Einzeltöne,
  Adlibs, Vocal-Chops) bleiben 1 263 Dateien aus 22 Packs.
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
   gemessen: 54,5 % gegen 54,8 % — Rauschen. **2026-09-12 an den eigenen
   Loops nachgemessen und bestätigt:** Der stärkste Ton unter 250 Hz trifft
   den beschrifteten Grundton in 46,7 % der Fälle, das volle Band in 61,7 %.
   Der Bass ist schlechter, nicht besser. Endgültig erledigt.
8. **Direkte Tonleiter-Erkennung** (12 Klassen statt 24 Tonarten, eigenes
   gelerntes Profil je Tonleiter): 65,5 % gegen 65,8 %, die die vorhandene
   24-Klassen-Rechnung ohnehin liefert. Kein Gewinn.
9. **Profil aus den eigenen Dateien lernen** (2026-09-12, 60 beschriftete
   Loops, kreuzvalidiert): 55,0 % exakt und 58,3 % Tonleiter gegen 55,0 %
   und 61,7 % mit `edm`. Kein Gewinn — bei acht Beschriftungen und Klumpen
   von 9 bis 15 Dateien je Tonart ist auch keiner zu erwarten.
10. **Profil aus der ganzen Sample-Bibliothek lernen** (2026-09-20, 1263
   beschriftete Loops aus 22 Packs, pack-weise kreuzvalidiert — lernen auf
   21 Packs, prüfen auf dem 22.): 37,4 % gegen 38,3 % für `edm`. Damit ist
   Ablehnung 9 an zwanzigfacher Datenmenge bestätigt: **ein global
   gelerntes Profil bringt nichts.** Wichtig ist dabei der Unterschied
   zwischen den Prüfarten — dieselben Daten liefern 63,2 % bei
   Leave-one-out, aber 56,8 % pack-weise. Leave-one-out leckt, weil
   Dateien desselben Kits dieselbe Tonart und fast dasselbe Material
   haben. **Nur die pack-weise Zahl zählt**, sie beantwortet die richtige
   Frage: Was passiert bei einem neuen Pack?

---

11. **Den Taktanfang stärker gewichten** (2026-09-20, gebaut und gemessen,
   Schalter `--bar-weight` bleibt drin, Vorgabe 0 = aus). Der Gedanke stand
   als nächster Hebel in den offenen Punkten: Loops sind auf ganze Takte
   geschnitten, der Grundton steht meist auf der Eins. **Ergebnis: wirkungslos.**
   An 1 269 kuratierten Dateien (916 davon mit erkanntem Tempo): 35,5 % ohne,
   35,7 % bei Stärke 1, 35,6 % bei 2, 35,2 % bei 4. Auf den eigenen Dateien
   61,7 % ohne gegen 60,0 bis 61,7 % mit.

   **Der Grund ist strukturell und nicht durch Nachstellen zu beheben:** Das
   Analysefenster ist 16 384 Punkte lang, bei 11 kHz also 1,49 s. Ein Takt
   dauert bei 120 BPM 2,0 s, bei 140 BPM 1,71 s, bei 150 BPM 1,60 s — **ein
   einzelner Rahmen überdeckt 74 bis 93 % eines Takts.** „Der Rahmen auf der
   Eins" unterscheidet sich damit kaum von jedem anderen. Gemessen: Bei
   Stärke 4 (die Eins zählt fünffach gegenüber der Taktmitte) verschiebt
   sich das Chromagramm im Mittel um 2,1 %, und nur 11,9 % der Dateien
   ändern überhaupt ihre Tonart.

   Das Fenster ist so lang, *weil* bei 50 Hz Halbtöne nur 3 Hz auseinander
   liegen und genau dort die 808 spielt. Taktschärfe und Bassauflösung
   verlangen entgegengesetzte Fensterlängen. Wer den Hebel wiederbeleben
   will, muss zuerst dieses Problem lösen — etwa mit zwei Auflösungen
   nebeneinander.

   Zwei Nebenfunde, die dabei anfielen: **28 % der kuratierten Dateien
   haben gar kein erkanntes Tempo**, können also nie gewichtet werden. Und
   die Taktphase aus der Einsatz-Hüllkurve zu messen ist an Loops
   schlechter als sie auf 0 zu setzen (58,3 gegen 61,7 %) — an einem
   nachweislich vier Takte langen 120-BPM-Loop fand die Messung den ersten
   Taktanfang bei 0,679 s. Sie sucht sich die lauteste Stelle, und das ist
   oft die Snare auf der Zwei. Deshalb gilt bei bestätigter Looplänge
   jetzt Phase 0.

12. **Neuronales Netz hinter `estimateKey`** (2026-09-20, gebaut und
   gemessen). Nicht rundweg verworfen, aber **allein schlechter als die
   Profile auf dem Material des Nutzers** und im Verbund nur wenig besser.
   Zahlen und Aufbau stehen im README („Ein neuronales Netz -- was es
   bringt"), die Werkzeuge in `tools/train_net.py` und
   `tools/frames_io.py`.

   Kurzfassung: Nur auf EDM trainiert, schlaegt es auf GiantSteps jedes
   Profil (60,3 gegen 53,1 %) und faellt auf Hip-Hop auf 26,9 %.
   Gemischtes Trainingsmaterial macht es ausgewogen (56,5 / 50,5 %).
   Zusammen mit `mix` gewichtet (0,4 Netz, 0,6 Profil) ist es ueberall
   2,5 bis 4,3 Punkte besser, ohne irgendwo zu verlieren -- auf den
   eigenen Dateien allerdings plus/minus null.

   **Offen ist damit nur noch die Kostenfrage**, nicht die Machbarkeit:
   Im Plugin braeuchte es den Vorwaertspfad von Hand in C++ (vier
   Faltungsbloecke, zyklisch ueber die Tonhoehe) plus rund 450 kB
   Gewichte, fuer ein paar Punkte. Das 75-%-Ziel ruecken sie nicht in
   Reichweite.

   **Der Mischungsanteil 0,4 ist an denselben Pruefsaetzen abgelesen**, an
   denen er berichtet wird. Alles zwischen 0,25 und 0,5 wirkt aehnlich, es
   haengt also nicht an einer Messerschneide -- der wahre Gewinn liegt
   aber eher am unteren Rand.

---

## Aktueller Stand (2026-09-12)

* DSP-Kern, CLI, Tests (19 Fälle, grün), VST3 und Standalone bauen unter
  Windows/MSVC (VS Build Tools 2022, CMake 4.4.1). JUCE und Catch2 wurden
  lokal aus `../Voxx/build/_deps` genommen (`FETCHCONTENT_SOURCE_DIR_*`).
* Standalone per Computer-Use geprüft: Laden per Dialog (MP3), Anzeige,
  Tausch der Paralleltonart funktionieren. **Nicht geprüft:** Ziehen aus dem
  FL-Browser, Laden im FL-Projekt.
* **GiantSteps liegt jetzt vollständig vor** (604 Stücke, MD5-geprüft) unter
  `D:/Datasets/giantsteps/audio`, dazu GiantSteps MTG Key (1 158 Stücke
  mit Konfidenz 2) unter `D:/Datasets/giantsteps-mtg/audio`. Damit ist
  der offene Punkt „wo soll der Satz liegen" erledigt — **nicht** im
  Nextcloud-Ordner. Auf allen 604: `edm` 53,1 % exakt, 62,6 % Tonleiter,
  MIREX 63,0. Ein Profil neu auf allen 604 zu lernen bringt nichts
  (kreuzvalidiert 52,2 %); die 53,1 % des bestehenden `edm` sind zum Teil
  Selbstmessung, weil es auf der Hälfte dieser Stücke gelernt wurde.
* **Eigene Dateien, 60 beschriftete Loops: 61,7 % exakt, 66,7 % Tonleiter,
  MIREX 68,0** — mit dem neuen Vorgabeprofil `mix` (2026-09-20). Vorher
  55,0/61,7/62,5 mit `edm`. **Das 75-%-Ziel ist weiter verfehlt**, aber der
  Abstand ist von 20 auf 13 Punkte geschrumpft.
* **Die Profilwahl hängt am Genre — das ist der Kern der Sitzung vom
  2026-09-20.** `edm` wurde an EDM gelernt und ist dort stark (Tech House
  64,7 %, Techno 62,5 %); auf Hip-Hop, Trap, Cinematic und akustischem
  Material verliert es gegen `mix` (Loopmasters Hip Hop & Trap 76,7 gegen
  63,3 %). Weil das Material des Nutzers Trap und Hip-Hop ist, ist `mix`
  jetzt die Vorgabe. **Für EDM bleibt `--profile edm` richtig.**
* **Tempo, 56 Dateien mit Tempo im Namen: 45 exakt** (vorher 41, siehe
  Looplängen-Korrektur unten).
* Die 301 MP3s lagen nur im Scratchpad der Sitzung (nicht in `testdata/`,
  weil der Ordner von Nextcloud synchronisiert wird: ~850 MB für den
  vollen Satz). Neu laden: MD5-Dateien in `testdata/giantsteps/md5`,
  Quelle `https://www.cp.jku.at/datasets/giantsteps/backup/<id>.mp3`.
* Vorgaben jetzt: Schwelle 2, Exponent 0,5, Tonalitätsgewichtung 1, Profil
  `mix`. `edm` gehört zu genau diesen Einstellungen — wer sie ändert, muss
  es neu lernen (Mittel der Chroma-Spalten aus `--csv`, gedreht auf den
  beschrifteten Grundton), und `mix` als dessen Mittel mit Sha'ath gleich
  mit.
* Git: lokal, Branch `main`, zuletzt `9bb8a0e` (Looplängen-Korrektur und
  Tempo-Parser, 2026-09-12). **Kein GitHub-Remote** — falls gewünscht, muss
  das Repo dort noch angelegt werden.

## Offene Punkte / Nächste Schritte

- [x] **Erledigt 2026-09-12: 65 Testdateien ausgewertet** (61 Ghosthack-Loops,
      4 eigene Beats; 60 mit Tonart im Namen). Ergebnis: `edm` bleibt das
      beste Profil mit 55,0 % exakt und 61,7 % Tonleiter — **das 75-%-Ziel
      ist verfehlt**, und zwar praktisch auf GiantSteps-Niveau. Die Annahme,
      Loops aus Sample-Packs seien eindeutiger, war falsch. Ein eigenes
      Profil bringt nichts (Ablehnung 9), der Bass auch nicht (Ablehnung 7).
      Zahlen und Fehleranalyse stehen jetzt im README, Abschnitt
      „Messung an eigenen Loops“.
- [x] **Im Plugin gefunden und behoben (2026-09-12):** JUCEs MP3-Dekoder
      bricht bei LAME-Dateien aus FL Studio kurz vor Ende ab (66,1 von
      66,3 s). Der Prozessor hat das als Fehler gewertet und gar kein
      Ergebnis geliefert, während das CLI (minimp3) dieselbe Datei
      vollständig las. Jetzt wird ausgewertet, was lesbar war; ein Hinweis
      erscheint erst ab 1 s oder 2 % Verlust. Plugin und CLI liefern
      dasselbe Ergebnis (A Minor, 442,6 Hz, 87 BPM).
- [ ] Die vier eigenen Beats: 2 von 4 exakt, 3 von 4 auf die
      Tonleiter. `cmin_Düster_4_zig.mp3` ist durchgängig (alle sechs
      20-Sekunden-Abschnitte) E Minor, ein Eb kommt im Bass praktisch nicht
      vor — die Beschriftung war die Auto-Tune-Einstellung.
- [x] **Erledigt 2026-09-12: Tempo an echten Loops geprüft und korrigiert**
      (45 statt 41 von 56 exakt). Verfahren und Zahlen stehen im README,
      Abschnitt „Messung an eigenen Loops → Tempo“.
      Was dabei **offen bleibt**: zwei echte Aussetzer der Einsatzerkennung
      (`Anticipate` 95,9 statt 120, `Visualize` 68,6 statt 120) und die
      Spitzheits-Schwelle 7,5, die weiterhin nur aus synthetischen Signalen
      stammt.
- [ ] **Bekannte Schwachstelle der 3/4-Regel:** Hat die 110-BPM-Vorliebe die
      Einsatzmessung selbst schon halbiert (gemessen 80 statt 160), landet
      der 3/4-Weg eine Oktave zu tief — 60 statt 120. An allen 65 eigenen
      Dateien tritt das nicht auf; es brauchte ein völlig gleichförmiges
      Kunstsignal, um es zu erzeugen. Falls es je an echtem Material
      auffällt: Die Kandidaten der Looplänge müssten dann nach der Nähe zur
      110-BPM-Mitte ausgewählt werden statt nach der Nähe zur Messung.
- [x] **Erledigt 2026-09-20: GiantSteps und GiantSteps MTG liegen auf D:**
      (`D:/Datasets/giantsteps/audio`, 604 Stücke, 834 MB;
      `D:/Datasets/giantsteps-mtg/audio`, 1 158 Stücke, 1,6 GB). Beide
      per MD5 geprüft. Die Beschriftungen der MTG-Sammlung haben eine
      Konfidenzspalte — nur Konfidenz 2 mit eindeutiger Tonart ist
      brauchbar, das sind 1 159 von 1 486.
- [ ] **Die eigenen Dateien bleiben unter 75 % (jetzt 61,7 % mit `mix`).**
      Profile sind damit weitgehend ausgereizt: vier gemessen, zwei
      gelernt (GiantSteps und 1263 Loops), eines gemischt. Nur die
      Mischung brachte etwas (+6,7 Punkte). Der Taktanfang-Hebel ist
      gebaut, gemessen und widerlegt (Ablehnung 11) — das Analysefenster
      ist länger als ein Takt. **Damit sind die naheliegenden Wege alle
      gegangen.** Was bleibt: zwei Fensterlängen nebeneinander (kurz für
      den Takt, lang für den Bass), oder ein neuronales Netz hinter
      `estimateKey`. Noch
      nicht geprüft und zu diesem Material passend: **den Taktanfang stärker
      gewichten.** Die Dateien sind erwiesenermaßen exakt auf ganze Takte
      geschnitten (55 von 61 über die Looplänge bestätigt), und bei Loops
      steht der Grundton meist auf der Eins. Danach bliebe nur noch ein
      neuronales Netz hinter `estimateKey`.
      **Aber zuerst ehrlich rechnen:** Neun der 60 Dateien sind Einzelspuren
      eines Construction Kits, deren Tonleiter im Signal gar nicht steht
      (siehe README). Ein Verfahren kann diese neun nicht gewinnen — ohne
      sie liegt Keyy bei 60,8 % exakt und 68,6 % Tonleiter.
- [ ] Plugin in FL Studio testen: Ziehen aus dem FL-Browser, Speichern im
      Projekt.
- [ ] Später: Listen-Modus, Manual-Modus, Klaviatur mit Skalentönen,
      Übergabe an PitchSnap/Voxx, eigene Oberfläche im tooL8-Design
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

Die CSVs der Sitzung vom 2026-09-12 lagen ebenfalls nur im Scratchpad. Sie
sind in Sekunden neu erzeugt, die Audiodateien liegen ja in `testdata/`:

```bat
keyy-cli --batch testdata --profile all --csv eigene.csv
python tools/learn_profile.py eigene.csv --profile
```

Achtung bei der CSV: Bei `--profile all` stehen in den Spalten `erkannt` und
`verhaeltnis` die Werte des **ersten** Profils (krumhansl), nicht die von
`edm`. Wer die Fehlgriffe von `edm` auswerten will, ruft `--profile edm`
auf. Die Chroma-Spalten sind vom Profil unabhängig.

## Referenzen

* Schwesterprojekt **Voxx** (`../Voxx`): Vorlage für CMake, CI, Doku-Stil.
  `WavIO.h` stammt von dort (und ursprünglich aus PitchSnap), als Kopie —
  hier um WAVE_FORMAT_EXTENSIBLE erweitert.
* Tonart-Profile: Krumhansl & Kessler (1982), Temperley (2007, Kostka-Payne),
  Sha'ath (2011, KeyFinder; Werte wie in Essentia).
* MIREX-Wertung: exakt 1, Quinte 0,5, Paralleltonart 0,3, gleichnamig 0,2.
