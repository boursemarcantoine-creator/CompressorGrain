# CompressorGrain (plugin VST3)

Un compresseur avec 2 boutons :

- **Compression** : ratio réglable de 2:1 à 8:1 (threshold fixe à -18 dB, attack 10ms, release 100ms — modifiables dans `PluginProcessor.cpp` si besoin).
- **Grain** (0–100%) : en montant, ajoute progressivement
  - une bosse dans les basses à **72 Hz**
  - une bosse dans les médiums à **621 Hz**
  - un mix parallèle d'un signal filtré en passe-haut à **6 kHz**, pour donner du "grain"/de la présence dans l'aigu.

## Installer les outils (une seule fois)

Tu pars de zéro, voici ce qu'il faut :

1. **CMake** — https://cmake.org/download/ (coche "Add to PATH" à l'installation)
2. Un compilateur C++ :
   - **Windows** : Visual Studio 2022 (Community, gratuit) avec le workload "Desktop development with C++"
   - **macOS** : Xcode (depuis l'App Store), puis lancer une fois `xcode-select --install` dans le Terminal
3. Rien d'autre à installer : **JUCE est téléchargé automatiquement** par CMake au premier build (voir `CMakeLists.txt`).

## Compiler le plugin

Ouvre un terminal dans le dossier `CompressorGrain/` et lance :

```bash
cmake -B build
cmake --build build --config Release
```

Le premier `cmake -B build` va télécharger JUCE (ça prend quelques minutes, connexion internet nécessaire). Les fois suivantes seront rapides.

## Où trouver le plugin une fois compilé

- **VST3** : `build/CompressorGrain_artefacts/Release/VST3/CompressorGrain.vst3`
  - Windows : copie-le dans `C:\Program Files\Common Files\VST3\`
  - macOS : copie-le dans `/Library/Audio/Plug-Ins/VST3/`
- Une version **Standalone** (exécutable seul, sans DAW) est aussi générée dans `build/CompressorGrain_artefacts/Release/Standalone/` — pratique pour tester rapidement au casque avant d'ouvrir ton DAW.

Après la copie dans le dossier VST3, relance ton DAW (Ableton, FL Studio, Reaper...) et fais un rescan des plugins si besoin.

## Modifier le comportement

Tout le DSP est dans `Source/PluginProcessor.cpp` :
- `compressor.setThreshold(...)`, `setAttack(...)`, `setRelease(...)` : réglages fixes du compresseur.
- `updateGrainFilters()` : contrôle l'intensité des bosses (72 Hz / 621 Hz) et du mix haute fréquence (6 kHz) en fonction du slider Grain.
