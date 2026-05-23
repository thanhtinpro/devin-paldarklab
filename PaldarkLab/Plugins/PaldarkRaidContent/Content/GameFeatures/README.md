# PaldarkRaidContent — GameFeatures content directory

This directory is intentionally tracked (with this README) but empty of `.uasset`
files. The actual `UGameFeatureData` asset must be authored by a designer inside
the UE Editor after the project builds:

1. Generate Project Files + build `PaldarkLab` Editor target so that the
   `PaldarkRaidContent` plugin module compiles.
2. In the Editor, browse to **Plugins → Game Features → Paldark Raid Content**.
3. Right-click → **Create Game Feature Data → GameFeatureData**.
4. Save as `GameFeatureData_RaidContent.uasset` inside this directory
   (`PaldarkLab/Plugins/PaldarkRaidContent/Content/GameFeatures/`).
5. Open the new asset and configure:
   - **InitialState**: `Active` (so the plugin auto-activates when the
     project loads).
   - **Actions**: add `UGameFeatureAction_AddComponents` / `_AddAbilities` /
     `_AddInputContextMapping` entries as the raid feature needs.
6. Restart the Editor (or run `LoadPlugin PaldarkRaidContent` from the console)
   and verify the actions fire by checking `LogPaldark`.

W27-28 ships only the plugin module scaffold; the `.uasset` and any Action
authoring is a parallel designer task. The C++ side does not depend on the
asset existing — `PaldarkLab.uproject` enables the plugin with
`"Enabled": false` by default and designers flip it on once the asset is
authored.

## Why a game-feature plugin?

Game-feature plugins let you ship raid content (maps, Pal definitions, ability
sets) as a streamable, lifecycle-managed package instead of root-cooking
everything into the base project. The plugin can be **Registered → Loaded →
Activated → Deactivated → Unloaded** at runtime, which gives you:

- Per-experience content scoping (the `PX_RaidSandbox` experience activates
  the plugin in its OnLoaded handler; quieter experiences leave it
  deactivated).
- Smaller cooked package sizes for player builds that don't ship every map.
- Cleaner ownership — raid content lives next to its plugin module rather
  than scattered across `/Game/Paldark/...`.

See `PaldarkLab/README.md` § "AssetManager + Async load + Game Feature plugin
(W27-28)" for the full authoring + test loop walkthrough.
