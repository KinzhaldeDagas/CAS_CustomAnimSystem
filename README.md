# CustomAnimSupport Author Guide

## Native Model

Oblivion loads actor custom animations from actor-base `KFFZ` entries. Each entry is a relative `.kf` path under the actor model directory's `SpecialAnims` folder:

```text
Data\Meshes\<actor-model-directory>\SpecialAnims\<KFFZ-entry>.kf
```

For an NPC using:

```text
Characters\_Male\Skeleton.NIF
```

the native folder is:

```text
Data\Meshes\Characters\_Male\SpecialAnims\
```

A folder entry such as `MyAttackPack\OneHandAttackRight.kf` resolves to:

```text
Data\Meshes\Characters\_Male\SpecialAnims\MyAttackPack\OneHandAttackRight.kf
```

## Authoring Rules

- Target an NPC or creature base, or a live actor reference whose base can be resolved.
- Put KFs in the target actor model's native `SpecialAnims` folder, or stage them there from `Data\Meshes\AnimGroupOverride`.
- Use native Oblivion animation group names inside the KF sequence. The plugin does not add new animation groups.
- Prefix files with native movement/weapon prefixes only when they match Oblivion's parsed key model, for example `OneHandAttackRight.kf`.
- Validate movement KFs. Oblivion can report `Animate in Place` for zero movement vectors and morph errors when same-morph-key controller counts differ.
- Treat native power attack groups as volatile.

## Direct Native Folder Workflow

Use this when your files are already in the final native folder.

```text
SomeNPCRef.CASValidateAnimationTarget
SomeNPCRef.CASValidateSpecialAnimFolder "MyAttackPack"
SomeNPCRef.SetActorAnimationPath 0 1 "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimLoadState "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimVariants "MyAttackPack"
SomeNPCRef.CASValidateAnimGroupLoaded AttackRight
SomeNPCRef.CASPlayAnimationPath "MyAttackPack\OneHandAttackRight.kf" AttackRight 1
SomeNPCRef.IsAnimSequencePlaying "MyAttackPack\OneHandAttackRight.kf" 0
```

`SetActorAnimationPath 0 1 "MyAttackPack"` scans the native `SpecialAnims\MyAttackPack` folder, registers safe `.kf` entries as actor-base `KFFZ`, persists them in the plugin co-save, and reloads the calling actor when possible.

## kNVSE-Style Package Intake

Use this when a source package is organized under:

```text
Data\Meshes\AnimGroupOverride\<folder>\
```

Run the read-only diagnostic first:

```text
SomeNPCRef.CASValidateKNVSELayout "Data\Meshes\AnimGroupOverride"
```

Then stage one folder into the calling actor base's native `SpecialAnims` folder:

```text
SomeNPCRef.CASStageAnimGroupOverride "MyAttackPack"
```

The staging command copies loose `.kf` files only when the destination is missing and validates the destination through Oblivion's native SpecialAnims loader/parser, staging is asset-only: it does not register actor-base `KFFZ`, does not persist co-save entries, and does not reload the actor. Files reported as `invalid` are not accepted.

After staging, validate the native folder, then activate the pack through the scope you actually intend to use:

```text
SomeNPCRef.CASValidateSpecialAnimFolder "MyAttackPack"
SomeNPCRef.SetActorAnimationPath 0 1 "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimLoadState "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimVariants "MyAttackPack"
```

For weapon-scoped, weapon-name, race, or globals, use the matching mapping command instead of `SetActorAnimationPath` so the pack is not globally active for every weapon on the same actor base.

## Weapon-Scoped Shim

Oblivion does not expose a native kNVSE weapon override style map. `SetWeaponAnimationPath` is implemented as a plugin-managed shim: it records a weapon form plus a native `SpecialAnims` folder or `.kf`, then applies that mapping to a live actor's own actor-base `KFFZ` list when the actor has that weapon equipped.

Example:

```text
SetWeaponAnimationPath MySpearWeapon 0 1 "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimFolder "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimLoadState "MyAttackPack"
SomeNPCRef.CASValidateAnimGroupLoaded AttackRight
```

If the command is dot-called on an actor currently holding `MySpearWeapon`, the plugin attempts an immediate apply. Otherwise the animation hooks apply the mapping the next time that actor requests an animation group while the weapon is equipped. The mapped files still must exist under the target actor model's native `SpecialAnims` directory.

Disable the future auto-apply mapping with:

```text
SetWeaponAnimationPath MySpearWeapon 0 0 "MyAttackPack"
```

Disabling a mapping does not delete loose files or patch source plugins, the plugin removes actor-base `KFFZ` paths only when they were newly added by that weapon-scoped apply, and it can use pre-existing scoped paths to prune matching live decoded animation-maps. Pre-existing actor-base `KFFZ`

## Weapon Name Auto Rules

Use this when a pack should apply to any equipped weapon whose full name contains a token such as `Spear`, `Halberd`, or a mod-added weapon family name. This is plugin-managed matching over the equipped-weapon path; `Spear` is only the default KSTN example rule.

Command form:

```text
SomeNPCRef.CASSetAutoWeaponAnimationPath "Spear" 1 "KSTN_Spear_1H" 1
SomeNPCRef.CASValidateAutoWeaponAnimationRules "Spear"
SomeNPCRef.CASApplyAutoWeaponAnimations 1
SomeNPCRef.CASValidateSpecialAnimLoadState "KSTN_Spear_1H"
```

`CASValidateAutoWeaponAnimationRules` is the read-only pre-apply check for user-authored packs. It uses the same equipped-weapon lookup and `weaponNameContains` match as the runtime hook, then validates the mapped `.kf` or folder through the native SpecialAnims loader/parser path without adding `KFFZ` entries or reloading the actor.

Manifest form for `Data\OBSE\Plugins\CustomAnimSupport\*.json`:

```json
[
  {
    "weaponNameContains": "Spear",
    "folder": "KSTN_Spear_1H"
  }
]
```

The folder still has to exist under each matching actor model's native `SpecialAnims` directory. On the default humanoid model, that means:

```text
Data\Meshes\Characters\_Male\SpecialAnims\KSTN_Spear_1H\
```

Multiple rules can be loaded. When a live actor equips a matching weapon, the hook applies every matching rule to that actor's own actor-base `KFFZ` list and records the actor/weapon/anim-data. The attack-preference hook then looks for loaded sequences under the configured rule paths and calls `ActorAnimData_PlaySequence` path for matching attack-like groups. If no matching sequence is loaded, it falls back to Oblivion's original selector.

Disable a command-added rule with:

```text
SomeNPCRef.CASSetAutoWeaponAnimationPath "Spear" 0 "KSTN_Spear_1H" 1
```

The command-added rules persist in the CustomAnimSupport co-save. Manifest-loaded rules are config entries and are reloaded from the manifest directory on OBSE load messages. Disabling/removing a rule prevents future application; it does not delete loose files.

## Manifest Workflow

For automatic actor-base registration, place JSON manifests in:

```text
Data\OBSE\Plugins\CustomAnimSupport\*.json
```

Example:

```json
[
  {
    "mod": "MyMod.esp",
    "form": "000800",
    "folder": "MyAttackPack"
  }
]
```

The target can resolve to an NPC base, creature base, actor reference base, `TESRace`, `TESGlobal`, or a `weaponNameContains` auto-weapon rule entry.

Actor-base and actor-reference targets directly mutate that base's native `KFFZ` list. Race and global targets are plugin-managed scopes layered over the decoded actor-base loader: when a live actor later reaches `Actor_LoadAnimGroup_`, the plugin checks whether the actor's NPC race pointer equals the race target, or whether the global target's current value is non-zero, then applies the mapped folder to that actor's own actor-base `KFFZ` list and reloads that actor. `CASApplyTargetMappings` lets you run that same plugin-managed race/global check immediately on a selected live actor. `CASSetRaceAnimationPath` and `CASSetGlobalAnimationPath` are direct command alternatives when a script wants to add/remove one persistent race/global mapping without a manifest. `CASGetTargetAnimationMappingCount` reports the loaded mapping table before actor matching is tested. `CASGetTargetAnimationMatchCount` reports whether loaded race/global mappings currently match a dot-called actor before any reload or asset validation. `CASValidateTargetAnimationMappings` checks the matching mapped `.kf` or folder assets for that actor without mutating `KFFZ` or reloading live `ActorAnimData`.. Phew

Race/global and weapon-name target folders must still exist under the matching actor model's native `SpecialAnims` directory. The JSON `forms` field is supported as an array of target form IDs. A game `FormList`/`FLST` remains unsupported in Oblivion.

Manual testing:

```text
SomeNPCRef.CASValidateSpecialAnimManifest "Data\OBSE\Plugins\CustomAnimSupport\my_pack.json"
SomeNPCRef.CASLoadSpecialAnimManifest "Data\OBSE\Plugins\CustomAnimSupport\my_pack.json"
CASGetTargetAnimationMappingCount
SomeNPCRef.CASGetTargetAnimationMatchCount
SomeNPCRef.CASValidateTargetAnimationMappings
SomeNPCRef.CASApplyTargetMappings 1
SomeNPCRef.CASValidateSpecialAnimLoadState "MyAttackPack"
```

Direct race/global command tests:

```text
SomeNPCRef.CASSetRaceAnimationPath MyRace 1 "MyAttackPack" 1
SomeNPCRef.CASSetGlobalAnimationPath MyGlobal 1 "MyAttackPack" 1
CASGetTargetAnimationMappingCount
SomeNPCRef.CASGetTargetAnimationMatchCount
SomeNPCRef.CASValidateTargetAnimationMappings
```

## What Is Not Supported

- Native weapon-scoped animation override maps. The current weapon command is plugin-managed over actor-base `SpecialAnims`.
- Form-list or form target expansion. Race/global targets are available only as plugin-managed manifest mappings over actor-base `SpecialAnims`, not as native engine override maps.
- First-person folder separation through `_1stperson`.
- Condition polling, priority stacks, or `SetAnimationPathCondition`. Use `CASGetConditionPollingSupport` when testing packages; it should.. return `0` needs to be decoded more here..
- BSA override enumeration.
- Dynamic new animation groups.

Those inputs are either rejected, counted as unsupported, or accepted only for familiar call shape. JSON `condition` and `pollCondition` fields are counted as `conditionFields`; entries carrying those fields are counted as `conditionalEntries` and skipped by load/stage commands instead of being imported as unconditional `KFFZ` entries.

## Validation Commands :)

1. Validate the target:

```text
SomeNPCRef.CASValidateAnimationTarget
```

2. Validate or stage assets:

```text
SomeNPCRef.CASValidateKNVSELayout "Data\Meshes\AnimGroupOverride"
SomeNPCRef.CASStageAnimGroupOverride "MyAttackPack"
```

3. Validate the native folder, then activate the intended scope before checking live load state:

```text
SomeNPCRef.CASValidateSpecialAnimFolder "MyAttackPack"
SomeNPCRef.SetActorAnimationPath 0 1 "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimLoadState "MyAttackPack"
SomeNPCRef.CASValidateSpecialAnimVariants "MyAttackPack"
```

4. Validate the group Oblivion will request:

```text
SomeNPCRef.CASValidateAnimGroupLoaded AttackRight
```

5. Test a specific file path:

```text
SomeNPCRef.CASPlayAnimationPath "MyAttackPack\OneHandAttackRight.kf" AttackRight 1
SomeNPCRef.IsAnimSequencePlaying "MyAttackPack\OneHandAttackRight.kf" 0
```

6. Save, reload, and re-run load-state validation:

```text
SomeNPCRef.CASValidateSpecialAnimLoadState "MyAttackPack"
SomeNPCRef.CASValidateSaveLoadState
```

For one-off diagnostics, clean up explicit actor-base activation after the test:

```text
SomeNPCRef.CASSetActorAnimationPath 0 "MyAttackPack"
```
