cmake_minimum_required(VERSION 3.22)

# C wird mitaktiviert, weil JUCE C-Quellen mitbringt (u.a. die Bibliotheken
# fuer Schriftdarstellung und Bildformate). Ohne das bricht die Konfiguration
# des Plugin-Ziels ab.
project(Keyy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Die Quelltexte sind UTF-8 - Umlaute stehen in Kommentaren und in den
# Texten der Oberflaeche. MSVC liest sie sonst in der Codepage des Systems.
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Wie bei Voxx: Das Plugin braucht JUCE (grosser Download beim ersten
# Konfigurieren), DSP-Kern, Tests und Offline-Werkzeug kommen ohne aus.
option(KEYY_BUILD_PLUGIN "VST3- und Standalone-Ziele bauen (benoetigt JUCE)" ON)
option(KEYY_BUILD_TESTS  "Unit-Tests bauen (benoetigt Catch2)"               ON)
option(KEYY_BUILD_CLI    "Offline-Kommandozeilenwerkzeug bauen"              ON)

# Nach dem Bauen wird das VST3 in den System-Ordner kopiert. Das braucht
# unter Windows Adminrechte; ohne sie scheitert der Bau an dieser Stelle,
# obwohl das Plugin laengst fertig ist.
option(KEYY_COPY_PLUGIN  "Gebautes Plugin in den System-Ordner kopieren"     ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# DSP-Kern: reines C++17, keine Abhaengigkeiten.
# Genau deshalb ist er in Tests, im CLI und im Plugin derselbe Code.
# ---------------------------------------------------------------------------
add_library(keyy_dsp STATIC
    source/dsp/ChromaAccumulator.cpp
    source/dsp/Fft.cpp
    source/dsp/Key.cpp
    source/dsp/KeyAnalyzer.cpp
    source/dsp/KeyEstimator.cpp
    source/dsp/KeyProfiles.cpp
    source/dsp/TempoEstimator.cpp)

target_include_directories(keyy_dsp PUBLIC source)

# Der DSP-Kern landet auch in der VST3-Shared-Library.
set_target_properties(keyy_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(keyy_dsp PRIVATE /W4)
else()
    target_compile_options(keyy_dsp PRIVATE -Wall -Wextra)
endif()

# ---------------------------------------------------------------------------
# Offline-Werkzeug
# ---------------------------------------------------------------------------
if(KEYY_BUILD_CLI)
    # minimp3: MP3-Decoder in einer einzelnen Header-Datei, gemeinfrei (CC0).
    # Sample-Packs und der GiantSteps-Datensatz liegen oft als MP3 vor, und
    # das CLI soll ohne JUCE auskommen. Das Repository hat keine
    # Versionsmarken, deshalb steht der Commit fest drin. SOURCE_SUBDIR zeigt
    # absichtlich ins Leere: Heruntergeladen wird nur der Header, gebaut wird
    # davon nichts.
    FetchContent_Declare(minimp3
        GIT_REPOSITORY https://github.com/lieff/minimp3.git
        GIT_TAG        ea99364f61c14656440e8d77e9c233ccf3124633
        SOURCE_SUBDIR  nicht-bauen)
    FetchContent_MakeAvailable(minimp3)

    find_package(Threads REQUIRED)

    add_executable(keyy-cli tools/KeyyCli.cpp)
    target_link_libraries(keyy-cli PRIVATE keyy_dsp Threads::Threads)
    target_include_directories(keyy-cli PRIVATE tools)
    target_include_directories(keyy-cli SYSTEM PRIVATE ${minimp3_SOURCE_DIR})

    # minimp3_ex.h bindet unter Windows windows.h ein. Dessen Makros min
    # und max zerschiessen sonst jedes std::min im CLI.
    if(WIN32)
        target_compile_definitions(keyy-cli PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()

    if(MSVC)
        target_compile_definitions(keyy-cli PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(KEYY_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.2
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)

    add_executable(keyy-tests
        tests/AnalyzerTests.cpp
        tests/ChromaTests.cpp
        tests/KeyTests.cpp
        tests/TempoTests.cpp)

    target_link_libraries(keyy-tests PRIVATE keyy_dsp Catch2::Catch2WithMain)
    target_include_directories(keyy-tests PRIVATE tests)

    enable_testing()
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(keyy-tests)
endif()

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
if(KEYY_BUILD_PLUGIN)
    # Feste Version, dieselbe wie in Voxx.
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)

    # AU fehlt bewusst, wie bei Voxx: nur macOS, nie gebaut, nie geprueft.
    juce_add_plugin(Keyy
        COMPANY_NAME              "tooL8"
        PLUGIN_MANUFACTURER_CODE  TooL
        PLUGIN_CODE               Keyy
        FORMATS                   VST3 Standalone
        PRODUCT_NAME              "Keyy"
        IS_SYNTH                  FALSE
        NEEDS_MIDI_INPUT          FALSE
        NEEDS_MIDI_OUTPUT         FALSE
        IS_MIDI_EFFECT            FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD   ${KEYY_COPY_PLUGIN})

    target_sources(Keyy PRIVATE
        source/plugin/PluginEditor.cpp
        source/plugin/PluginProcessor.cpp)

    # MP3 ist in JUCE abgeschaltet, solange man es nicht ausdruecklich
    # anfordert. WAV, AIFF, FLAC und Ogg sind ohnehin dabei.
    target_compile_definitions(Keyy PUBLIC
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(Keyy PRIVATE
        keyy_dsp
        juce::juce_audio_utils
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endif()
