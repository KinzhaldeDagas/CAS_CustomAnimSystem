# CAS Custom Animation System

CAS Custom Animation System is an OBSE plugin for Oblivion that helps mod authors package and apply custom actor animations through Oblivion's native `SpecialAnims` path. It is designed to feel familiar to kNVSE users where practical, while staying within behavior currently observed for Oblivion.

This is not a full kNVSE port. CAS uses actor-base `KFFZ` / `SpecialAnims` entries, plus plugin-managed targeting for weapon, weapon-name, race, and global scopes.

## What CAS Does

- Registers loose `.kf` files from native `SpecialAnims` folders.
- Stages kNVSE-style `AnimGroupOverride` folders into Oblivion's native layout.
- Applies packs to actor bases, specific weapon forms, weapon-name tokens, races, or global-controlled scopes.
- Persists command-added mappings in the CAS co-save.
- Provides validation commands for target actors, folders, manifests, variants, and live load state.

## Folder Layout

Native Oblivion destination:

```text
Data\Meshes\<actor-model-directory>\SpecialAnims\<PackName>\<AnimationFile>.kf
```

For the default humanoid model:

```text
Data\Meshes\Characters\_Male\SpecialAnims\MyAttackPack\OneHandAttackRight.kf
Data\Meshes\Characters\_Male\SpecialAnims\MyAttackPack\OneHandAttackLeft.kf
Data\Meshes\Characters\_Male\SpecialAnims\MyAttackPack\OnehandSkill1AttackPower.kf
Data\Meshes\Characters\_Male\SpecialAnims\MyAttackPack\OneHandIdle.kf
```

kNVSE-style intake/staging source:

```text
Data\Meshes\AnimGroupOverride\MyAttackPack\OneHandAttackRight.kf
Data\Meshes\AnimGroupOverride\MyAttackPack\OneHandAttackLeft.kf
```

`AnimGroupOverride` is an intake layout only. Staging copies loose `.kf` files into the native `SpecialAnims` destination, but activation still requires a CAS command or manifest mapping.

## Basic Workflow

1. Put your `.kf` files in a native `SpecialAnims` folder, or place them under `AnimGroupOverride` for staging.
2. Validate the source folder.
3. Stage files if using `AnimGroupOverride`.
4. Validate the native `SpecialAnims` folder.
5. Activate the pack for the intended scope.
6. Test attack, idle, equip, unequip, weapon switch, save, and reload.

Example staging flow:

```text
SomeActorRef.CASValidateKNVSELayout "Data\Meshes\AnimGroupOverride"
SomeActorRef.CASStageAnimGroupOverride "MyAttackPack"
SomeActorRef.CASValidateSpecialAnimFolder "MyAttackPack"
```

Example direct actor activation:

```text
SomeActorRef.SetActorAnimationPath 0 1 "MyAttackPack"
SomeActorRef.CASValidateSpecialAnimLoadState "MyAttackPack"
SomeActorRef.CASValidateSpecialAnimVariants "MyAttackPack"
```

`SetActorAnimationPath 0 1 "MyAttackPack"` scans the target actor model's native `SpecialAnims\MyAttackPack` folder, registers safe `.kf` entries as actor-base `KFFZ`, persists the mapping, and reloads the calling actor when possible.

## Targeting Options

### Actor Base

Use this when the actor base should use the pack directly:

```text
SomeActorRef.SetActorAnimationPath 0 1 "MyAttackPack"
```

### Specific Weapon Form

Use this when a pack should apply while a specific weapon form is equipped:

```text
SetWeaponAnimationPath MySpearWeapon 0 1 "KSTN_Spear_1H"
```

This is a plugin-managed shim over actor-base `SpecialAnims`. CAS does not currently expose a decoded native weapon override map.

### Weapon Name Token

Use this when a pack should apply to any equipped weapon whose full name contains a token:

```text
SomeActorRef.CASSetAutoWeaponAnimationPath "Spear" 1 "KSTN_Spear_1H" 1
SomeActorRef.CASValidateAutoWeaponAnimationRules "Spear"
SomeActorRef.CASApplyAutoWeaponAnimations 1
```

`Spear` is only a matching token. It is not a new native Oblivion weapon type.

### Race Scope

Use this when a pack should apply to actors matching a race:

```text
SomeActorRef.CASSetRaceAnimationPath MyRace 1 "MyRacePack" 1
SomeActorRef.CASValidateTargetAnimationMappings
SomeActorRef.CASApplyTargetMappings 1
```

### Global Scope

Use this when a pack should apply while a global variable is non-zero:

```text
SomeActorRef.CASSetGlobalAnimationPath MyGlobal 1 "MyGlobalPack" 1
SomeActorRef.CASValidateTargetAnimationMappings
SomeActorRef.CASApplyTargetMappings 1
```

Race and global targets are plugin-managed scopes layered over the observed actor-base `SpecialAnims` path.

## Manifest Workflow

Place JSON manifests in:

```text
Data\OBSE\Plugins\CustomAnimSupport\*.json
```

Actor target example:

```json
[
  {
    "mod": "MyMod.esp",
    "form": "000800",
    "folder": "MyAttackPack"
  }
]
```

Weapon-name example:

```json
[
  {
    "weaponNameContains": "Spear",
    "folder": "KSTN_Spear_1H"
  }
]
```

Validation and load example:

```text
SomeActorRef.CASValidateSpecialAnimManifest "Data\OBSE\Plugins\CustomAnimSupport\my_pack.json"
SomeActorRef.CASLoadSpecialAnimManifest "Data\OBSE\Plugins\CustomAnimSupport\my_pack.json"
CASGetTargetAnimationMappingCount
SomeActorRef.CASGetTargetAnimationMatchCount
SomeActorRef.CASValidateTargetAnimationMappings
SomeActorRef.CASApplyTargetMappings 1
```

## Animation File Names

Use Oblivion animation group file names that the engine already requests. CAS does not create new animation groups.

Common examples:

```text
OneHandIdle.kf
OneHandAttackRight.kf
OneHandAttackLeft.kf
OnehandSkill1AttackPower.kf
TwoHandAttackRight.kf
```

If Oblivion never requests the group, CAS cannot make it play just because the file exists.

## Suggested In-Game Test

For a weapon-name pack:

```text
Player.CASValidateSpecialAnimFolder "KSTN_Spear_1H"
Player.CASSetAutoWeaponAnimationPath "Spear" 1 "KSTN_Spear_1H" 1
Player.CASValidateAutoWeaponAnimationRules "Spear"
Player.CASApplyAutoWeaponAnimations 1
Player.CASValidateSpecialAnimLoadState "KSTN_Spear_1H"
Player.CASValidateSpecialAnimVariants "KSTN_Spear_1H"
```

Then verify:

1. Equip the matching weapon.
2. Perform normal attacks.
3. Perform power attacks.
4. Unequip the weapon.
5. Equip a non-matching weapon of the same broad weapon class.
6. Confirm the non-matching weapon is not stuck on the custom pack.
7. Re-equip the matching weapon.
8. Save, reload, and repeat the equip/attack checks.

## Current Limitations

The following are unsupported or not release-ready in the current implementation:

- First-person-only animation folder separation.
- BSA override enumeration.
- Condition polling and priority stacks.
- FormList / FLST expansion.
- Native decoded weapon, race, or global override maps.
- Dynamic new animation groups.

JSON `condition` and `pollCondition` fields are recognized for diagnostics but skipped by load and stage commands instead of being imported as unconditional `KFFZ` entries.

## Troubleshooting

If staging succeeds but animations do not play, activate the staged folder with the intended CAS scope. Staging is asset-only.

If vanilla attacks play instead, run `CASValidateSpecialAnimVariants` and confirm the attack file names match Oblivion groups that are actually requested.

If a non-matching weapon appears stuck on a custom pack, remove broad actor-base activation for weapon-specific packs and use weapon form or weapon-name targeting instead.

If first-person-specific files do not apply, that path is currently unsupported.

If condition fields appear ignored, that behavior is expected for now; condition polling still requires more Oblivion-specific decoding before it can be treated as supported.
