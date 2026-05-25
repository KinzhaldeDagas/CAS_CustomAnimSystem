// User Defines
#include "config.h"

// OBSE
#include "obse/GameAPI.h"
#include "obse/GameData.h"
#include "obse/GameObjects.h"
#include "obse/GameProcess.h"
#include "obse/GameForms.h"
#include "obse/GameRTTI.h"
#include "obse/GameTasks.h"
#include "obse/PluginAPI.h"
#include "obse/CommandTable.h"
#include "obse/ParamInfos.h"
#include "obse/ScriptUtils.h"
#include "obse/Utilities.h"
#include "obse_common/SafeWrite.h"

// C/C++
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Windows
#include <Windows.h>
#include <shlobj.h>	// CSIDL_MYDOCUMENTS

// ================================
// Handles
// ================================

PluginHandle g_pluginHandle = kPluginHandle_Invalid;
OBSEMessagingInterface* g_messaging = NULL;
OBSESerializationInterface* g_serialization = NULL;
OBSEStringVarInterface* g_stringVar = NULL;

// ================================
// Decoded Oblivion animation calls
// ================================

namespace CAS
{
	static const UInt32 kOpcodeBase = 0x70C0; // Current command registration uses 0x70C0-0x70F3.
	static const UInt32 kModelLoaderLoadKF = 0x00439FF0;
	static const UInt32 kBuildActorModelKFList = 0x00435830;
	static const UInt32 kNiControllerSequenceGetControlledBlockName = 0x006C66B0;
	static const UInt32 kActorAnimDataMapLookup = 0x00470960;
	static const UInt32 kActorAnimDataMapRemove = 0x004708B0;
	static const UInt32 kActorAnimDataInstallKFModel = 0x00474070;
	static const UInt32 kActorAnimDataLoadKFFZSpecialAnims = 0x00476080;
	static const UInt32 kActorAnimDataPlayEncodedGroup = 0x00476260;
	static const UInt32 kActorAnimDataPlaySequence = 0x00474530;
	static const UInt32 kActorAnimDataAnimsLoading = 0x004712B0;
	static const UInt32 kActorAnimDataClearSlot = 0x00470FC0;
	static const UInt32 kActorAnimDataIdleInactive = 0x00472EA0;
	static const UInt32 kActorAnimDataQueueIdle = 0x00477DB0;
	static const UInt32 kNiControllerManagerRemoveSequence = 0x006C4A10;
	static const UInt32 kQueuedIdleKFLoader = 0x00474C50;
	static const UInt32 kQueuedIdleReadyHandler = 0x00476380;
	static const UInt32 kActorAnimDataPlayAnimGroup = 0x00477B60;
	static const UInt32 kActorLoadAnimGroup = 0x005E5690;
	static const UInt32 kTESAnimationLoadKFFZChunk = 0x004688D0;
	static const UInt32 kConditionListEvaluate = 0x0056A510;
	static const UInt32 kConditionListEvaluateWrapper = 0x0056A950;
	static const UInt32 kConditionListLoadCondition = 0x0056A7B0;
	static const UInt32 kConditionLoad = 0x0056A970;
	static const UInt32 kPackageChooser = 0x00569020;
	static const UInt32 kPickIdleCommand = 0x00511FA0;
	static const UInt32 kIdleCandidateSearch = 0x005206B0;
	static const UInt32 kIdleRootSearch = 0x00521450;
	static const UInt32 kAnimSequenceSingleDeletingDestructor = 0x00473D70;
	static const UInt32 kAnimSequenceMultipleDeletingDestructor = 0x00473E30;
	static const UInt32 kMaxSpecialAnimPath = 512;
	static const UInt32 kMaxManifestPath = 520;
	static const UInt32 kSerializationRecordPersistentAnims = 'CASA';
	static const UInt32 kSerializationRecordWeaponAnims = 'CASW';
	static const UInt32 kSerializationRecordTargetAnims = 'CAST';
	static const UInt32 kSerializationRecordAutoWeaponRules = 'CASR';
	static const UInt32 kSerializationVersionPersistentAnims = 1;
	static const UInt32 kSerializationVersionWeaponAnims = 1;
	static const UInt32 kSerializationVersionTargetAnims = 1;
	static const UInt32 kSerializationVersionAutoWeaponRules = 1;
	static const char* kDefaultManifestDirectory = "Data\\OBSE\\Plugins\\CustomAnimSupport";
	static const char* kDefaultKNVSEAnimGroupOverrideDirectory = "Data\\Meshes\\AnimGroupOverride";
	static const UInt32 kAnimGroupInfoTable = 0x00B102E0;
	static const UInt32 kAnimMovementPrefixTable = 0x00B102B8;
	static const UInt32 kAnimWeaponPrefixTable = 0x00B102C8;
	static const UInt32 kAnimSlotNameTable = 0x00B108EC;
	static const UInt32 kAnimGroupInfoSize = 0x24;
	static const UInt32 kKFModelControllerSequenceOffset = 0x04;
	static const UInt32 kKFModelAnimGroupOffset = 0x08;
	static const UInt32 kKFModelRefCountOffset = 0x0C;
	static const UInt32 kDeferredInstallGuard = 256;
	static const UInt32 kActorAnimDataCurrentKeyOffset = 0x3C;
	static const UInt32 kActorAnimDataQueuedKeyOffset = 0x70;
	static const UInt32 kAnimIdleField04Offset = 0x04;
	static const UInt32 kAnimIdleSequenceOffset = 0x10;
	static const UInt32 kAnimIdleFormOffset = 0x24;
	static const UInt32 kBSAnimGroupSequencePathOffset = 0x08;
	static const UInt32 kBSAnimGroupSequenceField24Offset = 0x24;
	static const UInt32 kAnimSequenceEntrySequenceOffset = 0x04;
	static const UInt32 kAnimSequenceMultipleListOffset = 0x04;
	static const UInt32 kAnimSequenceMultipleListHeadOffset = 0x04;
	static const UInt32 kAnimSequenceMultipleListCountOffset = 0x0C;
	static const UInt32 kListNodeNextOffset = 0x00;
	static const UInt32 kListNodeDataOffset = 0x08;
	static const UInt32 kSequenceListGuard = 128;
	static const UInt32 kTESFormFlagDisabled = 0x20;
	static const char* kDefaultAutoWeaponSpecialAnimFolder = "KSTN_Spear_1H";
	static const char* kDefaultAutoWeaponNameNeedle = "Spear";
	static const UInt32 kActorLoadAnimGroupPatchSize = 5;
	static const UInt32 kActorAnimDataPlayEncodedGroupPatchSize = 6;

	static NiNode* GetValidationSkeletonRoot(TESObjectREFR* ref, TESActorBase* actorBase);
	static ActorAnimData* GetActorBaseSpecialAnimData(TESObjectREFR* ref);
	static bool IsPlayerFirstPersonAnimData(TESObjectREFR* ref, ActorAnimData* animData);
	static bool ContainsCaseInsensitive(const char* haystack, const char* needle);
	static bool IsPersistableAnimationPath(const char* path);
	static bool PathMatchesOptionalFolder(const char* path, const char* folder);
	static bool HasQueuedRemoval(const std::vector<std::string>& removals, const char* animPath);
	static bool GetSpecialAnimFolderFromAnimPath(const char* animPath, char* outFolder, UInt32 outLen);

	typedef UInt16 (__thiscall * ActorLoadAnimGroupFn)(Actor* actor, UInt32 groupID, UInt32 arg2, UInt32 arg3);
	typedef UInt32 (__thiscall * ActorAnimDataPlayEncodedGroupFn)(ActorAnimData* animData, UInt32 encodedGroup, UInt32 slotArg);
	typedef void* (__stdcall * BuildActorModelKFListFn)(const char* modelPath, UInt32 useCache);
	typedef UInt32 (__thiscall * AnimSequenceEntryRemoveFn)(void* entry, UInt32 sequence);
	typedef void (__thiscall * AnimSequenceEntryDestroyFn)(void* entry, UInt32 freeMemory);

	struct ScopedInstalledAnim
	{
		std::string animPath;
		UInt16 key;
		UInt32 groupID;
		bool removeKFFZ;

		ScopedInstalledAnim() : key(0xFFFF), groupID(TESAnimGroup::kAnimGroup_Max), removeKFFZ(true) {}
		ScopedInstalledAnim(const char* path, UInt16 parsedKey, UInt32 parsedGroupID, bool removeActorBaseKFFZ) :
			animPath(path ? path : ""), key(parsedKey), groupID(parsedGroupID), removeKFFZ(removeActorBaseKFFZ) {}
	};

	struct ScopedFolderKey
	{
		UInt16 key;
		UInt32 groupID;
		std::string folder;

		ScopedFolderKey() : key(0xFFFF), groupID(TESAnimGroup::kAnimGroup_Max) {}
		ScopedFolderKey(UInt16 parsedKey, UInt32 parsedGroupID, const char* folderPath) :
			key(parsedKey), groupID(parsedGroupID), folder(folderPath ? folderPath : "") {}
	};

	struct AutoWeaponAnimRule
	{
		std::string nameContains;
		std::string animPath;
		bool persist;
		bool builtin;

		AutoWeaponAnimRule() : persist(false), builtin(false) {}
		AutoWeaponAnimRule(const char* needle, const char* path, bool persistRule, bool builtinRule) :
			nameContains(needle ? needle : ""), animPath(path ? path : ""), persist(persistRule), builtin(builtinRule) {}
	};

	struct AutoWeaponAnimAttempt
	{
		UInt32 actorRefID;
		UInt32 weaponRefID;
		UInt32 actorBaseRefID;
		UInt32 animDataID;
		UInt32 actorTypeID;
		UInt32 generation;
		bool loaded;
		std::vector<std::string> nameContains;
		std::vector<std::string> animPaths;
		std::vector<ScopedInstalledAnim> installed;
	};

	static ActorLoadAnimGroupFn s_actorLoadAnimGroupOriginal = NULL;
	static ActorAnimDataPlayEncodedGroupFn s_actorAnimDataPlayEncodedGroupOriginal = NULL;
	static bool s_actorLoadAnimGroupHookInstalled = false;
	static bool s_actorAnimDataPlayEncodedGroupHookInstalled = false;
	static bool s_autoWeaponAnimApplyGuard = false;
	static bool s_autoWeaponDefaultRuleInitialized = false;
	static UInt32 s_autoWeaponAnimGeneration = 1;
	static UInt32 s_autoWeaponProbeLogCount = 0;
	static UInt32 s_autoWeaponPowerReassertLogCount = 0;
	static UInt32 s_autoWeaponAttackPreferLogCount = 0;
	static UInt32 s_autoWeaponAttackFallbackLogCount = 0;
	static UInt32 s_actorBaseSpecialAnimFirstPersonBypassLogCount = 0;
	static std::vector<AutoWeaponAnimRule> s_autoWeaponAnimRules;
	static std::vector<AutoWeaponAnimAttempt> s_autoWeaponAnimAttempts;

	class ExactStringMatcher
	{
	public:
		ExactStringMatcher(const char* value) : m_value(value) {}

		bool Accept(const char* info)
		{
			return info && m_value && std::strcmp(info, m_value) == 0;
		}

	private:
		const char* m_value;
	};

	static void NormalizeSlashes(char* path)
	{
		for (char* cur = path; cur && *cur; ++cur)
		{
			if (*cur == '/')
				*cur = '\\';
		}
	}

	static bool IsSlash(char value)
	{
		return value == '\\' || value == '/';
	}

	static bool IsKFPath(const char* path)
	{
		if (!path)
			return false;

		const char* dot = std::strrchr(path, '.');
		return dot && _stricmp(dot, ".kf") == 0;
	}

	static bool IsExactIdleKFPath(const char* path)
	{
		if (!path)
			return false;

		const char* dot = std::strrchr(path, '.');
		return dot && std::strcmp(dot, ".kf") == 0;
	}

	static bool IsJSONPath(const char* path)
	{
		if (!path)
			return false;

		const char* dot = std::strrchr(path, '.');
		return dot && _stricmp(dot, ".json") == 0;
	}

	static bool IsSafeRelativePath(const char* path)
	{
		if (!path || !path[0])
			return false;

		if (IsSlash(path[0]) || std::strchr(path, ':'))
			return false;

		const char* cur = path;
		while (*cur)
		{
			if (cur[0] == '.' && cur[1] == '.' && (cur[2] == 0 || IsSlash(cur[2])))
				return false;

			if (IsSlash(cur[0]) && cur[1] == '.' && cur[2] == '.' && (cur[3] == 0 || IsSlash(cur[3])))
				return false;

			++cur;
		}

		return true;
	}

	static bool DirectoryExists(const char* path)
	{
		const DWORD attrs = GetFileAttributesA(path);
		return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static bool FileExists(const char* path)
	{
		const DWORD attrs = GetFileAttributesA(path);
		return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
	}

	static TESActorBase* GetActorBaseFromRef(TESObjectREFR* ref)
	{
		if (!ref || !ref->baseForm)
			return NULL;

		return OBLIVION_CAST(ref->baseForm, TESForm, TESActorBase);
	}

	static TESActorBase* ResolveActorBase(TESObjectREFR* ref, TESActorBase* explicitBase)
	{
		if (explicitBase)
			return explicitBase;

		return GetActorBaseFromRef(ref);
	}

	static TESAnimation* GetAnimationList(TESActorBase* actorBase)
	{
		if (!actorBase)
			return NULL;

		return OBLIVION_CAST(actorBase, TESActorBase, TESAnimation);
	}

	static bool HasExactAnimation(TESAnimation* anim, const char* animPath)
	{
		if (!anim || !animPath || !animPath[0])
			return false;

		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			if (std::strcmp(cur->animationName, animPath) == 0)
				return true;
		}

		return false;
	}

	static UInt32 CountAnimations(TESAnimation* anim)
	{
		UInt32 count = 0;

		if (!anim)
			return count;

		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
			++count;

		return count;
	}

	static const char* GetAnimationAt(TESAnimation* anim, UInt32 index)
	{
		if (!anim)
			return NULL;

		UInt32 curIndex = 0;
		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			if (curIndex == index)
				return cur->animationName;

			++curIndex;
		}

		return NULL;
	}

	static bool AddAnimation(TESAnimation* anim, const char* animPath)
	{
		if (!anim || !animPath || !animPath[0])
			return false;

		if (HasExactAnimation(anim, animPath))
			return false;

		const UInt32 len = std::strlen(animPath);
		TESAnimation::AnimationNode* newNode = (TESAnimation::AnimationNode*)FormHeap_Allocate(sizeof(TESAnimation::AnimationNode));
		if (!newNode)
			return false;

		newNode->animationName = (char*)FormHeap_Allocate(len + 1);
		if (!newNode->animationName)
		{
			FormHeap_Free(newNode);
			return false;
		}

		std::memcpy(newNode->animationName, animPath, len + 1);
		newNode->next = NULL;

		AnimationVisitor visitor(&anim->data);
		visitor.Append(newNode);
		return true;
	}

	static bool RemoveAnimation(TESAnimation* anim, const char* animPath)
	{
		if (!anim || !animPath || !animPath[0])
			return false;

		AnimationVisitor visitor(&anim->data);
		ExactStringMatcher matcher(animPath);
		return visitor.RemoveIf(matcher) != 0;
	}

	static UInt32 RemoveAnimationsInFolder(TESAnimation* anim, const char* folder)
	{
		if (!anim || !folder || !folder[0])
			return 0;

		std::vector<std::string> removals;
		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			const char* entry = cur->animationName;
			if (IsPersistableAnimationPath(entry) && PathMatchesOptionalFolder(entry, folder) && !HasQueuedRemoval(removals, entry))
				removals.push_back(entry);
		}

		UInt32 removed = 0;
		for (std::vector<std::string>::const_iterator it = removals.begin(); it != removals.end(); ++it)
		{
			UInt32 before = CountAnimations(anim);
			if (RemoveAnimation(anim, it->c_str()))
			{
				UInt32 after = CountAnimations(anim);
				removed += before > after ? before - after : 1;
			}
		}

		return removed;
	}

	static const char* GetActorBaseModelPath(TESActorBase* actorBase)
	{
		if (!actorBase)
			return NULL;

		const char* path = actorBase->model.GetModelPath();
		if (path && path[0])
			return path;

		TESCreature* creature = OBLIVION_CAST(actorBase, TESActorBase, TESCreature);
		if (creature && creature->modelList.modelList.nifPath)
			return creature->modelList.modelList.nifPath;

		return NULL;
	}

	static bool GetModelDirectory(TESActorBase* actorBase, char* outDir, UInt32 outLen)
	{
		if (!outDir || outLen == 0)
			return false;

		outDir[0] = 0;

		const char* modelPath = GetActorBaseModelPath(actorBase);
		if (!modelPath || !modelPath[0])
			return false;

		strncpy_s(outDir, outLen, modelPath, _TRUNCATE);
		NormalizeSlashes(outDir);

		char* lastSlash = std::strrchr(outDir, '\\');
		if (!lastSlash)
			return false;

		*lastSlash = 0;
		return outDir[0] != 0;
	}

	static bool GetSpecialAnimsRoot(TESActorBase* actorBase, char* outDir, UInt32 outLen)
	{
		char modelDir[260];
		if (!GetModelDirectory(actorBase, modelDir, sizeof(modelDir)))
			return false;

		return sprintf_s(outDir, outLen, "Data\\Meshes\\%s\\SpecialAnims", modelDir) > 0;
	}

	static bool GetSpecialAnimDiskPath(TESActorBase* actorBase, const char* animPath, char* outPath, UInt32 outLen)
	{
		char root[520];
		if (!GetSpecialAnimsRoot(actorBase, root, sizeof(root)))
			return false;

		return sprintf_s(outPath, outLen, "%s\\%s", root, animPath) > 0;
	}

	static bool GetSpecialAnimLoaderPath(TESActorBase* actorBase, const char* animPath, char* outPath, UInt32 outLen)
	{
		char modelDir[260];
		if (!GetModelDirectory(actorBase, modelDir, sizeof(modelDir)))
			return false;

		return sprintf_s(outPath, outLen, "%s\\SpecialAnims\\%s", modelDir, animPath) > 0;
	}

	struct DiscoverResult
	{
		UInt32 found;
		UInt32 added;
		UInt32 skipped;
		UInt32 errors;
	};

	struct ParsedSpecialAnimAssetSummary
	{
		std::string animPath;
		std::string loaderPath;
		UInt16 key;
		UInt32 groupID;
		UInt8 morphKey;
		UInt32 controllerCount;
	};

	struct ManifestEntry
	{
		std::string folder;
		std::string modName;
		std::string weaponNameContains;
		std::vector<std::string> forms;
		UInt32 ignoredFields;
		UInt32 conditionFields;
		bool hasCondition;
		bool hasPollCondition;
		bool hasPriority;
		bool hasBSA;
		bool hasMatchBaseAnimGroup;

		ManifestEntry() : ignoredFields(0), conditionFields(0), hasCondition(false), hasPollCondition(false), hasPriority(false), hasBSA(false), hasMatchBaseAnimGroup(false) {}
	};

	struct ManifestResult
	{
		UInt32 files;
		UInt32 entries;
		UInt32 targets;
		UInt32 found;
		UInt32 added;
		UInt32 skipped;
		UInt32 unsupported;
		UInt32 errors;
		UInt32 ignoredFields;
		UInt32 conditionFields;
		UInt32 conditionalEntries;

		ManifestResult() : files(0), entries(0), targets(0), found(0), added(0), skipped(0), unsupported(0), errors(0), ignoredFields(0), conditionFields(0), conditionalEntries(0) {}
	};

	struct PersistentAnimEntry
	{
		UInt32 formID;
		std::string animPath;

		PersistentAnimEntry(UInt32 id, const char* path) : formID(id), animPath(path ? path : "") {}
	};

	struct WeaponAnimEntry
	{
		UInt32 weaponFormID;
		UInt32 firstPerson;
		std::string animPath;

		WeaponAnimEntry(UInt32 id, UInt32 firstPersonFlag, const char* path) : weaponFormID(id), firstPerson(firstPersonFlag), animPath(path ? path : "") {}
	};

	struct WeaponAnimAttempt
	{
		UInt32 actorRefID;
		UInt32 weaponRefID;
		UInt32 actorBaseRefID;
		UInt32 animDataID;
		UInt32 actorTypeID;
		UInt32 generation;
		bool loaded;
		std::vector<ScopedInstalledAnim> installed;
	};

	enum TargetAnimScope
	{
		kTargetAnimScope_None = 0,
		kTargetAnimScope_Race = 1,
		kTargetAnimScope_Global = 2
	};

	struct TargetAnimEntry
	{
		UInt32 targetFormID;
		UInt32 scope;
		bool persist;
		std::string animPath;

		TargetAnimEntry(UInt32 id, UInt32 targetScope, bool persistMapping, const char* path) :
			targetFormID(id), scope(targetScope), persist(persistMapping), animPath(path ? path : "") {}
	};

	struct TargetAnimAttempt
	{
		UInt32 actorRefID;
		UInt32 targetFormID;
		UInt32 scope;
		UInt32 actorBaseRefID;
		UInt32 animDataID;
		UInt32 actorTypeID;
		UInt32 generation;
		bool loaded;
	};

	struct PruneResult
	{
		UInt32 checked;
		UInt32 missing;
		UInt32 removed;
		UInt32 skipped;
		UInt32 errors;

		PruneResult() : checked(0), missing(0), removed(0), skipped(0), errors(0) {}
	};

	struct ActorPathChangeResult
	{
		bool folderMode;
		bool changed;
		bool loaded;
		UInt32 found;
		UInt32 added;
		UInt32 removed;
		UInt32 persistedRemoved;
		UInt32 removedLive;
		UInt32 clearedSlots;
		UInt32 skipped;
		UInt32 errors;

		ActorPathChangeResult() : folderMode(false), changed(false), loaded(false), found(0), added(0), removed(0), persistedRemoved(0), removedLive(0), clearedSlots(0), skipped(0), errors(0) {}
	};

	struct WeaponScopedApplyResult
	{
		UInt32 mappings;
		UInt32 found;
		UInt32 added;
		UInt32 existing;
		UInt32 invalid;
		UInt32 errors;
		bool loaded;

		WeaponScopedApplyResult() : mappings(0), found(0), added(0), existing(0), invalid(0), errors(0), loaded(false) {}
	};

	struct ScopedRevertResult
	{
		UInt32 attempts;
		UInt32 paths;
		UInt32 removedKFFZ;
		UInt32 removedLive;
		UInt32 clearedSlots;
		UInt32 restoredVanilla;
		UInt32 errors;

		ScopedRevertResult() : attempts(0), paths(0), removedKFFZ(0), removedLive(0), clearedSlots(0), restoredVanilla(0), errors(0) {}

		void Add(const ScopedRevertResult& other)
		{
			attempts += other.attempts;
			paths += other.paths;
			removedKFFZ += other.removedKFFZ;
			removedLive += other.removedLive;
			clearedSlots += other.clearedSlots;
			restoredVanilla += other.restoredVanilla;
			errors += other.errors;
		}
	};

	struct TargetScopedApplyResult
	{
		UInt32 mappings;
		UInt32 matched;
		UInt32 foundTargets;
		UInt32 loadedTargets;
		UInt32 found;
		UInt32 added;
		UInt32 existing;
		UInt32 invalid;
		UInt32 errors;
		bool loaded;

		TargetScopedApplyResult() : mappings(0), matched(0), foundTargets(0), loadedTargets(0), found(0), added(0), existing(0), invalid(0), errors(0), loaded(false) {}
	};

	struct TargetMappingDiagnostics
	{
		UInt32 total;
		UInt32 race;
		UInt32 global;
		UInt32 persisted;
		UInt32 config;
		UInt32 invalid;

		TargetMappingDiagnostics() : total(0), race(0), global(0), persisted(0), config(0), invalid(0) {}
	};

	struct TargetMatchDiagnostics
	{
		UInt32 configured;
		UInt32 matchedMappings;
		UInt32 matchedTargets;
		UInt32 race;
		UInt32 global;
		UInt32 invalid;
		UInt32 actorBaseFormID;
		UInt32 actorRaceFormID;

		TargetMatchDiagnostics() : configured(0), matchedMappings(0), matchedTargets(0), race(0), global(0), invalid(0), actorBaseFormID(0), actorRaceFormID(0) {}
	};

	struct TargetValidationDiagnostics
	{
		UInt32 configured;
		UInt32 matchedMappings;
		UInt32 matchedTargets;
		UInt32 checkedMappings;
		UInt32 validMappings;
		UInt32 foundKfs;
		UInt32 invalidMappings;
		UInt32 race;
		UInt32 global;
		UInt32 errors;
		UInt32 actorBaseFormID;
		UInt32 actorRaceFormID;

		TargetValidationDiagnostics() : configured(0), matchedMappings(0), matchedTargets(0), checkedMappings(0), validMappings(0), foundKfs(0), invalidMappings(0), race(0), global(0), errors(0), actorBaseFormID(0), actorRaceFormID(0) {}
	};

	struct AutoWeaponRuleValidationDiagnostics
	{
		UInt32 configuredRules;
		UInt32 matchedRules;
		UInt32 checkedRules;
		UInt32 validRules;
		UInt32 foundKfs;
		UInt32 invalidRules;
		UInt32 errors;
		UInt32 actorBaseFormID;
		UInt32 weaponFormID;
		UInt32 weaponSource;
		UInt32 weaponProcess;
		UInt32 weaponEntryData;

		AutoWeaponRuleValidationDiagnostics() : configuredRules(0), matchedRules(0), checkedRules(0), validRules(0), foundKfs(0), invalidRules(0), errors(0), actorBaseFormID(0), weaponFormID(0), weaponSource(0), weaponProcess(0), weaponEntryData(0) {}
	};

	struct KNVSELayoutResult
	{
		UInt32 files;
		UInt32 entries;
		UInt32 targets;
		UInt32 folders;
		UInt32 kfs;
		UInt32 firstPersonKfs;
		UInt32 nativeReady;
		UInt32 requiresStaging;
		UInt32 unsupported;
		UInt32 ignoredFields;
		UInt32 conditionFields;
		UInt32 conditionalEntries;
		UInt32 errors;

		KNVSELayoutResult() : files(0), entries(0), targets(0), folders(0), kfs(0), firstPersonKfs(0), nativeReady(0), requiresStaging(0), unsupported(0), ignoredFields(0), conditionFields(0), conditionalEntries(0), errors(0) {}
	};

	struct KNVSEStageResult
	{
		UInt32 files;
		UInt32 entries;
		UInt32 targets;
		UInt32 found;
		UInt32 copied;
		UInt32 existing;
		UInt32 validated;
		UInt32 registered;
		UInt32 invalid;
		UInt32 skipped;
		UInt32 unsupported;
		UInt32 ignoredFields;
		UInt32 conditionFields;
		UInt32 conditionalEntries;
		UInt32 errors;

		KNVSEStageResult() : files(0), entries(0), targets(0), found(0), copied(0), existing(0), validated(0), registered(0), invalid(0), skipped(0), unsupported(0), ignoredFields(0), conditionFields(0), conditionalEntries(0), errors(0) {}
	};

	struct KNVSEFolderStats
	{
		UInt32 kfs;
		UInt32 firstPersonKfs;
		UInt32 conditionalKfs;
		UInt32 stageableKfs;

		KNVSEFolderStats() : kfs(0), firstPersonKfs(0), conditionalKfs(0), stageableKfs(0) {}
	};

	static std::vector<PersistentAnimEntry> s_persistentAnims;
	static std::vector<WeaponAnimEntry> s_weaponAnimEntries;
	static std::vector<WeaponAnimAttempt> s_weaponAnimAttempts;
	static std::vector<TargetAnimEntry> s_targetAnimEntries;
	static std::vector<TargetAnimAttempt> s_targetAnimAttempts;
	static UInt32 s_weaponAnimGeneration = 1;
	static UInt32 s_targetAnimGeneration = 1;
	static bool s_weaponScopedApplyGuard = false;
	static bool s_targetScopedApplyGuard = false;
	static UInt32 s_weaponScopedProbeLogCount = 0;
	static UInt32 s_weaponScopedPowerReassertLogCount = 0;
	static UInt32 s_weaponScopedAttackPreferLogCount = 0;
	static UInt32 s_weaponScopedAttackFallbackLogCount = 0;
	static UInt32 s_targetScopedPowerReassertLogCount = 0;
	static UInt32 s_targetScopedAttackPreferLogCount = 0;
	static UInt32 s_targetScopedAttackFallbackLogCount = 0;

	static void AddDiscoverResult(ManifestResult* manifest, const DiscoverResult& discovered)
	{
		if (!manifest)
			return;

		manifest->found += discovered.found;
		manifest->added += discovered.added;
		manifest->skipped += discovered.skipped;
		manifest->errors += discovered.errors;
	}

	static void AddManifestResult(ManifestResult* aggregate, const ManifestResult& loaded)
	{
		if (!aggregate)
			return;

		aggregate->files += loaded.files;
		aggregate->entries += loaded.entries;
		aggregate->targets += loaded.targets;
		aggregate->found += loaded.found;
		aggregate->added += loaded.added;
		aggregate->skipped += loaded.skipped;
		aggregate->unsupported += loaded.unsupported;
		aggregate->errors += loaded.errors;
		aggregate->ignoredFields += loaded.ignoredFields;
		aggregate->conditionFields += loaded.conditionFields;
		aggregate->conditionalEntries += loaded.conditionalEntries;
	}

	static bool EntryHasUnsupportedConditionPolling(const ManifestEntry& entry)
	{
		return entry.hasCondition || entry.hasPollCondition;
	}

	static bool LoadSpecialAnimationsForRef(TESObjectREFR* ref, TESActorBase* explicitBase);
	static MiddleHighProcess* ExtractMiddleHighProcess(TESObjectREFR* ref);
	static ActorAnimData* GetActorAnimData(TESObjectREFR* ref);
	static ActorAnimData* GetActorBaseSpecialAnimData(TESObjectREFR* ref);
	static bool IsActorReference(TESObjectREFR* ref);
	static TESObjectWEAP* GetEquippedWeaponForRef(TESObjectREFR* ref, UInt32* outSource = NULL, UInt32* outProcess = NULL, UInt32* outEntryData = NULL);
	static bool RefOwnsActorBaseSpecialAnimData(TESObjectREFR* ref, ActorAnimData* animData);

	static TESForm* GetAnimIdleForm(UInt32 idleObject)
	{
		return idleObject ? *(TESForm**)(idleObject + kAnimIdleFormOffset) : NULL;
	}

	static bool IsIdleInactive(ActorAnimData* animData)
	{
		return !animData || ThisStdCall(kActorAnimDataIdleInactive, animData) != 0;
	}

	static bool IsNativePickIdleGateOpen(ActorAnimData* animData, TESIdleForm* idle, bool printToConsole)
	{
		if (!animData || !idle)
			return false;

		UInt32 queuedIdleObject = animData->unkC8[2];
		if (GetAnimIdleForm(queuedIdleObject) == idle)
		{
			if (printToConsole)
				Console_Print("CASPlayIdleForm: IDLE %08X is already queued", idle->refID);
			return false;
		}

		if (IsIdleInactive(animData))
			return true;

		UInt32 currentIdleObject = animData->unkC8[1];
		if (!currentIdleObject)
		{
			if (printToConsole)
				Console_Print("CASPlayIdleForm: native PickIdle gate blocked by pending queued idle");
			return false;
		}

		if (*(UInt32*)(currentIdleObject + kAnimIdleField04Offset) != 3)
			return true;

		UInt32 currentSequence = *(UInt32*)(currentIdleObject + kAnimIdleSequenceOffset);
		if (currentSequence && !*(UInt32*)(currentSequence + kBSAnimGroupSequenceField24Offset))
			return true;

		if (printToConsole)
			Console_Print("CASPlayIdleForm: native PickIdle gate blocked by active current idle");
		return false;
	}

	static bool IsPersistableAnimationPath(const char* path)
	{
		return path && path[0] && IsSafeRelativePath(path) && IsKFPath(path);
	}

	static bool AddPersistentAnimation(TESActorBase* actorBase, const char* animPath)
	{
		if (!actorBase || !actorBase->refID || !IsPersistableAnimationPath(animPath))
			return false;

		for (std::vector<PersistentAnimEntry>::const_iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end(); ++it)
		{
			if (it->formID == actorBase->refID && std::strcmp(it->animPath.c_str(), animPath) == 0)
				return false;
		}

		s_persistentAnims.push_back(PersistentAnimEntry(actorBase->refID, animPath));
		return true;
	}

	static UInt32 RemovePersistentAnimation(TESActorBase* actorBase, const char* animPath)
	{
		if (!actorBase || !actorBase->refID || !animPath || !animPath[0])
			return 0;

		UInt32 removed = 0;
		for (std::vector<PersistentAnimEntry>::iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end();)
		{
			if (it->formID == actorBase->refID && std::strcmp(it->animPath.c_str(), animPath) == 0)
			{
				it = s_persistentAnims.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		return removed;
	}

	static UInt32 RemovePersistentAnimationsInFolder(TESActorBase* actorBase, const char* folder)
	{
		if (!actorBase || !actorBase->refID || !folder || !folder[0])
			return 0;

		UInt32 removed = 0;
		for (std::vector<PersistentAnimEntry>::iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end();)
		{
			if (it->formID == actorBase->refID && IsPersistableAnimationPath(it->animPath.c_str()) && PathMatchesOptionalFolder(it->animPath.c_str(), folder))
			{
				it = s_persistentAnims.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		return removed;
	}

	static UInt32 CountPersistentAnimations(TESActorBase* actorBase)
	{
		if (!actorBase)
			return s_persistentAnims.size();

		UInt32 count = 0;
		for (std::vector<PersistentAnimEntry>::const_iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end(); ++it)
		{
			if (it->formID == actorBase->refID)
				++count;
		}

		return count;
	}

	static UInt32 ClearPersistentAnimations(TESActorBase* actorBase)
	{
		if (!actorBase)
		{
			UInt32 removed = s_persistentAnims.size();
			s_persistentAnims.clear();
			return removed;
		}

		UInt32 removed = 0;
		for (std::vector<PersistentAnimEntry>::iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end();)
		{
			if (it->formID == actorBase->refID)
			{
				it = s_persistentAnims.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		return removed;
	}

	static bool HasQueuedRemoval(const std::vector<std::string>& removals, const char* animPath)
	{
		if (!animPath)
			return false;

		for (std::vector<std::string>::const_iterator it = removals.begin(); it != removals.end(); ++it)
		{
			if (std::strcmp(it->c_str(), animPath) == 0)
				return true;
		}

		return false;
	}

	class ManifestParser
	{
	public:
		ManifestParser(const std::string& text) :
			m_begin(text.c_str()), m_cur(text.c_str()), m_end(text.c_str() + text.size())
		{
		}

		bool Parse(std::vector<ManifestEntry>* entries)
		{
			if (!entries)
				return false;

			SkipWhitespace();
			if (Consume('['))
			{
				if (!ParseEntryArray(entries))
					return false;
			}
			else if (Peek() == '{')
			{
				if (!ParseEntryObject(entries))
					return false;
			}
			else
			{
				return false;
			}

			SkipWhitespace();
			return m_cur == m_end;
		}

	private:
		const char* m_begin;
		const char* m_cur;
		const char* m_end;

		char Peek() const
		{
			return m_cur < m_end ? *m_cur : 0;
		}

		void SkipWhitespace()
		{
			while (m_cur < m_end && std::isspace((unsigned char)*m_cur))
				++m_cur;
		}

		bool Consume(char value)
		{
			SkipWhitespace();
			if (Peek() != value)
				return false;

			++m_cur;
			return true;
		}

		bool ParseString(std::string* out)
		{
			if (!out || !Consume('"'))
				return false;

			out->clear();
			while (m_cur < m_end)
			{
				char value = *m_cur++;
				if (value == '"')
					return true;

				if (value == '\\')
				{
					if (m_cur >= m_end)
						return false;

					char escaped = *m_cur++;
					switch (escaped)
					{
					case '"':
					case '\\':
					case '/':
						out->push_back(escaped);
						break;
					case 'b':
						out->push_back('\b');
						break;
					case 'f':
						out->push_back('\f');
						break;
					case 'n':
						out->push_back('\n');
						break;
					case 'r':
						out->push_back('\r');
						break;
					case 't':
						out->push_back('\t');
						break;
					case 'u':
						if ((m_end - m_cur) < 4)
							return false;
						m_cur += 4;
						out->push_back('?');
						break;
					default:
						return false;
					}
				}
				else
				{
					out->push_back(value);
				}
			}

			return false;
		}

		bool ParseLiteral(const char* literal)
		{
			const char* start = m_cur;
			while (*literal)
			{
				if (m_cur >= m_end || *m_cur != *literal)
				{
					m_cur = start;
					return false;
				}

				++m_cur;
				++literal;
			}

			return true;
		}

		bool SkipNumber()
		{
			const char* start = m_cur;
			if (Peek() == '-')
				++m_cur;

			while (m_cur < m_end && std::isdigit((unsigned char)*m_cur))
				++m_cur;

			if (Peek() == '.')
			{
				++m_cur;
				while (m_cur < m_end && std::isdigit((unsigned char)*m_cur))
					++m_cur;
			}

			if (Peek() == 'e' || Peek() == 'E')
			{
				++m_cur;
				if (Peek() == '+' || Peek() == '-')
					++m_cur;

				while (m_cur < m_end && std::isdigit((unsigned char)*m_cur))
					++m_cur;
			}

			return m_cur != start;
		}

		bool SkipValue()
		{
			SkipWhitespace();
			if (Peek() == '"')
			{
				std::string ignored;
				return ParseString(&ignored);
			}

			if (Consume('{'))
			{
				SkipWhitespace();
				if (Consume('}'))
					return true;

				while (m_cur < m_end)
				{
					std::string key;
					if (!ParseString(&key) || !Consume(':') || !SkipValue())
						return false;

					if (Consume('}'))
						return true;

					if (!Consume(','))
						return false;
				}

				return false;
			}

			if (Consume('['))
			{
				SkipWhitespace();
				if (Consume(']'))
					return true;

				while (m_cur < m_end)
				{
					if (!SkipValue())
						return false;

					if (Consume(']'))
						return true;

					if (!Consume(','))
						return false;
				}

				return false;
			}

			return ParseLiteral("true") || ParseLiteral("false") || ParseLiteral("null") || SkipNumber();
		}

		bool ParseFormTargets(ManifestEntry* entry)
		{
			if (!entry)
				return false;

			SkipWhitespace();
			if (Peek() == '"')
			{
				std::string form;
				if (!ParseString(&form))
					return false;

				entry->forms.push_back(form);
				return true;
			}

			if (!Consume('['))
				return SkipValue();

			SkipWhitespace();
			if (Consume(']'))
				return true;

			while (m_cur < m_end)
			{
				std::string form;
				if (Peek() == '"')
				{
					if (!ParseString(&form))
						return false;

					entry->forms.push_back(form);
				}
				else if (!SkipValue())
				{
					return false;
				}

				if (Consume(']'))
					return true;

				if (!Consume(','))
					return false;
			}

			return false;
		}

		bool ParseEntryArray(std::vector<ManifestEntry>* entries)
		{
			SkipWhitespace();
			if (Consume(']'))
				return true;

			while (m_cur < m_end)
			{
				if (Peek() == '{')
				{
					if (!ParseEntryObject(entries))
						return false;
				}
				else if (!SkipValue())
				{
					return false;
				}

				if (Consume(']'))
					return true;

				if (!Consume(','))
					return false;
			}

			return false;
		}

		bool ParseEntryObject(std::vector<ManifestEntry>* entries)
		{
			if (!entries || !Consume('{'))
				return false;

			ManifestEntry entry;
			bool hasFolder = false;
			bool hasNestedEntries = false;

			SkipWhitespace();
			if (Consume('}'))
				return true;

			while (m_cur < m_end)
			{
				std::string key;
				if (!ParseString(&key) || !Consume(':'))
					return false;

				if (_stricmp(key.c_str(), "folder") == 0)
				{
					if (!ParseString(&entry.folder))
						return false;
					hasFolder = true;
				}
				else if (_stricmp(key.c_str(), "mod") == 0)
				{
					if (!ParseString(&entry.modName))
						return false;
				}
				else if (_stricmp(key.c_str(), "weaponNameContains") == 0 ||
					_stricmp(key.c_str(), "nameContains") == 0 ||
					_stricmp(key.c_str(), "weaponName") == 0)
				{
					if (!ParseString(&entry.weaponNameContains))
						return false;
				}
				else if (_stricmp(key.c_str(), "form") == 0 || _stricmp(key.c_str(), "forms") == 0)
				{
					if (!ParseFormTargets(&entry))
						return false;
				}
				else if (_stricmp(key.c_str(), "condition") == 0)
				{
					entry.hasCondition = true;
					++entry.ignoredFields;
					++entry.conditionFields;
					if (Peek() == '"')
					{
						std::string ignored;
						if (!ParseString(&ignored))
							return false;
					}
					else if (!SkipValue())
					{
						return false;
					}
				}
				else if (_stricmp(key.c_str(), "pollCondition") == 0)
				{
					entry.hasPollCondition = true;
					++entry.ignoredFields;
					++entry.conditionFields;
					if (!SkipValue())
						return false;
				}
				else if (_stricmp(key.c_str(), "priority") == 0)
				{
					entry.hasPriority = true;
					++entry.ignoredFields;
					if (!SkipValue())
						return false;
				}
				else if (_stricmp(key.c_str(), "bsa") == 0)
				{
					entry.hasBSA = true;
					++entry.ignoredFields;
					if (!SkipValue())
						return false;
				}
				else if (_stricmp(key.c_str(), "matchBaseAnimGroup") == 0)
				{
					entry.hasMatchBaseAnimGroup = true;
					++entry.ignoredFields;
					if (!SkipValue())
						return false;
				}
				else if (_stricmp(key.c_str(), "entries") == 0 || _stricmp(key.c_str(), "animations") == 0)
				{
					if (!Consume('[') || !ParseEntryArray(entries))
						return false;
					hasNestedEntries = true;
				}
				else
				{
					++entry.ignoredFields;
					if (!SkipValue())
						return false;
				}

				if (Consume('}'))
				{
					if (hasFolder)
						entries->push_back(entry);
					return hasFolder || hasNestedEntries;
				}

				if (!Consume(','))
					return false;
			}

			return false;
		}
	};

	static void AppendRelativePath(char* outPath, UInt32 outLen, const char* prefix, const char* fileName)
	{
		if (!prefix || !prefix[0])
			strncpy_s(outPath, outLen, fileName, _TRUNCATE);
		else
			sprintf_s(outPath, outLen, "%s\\%s", prefix, fileName);

		NormalizeSlashes(outPath);
	}

	static void TrimTrailingSlashes(char* path)
	{
		if (!path)
			return;

		UInt32 len = std::strlen(path);
		while (len > 0 && IsSlash(path[len - 1]))
			path[--len] = 0;
	}

	static bool NormalizeRelativeFolder(const char* folder, char* outFolder, UInt32 outLen)
	{
		if (!outFolder || outLen == 0)
			return false;

		outFolder[0] = 0;
		if (!folder || !folder[0])
			return true;

		strncpy_s(outFolder, outLen, folder, _TRUNCATE);
		NormalizeSlashes(outFolder);
		while (IsSlash(outFolder[0]))
			std::memmove(outFolder, outFolder + 1, std::strlen(outFolder));
		TrimTrailingSlashes(outFolder);

		return !outFolder[0] || IsSafeRelativePath(outFolder);
	}

	static void BumpWeaponAnimationGeneration()
	{
		++s_weaponAnimGeneration;
		if (!s_weaponAnimGeneration)
			s_weaponAnimGeneration = 1;
	}

	static void BumpTargetAnimationGeneration()
	{
		++s_targetAnimGeneration;
		if (!s_targetAnimGeneration)
			s_targetAnimGeneration = 1;
	}

	static void BumpAutoWeaponAnimationGeneration()
	{
		++s_autoWeaponAnimGeneration;
		if (!s_autoWeaponAnimGeneration)
			s_autoWeaponAnimGeneration = 1;
	}

	static bool NormalizeWeaponAnimationPath(const char* path, char* outPath, UInt32 outLen, bool* outFolderMode)
	{
		if (outFolderMode)
			*outFolderMode = false;
		if (!outPath || outLen == 0)
			return false;

		outPath[0] = 0;
		if (!path || !path[0])
			return false;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		strncpy_s(cleanPath, sizeof(cleanPath), path, _TRUNCATE);
		NormalizeSlashes(cleanPath);

		if (IsKFPath(cleanPath))
		{
			if (!IsPersistableAnimationPath(cleanPath))
				return false;

			strncpy_s(outPath, outLen, cleanPath, _TRUNCATE);
			return outPath[0] != 0;
		}

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(cleanPath, cleanFolder, sizeof(cleanFolder)) || !cleanFolder[0])
			return false;

		if (outFolderMode)
			*outFolderMode = true;
		strncpy_s(outPath, outLen, cleanFolder, _TRUNCATE);
		return outPath[0] != 0;
	}

	static bool IsWeaponForm(TESForm* form)
	{
		return form && form->typeID == kFormType_Weapon;
	}

	static UInt32 GetTargetAnimScopeForForm(TESForm* form)
	{
		if (!form)
			return kTargetAnimScope_None;

		if (form->typeID == kFormType_Race)
			return kTargetAnimScope_Race;
		if (form->typeID == kFormType_Global)
			return kTargetAnimScope_Global;

		return kTargetAnimScope_None;
	}

	static const char* GetTargetAnimScopeName(UInt32 scope)
	{
		switch (scope)
		{
		case kTargetAnimScope_Race:
			return "Race";
		case kTargetAnimScope_Global:
			return "Global";
		default:
			return "Unknown";
		}
	}

	static bool IsTargetAnimForm(TESForm* form)
	{
		return GetTargetAnimScopeForForm(form) != kTargetAnimScope_None;
	}

	static bool HasWeaponAnimationMapping(UInt32 weaponFormID)
	{
		if (!weaponFormID)
			return false;

		for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
		{
			if (it->weaponFormID == weaponFormID)
				return true;
		}

		return false;
	}

	static UInt32 CountWeaponAnimationMappings(UInt32 weaponFormID)
	{
		UInt32 count = 0;
		for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
		{
			if (!weaponFormID || it->weaponFormID == weaponFormID)
				++count;
		}

		return count;
	}

	static bool HasTargetAnimationMapping(UInt32 targetFormID, UInt32 scope)
	{
		if (!targetFormID || scope == kTargetAnimScope_None)
			return false;

		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (it->targetFormID == targetFormID && it->scope == scope)
				return true;
		}

		return false;
	}

	static UInt32 CountTargetAnimationMappings(UInt32 targetFormID)
	{
		UInt32 count = 0;
		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (!targetFormID || it->targetFormID == targetFormID)
				++count;
		}

		return count;
	}

	static TargetMappingDiagnostics GetTargetAnimationMappingDiagnostics(TESForm* target)
	{
		TargetMappingDiagnostics diagnostics;
		UInt32 targetFormID = target ? target->refID : 0;
		UInt32 targetScope = target ? GetTargetAnimScopeForForm(target) : kTargetAnimScope_None;

		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (targetFormID && (it->targetFormID != targetFormID || it->scope != targetScope))
				continue;

			++diagnostics.total;
			if (it->persist)
				++diagnostics.persisted;
			else
				++diagnostics.config;

			TESForm* entryTarget = LookupFormByID(it->targetFormID);
			if (!entryTarget || GetTargetAnimScopeForForm(entryTarget) != it->scope)
			{
				++diagnostics.invalid;
				continue;
			}

			if (it->scope == kTargetAnimScope_Race)
				++diagnostics.race;
			else if (it->scope == kTargetAnimScope_Global)
				++diagnostics.global;
			else
				++diagnostics.invalid;
		}

		return diagnostics;
	}

	static void EnsureDefaultAutoWeaponAnimationRules()
	{
		if (s_autoWeaponDefaultRuleInitialized)
			return;

		s_autoWeaponDefaultRuleInitialized = true;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (NormalizeWeaponAnimationPath(kDefaultAutoWeaponSpecialAnimFolder, cleanPath, sizeof(cleanPath), NULL))
			s_autoWeaponAnimRules.push_back(AutoWeaponAnimRule(kDefaultAutoWeaponNameNeedle, cleanPath, false, true));
	}

	static bool AutoWeaponAnimationRuleEquals(const AutoWeaponAnimRule& rule, const char* nameContains, const char* animPath)
	{
		return !rule.nameContains.empty() &&
			!rule.animPath.empty() &&
			nameContains && nameContains[0] &&
			animPath && animPath[0] &&
			_stricmp(rule.nameContains.c_str(), nameContains) == 0 &&
			_stricmp(rule.animPath.c_str(), animPath) == 0;
	}

	static bool AddAutoWeaponAnimationRule(const char* nameContains, const char* path, bool persist, bool builtin)
	{
		EnsureDefaultAutoWeaponAnimationRules();

		if (!nameContains || !nameContains[0])
			return false;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), NULL))
			return false;

		for (std::vector<AutoWeaponAnimRule>::iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end(); ++it)
		{
			if (!AutoWeaponAnimationRuleEquals(*it, nameContains, cleanPath))
				continue;

			bool changed = false;
			if (persist && !it->persist)
			{
				it->persist = true;
				changed = true;
			}
			if (builtin && !it->builtin)
			{
				it->builtin = true;
				changed = true;
			}
			if (changed)
				BumpAutoWeaponAnimationGeneration();
			return changed;
		}

		s_autoWeaponAnimRules.push_back(AutoWeaponAnimRule(nameContains, cleanPath, persist, builtin));
		BumpAutoWeaponAnimationGeneration();
		return true;
	}

	static UInt32 RemoveAutoWeaponAnimationRule(const char* nameContains, const char* path)
	{
		EnsureDefaultAutoWeaponAnimationRules();

		if (!nameContains || !nameContains[0])
			return 0;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), NULL))
			return 0;

		UInt32 removed = 0;
		for (std::vector<AutoWeaponAnimRule>::iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end();)
		{
			if (AutoWeaponAnimationRuleEquals(*it, nameContains, cleanPath))
			{
				it = s_autoWeaponAnimRules.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		if (removed)
			BumpAutoWeaponAnimationGeneration();
		return removed;
	}

	static UInt32 ClearConfigAutoWeaponAnimationRules()
	{
		EnsureDefaultAutoWeaponAnimationRules();

		UInt32 removed = 0;
		for (std::vector<AutoWeaponAnimRule>::iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end();)
		{
			if (!it->persist && !it->builtin)
			{
				it = s_autoWeaponAnimRules.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		if (removed)
			BumpAutoWeaponAnimationGeneration();
		return removed;
	}

	static UInt32 ClearPersistentAutoWeaponAnimationRules()
	{
		EnsureDefaultAutoWeaponAnimationRules();

		UInt32 removed = 0;
		for (std::vector<AutoWeaponAnimRule>::iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end();)
		{
			if (it->persist)
			{
				it = s_autoWeaponAnimRules.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		if (removed)
			BumpAutoWeaponAnimationGeneration();
		return removed;
	}

	static UInt32 CountAutoWeaponAnimationRules(const char* nameContains)
	{
		EnsureDefaultAutoWeaponAnimationRules();

		UInt32 count = 0;
		for (std::vector<AutoWeaponAnimRule>::const_iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end(); ++it)
		{
			if (!nameContains || !nameContains[0] || _stricmp(it->nameContains.c_str(), nameContains) == 0)
				++count;
		}

		return count;
	}

	static void FindAutoWeaponAnimationRulesForWeapon(TESObjectWEAP* weapon, std::vector<AutoWeaponAnimRule>* outRules)
	{
		if (outRules)
			outRules->clear();
		if (!weapon || !outRules)
			return;

		EnsureDefaultAutoWeaponAnimationRules();

		const char* weaponName = GetFullName(weapon);
		if (!weaponName || !weaponName[0])
			return;

		for (std::vector<AutoWeaponAnimRule>::const_iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end(); ++it)
		{
			if (!it->nameContains.empty() && !it->animPath.empty() && ContainsCaseInsensitive(weaponName, it->nameContains.c_str()))
				outRules->push_back(*it);
		}
	}

	static bool HasAutoWeaponAnimationRuleForAttempt(const AutoWeaponAnimAttempt& attempt)
	{
		EnsureDefaultAutoWeaponAnimationRules();

		for (std::vector<std::string>::const_iterator pathIt = attempt.animPaths.begin(); pathIt != attempt.animPaths.end(); ++pathIt)
		{
			for (std::vector<std::string>::const_iterator nameIt = attempt.nameContains.begin(); nameIt != attempt.nameContains.end(); ++nameIt)
			{
				for (std::vector<AutoWeaponAnimRule>::const_iterator ruleIt = s_autoWeaponAnimRules.begin(); ruleIt != s_autoWeaponAnimRules.end(); ++ruleIt)
				{
					if (AutoWeaponAnimationRuleEquals(*ruleIt, nameIt->c_str(), pathIt->c_str()))
						return true;
				}
			}
		}

		return false;
	}

	static bool AddWeaponAnimationMapping(TESForm* weapon, UInt32 firstPerson, const char* path)
	{
		if (!IsWeaponForm(weapon))
			return false;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), NULL))
			return false;

		for (std::vector<WeaponAnimEntry>::iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
		{
			if (it->weaponFormID == weapon->refID && _stricmp(it->animPath.c_str(), cleanPath) == 0)
			{
				UInt32 cleanFirstPerson = firstPerson ? 1 : 0;
				if (it->firstPerson != cleanFirstPerson)
				{
					it->firstPerson = cleanFirstPerson;
					BumpWeaponAnimationGeneration();
				}
				return false;
			}
		}

		s_weaponAnimEntries.push_back(WeaponAnimEntry(weapon->refID, firstPerson ? 1 : 0, cleanPath));
		BumpWeaponAnimationGeneration();
		return true;
	}

	static bool AddTargetAnimationMapping(TESForm* target, const char* path, bool persist)
	{
		UInt32 scope = GetTargetAnimScopeForForm(target);
		if (!target || scope == kTargetAnimScope_None)
			return false;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), NULL))
			return false;

		for (std::vector<TargetAnimEntry>::iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (it->targetFormID == target->refID && it->scope == scope && _stricmp(it->animPath.c_str(), cleanPath) == 0)
			{
				if (persist && !it->persist)
				{
					it->persist = true;
					BumpTargetAnimationGeneration();
					return true;
				}
				return false;
			}
		}

		s_targetAnimEntries.push_back(TargetAnimEntry(target->refID, scope, persist, cleanPath));
		BumpTargetAnimationGeneration();
		return true;
	}

	static UInt32 RemoveTargetAnimationMapping(TESForm* target, const char* path)
	{
		UInt32 scope = GetTargetAnimScopeForForm(target);
		if (!target || scope == kTargetAnimScope_None)
			return 0;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), NULL))
			return 0;

		UInt32 removed = 0;
		for (std::vector<TargetAnimEntry>::iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end();)
		{
			if (it->targetFormID == target->refID && it->scope == scope && _stricmp(it->animPath.c_str(), cleanPath) == 0)
			{
				it = s_targetAnimEntries.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		if (removed)
			BumpTargetAnimationGeneration();
		return removed;
	}

	static UInt32 RemoveWeaponAnimationMapping(TESForm* weapon, const char* path)
	{
		if (!IsWeaponForm(weapon))
			return 0;

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), NULL))
			return 0;

		UInt32 removed = 0;
		for (std::vector<WeaponAnimEntry>::iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end();)
		{
			if (it->weaponFormID == weapon->refID && _stricmp(it->animPath.c_str(), cleanPath) == 0)
			{
				it = s_weaponAnimEntries.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		if (removed)
			BumpWeaponAnimationGeneration();
		return removed;
	}

	static UInt32 ClearWeaponAnimationMappings()
	{
		UInt32 removed = s_weaponAnimEntries.size();
		s_weaponAnimEntries.clear();
		if (removed)
			BumpWeaponAnimationGeneration();
		return removed;
	}

	static UInt32 ClearTargetAnimationMappings()
	{
		UInt32 removed = s_targetAnimEntries.size();
		s_targetAnimEntries.clear();
		if (removed)
			BumpTargetAnimationGeneration();
		return removed;
	}

	static UInt32 ClearConfigTargetAnimationMappings()
	{
		UInt32 removed = 0;
		for (std::vector<TargetAnimEntry>::iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end();)
		{
			if (!it->persist)
			{
				it = s_targetAnimEntries.erase(it);
				++removed;
			}
			else
			{
				++it;
			}
		}

		if (removed)
			BumpTargetAnimationGeneration();
		return removed;
	}

	static void ClearWeaponAnimationAttempts()
	{
		s_weaponAnimAttempts.clear();
		s_weaponScopedProbeLogCount = 0;
		s_weaponScopedPowerReassertLogCount = 0;
		s_weaponScopedAttackPreferLogCount = 0;
		s_weaponScopedAttackFallbackLogCount = 0;
	}

	static void ClearTargetAnimationAttempts()
	{
		s_targetAnimAttempts.clear();
		s_targetScopedPowerReassertLogCount = 0;
		s_targetScopedAttackPreferLogCount = 0;
		s_targetScopedAttackFallbackLogCount = 0;
	}

	static bool DeriveIdleAnimsRoot(const char* modelPath, char* outRoot, UInt32 outLen)
	{
		if (!outRoot || outLen == 0)
			return false;

		outRoot[0] = 0;
		if (!modelPath || !modelPath[0])
			return false;

		char path[260];
		strncpy_s(path, sizeof(path), modelPath, _TRUNCATE);

		char* idleAnims = std::strstr(path, "IdleAnims");
		if (idleAnims)
		{
			idleAnims[9] = 0;
			strncpy_s(outRoot, outLen, path, _TRUNCATE);
			return outRoot[0] != 0;
		}

		char* lastSlash = std::strrchr(path, '\\');
		if (!lastSlash)
			return false;

		lastSlash[1] = 0;
		return sprintf_s(outRoot, outLen, "%sIdleAnims", path) > 0;
	}

	static UInt32 LoadKFModel(const char* loaderPath)
	{
		if (!loaderPath || !loaderPath[0])
			return 0;

		ModelLoader* loader = ModelLoader::GetSingleton();
		if (!loader)
			return 0;

		return ThisStdCall(kModelLoaderLoadKF, loader, loaderPath);
	}

	static void ReleaseKFModel(UInt32 kfModel)
	{
		if (kfModel)
			InterlockedDecrement((volatile LONG*)(kfModel + kKFModelRefCountOffset));
	}

	static NiControllerSequence* GetKFModelControllerSequence(UInt32 kfModel)
	{
		return kfModel ? *(NiControllerSequence**)(kfModel + kKFModelControllerSequenceOffset) : NULL;
	}

	static TESAnimGroup* GetKFModelAnimGroup(UInt32 kfModel)
	{
		return kfModel ? *(TESAnimGroup**)(kfModel + kKFModelAnimGroupOffset) : NULL;
	}

	static UInt32 GetParsedAnimGroupID(TESAnimGroup* animGroup)
	{
		return animGroup ? *(UInt8*)((UInt32)animGroup + 0x08) : TESAnimGroup::kAnimGroup_Max;
	}

	static UInt32 GetParsedAnimGroupKey(TESAnimGroup* animGroup)
	{
		return animGroup ? *(UInt16*)((UInt32)animGroup + 0x08) : 0xFFFF;
	}

	static UInt8 GetParsedMorphKey(TESAnimGroup* animGroup)
	{
		return animGroup ? *(UInt8*)((UInt32)animGroup + 0x20) : 0;
	}

	static float GetParsedMovementLengthSquared(TESAnimGroup* animGroup)
	{
		if (!animGroup)
			return 0.0f;

		float x = *(float*)((UInt32)animGroup + 0x14);
		float y = *(float*)((UInt32)animGroup + 0x18);
		float z = *(float*)((UInt32)animGroup + 0x1C);
		return (x * x) + (y * y) + (z * z);
	}

	static UInt32 GetKFModelRefCount(UInt32 kfModel)
	{
		return kfModel ? *(UInt32*)(kfModel + kKFModelRefCountOffset) : 0;
	}

	static bool IsVolatilePowerAttackGroup(UInt32 groupID)
	{
		return groupID >= TESAnimGroup::kAnimGroup_AttackPower && groupID <= TESAnimGroup::kAnimGroup_AttackRightPower;
	}

	static bool IsNativeMovementAccumGroup(UInt32 groupID)
	{
		return groupID >= TESAnimGroup::kAnimGroup_Forward && groupID <= TESAnimGroup::kAnimGroup_DodgeRight;
	}

	static const char* GetVanillaMovementKFName(UInt32 groupID)
	{
		switch (groupID)
		{
		case TESAnimGroup::kAnimGroup_Forward:
			return "walkforward.kf";
		case TESAnimGroup::kAnimGroup_Backward:
			return "walkbackward.kf";
		case TESAnimGroup::kAnimGroup_Left:
			return "walkleft.kf";
		case TESAnimGroup::kAnimGroup_Right:
			return "walkright.kf";
		case TESAnimGroup::kAnimGroup_FastForward:
			return "walkfastforward.kf";
		case TESAnimGroup::kAnimGroup_FastBackward:
			return "walkfastbackward.kf";
		case TESAnimGroup::kAnimGroup_FastLeft:
			return "walkfastleft.kf";
		case TESAnimGroup::kAnimGroup_FastRight:
			return "walkfastright.kf";
		default:
			return NULL;
		}
	}

	static bool AnimGroupAllowsMultiple(UInt32 groupID)
	{
		if (groupID >= TESAnimGroup::kAnimGroup_Max)
			return false;

		return *(UInt8*)(kAnimGroupInfoTable + (groupID * kAnimGroupInfoSize) + 0x04) != 0;
	}

	static UInt32 CountDeferredInstallModels(ActorAnimData* animData)
	{
		if (!animData || !animData->unkB4)
			return 0;

		UInt32 count = 1;
		UInt32 node = animData->unkB8;
		while (node && count < kDeferredInstallGuard)
		{
			++count;
			node = *(UInt32*)(node + 4);
		}

		return count;
	}

	static UInt32 GetDeferredInstallKFModel(ActorAnimData* animData, UInt32 index)
	{
		if (!animData || !animData->unkB4)
			return 0;

		if (index == 0)
			return animData->unkB4;

		UInt32 node = animData->unkB8;
		for (UInt32 i = 1; node && i < kDeferredInstallGuard; ++i)
		{
			if (i == index)
				return *(UInt32*)node;
			node = *(UInt32*)(node + 4);
		}

		return 0;
	}

	static UInt32 GetAnimKeyMovementPrefix(UInt16 key)
	{
		return key >> 12;
	}

	static UInt32 GetAnimKeyWeaponPrefix(UInt16 key)
	{
		return (key >> 8) & 0xF;
	}

	static UInt32 GetAnimKeyGroupID(UInt16 key)
	{
		return key & 0xFF;
	}

	static bool IsActiveAnimKey(UInt16 key)
	{
		return GetAnimKeyGroupID(key) != 0xFF;
	}

	static bool IsNativeSavedSlotKey(UInt16 key)
	{
		return key != 0x00FF && key != 0xFFFF;
	}

	static bool LookupAnimationMapEntry(ActorAnimData* animData, UInt16 key, UInt32* outEntry)
	{
		if (outEntry)
			*outEntry = 0;

		if (!animData || !animData->map9C || !IsNativeSavedSlotKey(key))
			return false;

		UInt32 entry = 0;
		bool found = ThisStdCall(kActorAnimDataMapLookup, animData->map9C, (UInt32)key, &entry) != 0;
		if (found && outEntry)
			*outEntry = entry;

		return found && entry != 0;
	}

	struct NativeStringListNode
	{
		char* data;
		NativeStringListNode* next;
	};

	static bool ContainsAnimKey(const std::vector<UInt16>& keys, UInt16 key)
	{
		for (std::vector<UInt16>::const_iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if (*it == key)
				return true;
		}

		return false;
	}

	static void AddUniqueAnimKey(std::vector<UInt16>* keys, UInt16 key)
	{
		if (!keys || !IsNativeSavedSlotKey(key) || ContainsAnimKey(*keys, key))
			return;

		keys->push_back(key);
	}

	static bool ContainsScopedFolderKey(const std::vector<ScopedFolderKey>& keys, UInt16 key, const char* folder)
	{
		if (!folder || !folder[0])
			return false;

		for (std::vector<ScopedFolderKey>::const_iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if (it->key == key && _stricmp(it->folder.c_str(), folder) == 0)
				return true;
		}

		return false;
	}

	static void AddUniqueScopedFolderKey(std::vector<ScopedFolderKey>* keys, const ScopedInstalledAnim& installed)
	{
		if (!keys || !IsNativeSavedSlotKey(installed.key) || installed.groupID >= TESAnimGroup::kAnimGroup_Max || installed.animPath.empty())
			return;

		char folder[kMaxSpecialAnimPath];
		if (!GetSpecialAnimFolderFromAnimPath(installed.animPath.c_str(), folder, sizeof(folder)))
			return;

		if (ContainsScopedFolderKey(*keys, installed.key, folder))
			return;

		keys->push_back(ScopedFolderKey(installed.key, installed.groupID, folder));
	}

	static bool ShouldRestoreVanillaAfterScopedRemoval(const ScopedInstalledAnim& installed)
	{
		return installed.removeKFFZ &&
			IsNativeSavedSlotKey(installed.key) &&
			installed.groupID < TESAnimGroup::kAnimGroup_Max;
	}

	static bool MissingAnimKey(ActorAnimData* animData, UInt16 key)
	{
		return animData && IsNativeSavedSlotKey(key) && !LookupAnimationMapEntry(animData, key, NULL);
	}

	static UInt32 CountMissingAnimKeys(ActorAnimData* animData, const std::vector<UInt16>& keys)
	{
		UInt32 missing = 0;
		for (std::vector<UInt16>::const_iterator it = keys.begin(); it != keys.end(); ++it)
		{
			if (MissingAnimKey(animData, *it))
				++missing;
		}

		return missing;
	}

	static void FreeNativeStringList(NativeStringListNode* list)
	{
		for (NativeStringListNode* node = list; node; )
		{
			NativeStringListNode* next = node->next;
			if (node->data)
				FormHeap_Free(node->data);
			FormHeap_Free(node);
			node = next;
		}
	}

	static UInt32 RestoreVanillaActorModelSequencesForKeys(ActorAnimData* animData, TESActorBase* actorBase, const std::vector<UInt16>& keys, UInt32* outErrors)
	{
		if (!animData || !actorBase || keys.empty())
			return 0;

		UInt32 missing = CountMissingAnimKeys(animData, keys);
		if (!missing)
			return 0;

		const char* modelPath = GetActorBaseModelPath(actorBase);
		if (!modelPath || !modelPath[0])
		{
			if (outErrors)
				*outErrors += missing;
			return 0;
		}

		BuildActorModelKFListFn buildList = (BuildActorModelKFListFn)kBuildActorModelKFList;
		NativeStringListNode* list = (NativeStringListNode*)buildList(modelPath, 0);
		if (!list)
		{
			if (outErrors)
				*outErrors += missing;
			return 0;
		}

		UInt32 restored = 0;
		for (NativeStringListNode* node = list; node && missing; node = node->next)
		{
			if (!node->data || !node->data[0])
				continue;

			UInt32 kfModel = LoadKFModel(node->data);
			if (!kfModel)
				continue;

			TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
			UInt32 groupID = GetParsedAnimGroupID(animGroup);
			UInt16 parsedKey = (UInt16)GetParsedAnimGroupKey(animGroup);
			if (!ContainsAnimKey(keys, parsedKey) || !IsNativeSavedSlotKey(parsedKey) || groupID >= TESAnimGroup::kAnimGroup_Max || !MissingAnimKey(animData, parsedKey))
			{
				ReleaseKFModel(kfModel);
				continue;
			}

			ThisStdCall(kActorAnimDataInstallKFModel, animData, kfModel, 0);
			if (LookupAnimationMapEntry(animData, parsedKey, NULL))
			{
				++restored;
				--missing;
			}
		}

		FreeNativeStringList(list);
		if (missing && outErrors)
			*outErrors += missing;

		if (restored || missing)
		{
			_MESSAGE("CustomAnimSupport vanilla restore actorBase=%08X animData=%08X model=\"%s\" requested=%u restored=%u remaining=%u",
				actorBase ? actorBase->refID : 0,
				(UInt32)animData,
				modelPath,
				(UInt32)keys.size(),
				restored,
				missing);
		}

		return restored;
	}

	static bool ResolveLoadedAnimGroup(TESObjectREFR* ref, UInt32 groupID, UInt16* outKey, UInt32* outEntry)
	{
		if (outKey)
			*outKey = 0xFFFF;
		if (outEntry)
			*outEntry = 0;

		if (!ref || !IsActorReference(ref))
			return false;

		Actor* actor = OBLIVION_CAST(ref, TESObjectREFR, Actor);
		ActorAnimData* animData = GetActorAnimData(ref);
		if (!actor || !animData)
			return false;

		UInt16 encodedGroup = ThisStdCall(kActorLoadAnimGroup, actor, groupID, 0, 0) & 0xFFFF;
		if (outKey)
			*outKey = encodedGroup;
		if (!IsNativeSavedSlotKey(encodedGroup))
			return false;

		return LookupAnimationMapEntry(animData, encodedGroup, outEntry);
	}

	static UInt16 ResolveActorLoadAnimGroupKeyNoAutoHook(Actor* actor, UInt32 groupID)
	{
		if (!actor || groupID >= TESAnimGroup::kAnimGroup_Max)
			return 0xFFFF;

		if (s_actorLoadAnimGroupOriginal)
			return s_actorLoadAnimGroupOriginal(actor, groupID, 0, 0) & 0xFFFF;

		return ThisStdCall(kActorLoadAnimGroup, actor, groupID, 0, 0) & 0xFFFF;
	}

	static const char* GetPointerTableString(UInt32 table, UInt32 index, UInt32 count)
	{
		if (index >= count)
			return "";

		const char* value = *(const char**)(table + (index * sizeof(const char*)));
		return value ? value : "";
	}

	static UInt16 GetActorAnimDataKey(ActorAnimData* animData, UInt32 slot, bool queued)
	{
		if (!animData || slot >= SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*))
			return 0xFFFF;

		UInt32 offset = queued ? kActorAnimDataQueuedKeyOffset : kActorAnimDataCurrentKeyOffset;
		return *(UInt16*)((UInt32)animData + offset + (slot * sizeof(UInt16)));
	}

	static UInt32 GetAnimGroupNoteClass(UInt32 groupID)
	{
		if (groupID >= TESAnimGroup::kAnimGroup_Max)
			return 0;

		return *(UInt32*)(kAnimGroupInfoTable + (groupID * kAnimGroupInfoSize) + 0x0C);
	}

	static UInt32 GetAnimGroupRequiredNoteCount(TESAnimGroup* animGroup)
	{
		return animGroup ? *(UInt32*)((UInt32)animGroup + 0x0C) : 0;
	}

	static float* GetAnimGroupRequiredNoteTimes(TESAnimGroup* animGroup)
	{
		return animGroup ? *(float**)((UInt32)animGroup + 0x10) : NULL;
	}

	static UInt32 GetEndActionIndexForNoteClass(UInt32 noteClass)
	{
		switch (noteClass)
		{
		case 2:
		case 3:
		case 5:
			return 2;
		case 4:
			return 3;
		case 7:
			return 4;
		default:
			return 1;
		}
	}

	static char* GetControlledBlockName(NiControllerSequence* sequence, UInt32 index)
	{
		if (!sequence || index >= sequence->arraySize)
			return NULL;

		char* blockName = NULL;
		ThisStdCall(kNiControllerSequenceGetControlledBlockName, sequence, index, &blockName);
		return blockName;
	}

	static bool SkeletonHasObject(NiNode* rootNode, const char* objectName)
	{
		if (!rootNode || !objectName || !objectName[0])
			return false;

		UInt32 vtbl = *(UInt32*)rootNode;
		if (!vtbl)
			return false;

		UInt32 findObject = *(UInt32*)(vtbl + 0x58);
		if (!findObject)
			return false;

		return ThisStdCall(findObject, rootNode, objectName) != 0;
	}

	static UInt32 ValidateControllerSequenceSkeletonTargets(NiControllerSequence* sequence, NiNode* rootNode, const char* loaderPath, bool printToConsole)
	{
		if (!sequence || !rootNode)
			return 0;

		UInt32 issues = 0;
		for (UInt32 i = 0; i < sequence->arraySize; ++i)
		{
			char* blockName = GetControlledBlockName(sequence, i);
			if (blockName && blockName[0] && !SkeletonHasObject(rootNode, blockName))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimAssets: object \"%s\" in sequence but not skeleton for %s", blockName, loaderPath);
				++issues;
			}

			if (blockName)
				FormHeap_Free(blockName);
		}

		return issues;
	}

	static UInt32 ValidateNativeMovementMorphReference(TESActorBase* actorBase, TESAnimGroup* animGroup, NiControllerSequence* controllerSequence, const char* loaderPath, bool printToConsole)
	{
		if (!actorBase || !animGroup || !controllerSequence)
			return 0;

		UInt8 morphKey = GetParsedMorphKey(animGroup);
		UInt32 groupID = GetParsedAnimGroupID(animGroup);
		const char* vanillaFileName = GetVanillaMovementKFName(groupID);
		if (!morphKey || !vanillaFileName)
			return 0;

		char modelDir[260];
		if (!GetModelDirectory(actorBase, modelDir, sizeof(modelDir)))
			return 0;

		char vanillaLoaderPath[1024];
		if (sprintf_s(vanillaLoaderPath, sizeof(vanillaLoaderPath), "%s\\%s", modelDir, vanillaFileName) <= 0)
			return 0;

		if (_stricmp(vanillaLoaderPath, loaderPath) == 0)
			return 0;

		UInt32 vanillaKFModel = LoadKFModel(vanillaLoaderPath);
		if (!vanillaKFModel)
			return 0;

		UInt32 issues = 0;
		TESAnimGroup* vanillaAnimGroup = GetKFModelAnimGroup(vanillaKFModel);
		NiControllerSequence* vanillaSequence = GetKFModelControllerSequence(vanillaKFModel);
		if (vanillaAnimGroup && vanillaSequence && GetParsedMorphKey(vanillaAnimGroup) == morphKey && vanillaSequence->arraySize != controllerSequence->arraySize)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: morph controller count mismatch for %s against %s; custom=%u vanilla=%u morphKey=%u", loaderPath, vanillaLoaderPath, controllerSequence->arraySize, vanillaSequence->arraySize, morphKey);
			++issues;
		}

		ReleaseKFModel(vanillaKFModel);
		return issues;
	}

	static UInt32 ValidateParsedKFModel(TESActorBase* actorBase, UInt32 kfModel, const char* loaderPath, NiNode* skeletonRoot, bool printToConsole)
	{
		UInt32 issues = 0;
		TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
		if (!animGroup)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: no parsed TESAnimGroup in %s", loaderPath);
			return 1;
		}

		UInt32 groupID = GetParsedAnimGroupID(animGroup);
		const char* groupName = TESAnimGroup::StringForAnimGroupCode(groupID);
		if (groupID >= TESAnimGroup::kAnimGroup_Max || !groupName)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: invalid native group %04X in %s", GetParsedAnimGroupKey(animGroup), loaderPath);
			return 1;
		}

		if (IsVolatilePowerAttackGroup(groupID))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: %s in %s is volatile; Oblivion rebuilds native power attacks from weapon/skill state and can remove KFFZ-loaded entries", groupName, loaderPath);
			++issues;
		}

		NiControllerSequence* controllerSequence = GetKFModelControllerSequence(kfModel);
		if (!controllerSequence)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: missing controller sequence in %s", loaderPath);
			return 1;
		}

		if (IsNativeMovementAccumGroup(groupID) && GetParsedMovementLengthSquared(animGroup) == 0.0f)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: %s in %s has zero movement vector; native 0x472030 reports Animate in Place for movement groups", groupName, loaderPath);
			++issues;
		}

		issues += ValidateNativeMovementMorphReference(actorBase, animGroup, controllerSequence, loaderPath, printToConsole);

		if (groupID != TESAnimGroup::kAnimGroup_SpecialIdle)
		{
			UInt32 noteClass = GetAnimGroupNoteClass(groupID);
			bool isLooping = controllerSequence->cycleType == NiControllerSequence::kCycle_Loop;
			if ((noteClass == 1 && !isLooping) || (noteClass != 1 && isLooping))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimAssets: loop mode mismatch for %s in %s", groupName, loaderPath);
				++issues;
			}

			UInt32 endIndex = GetEndActionIndexForNoteClass(noteClass);
			UInt32 requiredCount = GetAnimGroupRequiredNoteCount(animGroup);
			float* requiredTimes = GetAnimGroupRequiredNoteTimes(animGroup);
			if (!requiredTimes || requiredCount <= endIndex)
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimAssets: missing required note data for %s in %s", groupName, loaderPath);
				++issues;
			}
			else if (requiredTimes[0] >= requiredTimes[endIndex])
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimAssets: Start note is not before End note for %s in %s", groupName, loaderPath);
				++issues;
			}
		}

		issues += ValidateControllerSequenceSkeletonTargets(controllerSequence, skeletonRoot, loaderPath, printToConsole);

		if (printToConsole && !issues)
			Console_Print("CASValidateSpecialAnimAssets: %s parsed as %s (%04X)", loaderPath, groupName, GetParsedAnimGroupKey(animGroup));

		return issues;
	}

	static UInt32 ValidateSpecialAnimAsset(TESActorBase* actorBase, const char* animPath, NiNode* skeletonRoot, bool printToConsole)
	{
		if (!actorBase || !animPath || !animPath[0])
			return 1;

		char loaderPath[1024];
		if (!GetSpecialAnimLoaderPath(actorBase, animPath, loaderPath, sizeof(loaderPath)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: actor base has no usable model path for \"%s\"", animPath);
			return 1;
		}

		UInt32 kfModel = LoadKFModel(loaderPath);
		if (!kfModel)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimAssets: native loader failed for %s", loaderPath);
			return 1;
		}

		UInt32 issues = ValidateParsedKFModel(actorBase, kfModel, loaderPath, skeletonRoot, printToConsole);
		ReleaseKFModel(kfModel);
		return issues;
	}

	static bool ReadSpecialAnimAssetSummary(TESActorBase* actorBase, const char* animPath, ParsedSpecialAnimAssetSummary* outSummary)
	{
		if (!actorBase || !animPath || !animPath[0] || !outSummary)
			return false;

		char loaderPath[1024];
		if (!GetSpecialAnimLoaderPath(actorBase, animPath, loaderPath, sizeof(loaderPath)))
			return false;

		UInt32 kfModel = LoadKFModel(loaderPath);
		if (!kfModel)
			return false;

		bool valid = false;
		TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
		NiControllerSequence* controllerSequence = GetKFModelControllerSequence(kfModel);
		if (animGroup && controllerSequence)
		{
			outSummary->animPath = animPath;
			outSummary->loaderPath = loaderPath;
			outSummary->key = (UInt16)GetParsedAnimGroupKey(animGroup);
			outSummary->groupID = GetParsedAnimGroupID(animGroup);
			outSummary->morphKey = GetParsedMorphKey(animGroup);
			outSummary->controllerCount = controllerSequence->arraySize;
			valid = true;
		}

		ReleaseKFModel(kfModel);
		return valid;
	}

	static UInt32 ValidateFolderMorphControllerCompatibility(const std::vector<ParsedSpecialAnimAssetSummary>& assets, bool printToConsole)
	{
		struct MorphControllerBucket
		{
			UInt8 morphKey;
			UInt32 controllerCount;
			const ParsedSpecialAnimAssetSummary* first;
		};

		std::vector<MorphControllerBucket> buckets;
		UInt32 issues = 0;
		for (std::vector<ParsedSpecialAnimAssetSummary>::const_iterator it = assets.begin(); it != assets.end(); ++it)
		{
			if (!it->morphKey)
				continue;

			MorphControllerBucket* bucket = NULL;
			for (std::vector<MorphControllerBucket>::iterator bucketIt = buckets.begin(); bucketIt != buckets.end(); ++bucketIt)
			{
				if (bucketIt->morphKey == it->morphKey)
				{
					bucket = &(*bucketIt);
					break;
				}
			}

			if (!bucket)
			{
				MorphControllerBucket newBucket = { it->morphKey, it->controllerCount, &(*it) };
				buckets.push_back(newBucket);
				continue;
			}

			if (bucket->controllerCount == it->controllerCount)
				continue;

			if (printToConsole)
			{
				const char* groupName = it->groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(it->groupID) : "";
				const char* firstGroupName = bucket->first && bucket->first->groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(bucket->first->groupID) : "";
				Console_Print("CASValidateSpecialAnimAssets: folder morph controller count mismatch morphKey=%u; %s (%s) controllers=%u differs from %s (%s) controllers=%u. ActorAnimData_PlaySequence 0x474530 requires equal counts before morphing same-key sequences.",
					it->morphKey,
					it->loaderPath.c_str(),
					groupName ? groupName : "",
					it->controllerCount,
					bucket->first ? bucket->first->loaderPath.c_str() : "",
					firstGroupName ? firstGroupName : "",
					bucket->controllerCount);
			}
			++issues;
		}

		return issues;
	}

	static bool ReadSpecialAnimParsedKey(TESActorBase* actorBase, const char* animPath, UInt16* outKey, UInt32* outGroupID, char* outLoaderPath, UInt32 outLoaderLen, bool printToConsole)
	{
		if (outKey)
			*outKey = 0xFFFF;
		if (outGroupID)
			*outGroupID = TESAnimGroup::kAnimGroup_Max;
		if (outLoaderPath && outLoaderLen)
			outLoaderPath[0] = 0;

		if (!actorBase || !animPath || !animPath[0])
			return false;

		char loaderPath[1024];
		if (!GetSpecialAnimLoaderPath(actorBase, animPath, loaderPath, sizeof(loaderPath)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: actor base has no usable model path for \"%s\"", animPath);
			return false;
		}

		if (outLoaderPath && outLoaderLen)
			strncpy_s(outLoaderPath, outLoaderLen, loaderPath, _TRUNCATE);

		UInt32 kfModel = LoadKFModel(loaderPath);
		if (!kfModel)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: native loader failed for %s", loaderPath);
			return false;
		}

		bool valid = false;
		TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
		if (!animGroup)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: no parsed TESAnimGroup in %s", loaderPath);
		}
		else
		{
			UInt32 groupID = GetParsedAnimGroupID(animGroup);
			UInt16 key = (UInt16)GetParsedAnimGroupKey(animGroup);
			const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : NULL;
			if (groupID >= TESAnimGroup::kAnimGroup_Max || !groupName)
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimLoadState: invalid native group %04X in %s", key, loaderPath);
			}
			else if (!GetKFModelControllerSequence(kfModel))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimLoadState: missing controller sequence for %s in %s", groupName, loaderPath);
			}
			else
			{
				if (outKey)
					*outKey = key;
				if (outGroupID)
					*outGroupID = groupID;
				valid = true;
			}
		}

		ReleaseKFModel(kfModel);
		return valid;
	}

	static bool FindDeferredInstallKey(ActorAnimData* animData, UInt16 key, UInt32* outKFModel)
	{
		if (outKFModel)
			*outKFModel = 0;

		UInt32 pending = CountDeferredInstallModels(animData);
		for (UInt32 i = 0; i < pending; ++i)
		{
			UInt32 kfModel = GetDeferredInstallKFModel(animData, i);
			TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
			if (animGroup && ((UInt16)GetParsedAnimGroupKey(animGroup)) == key)
			{
				if (outKFModel)
					*outKFModel = kfModel;
				return true;
			}
		}

		return false;
	}

	static UInt32 CountDeferredInstallKey(ActorAnimData* animData, UInt16 key)
	{
		UInt32 count = 0;
		UInt32 pending = CountDeferredInstallModels(animData);
		for (UInt32 i = 0; i < pending; ++i)
		{
			UInt32 kfModel = GetDeferredInstallKFModel(animData, i);
			TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
			if (animGroup && ((UInt16)GetParsedAnimGroupKey(animGroup)) == key)
				++count;
		}

		return count;
	}

	static UInt32 GetAnimSequenceEntryDestructor(UInt32 entry)
	{
		if (!entry)
			return 0;

		UInt32 vtbl = *(UInt32*)entry;
		return vtbl ? *(UInt32*)vtbl : 0;
	}

	static bool IsAnimSequenceMultipleEntry(UInt32 entry)
	{
		return GetAnimSequenceEntryDestructor(entry) == kAnimSequenceMultipleDeletingDestructor;
	}

	static bool IsAnimSequenceSingleEntry(UInt32 entry)
	{
		return GetAnimSequenceEntryDestructor(entry) == kAnimSequenceSingleDeletingDestructor;
	}

	static const char* GetAnimSequenceEntryKind(UInt32 entry)
	{
		if (!entry)
			return "none";
		if (IsAnimSequenceMultipleEntry(entry))
			return "multiple";
		if (IsAnimSequenceSingleEntry(entry))
			return "single";
		return "unknown";
	}

	static UInt32 CountMapEntrySequences(UInt32 entry)
	{
		if (!entry)
			return 0;

		if (IsAnimSequenceSingleEntry(entry))
			return *(UInt32*)(entry + kAnimSequenceEntrySequenceOffset) ? 1 : 0;

		if (IsAnimSequenceMultipleEntry(entry))
		{
			UInt32 list = *(UInt32*)(entry + kAnimSequenceMultipleListOffset);
			return list ? *(UInt32*)(list + kAnimSequenceMultipleListCountOffset) : 0;
		}

		return 0;
	}

	static const char* GetBSAnimGroupSequencePath(UInt32 sequence)
	{
		return sequence ? *(const char**)(sequence + kBSAnimGroupSequencePathOffset) : NULL;
	}

	static void CopyNormalizedPath(const char* source, char* outPath, UInt32 outLen)
	{
		if (!outPath || outLen == 0)
			return;

		outPath[0] = 0;
		if (!source)
			return;

		strncpy_s(outPath, outLen, source, _TRUNCATE);
		NormalizeSlashes(outPath);
	}

	static bool PathEqualsOrEndsWithComponent(const char* path, const char* suffix)
	{
		if (!path || !path[0] || !suffix || !suffix[0])
			return false;

		char cleanPath[1024];
		char cleanSuffix[1024];
		CopyNormalizedPath(path, cleanPath, sizeof(cleanPath));
		CopyNormalizedPath(suffix, cleanSuffix, sizeof(cleanSuffix));

		UInt32 pathLen = std::strlen(cleanPath);
		UInt32 suffixLen = std::strlen(cleanSuffix);
		if (!pathLen || !suffixLen || suffixLen > pathLen)
			return false;

		if (_stricmp(cleanPath, cleanSuffix) == 0)
			return true;

		const char* tail = cleanPath + (pathLen - suffixLen);
		if (_stricmp(tail, cleanSuffix) != 0)
			return false;

		return pathLen == suffixLen || IsSlash(cleanPath[pathLen - suffixLen - 1]);
	}

	static bool SequencePathMatchesSpecialAnimPath(const char* sequencePath, const char* loaderPath, const char* animPath)
	{
		if (PathEqualsOrEndsWithComponent(sequencePath, loaderPath))
			return true;

		char specialSuffix[1024];
		if (sprintf_s(specialSuffix, sizeof(specialSuffix), "SpecialAnims\\%s", animPath ? animPath : "") > 0)
		{
			if (PathEqualsOrEndsWithComponent(sequencePath, specialSuffix))
				return true;
		}

		return PathEqualsOrEndsWithComponent(sequencePath, animPath);
	}

	static bool SequencePathIsUnderSpecialAnimFolder(const char* sequencePath, const char* folder)
	{
		if (!sequencePath || !sequencePath[0] || !folder || !folder[0])
			return false;

		char cleanPath[1024];
		char cleanFolder[kMaxSpecialAnimPath];
		CopyNormalizedPath(sequencePath, cleanPath, sizeof(cleanPath));
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)) || !cleanFolder[0])
			return false;

		if (PathMatchesOptionalFolder(cleanPath, cleanFolder))
			return true;

		char specialPrefix[1024];
		if (sprintf_s(specialPrefix, sizeof(specialPrefix), "SpecialAnims\\%s", cleanFolder) <= 0)
			return false;

		UInt32 prefixLen = std::strlen(specialPrefix);
		for (const char* cur = cleanPath; *cur; ++cur)
		{
			if ((cur == cleanPath || IsSlash(cur[-1])) &&
				_strnicmp(cur, specialPrefix, prefixLen) == 0 &&
				(cur[prefixLen] == 0 || IsSlash(cur[prefixLen])))
			{
				return true;
			}
		}

		return false;
	}

	static bool SequenceMatchesAutoWeaponAnimPath(const char* sequencePath, const char* animPath)
	{
		if (!sequencePath || !sequencePath[0] || !animPath || !animPath[0])
			return false;

		if (IsKFPath(animPath))
			return SequencePathMatchesSpecialAnimPath(sequencePath, animPath, animPath);

		return SequencePathIsUnderSpecialAnimFolder(sequencePath, animPath);
	}

	static bool SequenceMatchesAutoWeaponAttempt(const char* sequencePath, const AutoWeaponAnimAttempt& attempt, const char** outMatchedPath)
	{
		if (outMatchedPath)
			*outMatchedPath = NULL;

		for (std::vector<std::string>::const_iterator it = attempt.animPaths.begin(); it != attempt.animPaths.end(); ++it)
		{
			if (SequenceMatchesAutoWeaponAnimPath(sequencePath, it->c_str()))
			{
				if (outMatchedPath)
					*outMatchedPath = it->c_str();
				return true;
			}
		}

		return false;
	}

	static UInt32 FindAutoWeaponAnimationSequenceInMapEntry(UInt32 entry, const AutoWeaponAnimAttempt& attempt, UInt32* outIndex, UInt32* outCount, const char** outMatchedPath)
	{
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = CountMapEntrySequences(entry);
		if (outMatchedPath)
			*outMatchedPath = NULL;

		if (!entry)
			return 0;

		if (IsAnimSequenceSingleEntry(entry))
		{
			UInt32 sequence = *(UInt32*)(entry + kAnimSequenceEntrySequenceOffset);
			if (SequenceMatchesAutoWeaponAttempt(GetBSAnimGroupSequencePath(sequence), attempt, outMatchedPath))
			{
				if (outIndex)
					*outIndex = 0;
				return sequence;
			}
			return 0;
		}

		if (!IsAnimSequenceMultipleEntry(entry))
			return 0;

		UInt32 list = *(UInt32*)(entry + kAnimSequenceMultipleListOffset);
		UInt32 node = list ? *(UInt32*)(list + kAnimSequenceMultipleListHeadOffset) : 0;
		UInt32 index = 0;
		while (node && index < kSequenceListGuard)
		{
			UInt32 sequence = *(UInt32*)(node + kListNodeDataOffset);
			if (SequenceMatchesAutoWeaponAttempt(GetBSAnimGroupSequencePath(sequence), attempt, outMatchedPath))
			{
				if (outIndex)
					*outIndex = index;
				return sequence;
			}

			node = *(UInt32*)(node + kListNodeNextOffset);
			++index;
		}

		return 0;
	}

	static UInt32 FindSequenceMatchingSpecialAnimPathInMapEntry(UInt32 entry, const char* loaderPath, const char* animPath, UInt32* outIndex, UInt32* outCount)
	{
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = CountMapEntrySequences(entry);

		if (!entry)
			return 0;

		if (IsAnimSequenceSingleEntry(entry))
		{
			UInt32 sequence = *(UInt32*)(entry + kAnimSequenceEntrySequenceOffset);
			if (SequencePathMatchesSpecialAnimPath(GetBSAnimGroupSequencePath(sequence), loaderPath, animPath))
			{
				if (outIndex)
					*outIndex = 0;
				return sequence;
			}
			return 0;
		}

		if (!IsAnimSequenceMultipleEntry(entry))
			return 0;

		UInt32 list = *(UInt32*)(entry + kAnimSequenceMultipleListOffset);
		UInt32 node = list ? *(UInt32*)(list + kAnimSequenceMultipleListHeadOffset) : 0;
		UInt32 index = 0;
		while (node && index < kSequenceListGuard)
		{
			UInt32 sequence = *(UInt32*)(node + kListNodeDataOffset);
			if (SequencePathMatchesSpecialAnimPath(GetBSAnimGroupSequencePath(sequence), loaderPath, animPath))
			{
				if (outIndex)
					*outIndex = index;
				return sequence;
			}

			node = *(UInt32*)(node + kListNodeNextOffset);
			++index;
		}

		return 0;
	}

	static UInt32 FindSequenceUnderSpecialAnimFolderInMapEntry(UInt32 entry, const char* folder, UInt32* outIndex, UInt32* outCount)
	{
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = CountMapEntrySequences(entry);

		if (!entry || !folder || !folder[0])
			return 0;

		if (IsAnimSequenceSingleEntry(entry))
		{
			UInt32 sequence = *(UInt32*)(entry + kAnimSequenceEntrySequenceOffset);
			if (SequencePathIsUnderSpecialAnimFolder(GetBSAnimGroupSequencePath(sequence), folder))
			{
				if (outIndex)
					*outIndex = 0;
				return sequence;
			}
			return 0;
		}

		if (!IsAnimSequenceMultipleEntry(entry))
			return 0;

		UInt32 list = *(UInt32*)(entry + kAnimSequenceMultipleListOffset);
		UInt32 node = list ? *(UInt32*)(list + kAnimSequenceMultipleListHeadOffset) : 0;
		UInt32 index = 0;
		while (node && index < kSequenceListGuard)
		{
			UInt32 sequence = *(UInt32*)(node + kListNodeDataOffset);
			if (SequencePathIsUnderSpecialAnimFolder(GetBSAnimGroupSequencePath(sequence), folder))
			{
				if (outIndex)
					*outIndex = index;
				return sequence;
			}

			node = *(UInt32*)(node + kListNodeNextOffset);
			++index;
		}

		return 0;
	}

	static bool GetSpecialAnimFolderFromAnimPath(const char* animPath, char* outFolder, UInt32 outLen)
	{
		if (!outFolder || outLen == 0)
			return false;

		outFolder[0] = 0;
		if (!animPath || !animPath[0])
			return false;

		char cleanPath[kMaxSpecialAnimPath];
		CopyNormalizedPath(animPath, cleanPath, sizeof(cleanPath));
		char* slash = std::strrchr(cleanPath, '\\');
		if (slash)
			*slash = 0;

		return NormalizeRelativeFolder(cleanPath, outFolder, outLen) && outFolder[0] != 0;
	}

	static bool HasInstalledAnimPath(const std::vector<ScopedInstalledAnim>& installed, const char* animPath)
	{
		if (!animPath || !animPath[0])
			return false;

		for (std::vector<ScopedInstalledAnim>::const_iterator it = installed.begin(); it != installed.end(); ++it)
		{
			if (_stricmp(it->animPath.c_str(), animPath) == 0)
				return true;
		}

		return false;
	}

	static void AddInstalledAnimPath(std::vector<ScopedInstalledAnim>* installed, const char* animPath, UInt16 key, UInt32 groupID, bool removeKFFZ = true)
	{
		if (!installed || !animPath || !animPath[0] || HasInstalledAnimPath(*installed, animPath))
			return;

		installed->push_back(ScopedInstalledAnim(animPath, key, groupID, removeKFFZ));
	}

	static void AddParsedInstalledAnimPath(std::vector<ScopedInstalledAnim>* installed, TESActorBase* actorBase, const char* animPath, bool removeKFFZ = true)
	{
		if (!installed || !animPath || !animPath[0])
			return;

		UInt16 parsedKey = 0xFFFF;
		UInt32 parsedGroupID = TESAnimGroup::kAnimGroup_Max;
		char loaderPath[1024] = { 0 };
		ReadSpecialAnimParsedKey(actorBase, animPath, &parsedKey, &parsedGroupID, loaderPath, sizeof(loaderPath), false);
		AddInstalledAnimPath(installed, animPath, parsedKey, parsedGroupID, removeKFFZ);
	}

	static void AddParsedInstalledAnimPaths(std::vector<ScopedInstalledAnim>* installed, TESActorBase* actorBase, const std::vector<std::string>& addedPaths, bool removeKFFZ = true)
	{
		for (std::vector<std::string>::const_iterator it = addedPaths.begin(); it != addedPaths.end(); ++it)
			AddParsedInstalledAnimPath(installed, actorBase, it->c_str(), removeKFFZ);
	}

	static void AddRegisteredScopedAnimPaths(std::vector<ScopedInstalledAnim>* installed, TESActorBase* actorBase, const char* animPathOrFolder, bool removeKFFZ)
	{
		if (!installed || !actorBase || !animPathOrFolder || !animPathOrFolder[0])
			return;

		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
			return;

		if (IsKFPath(animPathOrFolder))
		{
			if (HasExactAnimation(anim, animPathOrFolder))
				AddParsedInstalledAnimPath(installed, actorBase, animPathOrFolder, removeKFFZ);
			return;
		}

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(animPathOrFolder, cleanFolder, sizeof(cleanFolder)) || !cleanFolder[0])
			return;

		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			if (IsPersistableAnimationPath(cur->animationName) && PathMatchesOptionalFolder(cur->animationName, cleanFolder))
				AddParsedInstalledAnimPath(installed, actorBase, cur->animationName, removeKFFZ);
		}
	}

	static void ReleaseNiRefObject(UInt32 object)
	{
		if (!object)
			return;

		if (!InterlockedDecrement((volatile LONG*)(object + 4)))
		{
			UInt32 vtbl = *(UInt32*)object;
			if (vtbl)
			{
				AnimSequenceEntryDestroyFn destroy = *(AnimSequenceEntryDestroyFn*)vtbl;
				if (destroy)
					destroy((void*)object, 1);
			}
		}
	}

	static UInt32 ClearActiveSlotsForSequence(ActorAnimData* animData, UInt32 sequence)
	{
		if (!animData || !sequence)
			return 0;

		UInt32 cleared = 0;
		for (UInt32 i = 0; i < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++i)
		{
			if ((UInt32)animData->animSequences[i] == sequence)
			{
				ThisStdCall(kActorAnimDataClearSlot, animData, i, 0);
				++cleared;
			}
		}

		return cleared;
	}

	static void RemoveSequenceFromKeyframeManager(ActorAnimData* animData, UInt32 sequence)
	{
		if (!animData || !animData->manager || !sequence)
			return;

		UInt32 removedSequence = 0;
		ThisStdCall(kNiControllerManagerRemoveSequence, animData->manager, &removedSequence, sequence);
		ReleaseNiRefObject(removedSequence);
	}

	static bool RemoveSequenceFromAnimSequenceEntry(UInt32 entry, UInt32 sequence)
	{
		if (!entry || !sequence)
			return false;

		UInt32 vtbl = *(UInt32*)entry;
		if (!vtbl)
			return false;

		AnimSequenceEntryRemoveFn remove = *(AnimSequenceEntryRemoveFn*)(vtbl + 0x08);
		return remove && remove((void*)entry, sequence) != 0;
	}

	static void DestroyAnimSequenceEntry(UInt32 entry)
	{
		if (!entry)
			return;

		UInt32 vtbl = *(UInt32*)entry;
		if (!vtbl)
			return;

		AnimSequenceEntryDestroyFn destroy = *(AnimSequenceEntryDestroyFn*)vtbl;
		if (destroy)
			destroy((void*)entry, 1);
	}

	static UInt32 RemoveInstalledSequenceFromAnimData(ActorAnimData* animData, TESActorBase* actorBase, const ScopedInstalledAnim& installed, UInt32* outClearedSlots, UInt32* outErrors)
	{
		if (outClearedSlots)
			*outClearedSlots = 0;

		if (!animData || !IsNativeSavedSlotKey(installed.key) || installed.animPath.empty())
			return 0;

		char loaderPath[1024] = { 0 };
		GetSpecialAnimLoaderPath(actorBase, installed.animPath.c_str(), loaderPath, sizeof(loaderPath));

		UInt32 removed = 0;
		for (UInt32 guard = 0; guard < kSequenceListGuard; ++guard)
		{
			UInt32 mapEntry = 0;
			if (!LookupAnimationMapEntry(animData, installed.key, &mapEntry))
				break;

			UInt32 sequenceIndex = 0xFFFFFFFF;
			UInt32 sequenceCount = 0;
			UInt32 sequence = FindSequenceMatchingSpecialAnimPathInMapEntry(mapEntry, loaderPath, installed.animPath.c_str(), &sequenceIndex, &sequenceCount);
			if (!sequence)
				break;

			UInt32 cleared = ClearActiveSlotsForSequence(animData, sequence);
			if (outClearedSlots)
				*outClearedSlots += cleared;

			RemoveSequenceFromKeyframeManager(animData, sequence);

			bool entryEmpty = RemoveSequenceFromAnimSequenceEntry(mapEntry, sequence);
			++removed;

			if (entryEmpty)
			{
				if (!ThisStdCall(kActorAnimDataMapRemove, animData->map9C, (UInt32)installed.key) && outErrors)
					++(*outErrors);
				DestroyAnimSequenceEntry(mapEntry);
				break;
			}

			if (sequenceCount <= 1)
				break;
		}

		return removed;
	}

	static UInt32 RemoveRemainingFolderSequencesFromAnimData(ActorAnimData* animData, const ScopedFolderKey& folderKey, UInt32* outClearedSlots, UInt32* outErrors)
	{
		if (outClearedSlots)
			*outClearedSlots = 0;

		if (!animData || !IsNativeSavedSlotKey(folderKey.key) || folderKey.folder.empty())
			return 0;

		UInt32 removed = 0;
		for (UInt32 guard = 0; guard < kSequenceListGuard; ++guard)
		{
			UInt32 mapEntry = 0;
			if (!LookupAnimationMapEntry(animData, folderKey.key, &mapEntry))
				break;

			UInt32 sequenceIndex = 0xFFFFFFFF;
			UInt32 sequenceCount = 0;
			UInt32 sequence = FindSequenceUnderSpecialAnimFolderInMapEntry(mapEntry, folderKey.folder.c_str(), &sequenceIndex, &sequenceCount);
			if (!sequence)
				break;

			UInt32 cleared = ClearActiveSlotsForSequence(animData, sequence);
			if (outClearedSlots)
				*outClearedSlots += cleared;

			RemoveSequenceFromKeyframeManager(animData, sequence);

			bool entryEmpty = RemoveSequenceFromAnimSequenceEntry(mapEntry, sequence);
			++removed;

			if (entryEmpty)
			{
				if (!ThisStdCall(kActorAnimDataMapRemove, animData->map9C, (UInt32)folderKey.key) && outErrors)
					++(*outErrors);
				DestroyAnimSequenceEntry(mapEntry);
				break;
			}

			if (sequenceCount <= 1)
				break;
		}

		if (removed)
		{
			const char* groupName = folderKey.groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(folderKey.groupID) : "";
			_MESSAGE("CustomAnimSupport scoped folder cleanup animData=%08X key=%04X group=%s folder=\"%s\" removed=%u",
				(UInt32)animData,
				folderKey.key,
				groupName ? groupName : "",
				folderKey.folder.c_str(),
				removed);
		}

		return removed;
	}

	static void AddUniqueActorAnimDataTarget(std::vector<ActorAnimData*>* targets, ActorAnimData* animData)
	{
		if (!targets || !animData)
			return;

		for (std::vector<ActorAnimData*>::const_iterator it = targets->begin(); it != targets->end(); ++it)
		{
			if (*it == animData)
				return;
		}

		targets->push_back(animData);
	}

	static void CollectActorAnimDataTargets(TESObjectREFR* ref, ActorAnimData* primaryAnimData, std::vector<ActorAnimData*>* targets)
	{
		if (!targets)
			return;

		if (!ref)
		{
			AddUniqueActorAnimDataTarget(targets, primaryAnimData);
			return;
		}

		if (!IsPlayerFirstPersonAnimData(ref, primaryAnimData))
			AddUniqueActorAnimDataTarget(targets, primaryAnimData);

		AddUniqueActorAnimDataTarget(targets, GetActorBaseSpecialAnimData(ref));
	}

	static ScopedRevertResult RevertInstalledScopedAnimations(TESObjectREFR* ref, ActorAnimData* animData, TESActorBase* actorBase, std::vector<ScopedInstalledAnim>* installed)
	{
		ScopedRevertResult result;
		if (!actorBase || !installed || installed->empty())
			return result;

		std::vector<ActorAnimData*> liveAnimDataTargets;
		CollectActorAnimDataTargets(ref, animData, &liveAnimDataTargets);

		std::vector<UInt16> restoreKeys;
		std::vector<ScopedFolderKey> scopedFolderKeys;
		TESAnimation* anim = GetAnimationList(actorBase);
		for (std::vector<ScopedInstalledAnim>::const_iterator it = installed->begin(); it != installed->end(); ++it)
		{
			if (it->animPath.empty())
				continue;

			++result.paths;
			if (ShouldRestoreVanillaAfterScopedRemoval(*it))
				AddUniqueAnimKey(&restoreKeys, it->key);
			AddUniqueScopedFolderKey(&scopedFolderKeys, *it);

			if (it->removeKFFZ && anim && RemoveAnimation(anim, it->animPath.c_str()))
				++result.removedKFFZ;

			for (std::vector<ActorAnimData*>::const_iterator targetIt = liveAnimDataTargets.begin(); targetIt != liveAnimDataTargets.end(); ++targetIt)
			{
				UInt32 clearedSlots = 0;
				UInt32 errors = 0;
				result.removedLive += RemoveInstalledSequenceFromAnimData(*targetIt, actorBase, *it, &clearedSlots, &errors);
				result.clearedSlots += clearedSlots;
				result.errors += errors;
			}
		}

		if (!scopedFolderKeys.empty())
		{
			for (std::vector<ActorAnimData*>::const_iterator targetIt = liveAnimDataTargets.begin(); targetIt != liveAnimDataTargets.end(); ++targetIt)
			{
				for (std::vector<ScopedFolderKey>::const_iterator folderIt = scopedFolderKeys.begin(); folderIt != scopedFolderKeys.end(); ++folderIt)
				{
					UInt32 clearedSlots = 0;
					UInt32 errors = 0;
					result.removedLive += RemoveRemainingFolderSequencesFromAnimData(*targetIt, *folderIt, &clearedSlots, &errors);
					result.clearedSlots += clearedSlots;
					result.errors += errors;
				}
			}
		}

		if (!restoreKeys.empty())
		{
			for (std::vector<ActorAnimData*>::const_iterator targetIt = liveAnimDataTargets.begin(); targetIt != liveAnimDataTargets.end(); ++targetIt)
			{
				UInt32 errors = 0;
				result.restoredVanilla += RestoreVanillaActorModelSequencesForKeys(*targetIt, actorBase, restoreKeys, &errors);
				result.errors += errors;
			}
		}

		installed->clear();
		return result;
	}

	static ScopedRevertResult RemoveLiveRegisteredSequencesForPath(TESObjectREFR* ref, TESActorBase* actorBase, const char* animPathOrFolder)
	{
		ScopedRevertResult result;
		if (!ref || !actorBase || !animPathOrFolder || !animPathOrFolder[0] || GetActorBaseFromRef(ref) != actorBase)
			return result;

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);

		std::vector<ScopedInstalledAnim> installed;
		if (IsKFPath(animPathOrFolder))
			AddParsedInstalledAnimPath(&installed, actorBase, animPathOrFolder, false);
		else
			AddRegisteredScopedAnimPaths(&installed, actorBase, animPathOrFolder, false);

		return RevertInstalledScopedAnimations(ref, animData, actorBase, &installed);
	}

	static bool PathMatchesOptionalFolder(const char* path, const char* folder)
	{
		if (!path || !path[0])
			return false;

		if (!folder || !folder[0])
			return true;

		UInt32 folderLen = std::strlen(folder);
		UInt32 pathLen = std::strlen(path);
		if (folderLen > pathLen)
			return false;

		return _strnicmp(path, folder, folderLen) == 0 && (pathLen == folderLen || IsSlash(path[folderLen]));
	}

	static void ScanSpecialAnimDirectory(const char* root, const char* relativePrefix, TESAnimation* anim, DiscoverResult* result, TESActorBase* actorBase, NiNode* skeletonRoot, bool trackPersistent, bool validateAssets = false, bool printToConsole = false, std::vector<ParsedSpecialAnimAssetSummary>* assetSummaries = NULL, std::vector<std::string>* addedPaths = NULL)
	{
		if (!root || !result)
			return;

		char searchPath[1024];
		if (relativePrefix && relativePrefix[0])
			sprintf_s(searchPath, sizeof(searchPath), "%s\\%s\\*", root, relativePrefix);
		else
			sprintf_s(searchPath, sizeof(searchPath), "%s\\*", root);

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return;

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			char relativePath[kMaxSpecialAnimPath];
			AppendRelativePath(relativePath, sizeof(relativePath), relativePrefix, data.cFileName);

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (IsSafeRelativePath(relativePath))
					ScanSpecialAnimDirectory(root, relativePath, anim, result, actorBase, skeletonRoot, trackPersistent, validateAssets, printToConsole, assetSummaries, addedPaths);
				else
					++result->errors;

				continue;
			}

			if (!IsKFPath(relativePath))
				continue;

			++result->found;
			if (!IsSafeRelativePath(relativePath))
			{
				++result->errors;
				continue;
			}

			if (validateAssets)
			{
				result->errors += ValidateSpecialAnimAsset(actorBase, relativePath, skeletonRoot, printToConsole);
				if (assetSummaries)
				{
					ParsedSpecialAnimAssetSummary summary;
					if (ReadSpecialAnimAssetSummary(actorBase, relativePath, &summary))
						assetSummaries->push_back(summary);
				}
			}

			if (anim)
			{
				bool added = AddAnimation(anim, relativePath);
				if (added)
				{
					++result->added;
					if (addedPaths && !HasQueuedRemoval(*addedPaths, relativePath))
						addedPaths->push_back(relativePath);
				}
				else
					++result->skipped;

				if (trackPersistent && (added || HasExactAnimation(anim, relativePath)))
					AddPersistentAnimation(actorBase, relativePath);
			}
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
	}

	static DiscoverResult ValidateSpecialAnimFolder(TESActorBase* actorBase, const char* folder, NiNode* skeletonRoot, bool printToConsole)
	{
		DiscoverResult result = { 0, 0, 0, 0 };
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimFolder: no actor base");
			++result.errors;
			return result;
		}

		char root[520];
		if (!GetSpecialAnimsRoot(actorBase, root, sizeof(root)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimFolder: actor base has no usable model directory");
			++result.errors;
			return result;
		}

		if (!DirectoryExists(root))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimFolder: missing folder %s", root);
			++result.errors;
			return result;
		}

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimFolder: unsafe folder \"%s\"", folder ? folder : "");
			++result.errors;
			return result;
		}

		char scanRoot[1024];
		if (cleanFolder[0])
			sprintf_s(scanRoot, sizeof(scanRoot), "%s\\%s", root, cleanFolder);
		else
			strncpy_s(scanRoot, sizeof(scanRoot), root, _TRUNCATE);

		if (!DirectoryExists(scanRoot))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimFolder: missing folder %s", scanRoot);
			++result.errors;
			return result;
		}

		std::vector<ParsedSpecialAnimAssetSummary> assetSummaries;
		ScanSpecialAnimDirectory(root, cleanFolder, NULL, &result, actorBase, skeletonRoot, false, true, printToConsole, &assetSummaries);
		result.errors += ValidateFolderMorphControllerCompatibility(assetSummaries, printToConsole);
		if (!result.found)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimFolder: no .kf files under %s", scanRoot);
			++result.errors;
		}

		return result;
	}

	static DiscoverResult DiscoverSpecialAnimations(TESActorBase* actorBase, const char* folder, bool trackPersistent, std::vector<std::string>* addedPaths = NULL)
	{
		DiscoverResult result = { 0, 0, 0, 0 };
		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
			return result;

		char root[520];
		if (!GetSpecialAnimsRoot(actorBase, root, sizeof(root)))
		{
			++result.errors;
			return result;
		}

		if (!DirectoryExists(root))
			return result;

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)))
		{
			++result.errors;
			return result;
		}

		if (cleanFolder[0])
		{
			char scanRoot[1024];
			sprintf_s(scanRoot, sizeof(scanRoot), "%s\\%s", root, cleanFolder);
			if (!DirectoryExists(scanRoot))
			{
				++result.errors;
				return result;
			}
		}

		ScanSpecialAnimDirectory(root, cleanFolder, anim, &result, actorBase, NULL, trackPersistent, false, false, NULL, addedPaths);
		return result;
	}

	static PruneResult PruneMissingSpecialAnimations(TESActorBase* actorBase, const char* folder, bool printToConsole)
	{
		PruneResult result;
		TESAnimation* anim = GetAnimationList(actorBase);
		if (!actorBase || !anim)
		{
			if (printToConsole)
				Console_Print("CASPruneMissingSpecialAnims: no actor base animation list");
			++result.errors;
			return result;
		}

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)))
		{
			if (printToConsole)
				Console_Print("CASPruneMissingSpecialAnims: unsafe folder \"%s\"", folder ? folder : "");
			++result.errors;
			return result;
		}

		std::vector<std::string> removals;
		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			const char* entry = cur->animationName;
			if (!PathMatchesOptionalFolder(entry, cleanFolder))
				continue;

			++result.checked;
			if (!IsPersistableAnimationPath(entry))
			{
				if (printToConsole)
					Console_Print("CASPruneMissingSpecialAnims: skipping non-persistable KFFZ entry \"%s\"", entry ? entry : "");
				++result.skipped;
				continue;
			}

			char diskPath[1024];
			if (!GetSpecialAnimDiskPath(actorBase, entry, diskPath, sizeof(diskPath)))
			{
				if (printToConsole)
					Console_Print("CASPruneMissingSpecialAnims: actor base has no usable model path for \"%s\"", entry);
				++result.errors;
				continue;
			}

			if (!FileExists(diskPath))
			{
				++result.missing;
				if (!HasQueuedRemoval(removals, entry))
					removals.push_back(entry);
				if (printToConsole)
					Console_Print("CASPruneMissingSpecialAnims: missing \"%s\" at %s", entry, diskPath);
			}
		}

		for (std::vector<std::string>::const_iterator it = removals.begin(); it != removals.end(); ++it)
		{
			UInt32 before = CountAnimations(anim);
			if (RemoveAnimation(anim, it->c_str()))
			{
				UInt32 after = CountAnimations(anim);
				result.removed += before > after ? before - after : 1;
			}
			RemovePersistentAnimation(actorBase, it->c_str());
		}

		return result;
	}

	static bool ReadTextFile(const char* path, std::string* out)
	{
		if (!path || !path[0] || !out)
			return false;

		if (!IsSafeRelativePath(path))
			return false;

		FILE* file = NULL;
		if (fopen_s(&file, path, "rb") != 0 || !file)
			return false;

		if (fseek(file, 0, SEEK_END) != 0)
		{
			fclose(file);
			return false;
		}

		long size = ftell(file);
		if (size < 0)
		{
			fclose(file);
			return false;
		}

		if (fseek(file, 0, SEEK_SET) != 0)
		{
			fclose(file);
			return false;
		}

		out->assign((size_t)size, '\0');
		if (size > 0 && fread(&(*out)[0], 1, (size_t)size, file) != (size_t)size)
		{
			fclose(file);
			out->clear();
			return false;
		}

		fclose(file);
		return true;
	}

	static bool ParseFormIDString(const std::string& value, UInt32* outFormID)
	{
		if (!outFormID)
			return false;

		const char* start = value.c_str();
		while (*start && std::isspace((unsigned char)*start))
			++start;

		if (start[0] == '0' && (start[1] == 'x' || start[1] == 'X'))
			start += 2;

		char* end = NULL;
		unsigned long parsed = std::strtoul(start, &end, 16);
		if (end == start)
			return false;

		while (end && *end && std::isspace((unsigned char)*end))
			++end;

		if (end && *end)
			return false;

		*outFormID = (UInt32)parsed;
		return true;
	}

	static TESForm* ResolveManifestForm(const std::string& formID, const std::string& modName);

	static TESActorBase* GetActorBaseFromForm(TESForm* form)
	{
		if (!form)
			return NULL;

		TESActorBase* actorBase = OBLIVION_CAST(form, TESForm, TESActorBase);
		if (actorBase)
			return actorBase;

		TESObjectREFR* ref = OBLIVION_CAST(form, TESForm, TESObjectREFR);
		return GetActorBaseFromRef(ref);
	}

	static const char* GetFormTypeName(UInt32 typeID)
	{
		switch (typeID)
		{
		case kFormType_Race:
			return "Race";
		case kFormType_Global:
			return "Global";
		case kFormType_Class:
			return "Class";
		case kFormType_Faction:
			return "Faction";
		case kFormType_Armor:
			return "Armor";
		case kFormType_Clothing:
			return "Clothing";
		case kFormType_Weapon:
			return "Weapon";
		case kFormType_Ammo:
			return "Ammo";
		case kFormType_NPC:
			return "NPC";
		case kFormType_Creature:
			return "Creature";
		case kFormType_LeveledCreature:
			return "LeveledCreature";
		case kFormType_LeveledItem:
			return "LeveledItem";
		case kFormType_LeveledSpell:
			return "LeveledSpell";
		case kFormType_ANIO:
			return "AnimObject";
		case kFormType_TOFT:
			return "TOFT";
		case kFormType_REFR:
			return "Reference";
		case kFormType_ACHR:
			return "ActorReference";
		case kFormType_ACRE:
			return "CreatureReference";
		case kFormType_Idle:
			return "Idle";
		default:
			return "Unknown";
		}
	}

	static bool IsLeveledListForm(TESForm* form)
	{
		if (!form)
			return false;

		return form->typeID == kFormType_LeveledCreature ||
			form->typeID == kFormType_LeveledItem ||
			form->typeID == kFormType_LeveledSpell;
	}

	static TESActorBase* ResolveManifestActorBaseWithForm(const std::string& formID, const std::string& modName, TESForm** outForm)
	{
		TESForm* form = ResolveManifestForm(formID, modName);
		if (outForm)
			*outForm = form;
		return GetActorBaseFromForm(form);
	}

	static void PrintUnsupportedManifestTarget(const char* context, const std::string& formID, const std::string& modName, TESForm* form, bool printToConsole)
	{
		if (!form)
			return;

		if (printToConsole)
		{
			if (IsLeveledListForm(form))
			{
				Console_Print("%s: form \"%s\" mod=\"%s\" resolved=%08X type=%02X (%s) is an Oblivion leveled list, not a FLST/form-list target; leveled lists use chance/level/count resolution and are not expanded for animation mappings",
					context ? context : "CustomAnimSupport",
					formID.c_str(),
					modName.c_str(),
					form->refID,
					form->typeID,
					GetFormTypeName(form->typeID));
				return;
			}

			Console_Print("%s: form \"%s\" mod=\"%s\" resolved=%08X type=%02X (%s) is not an actor-base/reference or plugin-managed race/global target; weapon/form-list and other kNVSE target semantics are unsupported by the decoded Oblivion KFFZ path",
				context ? context : "CustomAnimSupport",
				formID.c_str(),
				modName.c_str(),
				form->refID,
				form->typeID,
				GetFormTypeName(form->typeID));
		}
	}

	static void LogUnsupportedManifestTarget(const char* context, const std::string& formID, const std::string& modName, TESForm* form)
	{
		if (!form)
			return;

		if (IsLeveledListForm(form))
		{
			_MESSAGE("CustomAnimSupport %s unsupported manifest target form=\"%s\" mod=\"%s\" resolved=%08X type=%02X (%s); Oblivion leveled lists are chance/level/count resolved and are not FLST/form-list animation target containers",
				context ? context : "manifest",
				formID.c_str(),
				modName.c_str(),
				form->refID,
				form->typeID,
				GetFormTypeName(form->typeID));
			return;
		}

		_MESSAGE("CustomAnimSupport %s unsupported manifest target form=\"%s\" mod=\"%s\" resolved=%08X type=%02X (%s); decoded Oblivion KFFZ target scope is actor-base/reference, with plugin-managed race/global mappings layered above it",
			context ? context : "manifest",
			formID.c_str(),
			modName.c_str(),
			form->refID,
			form->typeID,
			GetFormTypeName(form->typeID));
	}

	static void PrintTargetManifestMapping(const char* context, const std::string& formID, const std::string& modName, TESForm* form, const char* folder, bool printToConsole)
	{
		if (!form || !printToConsole)
			return;

		Console_Print("%s: form \"%s\" mod=\"%s\" resolved=%08X type=%02X (%s) maps folder \"%s\" through plugin-managed %s target scope",
			context ? context : "CustomAnimSupport",
			formID.c_str(),
			modName.c_str(),
			form->refID,
			form->typeID,
			GetFormTypeName(form->typeID),
			folder ? folder : "",
			GetTargetAnimScopeName(GetTargetAnimScopeForForm(form)));
	}

	static void LogTargetManifestMapping(const char* context, const std::string& formID, const std::string& modName, TESForm* form, const char* folder, bool added, bool persist)
	{
		if (!form)
			return;

		_MESSAGE("CustomAnimSupport %s target manifest mapping form=\"%s\" mod=\"%s\" resolved=%08X type=%02X (%s) scope=%s folder=\"%s\" added=%u persist=%u mappings=%u",
			context ? context : "manifest",
			formID.c_str(),
			modName.c_str(),
			form->refID,
			form->typeID,
			GetFormTypeName(form->typeID),
			GetTargetAnimScopeName(GetTargetAnimScopeForForm(form)),
			folder ? folder : "",
			added ? 1 : 0,
			persist ? 1 : 0,
			CountTargetAnimationMappings(form->refID));
	}

	static bool ReadExactRecordData(void* out, UInt32 length)
	{
		return g_serialization && g_serialization->ReadRecordData(out, length) == length;
	}

	static void DiscardRecordBytes(UInt32 length)
	{
		if (!g_serialization)
			return;

		char buffer[128];
		while (length)
		{
			UInt32 chunk = length < sizeof(buffer) ? length : sizeof(buffer);
			UInt32 read = g_serialization->ReadRecordData(buffer, chunk);
			if (!read)
				return;
			length -= read;
		}
	}

	static bool ReadRecordString(UInt32 length, std::string* out)
	{
		if (!out)
			return false;

		out->clear();
		if (length == 0 || length >= kMaxSpecialAnimPath)
		{
			DiscardRecordBytes(length);
			return false;
		}

		std::vector<char> buffer(length + 1, 0);
		if (!ReadExactRecordData(&buffer[0], length))
			return false;

		out->assign(&buffer[0], length);
		return true;
	}

	static bool ApplyPersistentAnimation(UInt32 formID, const char* animPath)
	{
		if (!formID || !IsPersistableAnimationPath(animPath))
			return false;

		TESActorBase* actorBase = GetActorBaseFromForm(LookupFormByID(formID));
		TESAnimation* anim = GetAnimationList(actorBase);
		if (!actorBase || !anim)
			return false;

		AddAnimation(anim, animPath);
		AddPersistentAnimation(actorBase, animPath);
		return true;
	}

	static void SavePersistentAnimations()
	{
		if (!g_serialization)
			return;

		UInt32 count = 0;
		for (std::vector<PersistentAnimEntry>::const_iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end(); ++it)
		{
			if (it->formID && IsPersistableAnimationPath(it->animPath.c_str()) && it->animPath.length() < kMaxSpecialAnimPath)
				++count;
		}

		g_serialization->OpenRecord(kSerializationRecordPersistentAnims, kSerializationVersionPersistentAnims);
		g_serialization->WriteRecordData(&count, sizeof(count));

		for (std::vector<PersistentAnimEntry>::const_iterator it = s_persistentAnims.begin(); it != s_persistentAnims.end(); ++it)
		{
			if (!it->formID || !IsPersistableAnimationPath(it->animPath.c_str()) || it->animPath.length() >= kMaxSpecialAnimPath)
				continue;

			UInt32 formID = it->formID;
			UInt32 length = it->animPath.length();
			g_serialization->WriteRecordData(&formID, sizeof(formID));
			g_serialization->WriteRecordData(&length, sizeof(length));
			g_serialization->WriteRecordData(it->animPath.c_str(), length);
		}

		_MESSAGE("CustomAnimSupport saved persisted animations: %u", count);
	}

	static void SaveWeaponAnimationMappings()
	{
		if (!g_serialization)
			return;

		UInt32 count = 0;
		for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
		{
			TESForm* weapon = LookupFormByID(it->weaponFormID);
			if (IsWeaponForm(weapon) && it->animPath.length() < kMaxSpecialAnimPath)
				++count;
		}

		g_serialization->OpenRecord(kSerializationRecordWeaponAnims, kSerializationVersionWeaponAnims);
		g_serialization->WriteRecordData(&count, sizeof(count));

		for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
		{
			TESForm* weapon = LookupFormByID(it->weaponFormID);
			if (!IsWeaponForm(weapon) || it->animPath.length() >= kMaxSpecialAnimPath)
				continue;

			UInt32 weaponFormID = it->weaponFormID;
			UInt32 firstPerson = it->firstPerson ? 1 : 0;
			UInt32 length = it->animPath.length();
			g_serialization->WriteRecordData(&weaponFormID, sizeof(weaponFormID));
			g_serialization->WriteRecordData(&firstPerson, sizeof(firstPerson));
			g_serialization->WriteRecordData(&length, sizeof(length));
			g_serialization->WriteRecordData(it->animPath.c_str(), length);
		}

		_MESSAGE("CustomAnimSupport saved weapon animation mappings: %u", count);
	}

	static void SaveTargetAnimationMappings()
	{
		if (!g_serialization)
			return;

		UInt32 count = 0;
		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			TESForm* target = LookupFormByID(it->targetFormID);
			if (it->persist && IsTargetAnimForm(target) && it->scope == GetTargetAnimScopeForForm(target) && it->animPath.length() < kMaxSpecialAnimPath)
				++count;
		}

		if (!count)
		{
			_MESSAGE("CustomAnimSupport saved target animation mappings: 0");
			return;
		}

		g_serialization->OpenRecord(kSerializationRecordTargetAnims, kSerializationVersionTargetAnims);
		g_serialization->WriteRecordData(&count, sizeof(count));

		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			TESForm* target = LookupFormByID(it->targetFormID);
			if (!it->persist || !IsTargetAnimForm(target) || it->scope != GetTargetAnimScopeForForm(target) || it->animPath.length() >= kMaxSpecialAnimPath)
				continue;

			UInt32 targetFormID = it->targetFormID;
			UInt32 scope = it->scope;
			UInt32 length = it->animPath.length();
			g_serialization->WriteRecordData(&targetFormID, sizeof(targetFormID));
			g_serialization->WriteRecordData(&scope, sizeof(scope));
			g_serialization->WriteRecordData(&length, sizeof(length));
			g_serialization->WriteRecordData(it->animPath.c_str(), length);
		}

		_MESSAGE("CustomAnimSupport saved target animation mappings: %u", count);
	}

	static void SaveAutoWeaponAnimationRules()
	{
		if (!g_serialization)
			return;

		EnsureDefaultAutoWeaponAnimationRules();

		UInt32 count = 0;
		for (std::vector<AutoWeaponAnimRule>::const_iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end(); ++it)
		{
			if (it->persist && !it->nameContains.empty() && !it->animPath.empty() &&
				it->nameContains.length() < kMaxSpecialAnimPath && it->animPath.length() < kMaxSpecialAnimPath)
			{
				++count;
			}
		}

		if (!count)
		{
			_MESSAGE("CustomAnimSupport saved auto-weapon animation rules: 0");
			return;
		}

		g_serialization->OpenRecord(kSerializationRecordAutoWeaponRules, kSerializationVersionAutoWeaponRules);
		g_serialization->WriteRecordData(&count, sizeof(count));

		for (std::vector<AutoWeaponAnimRule>::const_iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end(); ++it)
		{
			if (!it->persist || it->nameContains.empty() || it->animPath.empty() ||
				it->nameContains.length() >= kMaxSpecialAnimPath || it->animPath.length() >= kMaxSpecialAnimPath)
			{
				continue;
			}

			UInt32 nameLength = it->nameContains.length();
			UInt32 pathLength = it->animPath.length();
			g_serialization->WriteRecordData(&nameLength, sizeof(nameLength));
			g_serialization->WriteRecordData(it->nameContains.c_str(), nameLength);
			g_serialization->WriteRecordData(&pathLength, sizeof(pathLength));
			g_serialization->WriteRecordData(it->animPath.c_str(), pathLength);
		}

		_MESSAGE("CustomAnimSupport saved auto-weapon animation rules: %u", count);
	}

	static void LoadPersistentAnimations()
	{
		if (!g_serialization)
			return;

		ClearPersistentAnimations(NULL);
		ClearWeaponAnimationMappings();
		ClearWeaponAnimationAttempts();
		ClearTargetAnimationMappings();
		ClearTargetAnimationAttempts();
		ClearPersistentAutoWeaponAnimationRules();
		s_autoWeaponAnimAttempts.clear();

		UInt32 restored = 0;
		UInt32 weaponRestored = 0;
		UInt32 targetRestored = 0;
		UInt32 autoWeaponRuleRestored = 0;
		UInt32 skipped = 0;
		UInt32 type = 0;
		UInt32 version = 0;
		UInt32 length = 0;
		while (g_serialization->GetNextRecordInfo(&type, &version, &length))
		{
			if (type != kSerializationRecordPersistentAnims &&
				type != kSerializationRecordWeaponAnims &&
				type != kSerializationRecordTargetAnims &&
				type != kSerializationRecordAutoWeaponRules)
			{
				_MESSAGE("CustomAnimSupport skipped unknown serialization record %08X", type);
				continue;
			}

			if (type == kSerializationRecordPersistentAnims && version != kSerializationVersionPersistentAnims)
			{
				_MESSAGE("CustomAnimSupport skipped persisted animation record version %u", version);
				continue;
			}

			if (type == kSerializationRecordWeaponAnims && version != kSerializationVersionWeaponAnims)
			{
				_MESSAGE("CustomAnimSupport skipped weapon animation record version %u", version);
				continue;
			}

			if (type == kSerializationRecordTargetAnims && version != kSerializationVersionTargetAnims)
			{
				_MESSAGE("CustomAnimSupport skipped target animation record version %u", version);
				continue;
			}

			if (type == kSerializationRecordAutoWeaponRules && version != kSerializationVersionAutoWeaponRules)
			{
				_MESSAGE("CustomAnimSupport skipped auto-weapon animation rule record version %u", version);
				continue;
			}

			UInt32 count = 0;
			if (!ReadExactRecordData(&count, sizeof(count)))
			{
				++skipped;
				continue;
			}

			for (UInt32 i = 0; i < count; ++i)
			{
				if (type == kSerializationRecordPersistentAnims)
				{
					UInt32 savedFormID = 0;
					UInt32 pathLength = 0;
					if (!ReadExactRecordData(&savedFormID, sizeof(savedFormID)) || !ReadExactRecordData(&pathLength, sizeof(pathLength)))
					{
						++skipped;
						break;
					}

					std::string animPath;
					if (!ReadRecordString(pathLength, &animPath))
					{
						++skipped;
						continue;
					}

					UInt32 resolvedFormID = 0;
					if (!g_serialization->ResolveRefID(savedFormID, &resolvedFormID))
					{
						++skipped;
						continue;
					}

					if (ApplyPersistentAnimation(resolvedFormID, animPath.c_str()))
						++restored;
					else
						++skipped;
				}
				else if (type == kSerializationRecordWeaponAnims)
				{
					UInt32 savedWeaponFormID = 0;
					UInt32 firstPerson = 0;
					UInt32 pathLength = 0;
					if (!ReadExactRecordData(&savedWeaponFormID, sizeof(savedWeaponFormID)) ||
						!ReadExactRecordData(&firstPerson, sizeof(firstPerson)) ||
						!ReadExactRecordData(&pathLength, sizeof(pathLength)))
					{
						++skipped;
						break;
					}

					std::string animPath;
					if (!ReadRecordString(pathLength, &animPath))
					{
						++skipped;
						continue;
					}

					UInt32 resolvedWeaponFormID = 0;
					if (!g_serialization->ResolveRefID(savedWeaponFormID, &resolvedWeaponFormID))
					{
						++skipped;
						continue;
					}

					TESForm* weapon = LookupFormByID(resolvedWeaponFormID);
					if (AddWeaponAnimationMapping(weapon, firstPerson, animPath.c_str()))
						++weaponRestored;
					else
						++skipped;
				}
				else if (type == kSerializationRecordTargetAnims)
				{
					UInt32 savedTargetFormID = 0;
					UInt32 savedScope = 0;
					UInt32 pathLength = 0;
					if (!ReadExactRecordData(&savedTargetFormID, sizeof(savedTargetFormID)) ||
						!ReadExactRecordData(&savedScope, sizeof(savedScope)) ||
						!ReadExactRecordData(&pathLength, sizeof(pathLength)))
					{
						++skipped;
						break;
					}

					std::string animPath;
					if (!ReadRecordString(pathLength, &animPath))
					{
						++skipped;
						continue;
					}

					UInt32 resolvedTargetFormID = 0;
					if (!g_serialization->ResolveRefID(savedTargetFormID, &resolvedTargetFormID))
					{
						++skipped;
						continue;
					}

					TESForm* target = LookupFormByID(resolvedTargetFormID);
					if (target && savedScope == GetTargetAnimScopeForForm(target) && AddTargetAnimationMapping(target, animPath.c_str(), true))
						++targetRestored;
					else
						++skipped;
				}
				else if (type == kSerializationRecordAutoWeaponRules)
				{
					UInt32 nameLength = 0;
					if (!ReadExactRecordData(&nameLength, sizeof(nameLength)))
					{
						++skipped;
						break;
					}

					std::string nameContains;
					if (!ReadRecordString(nameLength, &nameContains))
					{
						++skipped;
						continue;
					}

					UInt32 pathLength = 0;
					if (!ReadExactRecordData(&pathLength, sizeof(pathLength)))
					{
						++skipped;
						break;
					}

					std::string animPath;
					if (!ReadRecordString(pathLength, &animPath))
					{
						++skipped;
						continue;
					}

					if (AddAutoWeaponAnimationRule(nameContains.c_str(), animPath.c_str(), true, false))
						++autoWeaponRuleRestored;
					else
						++skipped;
				}
			}
		}

		_MESSAGE("CustomAnimSupport loaded persisted animations: restored=%u weaponMappings=%u targetMappings=%u autoWeaponRules=%u skipped=%u active=%u activeWeaponMappings=%u activeTargetMappings=%u activeAutoWeaponRules=%u weaponGeneration=%u targetGeneration=%u autoWeaponGeneration=%u",
			restored,
			weaponRestored,
			targetRestored,
			autoWeaponRuleRestored,
			skipped,
			s_persistentAnims.size(),
			s_weaponAnimEntries.size(),
			s_targetAnimEntries.size(),
			CountAutoWeaponAnimationRules(NULL),
			s_weaponAnimGeneration,
			s_targetAnimGeneration,
			s_autoWeaponAnimGeneration);
	}

	static TESForm* ResolveManifestForm(const std::string& formID, const std::string& modName)
	{
		UInt32 refID = 0;
		if (!ParseFormIDString(formID, &refID))
			return NULL;

		if (!modName.empty())
		{
			if (!g_dataHandler || !*g_dataHandler)
				return NULL;

			UInt8 modIndex = (*g_dataHandler)->GetModIndex(modName.c_str());
			if (modIndex == 0xFF)
				return NULL;

			refID = (refID & 0x00FFFFFF) | (modIndex << 24);
		}

		return LookupFormByID(refID);
	}

	static bool AddUniqueTarget(std::vector<TESActorBase*>* targets, TESActorBase* actorBase)
	{
		if (!targets || !actorBase)
			return false;

		for (std::vector<TESActorBase*>::const_iterator it = targets->begin(); it != targets->end(); ++it)
		{
			if (*it == actorBase)
				return false;
		}

		targets->push_back(actorBase);
		return true;
	}

	static bool ShouldReloadRefForBase(TESObjectREFR* ref, TESActorBase* actorBase)
	{
		return ref && actorBase && GetActorBaseFromRef(ref) == actorBase;
	}

	static ManifestResult LoadSpecialAnimManifest(TESObjectREFR* ref, TESActorBase* explicitBase, const char* manifestPath, bool trackPersistent)
	{
		ManifestResult result;

		char cleanPath[kMaxManifestPath] = { 0 };
		strncpy_s(cleanPath, sizeof(cleanPath), manifestPath ? manifestPath : "", _TRUNCATE);
		NormalizeSlashes(cleanPath);

		std::string fileText;
		if (!ReadTextFile(cleanPath, &fileText))
		{
			++result.errors;
			return result;
		}
		result.files = 1;

		std::vector<ManifestEntry> entries;
		ManifestParser parser(fileText);
		if (!parser.Parse(&entries))
		{
			++result.errors;
			return result;
		}

		TESActorBase* fallbackBase = ResolveActorBase(ref, explicitBase);
		for (std::vector<ManifestEntry>::const_iterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt)
		{
			const ManifestEntry& entry = *entryIt;
			++result.entries;
			result.ignoredFields += entry.ignoredFields;
			result.conditionFields += entry.conditionFields;

			char folder[kMaxSpecialAnimPath] = { 0 };
			if (!NormalizeRelativeFolder(entry.folder.c_str(), folder, sizeof(folder)) || !folder[0])
			{
				++result.errors;
				continue;
			}

			if (EntryHasUnsupportedConditionPolling(entry))
			{
				++result.conditionalEntries;
				++result.unsupported;
				_MESSAGE("CustomAnimSupport manifest load skipped conditional entry folder=\"%s\" condition=%u pollCondition=%u; no verified Oblivion KFFZ/SpecialAnims condition polling map",
					folder,
					entry.hasCondition ? 1 : 0,
					entry.hasPollCondition ? 1 : 0);
				continue;
			}

			if (!entry.weaponNameContains.empty())
			{
				++result.targets;
				bool added = AddAutoWeaponAnimationRule(entry.weaponNameContains.c_str(), folder, false, false);
				if (added)
					++result.added;
				else
					++result.skipped;
				_MESSAGE("CustomAnimSupport manifest load auto-weapon rule weaponNameContains=\"%s\" folder=\"%s\" added=%u rules=%u generation=%u",
					entry.weaponNameContains.c_str(),
					folder,
					added ? 1 : 0,
					CountAutoWeaponAnimationRules(NULL),
					s_autoWeaponAnimGeneration);
				continue;
			}

			std::vector<TESActorBase*> targets;
			if (!entry.forms.empty())
			{
				for (std::vector<std::string>::const_iterator formIt = entry.forms.begin(); formIt != entry.forms.end(); ++formIt)
				{
					TESForm* form = NULL;
					TESActorBase* actorBase = ResolveManifestActorBaseWithForm(*formIt, entry.modName, &form);
					if (actorBase)
						AddUniqueTarget(&targets, actorBase);
					else if (IsTargetAnimForm(form))
					{
						++result.targets;
						bool added = AddTargetAnimationMapping(form, folder, false);
						if (added)
							++result.added;
						else
							++result.skipped;
						LogTargetManifestMapping("load", *formIt, entry.modName, form, folder, added, false);
					}
					else if (form)
					{
						++result.unsupported;
						LogUnsupportedManifestTarget("load", *formIt, entry.modName, form);
					}
					else
						++result.errors;
				}
			}
			else if (fallbackBase)
			{
				AddUniqueTarget(&targets, fallbackBase);
			}
			else
			{
				++result.errors;
			}

			for (std::vector<TESActorBase*>::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt)
			{
				TESActorBase* actorBase = *targetIt;
				++result.targets;
				DiscoverResult discovered = DiscoverSpecialAnimations(actorBase, folder, trackPersistent);
				AddDiscoverResult(&result, discovered);
				if (discovered.added && ShouldReloadRefForBase(ref, actorBase))
					LoadSpecialAnimationsForRef(ref, actorBase);
			}
		}

		return result;
	}

	static ManifestResult ValidateSpecialAnimManifest(TESObjectREFR* ref, TESActorBase* explicitBase, const char* manifestPath, bool printToConsole)
	{
		ManifestResult result;

		char cleanPath[kMaxManifestPath] = { 0 };
		strncpy_s(cleanPath, sizeof(cleanPath), manifestPath ? manifestPath : "", _TRUNCATE);
		NormalizeSlashes(cleanPath);

		std::string fileText;
		if (!ReadTextFile(cleanPath, &fileText))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimManifest: could not read \"%s\"", cleanPath);
			++result.errors;
			return result;
		}
		result.files = 1;

		std::vector<ManifestEntry> entries;
		ManifestParser parser(fileText);
		if (!parser.Parse(&entries))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimManifest: invalid JSON shape in \"%s\"", cleanPath);
			++result.errors;
			return result;
		}

		TESActorBase* fallbackBase = ResolveActorBase(ref, explicitBase);
		UInt32 entryIndex = 0;
		for (std::vector<ManifestEntry>::const_iterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt, ++entryIndex)
		{
			const ManifestEntry& entry = *entryIt;
			++result.entries;
			result.ignoredFields += entry.ignoredFields;
			result.conditionFields += entry.conditionFields;

			char folder[kMaxSpecialAnimPath] = { 0 };
			if (!NormalizeRelativeFolder(entry.folder.c_str(), folder, sizeof(folder)) || !folder[0])
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimManifest: entry %u unsafe folder \"%s\"", entryIndex, entry.folder.c_str());
				++result.errors;
				continue;
			}

			if (EntryHasUnsupportedConditionPolling(entry))
			{
				++result.conditionalEntries;
				++result.unsupported;
				_MESSAGE("CustomAnimSupport CASValidateSpecialAnimManifest skipped conditional entry index=%u folder=\"%s\" condition=%u pollCondition=%u; no verified Oblivion KFFZ/SpecialAnims condition polling map",
					entryIndex,
					folder,
					entry.hasCondition ? 1 : 0,
					entry.hasPollCondition ? 1 : 0);
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimManifest: entry %u folder \"%s\" has condition/pollCondition and will not be imported by native KFFZ loading", entryIndex, folder);
				continue;
			}

			if (!entry.weaponNameContains.empty())
			{
				++result.targets;
				++result.found;
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimManifest: entry %u auto-weapon rule weaponNameContains=\"%s\" folder=\"%s\"", entryIndex, entry.weaponNameContains.c_str(), folder);
				_MESSAGE("CustomAnimSupport CASValidateSpecialAnimManifest auto-weapon rule index=%u weaponNameContains=\"%s\" folder=\"%s\"",
					entryIndex,
					entry.weaponNameContains.c_str(),
					folder);
				continue;
			}

			std::vector<TESActorBase*> targets;
			if (!entry.forms.empty())
			{
				for (std::vector<std::string>::const_iterator formIt = entry.forms.begin(); formIt != entry.forms.end(); ++formIt)
				{
					TESForm* form = NULL;
					TESActorBase* actorBase = ResolveManifestActorBaseWithForm(*formIt, entry.modName, &form);
					if (actorBase)
					{
						AddUniqueTarget(&targets, actorBase);
					}
					else if (IsTargetAnimForm(form))
					{
						++result.targets;
						++result.found;
						PrintTargetManifestMapping("CASValidateSpecialAnimManifest", *formIt, entry.modName, form, folder, printToConsole);
					}
					else if (form)
					{
						if (printToConsole)
							PrintUnsupportedManifestTarget("CASValidateSpecialAnimManifest", *formIt, entry.modName, form, printToConsole);
						++result.unsupported;
					}
					else
					{
						if (printToConsole)
							Console_Print("CASValidateSpecialAnimManifest: entry %u unresolved form \"%s\" mod=\"%s\"", entryIndex, formIt->c_str(), entry.modName.c_str());
						++result.errors;
					}
				}
			}
			else if (fallbackBase)
			{
				AddUniqueTarget(&targets, fallbackBase);
			}
			else
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimManifest: entry %u has no form target and no calling actor base", entryIndex);
				++result.errors;
			}

			for (std::vector<TESActorBase*>::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt)
			{
				++result.targets;
				DiscoverResult validated = ValidateSpecialAnimFolder(*targetIt, folder, GetValidationSkeletonRoot(ref, *targetIt), printToConsole);
				AddDiscoverResult(&result, validated);
			}
		}

		return result;
	}

	static ManifestResult LoadSpecialAnimManifestDirectory(const char* directory, bool trackPersistent)
	{
		ManifestResult result;

		char cleanDir[kMaxManifestPath] = { 0 };
		if (directory && directory[0])
			strncpy_s(cleanDir, sizeof(cleanDir), directory, _TRUNCATE);
		else
			strncpy_s(cleanDir, sizeof(cleanDir), kDefaultManifestDirectory, _TRUNCATE);

		NormalizeSlashes(cleanDir);
		TrimTrailingSlashes(cleanDir);
		if (!cleanDir[0] || !IsSafeRelativePath(cleanDir))
		{
			++result.errors;
			return result;
		}

		if (!DirectoryExists(cleanDir))
			return result;

		UInt32 clearedTargetMappings = ClearConfigTargetAnimationMappings();
		if (clearedTargetMappings)
		{
			ClearTargetAnimationAttempts();
			_MESSAGE("CustomAnimSupport manifest directory reload cleared config target mappings: directory=\"%s\" cleared=%u generation=%u",
				cleanDir,
				clearedTargetMappings,
				s_targetAnimGeneration);
		}

		UInt32 clearedAutoWeaponRules = ClearConfigAutoWeaponAnimationRules();
		if (clearedAutoWeaponRules)
		{
			s_autoWeaponAnimAttempts.clear();
			_MESSAGE("CustomAnimSupport manifest directory reload cleared config auto-weapon rules: directory=\"%s\" cleared=%u generation=%u",
				cleanDir,
				clearedAutoWeaponRules,
				s_autoWeaponAnimGeneration);
		}

		char searchPath[kMaxManifestPath + 16] = { 0 };
		if (sprintf_s(searchPath, sizeof(searchPath), "%s\\*.json", cleanDir) <= 0)
		{
			++result.errors;
			return result;
		}

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return result;

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			if (!IsJSONPath(data.cFileName))
				continue;

			char manifestPath[kMaxManifestPath] = { 0 };
			if (sprintf_s(manifestPath, sizeof(manifestPath), "%s\\%s", cleanDir, data.cFileName) <= 0)
			{
				++result.errors;
				continue;
			}

			ManifestResult loaded = LoadSpecialAnimManifest(NULL, NULL, manifestPath, trackPersistent);
			AddManifestResult(&result, loaded);
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
		return result;
	}

	static ManifestResult ValidateSpecialAnimManifestDirectory(TESObjectREFR* ref, const char* directory, bool printToConsole)
	{
		ManifestResult result;

		char cleanDir[kMaxManifestPath] = { 0 };
		if (directory && directory[0])
			strncpy_s(cleanDir, sizeof(cleanDir), directory, _TRUNCATE);
		else
			strncpy_s(cleanDir, sizeof(cleanDir), kDefaultManifestDirectory, _TRUNCATE);

		NormalizeSlashes(cleanDir);
		TrimTrailingSlashes(cleanDir);
		if (!cleanDir[0] || !IsSafeRelativePath(cleanDir))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimManifests: unsafe directory \"%s\"", cleanDir);
			++result.errors;
			return result;
		}

		if (!DirectoryExists(cleanDir))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimManifests: missing directory %s", cleanDir);
			++result.errors;
			return result;
		}

		char searchPath[kMaxManifestPath + 16] = { 0 };
		if (sprintf_s(searchPath, sizeof(searchPath), "%s\\*.json", cleanDir) <= 0)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimManifests: directory path too long \"%s\"", cleanDir);
			++result.errors;
			return result;
		}

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimManifests: no .json manifests under %s", cleanDir);
			++result.errors;
			return result;
		}

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			if (!IsJSONPath(data.cFileName))
				continue;

			char manifestPath[kMaxManifestPath] = { 0 };
			if (sprintf_s(manifestPath, sizeof(manifestPath), "%s\\%s", cleanDir, data.cFileName) <= 0)
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimManifests: manifest path too long \"%s\\%s\"", cleanDir, data.cFileName);
				++result.errors;
				continue;
			}

			ManifestResult validated = ValidateSpecialAnimManifest(ref, NULL, manifestPath, printToConsole);
			AddManifestResult(&result, validated);
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
		return result;
	}

	static bool PathHasSegment(const char* path, const char* segment)
	{
		if (!path || !segment || !segment[0])
			return false;

		UInt32 segmentLen = std::strlen(segment);
		const char* cur = path;
		while (*cur)
		{
			while (IsSlash(*cur))
				++cur;

			const char* start = cur;
			while (*cur && !IsSlash(*cur))
				++cur;

			if ((UInt32)(cur - start) == segmentLen && _strnicmp(start, segment, segmentLen) == 0)
				return true;
		}

		return false;
	}

	static bool PathHasKNVSEConditionalFolder(const char* path)
	{
		return PathHasSegment(path, "mod1") ||
			PathHasSegment(path, "mod2") ||
			PathHasSegment(path, "mod3") ||
			PathHasSegment(path, "hurt") ||
			PathHasSegment(path, "human") ||
			PathHasSegment(path, "male") ||
			PathHasSegment(path, "female");
	}

	static void ScanKNVSEKFDirectory(const char* root, const char* relativePrefix, KNVSELayoutResult* result, KNVSEFolderStats* stats, bool printToConsole)
	{
		if (!root || !result || !stats)
			return;

		char searchPath[1024];
		if (relativePrefix && relativePrefix[0])
			sprintf_s(searchPath, sizeof(searchPath), "%s\\%s\\*", root, relativePrefix);
		else
			sprintf_s(searchPath, sizeof(searchPath), "%s\\*", root);

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return;

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			char relativePath[kMaxSpecialAnimPath];
			AppendRelativePath(relativePath, sizeof(relativePath), relativePrefix, data.cFileName);

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				++result->folders;
				if (IsSafeRelativePath(relativePath))
					ScanKNVSEKFDirectory(root, relativePath, result, stats, printToConsole);
				else
					++result->errors;

				continue;
			}

			if (!IsKFPath(relativePath))
				continue;

			++result->kfs;
			++stats->kfs;

			bool firstPerson = PathHasSegment(relativePath, "_1stperson");
			bool conditional = PathHasKNVSEConditionalFolder(relativePath);

			if (firstPerson)
			{
				++result->firstPersonKfs;
				++stats->firstPersonKfs;
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: unsupported _1stperson KF %s", relativePath);
			}

			if (conditional)
			{
				++stats->conditionalKfs;
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: unsupported kNVSE conditional folder in %s", relativePath);
			}

			if (!firstPerson && !conditional)
				++stats->stageableKfs;
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
	}

	static UInt32 CountMissingStageableKNVSEKFs(const char* sourceRoot, const char* relativePrefix, const char* nativeRoot, const char* destinationFolder, UInt32* outChecked)
	{
		if (outChecked)
			*outChecked = 0;
		if (!sourceRoot || !nativeRoot || !destinationFolder || !destinationFolder[0])
			return 0;

		char searchPath[1024];
		if (relativePrefix && relativePrefix[0])
			sprintf_s(searchPath, sizeof(searchPath), "%s\\%s\\*", sourceRoot, relativePrefix);
		else
			sprintf_s(searchPath, sizeof(searchPath), "%s\\*", sourceRoot);

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return 0;

		UInt32 checked = 0;
		UInt32 missing = 0;
		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			char relativePath[kMaxSpecialAnimPath];
			AppendRelativePath(relativePath, sizeof(relativePath), relativePrefix, data.cFileName);
			if (!IsSafeRelativePath(relativePath))
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				UInt32 childChecked = 0;
				missing += CountMissingStageableKNVSEKFs(sourceRoot, relativePath, nativeRoot, destinationFolder, &childChecked);
				checked += childChecked;
				continue;
			}

			if (!IsKFPath(relativePath) ||
				PathHasSegment(relativePath, "_1stperson") ||
				PathHasKNVSEConditionalFolder(relativePath))
			{
				continue;
			}

			char nativeRelativePath[kMaxSpecialAnimPath];
			AppendRelativePath(nativeRelativePath, sizeof(nativeRelativePath), destinationFolder, relativePath);
			if (!IsPersistableAnimationPath(nativeRelativePath))
				continue;

			++checked;
			char destinationPath[1024];
			if (sprintf_s(destinationPath, sizeof(destinationPath), "%s\\%s", nativeRoot, nativeRelativePath) <= 0 ||
				!FileExists(destinationPath))
			{
				++missing;
			}
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
		if (outChecked)
			*outChecked = checked;
		return missing;
	}

	static void NoteKNVSEUnsupportedField(KNVSELayoutResult* result, bool present, const char* fieldName, bool printToConsole)
	{
		if (!result || !present)
			return;

		++result->unsupported;
		if (printToConsole)
		{
			if (_stricmp(fieldName, "condition") == 0 || _stricmp(fieldName, "pollCondition") == 0)
				Console_Print("CASValidateKNVSELayout: JSON field \"%s\" is accepted for diagnostics only; IDA shows idle/package condition evaluation but no KFFZ/SpecialAnims condition polling map", fieldName);
			else
				Console_Print("CASValidateKNVSELayout: JSON field \"%s\" is accepted for diagnostics only; Oblivion-native KFFZ has no verified equivalent", fieldName);
		}
	}

	static void DiagnoseKNVSEUnsupportedFields(const ManifestEntry& entry, KNVSELayoutResult* result, bool printToConsole)
	{
		if (!result)
			return;

		NoteKNVSEUnsupportedField(result, entry.hasCondition, "condition", printToConsole);
		NoteKNVSEUnsupportedField(result, entry.hasPollCondition, "pollCondition", printToConsole);
		NoteKNVSEUnsupportedField(result, entry.hasPriority, "priority", printToConsole);
		NoteKNVSEUnsupportedField(result, entry.hasBSA, "bsa", printToConsole);
		NoteKNVSEUnsupportedField(result, entry.hasMatchBaseAnimGroup, "matchBaseAnimGroup", printToConsole);
	}

	static void DiagnoseKNVSEActorTarget(TESObjectREFR* ref, TESActorBase* actorBase, const char* root, const char* sourceFolder, const char* destinationFolder, const KNVSEFolderStats& stats, KNVSELayoutResult* result, bool printToConsole)
	{
		if (!result)
			return;

		if (!actorBase)
		{
			++result->unsupported;
			return;
		}

		++result->targets;
		if (!root || !sourceFolder || !sourceFolder[0] || !destinationFolder || !destinationFolder[0])
		{
			++result->errors;
			return;
		}

		char nativeRoot[520];
		if (!GetSpecialAnimsRoot(actorBase, nativeRoot, sizeof(nativeRoot)))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: target %08X has no usable native SpecialAnims root", actorBase->refID);
			return;
		}

		char nativeFolder[1024];
		if (sprintf_s(nativeFolder, sizeof(nativeFolder), "%s\\%s", nativeRoot, destinationFolder) <= 0)
		{
			++result->errors;
			return;
		}

		UInt32 stageableKfs = stats.stageableKfs;
		if (DirectoryExists(nativeFolder))
		{
			DiscoverResult nativeCheck = ValidateSpecialAnimFolder(actorBase, destinationFolder, GetValidationSkeletonRoot(ref, actorBase), printToConsole);
			result->nativeReady += nativeCheck.found;
			result->errors += nativeCheck.errors;
			UInt32 checkedStageable = 0;
			UInt32 missingStageable = 0;
			char sourceRoot[1024];
			if (sprintf_s(sourceRoot, sizeof(sourceRoot), "%s\\%s", root, sourceFolder) > 0)
				missingStageable = CountMissingStageableKNVSEKFs(sourceRoot, NULL, nativeRoot, destinationFolder, &checkedStageable);
			else
				++result->errors;
			result->requiresStaging += missingStageable;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: target %08X native folder ready %s found=%u checkedSource=%u missingStageable=%u issues=%u", actorBase->refID, nativeFolder, nativeCheck.found, checkedStageable, missingStageable, nativeCheck.errors);
		}
		else
		{
			result->requiresStaging += stageableKfs;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: target %08X requires staging %u KF(s) under %s", actorBase->refID, stageableKfs, nativeFolder);
		}
	}

	static void DiagnoseKNVSEManifestEntry(TESObjectREFR* ref, TESActorBase* fallbackBase, const char* root, const ManifestEntry& entry, KNVSELayoutResult* result, bool printToConsole)
	{
		if (!root || !result)
			return;

		++result->entries;
		result->ignoredFields += entry.ignoredFields;
		result->conditionFields += entry.conditionFields;
		DiagnoseKNVSEUnsupportedFields(entry, result, printToConsole);

		char folder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(entry.folder.c_str(), folder, sizeof(folder)) || !folder[0])
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: unsafe or empty folder \"%s\"", entry.folder.c_str());
			return;
		}

		char sourceFolder[1024];
		if (sprintf_s(sourceFolder, sizeof(sourceFolder), "%s\\%s", root, folder) <= 0)
		{
			++result->errors;
			return;
		}

		KNVSEFolderStats stats;
		if (DirectoryExists(sourceFolder))
		{
			++result->folders;
			ScanKNVSEKFDirectory(sourceFolder, NULL, result, &stats, printToConsole);
		}
		else
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: missing kNVSE source folder %s", sourceFolder);
		}

		if (EntryHasUnsupportedConditionPolling(entry))
		{
			++result->conditionalEntries;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: folder \"%s\" has condition/pollCondition and is not stageable/importable until a native KFFZ condition polling path is verified", folder);
			return;
		}

		if (entry.forms.empty())
		{
			if (fallbackBase)
			{
				DiagnoseKNVSEActorTarget(ref, fallbackBase, root, folder, folder, stats, result, printToConsole);
			}
			else
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: folder \"%s\" has no actor-base target; global kNVSE overrides are unsupported", folder);
			}

			return;
		}

		std::vector<TESActorBase*> targets;
		for (std::vector<std::string>::const_iterator formIt = entry.forms.begin(); formIt != entry.forms.end(); ++formIt)
		{
			TESForm* form = ResolveManifestForm(*formIt, entry.modName);
			TESActorBase* actorBase = GetActorBaseFromForm(form);
			if (actorBase)
			{
				AddUniqueTarget(&targets, actorBase);
			}
			else if (IsTargetAnimForm(form))
			{
				++result->targets;
				if (stats.stageableKfs)
					result->requiresStaging += stats.stageableKfs;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: form \"%s\" mod=\"%s\" resolved=%08X type=%02X (%s) is a plugin-managed %s target; package KFs still need a native SpecialAnims folder for each matching actor base",
						formIt->c_str(),
						entry.modName.c_str(),
						form->refID,
						form->typeID,
						GetFormTypeName(form->typeID),
						GetTargetAnimScopeName(GetTargetAnimScopeForForm(form)));
			}
			else
			{
				++result->unsupported;
				if (form)
					PrintUnsupportedManifestTarget("CASValidateKNVSELayout", *formIt, entry.modName, form, printToConsole);
				else if (printToConsole)
					Console_Print("CASValidateKNVSELayout: unresolved form \"%s\" mod=\"%s\"", formIt->c_str(), entry.modName.c_str());
			}
		}

		for (std::vector<TESActorBase*>::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt)
			DiagnoseKNVSEActorTarget(ref, *targetIt, root, folder, folder, stats, result, printToConsole);
	}

	static void ValidateKNVSEManifestFile(TESObjectREFR* ref, TESActorBase* fallbackBase, const char* root, const char* manifestPath, KNVSELayoutResult* result, bool printToConsole)
	{
		if (!manifestPath || !result)
			return;

		std::string fileText;
		if (!ReadTextFile(manifestPath, &fileText))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: could not read manifest %s", manifestPath);
			return;
		}

		++result->files;
		std::vector<ManifestEntry> entries;
		ManifestParser parser(fileText);
		if (!parser.Parse(&entries))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: invalid JSON shape in %s", manifestPath);
			return;
		}

		if (printToConsole)
			Console_Print("CASValidateKNVSELayout: manifest %s entries=%u", manifestPath, (UInt32)entries.size());

		for (std::vector<ManifestEntry>::const_iterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt)
			DiagnoseKNVSEManifestEntry(ref, fallbackBase, root, *entryIt, result, printToConsole);
	}

	static bool IsPluginFolderName(const char* name)
	{
		if (!name)
			return false;

		const char* dot = std::strrchr(name, '.');
		return dot && (_stricmp(dot, ".esp") == 0 || _stricmp(dot, ".esm") == 0);
	}

	static void DiagnoseKNVSELooseFolder(TESObjectREFR* ref, TESActorBase* fallbackBase, const char* root, const char* folderName, KNVSELayoutResult* result, bool printToConsole)
	{
		if (!root || !folderName || !result)
			return;

		char folder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folderName, folder, sizeof(folder)) || !folder[0])
		{
			++result->errors;
			return;
		}

		char sourceFolder[1024];
		if (sprintf_s(sourceFolder, sizeof(sourceFolder), "%s\\%s", root, folder) <= 0)
		{
			++result->errors;
			return;
		}

		KNVSEFolderStats stats;
		++result->folders;
		ScanKNVSEKFDirectory(sourceFolder, NULL, result, &stats, printToConsole);

		if (stats.kfs == 0)
			return;

		if (fallbackBase)
		{
			DiagnoseKNVSEActorTarget(ref, fallbackBase, root, folder, folder, stats, result, printToConsole);
		}
		else
		{
			++result->unsupported;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: loose folder %s contains %u KF(s) but has no actor-base target; pass actorBase or use a JSON form target", folder, stats.kfs);
		}
	}

	static void ValidateKNVSEModFolder(TESObjectREFR* ref, const char* root, const char* modFolderName, KNVSELayoutResult* result, bool printToConsole)
	{
		if (!root || !modFolderName || !result)
			return;

		if (!g_dataHandler || !*g_dataHandler)
		{
			++result->errors;
			return;
		}

		UInt8 modIndex = (*g_dataHandler)->GetModIndex(modFolderName);
		if (modIndex == 0xFF)
		{
			++result->unsupported;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: mod folder %s is not loaded", modFolderName);
			return;
		}

		char modFolder[1024];
		if (sprintf_s(modFolder, sizeof(modFolder), "%s\\%s", root, modFolderName) <= 0)
		{
			++result->errors;
			return;
		}

		char searchPath[1024];
		if (sprintf_s(searchPath, sizeof(searchPath), "%s\\*", modFolder) <= 0)
		{
			++result->errors;
			return;
		}

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return;

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;

			if (data.cFileName[0] == '_')
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: mod-index/global POV folder %s\\%s is unsupported without an actor-base target", modFolderName, data.cFileName);
				continue;
			}

			UInt32 localFormID = 0;
			if (!ParseFormIDString(data.cFileName, &localFormID))
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: mod folder child %s\\%s is not a form id", modFolderName, data.cFileName);
				continue;
			}

			UInt32 formID = (localFormID & 0x00FFFFFF) | (modIndex << 24);
			TESForm* form = LookupFormByID(formID);
			TESActorBase* actorBase = GetActorBaseFromForm(form);
			if (!actorBase)
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASValidateKNVSELayout: form folder %s\\%s resolved to %08X type=%02X (%s) but is not an actor base/reference; race/global targets are only accepted from JSON manifests as plugin-managed mappings, weapon/form-list/global-folder kNVSE targets remain unsupported by the decoded Oblivion KFFZ path",
						modFolderName,
						data.cFileName,
						formID,
						form ? form->typeID : 0,
						form ? GetFormTypeName(form->typeID) : "Missing");
				continue;
			}

			char relativeFolder[kMaxSpecialAnimPath] = { 0 };
			AppendRelativePath(relativeFolder, sizeof(relativeFolder), modFolderName, data.cFileName);
			DiagnoseKNVSELooseFolder(ref, actorBase, root, relativeFolder, result, printToConsole);
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
	}

	static KNVSELayoutResult ValidateKNVSELayout(TESObjectREFR* ref, const char* directory, TESActorBase* explicitBase, bool printToConsole)
	{
		KNVSELayoutResult result;

		char cleanDir[kMaxManifestPath] = { 0 };
		if (directory && directory[0])
			strncpy_s(cleanDir, sizeof(cleanDir), directory, _TRUNCATE);
		else
			strncpy_s(cleanDir, sizeof(cleanDir), kDefaultKNVSEAnimGroupOverrideDirectory, _TRUNCATE);

		NormalizeSlashes(cleanDir);
		TrimTrailingSlashes(cleanDir);
		if (!cleanDir[0] || !IsSafeRelativePath(cleanDir))
		{
			++result.errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: unsafe directory \"%s\"", cleanDir);
			return result;
		}

		if (!DirectoryExists(cleanDir))
		{
			++result.errors;
			if (printToConsole)
				Console_Print("CASValidateKNVSELayout: missing directory %s", cleanDir);
			return result;
		}

		TESActorBase* fallbackBase = ResolveActorBase(ref, explicitBase);

		char searchPath[kMaxManifestPath + 16] = { 0 };
		if (sprintf_s(searchPath, sizeof(searchPath), "%s\\*", cleanDir) <= 0)
		{
			++result.errors;
			return result;
		}

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
		{
			++result.errors;
			return result;
		}

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!IsSafeRelativePath(data.cFileName))
				{
					++result.errors;
					continue;
				}

				if (IsPluginFolderName(data.cFileName))
					ValidateKNVSEModFolder(ref, cleanDir, data.cFileName, &result, printToConsole);
				else
					DiagnoseKNVSELooseFolder(ref, fallbackBase, cleanDir, data.cFileName, &result, printToConsole);

				continue;
			}

			if (IsJSONPath(data.cFileName))
			{
				char manifestPath[kMaxManifestPath] = { 0 };
				if (sprintf_s(manifestPath, sizeof(manifestPath), "%s\\%s", cleanDir, data.cFileName) <= 0)
				{
					++result.errors;
					continue;
				}

				ValidateKNVSEManifestFile(ref, fallbackBase, cleanDir, manifestPath, &result, printToConsole);
			}
			else
			{
				const char* dot = std::strrchr(data.cFileName, '.');
				if (dot && _stricmp(dot, ".bsa") == 0)
				{
					++result.files;
					++result.unsupported;
					if (printToConsole)
						Console_Print("CASValidateKNVSELayout: BSA intake is diagnostic-only and not imported by Oblivion-native KFFZ path: %s", data.cFileName);
				}
			}
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
		return result;
	}

	static bool EnsureDirectoryTree(const char* directory)
	{
		if (!directory || !directory[0])
			return false;

		char clean[1024];
		strncpy_s(clean, sizeof(clean), directory, _TRUNCATE);
		NormalizeSlashes(clean);
		TrimTrailingSlashes(clean);
		if (!clean[0] || !IsSafeRelativePath(clean))
			return false;

		if (DirectoryExists(clean))
			return true;

		for (char* cur = clean; *cur; ++cur)
		{
			if (!IsSlash(*cur))
				continue;

			char saved = *cur;
			*cur = 0;
			if (clean[0] && !DirectoryExists(clean))
			{
				if (!CreateDirectoryA(clean, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
				{
					*cur = saved;
					return false;
				}
			}
			*cur = saved;
		}

		if (!DirectoryExists(clean))
			return CreateDirectoryA(clean, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;

		return true;
	}

	static bool GetParentDirectory(const char* path, char* outDirectory, UInt32 outLen)
	{
		if (!path || !outDirectory || outLen == 0)
			return false;

		outDirectory[0] = 0;
		const char* slash = std::strrchr(path, '\\');
		if (!slash)
			return false;

		UInt32 len = (UInt32)(slash - path);
		if (len == 0 || len >= outLen)
			return false;

		memcpy(outDirectory, path, len);
		outDirectory[len] = 0;
		return true;
	}

	static void NoteKNVSEStageUnsupportedField(KNVSEStageResult* result, bool present, const char* fieldName, bool printToConsole)
	{
		if (!result || !present)
			return;

		++result->unsupported;
		if (printToConsole)
		{
			if (_stricmp(fieldName, "condition") == 0 || _stricmp(fieldName, "pollCondition") == 0)
				Console_Print("CASStageKNVSELayout: JSON field \"%s\" is not evaluated while staging; IDA shows no KFFZ/SpecialAnims condition polling map", fieldName);
			else
				Console_Print("CASStageKNVSELayout: JSON field \"%s\" is ignored while staging to native SpecialAnims", fieldName);
		}
	}

	static void StageKNVSEUnsupportedFields(const ManifestEntry& entry, KNVSEStageResult* result, bool printToConsole)
	{
		if (!result)
			return;

		NoteKNVSEStageUnsupportedField(result, entry.hasCondition, "condition", printToConsole);
		NoteKNVSEStageUnsupportedField(result, entry.hasPollCondition, "pollCondition", printToConsole);
		NoteKNVSEStageUnsupportedField(result, entry.hasPriority, "priority", printToConsole);
		NoteKNVSEStageUnsupportedField(result, entry.hasBSA, "bsa", printToConsole);
		NoteKNVSEStageUnsupportedField(result, entry.hasMatchBaseAnimGroup, "matchBaseAnimGroup", printToConsole);
	}

	static bool StageKNVSEKFFile(TESActorBase* actorBase, TESAnimation* anim, const char* sourceRoot, const char* sourceRelativePath, const char* nativeRoot, const char* destinationFolder, bool activate, KNVSEStageResult* result, bool printToConsole)
	{
		if (!actorBase || (activate && !anim) || !sourceRoot || !sourceRelativePath || !nativeRoot || !destinationFolder || !result)
			return false;

		char sourcePath[1024];
		if (sprintf_s(sourcePath, sizeof(sourcePath), "%s\\%s", sourceRoot, sourceRelativePath) <= 0)
		{
			++result->errors;
			return false;
		}

		char nativeRelativePath[kMaxSpecialAnimPath];
		AppendRelativePath(nativeRelativePath, sizeof(nativeRelativePath), destinationFolder, sourceRelativePath);
		if (!IsPersistableAnimationPath(nativeRelativePath))
		{
			++result->errors;
			return false;
		}

		char destinationPath[1024];
		if (sprintf_s(destinationPath, sizeof(destinationPath), "%s\\%s", nativeRoot, nativeRelativePath) <= 0)
		{
			++result->errors;
			return false;
		}

		bool staged = true;
		if (FileExists(destinationPath))
		{
			++result->existing;
		}
		else
		{
			char parentDirectory[1024];
			if (!GetParentDirectory(destinationPath, parentDirectory, sizeof(parentDirectory)) || !EnsureDirectoryTree(parentDirectory))
			{
				++result->errors;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: could not create native destination for %s", destinationPath);
				return false;
			}

			if (CopyFileA(sourcePath, destinationPath, TRUE))
			{
				++result->copied;
			}
			else
			{
				staged = false;
				++result->errors;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: failed to copy %s -> %s error=%u", sourcePath, destinationPath, GetLastError());
			}
		}

		if (!staged)
			return false;

		// Match 0x476080: native KFFZ playback loads from the final actor-model SpecialAnims path.
		UInt16 parsedKey = 0xFFFF;
		UInt32 parsedGroupID = TESAnimGroup::kAnimGroup_Max;
		char loaderPath[1024] = { 0 };
		if (!ReadSpecialAnimParsedKey(actorBase, nativeRelativePath, &parsedKey, &parsedGroupID, loaderPath, sizeof(loaderPath), false))
		{
			++result->invalid;
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: not accepting %s; copied destination did not parse through native SpecialAnims loader path %s", nativeRelativePath, loaderPath[0] ? loaderPath : destinationPath);
			return false;
		}

		++result->validated;
		bool added = false;
		if (activate)
		{
			added = AddAnimation(anim, nativeRelativePath);
			if (added)
				++result->registered;
			else
				++result->skipped;

			if (added || HasExactAnimation(anim, nativeRelativePath))
				AddPersistentAnimation(actorBase, nativeRelativePath);
		}

		if (printToConsole)
		{
			const char* groupName = parsedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(parsedGroupID) : "";
			Console_Print("CASStageKNVSELayout: staged %s as %s key=%04X group=%s activate=%u registered=%u", sourcePath, nativeRelativePath, parsedKey, groupName ? groupName : "", activate ? 1 : 0, added ? 1 : 0);
		}

		return true;
	}

	static void StageKNVSEKFDirectory(TESActorBase* actorBase, TESAnimation* anim, const char* sourceRoot, const char* relativePrefix, const char* nativeRoot, const char* destinationFolder, bool activate, KNVSEStageResult* result, bool printToConsole)
	{
		if (!actorBase || (activate && !anim) || !sourceRoot || !nativeRoot || !destinationFolder || !result)
			return;

		char searchPath[1024];
		if (relativePrefix && relativePrefix[0])
			sprintf_s(searchPath, sizeof(searchPath), "%s\\%s\\*", sourceRoot, relativePrefix);
		else
			sprintf_s(searchPath, sizeof(searchPath), "%s\\*", sourceRoot);

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return;

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			char relativePath[kMaxSpecialAnimPath];
			AppendRelativePath(relativePath, sizeof(relativePath), relativePrefix, data.cFileName);
			if (!IsSafeRelativePath(relativePath))
			{
				++result->errors;
				continue;
			}

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				StageKNVSEKFDirectory(actorBase, anim, sourceRoot, relativePath, nativeRoot, destinationFolder, activate, result, printToConsole);
				continue;
			}

			if (!IsKFPath(relativePath))
				continue;

			++result->found;
			if (PathHasSegment(relativePath, "_1stperson"))
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: skipping _1stperson KF %s; no decoded first-person KFFZ load path", relativePath);
				continue;
			}

			if (PathHasKNVSEConditionalFolder(relativePath))
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: skipping conditional kNVSE branch %s; native KFFZ has no verified folder-condition map", relativePath);
				continue;
			}

			StageKNVSEKFFile(actorBase, anim, sourceRoot, relativePath, nativeRoot, destinationFolder, activate, result, printToConsole);
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
	}

	static void StageKNVSEFolderForActor(TESObjectREFR* ref, TESActorBase* actorBase, const char* root, const char* sourceFolder, const char* destinationFolder, bool activate, KNVSEStageResult* result, bool printToConsole)
	{
		if (!root || !sourceFolder || !destinationFolder || !result)
			return;

		if (!actorBase)
		{
			++result->unsupported;
			return;
		}

		TESAnimation* anim = GetAnimationList(actorBase);
		if (activate && !anim)
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: target %08X has no TESAnimation list", actorBase->refID);
			return;
		}

		char cleanSourceFolder[kMaxSpecialAnimPath] = { 0 };
		char cleanDestinationFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(sourceFolder, cleanSourceFolder, sizeof(cleanSourceFolder)) || !cleanSourceFolder[0] ||
			!NormalizeRelativeFolder(destinationFolder, cleanDestinationFolder, sizeof(cleanDestinationFolder)) || !cleanDestinationFolder[0])
		{
			++result->errors;
			return;
		}

		char sourceRoot[1024];
		if (sprintf_s(sourceRoot, sizeof(sourceRoot), "%s\\%s", root, cleanSourceFolder) <= 0)
		{
			++result->errors;
			return;
		}

		if (!DirectoryExists(sourceRoot))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: missing source folder %s", sourceRoot);
			return;
		}

		char nativeRoot[520];
		if (!GetSpecialAnimsRoot(actorBase, nativeRoot, sizeof(nativeRoot)) || !EnsureDirectoryTree(nativeRoot))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: target %08X has no usable native SpecialAnims root", actorBase->refID);
			return;
		}

		char nativeDestination[1024];
		if (sprintf_s(nativeDestination, sizeof(nativeDestination), "%s\\%s", nativeRoot, cleanDestinationFolder) <= 0 || !EnsureDirectoryTree(nativeDestination))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: could not create destination folder for %s", cleanDestinationFolder);
			return;
		}

		++result->targets;
		if (printToConsole)
			Console_Print("CASStageKNVSELayout: staging %s -> %s for target %08X activate=%u", sourceRoot, nativeDestination, actorBase->refID, activate ? 1 : 0);

		StageKNVSEKFDirectory(actorBase, anim, sourceRoot, NULL, nativeRoot, cleanDestinationFolder, activate, result, printToConsole);

		if (activate && ShouldReloadRefForBase(ref, actorBase))
			LoadSpecialAnimationsForRef(ref, actorBase);
	}

	static void StageKNVSEManifestEntry(TESObjectREFR* ref, TESActorBase* fallbackBase, const char* root, const ManifestEntry& entry, KNVSEStageResult* result, bool printToConsole)
	{
		if (!root || !result)
			return;

		++result->entries;
		result->ignoredFields += entry.ignoredFields;
		result->conditionFields += entry.conditionFields;
		StageKNVSEUnsupportedFields(entry, result, printToConsole);

		char folder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(entry.folder.c_str(), folder, sizeof(folder)) || !folder[0])
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: unsafe or empty folder \"%s\"", entry.folder.c_str());
			return;
		}

		if (EntryHasUnsupportedConditionPolling(entry))
		{
			++result->conditionalEntries;
			++result->skipped;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: skipping folder \"%s\" because condition/pollCondition cannot be evaluated by the decoded KFFZ/SpecialAnims path", folder);
			return;
		}

		if (entry.forms.empty())
		{
			if (fallbackBase)
				StageKNVSEFolderForActor(ref, fallbackBase, root, folder, folder, false, result, printToConsole);
			else
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: folder \"%s\" has no actor-base target; global kNVSE overrides are unsupported", folder);
			}
			return;
		}

		std::vector<TESActorBase*> targets;
		for (std::vector<std::string>::const_iterator formIt = entry.forms.begin(); formIt != entry.forms.end(); ++formIt)
		{
			TESForm* form = NULL;
			TESActorBase* actorBase = ResolveManifestActorBaseWithForm(*formIt, entry.modName, &form);
			if (actorBase)
				AddUniqueTarget(&targets, actorBase);
			else if (IsTargetAnimForm(form))
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: form \"%s\" mod=\"%s\" resolved=%08X type=%02X (%s) is a plugin-managed %s target, but staging still needs a concrete actor-base SpecialAnims destination",
						formIt->c_str(),
						entry.modName.c_str(),
						form->refID,
						form->typeID,
						GetFormTypeName(form->typeID),
						GetTargetAnimScopeName(GetTargetAnimScopeForForm(form)));
			}
			else
			{
				++result->unsupported;
				if (form)
					PrintUnsupportedManifestTarget("CASStageKNVSELayout", *formIt, entry.modName, form, printToConsole);
				else if (printToConsole)
					Console_Print("CASStageKNVSELayout: unresolved form \"%s\" mod=\"%s\"", formIt->c_str(), entry.modName.c_str());
			}
		}

		for (std::vector<TESActorBase*>::const_iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt)
			StageKNVSEFolderForActor(ref, *targetIt, root, folder, folder, false, result, printToConsole);
	}

	static void StageKNVSEManifestFile(TESObjectREFR* ref, TESActorBase* fallbackBase, const char* root, const char* manifestPath, KNVSEStageResult* result, bool printToConsole)
	{
		if (!manifestPath || !result)
			return;

		std::string fileText;
		if (!ReadTextFile(manifestPath, &fileText))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: could not read manifest %s", manifestPath);
			return;
		}

		++result->files;
		std::vector<ManifestEntry> entries;
		ManifestParser parser(fileText);
		if (!parser.Parse(&entries))
		{
			++result->errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: invalid JSON shape in %s", manifestPath);
			return;
		}

		for (std::vector<ManifestEntry>::const_iterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt)
			StageKNVSEManifestEntry(ref, fallbackBase, root, *entryIt, result, printToConsole);
	}

	static void StageKNVSEModFolder(TESObjectREFR* ref, const char* root, const char* modFolderName, KNVSEStageResult* result, bool printToConsole)
	{
		if (!root || !modFolderName || !result)
			return;

		if (!g_dataHandler || !*g_dataHandler)
		{
			++result->errors;
			return;
		}

		UInt8 modIndex = (*g_dataHandler)->GetModIndex(modFolderName);
		if (modIndex == 0xFF)
		{
			++result->unsupported;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: mod folder %s is not loaded", modFolderName);
			return;
		}

		char modFolder[1024];
		if (sprintf_s(modFolder, sizeof(modFolder), "%s\\%s", root, modFolderName) <= 0)
		{
			++result->errors;
			return;
		}

		char searchPath[1024];
		if (sprintf_s(searchPath, sizeof(searchPath), "%s\\*", modFolder) <= 0)
		{
			++result->errors;
			return;
		}

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
			return;

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				continue;

			if (data.cFileName[0] == '_')
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: mod-index/global POV folder %s\\%s is unsupported without an actor-base target", modFolderName, data.cFileName);
				continue;
			}

			UInt32 localFormID = 0;
			if (!ParseFormIDString(data.cFileName, &localFormID))
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: mod folder child %s\\%s is not a form id", modFolderName, data.cFileName);
				continue;
			}

			UInt32 formID = (localFormID & 0x00FFFFFF) | (modIndex << 24);
			TESForm* form = LookupFormByID(formID);
			TESActorBase* actorBase = GetActorBaseFromForm(form);
			if (!actorBase)
			{
				++result->unsupported;
				if (printToConsole)
					Console_Print("CASStageKNVSELayout: form folder %s\\%s resolved to %08X type=%02X (%s) but is not an actor base/reference; race/global targets are only accepted from JSON manifests as plugin-managed mappings, and staging still needs a concrete actor-base SpecialAnims destination",
						modFolderName,
						data.cFileName,
						formID,
						form ? form->typeID : 0,
						form ? GetFormTypeName(form->typeID) : "Missing");
				continue;
			}

			char relativeFolder[kMaxSpecialAnimPath] = { 0 };
			AppendRelativePath(relativeFolder, sizeof(relativeFolder), modFolderName, data.cFileName);
			StageKNVSEFolderForActor(ref, actorBase, root, relativeFolder, relativeFolder, false, result, printToConsole);
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
	}

	static KNVSEStageResult StageKNVSELayout(TESObjectREFR* ref, const char* directory, TESActorBase* explicitBase, bool printToConsole)
	{
		KNVSEStageResult result;

		char cleanDir[kMaxManifestPath] = { 0 };
		if (directory && directory[0])
			strncpy_s(cleanDir, sizeof(cleanDir), directory, _TRUNCATE);
		else
			strncpy_s(cleanDir, sizeof(cleanDir), kDefaultKNVSEAnimGroupOverrideDirectory, _TRUNCATE);

		NormalizeSlashes(cleanDir);
		TrimTrailingSlashes(cleanDir);
		if (!cleanDir[0] || !IsSafeRelativePath(cleanDir))
		{
			++result.errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: unsafe directory \"%s\"", cleanDir);
			return result;
		}

		if (!DirectoryExists(cleanDir))
		{
			++result.errors;
			if (printToConsole)
				Console_Print("CASStageKNVSELayout: missing directory %s", cleanDir);
			return result;
		}

		TESActorBase* fallbackBase = ResolveActorBase(ref, explicitBase);

		char searchPath[kMaxManifestPath + 16] = { 0 };
		if (sprintf_s(searchPath, sizeof(searchPath), "%s\\*", cleanDir) <= 0)
		{
			++result.errors;
			return result;
		}

		WIN32_FIND_DATAA data;
		HANDLE find = FindFirstFileA(searchPath, &data);
		if (find == INVALID_HANDLE_VALUE)
		{
			++result.errors;
			return result;
		}

		do
		{
			if (std::strcmp(data.cFileName, ".") == 0 || std::strcmp(data.cFileName, "..") == 0)
				continue;

			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (!IsSafeRelativePath(data.cFileName))
				{
					++result.errors;
					continue;
				}

				if (IsPluginFolderName(data.cFileName))
				{
					StageKNVSEModFolder(ref, cleanDir, data.cFileName, &result, printToConsole);
				}
				else if (fallbackBase)
				{
					StageKNVSEFolderForActor(ref, fallbackBase, cleanDir, data.cFileName, data.cFileName, false, &result, printToConsole);
				}
				else
				{
					++result.unsupported;
					if (printToConsole)
						Console_Print("CASStageKNVSELayout: loose folder %s has no actor-base target; pass actorBase or dot-call on an actor", data.cFileName);
				}

				continue;
			}

			if (IsJSONPath(data.cFileName))
			{
				char manifestPath[kMaxManifestPath] = { 0 };
				if (sprintf_s(manifestPath, sizeof(manifestPath), "%s\\%s", cleanDir, data.cFileName) <= 0)
				{
					++result.errors;
					continue;
				}

				StageKNVSEManifestFile(ref, fallbackBase, cleanDir, manifestPath, &result, printToConsole);
			}
			else
			{
				const char* dot = std::strrchr(data.cFileName, '.');
				if (dot && _stricmp(dot, ".bsa") == 0)
				{
					++result.files;
					++result.unsupported;
					if (printToConsole)
						Console_Print("CASStageKNVSELayout: BSA intake is unsupported by native SpecialAnims staging: %s", data.cFileName);
				}
			}
		}
		while (FindNextFileA(find, &data));

		FindClose(find);
		return result;
	}

	static KNVSEStageResult StageAnimGroupOverrideFolder(TESObjectREFR* ref, TESActorBase* explicitBase, const char* folder, bool printToConsole)
	{
		KNVSEStageResult result;

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)) || !cleanFolder[0])
		{
			++result.errors;
			if (printToConsole)
				Console_Print("CASStageAnimGroupOverride: unsafe or empty folder \"%s\"", folder ? folder : "");
			return result;
		}

		TESActorBase* actorBase = ResolveActorBase(ref, explicitBase);
		if (!actorBase)
		{
			++result.errors;
			if (printToConsole)
				Console_Print("CASStageAnimGroupOverride: no actor-base target");
			return result;
		}

		StageKNVSEFolderForActor(ref, actorBase, kDefaultKNVSEAnimGroupOverrideDirectory, cleanFolder, cleanFolder, false, &result, printToConsole);
		return result;
	}

	static ManifestResult LoadDefaultManifests(const char* reason)
	{
		ManifestResult loaded = LoadSpecialAnimManifestDirectory(NULL, false);
		_MESSAGE("CustomAnimSupport manifest load (%s): files=%u entries=%u targets=%u found=%u added=%u existing=%u unsupported=%u errors=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u autoWeaponRules=%u autoWeaponGeneration=%u",
			reason ? reason : "manual",
			loaded.files,
			loaded.entries,
			loaded.targets,
			loaded.found,
			loaded.added,
			loaded.skipped,
			loaded.unsupported,
			loaded.errors,
			loaded.ignoredFields,
			loaded.conditionFields,
			loaded.conditionalEntries,
			CountAutoWeaponAnimationRules(NULL),
			s_autoWeaponAnimGeneration);
		return loaded;
	}

	static UInt32 ValidateSpecialAnimations(TESObjectREFR* ref, TESActorBase* actorBase, bool printToConsole)
	{
		UInt32 issues = 0;
		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
			return 1;

		char root[520];
		if (!GetSpecialAnimsRoot(actorBase, root, sizeof(root)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnims: actor base has no usable model directory");
			return 1;
		}

		NiNode* skeletonRoot = GetValidationSkeletonRoot(ref, actorBase);
		if (printToConsole && ref && actorBase && GetActorBaseFromRef(ref) == actorBase && !skeletonRoot)
			Console_Print("CASValidateSpecialAnims: no live skeleton available; skipping skeleton target checks");

		if (!DirectoryExists(root))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnims: missing folder %s", root);
			++issues;
		}

		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			const char* entry = cur->animationName;
			if (!IsSafeRelativePath(entry))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnims: unsafe KFFZ entry \"%s\"", entry ? entry : "");
				++issues;
				continue;
			}

			if (!IsKFPath(entry))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnims: non-KF KFFZ entry \"%s\"", entry);
				++issues;
				continue;
			}

			issues += ValidateSpecialAnimAsset(actorBase, entry, skeletonRoot, printToConsole);
		}

		return issues;
	}

	static bool HasMeshesPrefix(const char* path)
	{
		if (!path)
			return false;

		char cleanPath[260];
		strncpy_s(cleanPath, sizeof(cleanPath), path, _TRUNCATE);
		NormalizeSlashes(cleanPath);

		return _strnicmp(cleanPath, "Meshes\\", 7) == 0 || _stricmp(cleanPath, "Meshes") == 0;
	}

	static TESIdleForm* GetTopIdleForm(TESIdleForm* idle, bool* hitLimit)
	{
		if (hitLimit)
			*hitLimit = false;

		UInt32 guard = 0;
		while (idle && idle->parent && guard < 128)
		{
			idle = idle->parent;
			++guard;
		}

		if (idle && idle->parent && hitLimit)
			*hitLimit = true;

		return idle;
	}

	static UInt32 ValidateIdleForm(TESObjectREFR* ref, TESIdleForm* idle, TESActorBase* actorBase, bool printToConsole)
	{
		if (!idle)
		{
			if (printToConsole)
				Console_Print("CASValidateIdleForm: no IDLE form");
			return 1;
		}

		UInt32 issues = 0;
		const char* modelPath = idle->animModel.GetModelPath();
		UInt32 animType = idle->animFlags & 0x7F;
		bool highBitSet = (idle->animFlags & 0x80) != 0;
		bool exactKF = IsExactIdleKFPath(modelPath);

		if (printToConsole)
			Console_Print("CASValidateIdleForm: idle=%08X model=\"%s\" ANAM=%02X type=%u highBit=%u", idle->refID, modelPath ? modelPath : "", idle->animFlags & 0xFF, animType, highBitSet ? 1 : 0);

		if (!modelPath || !modelPath[0])
		{
			if (printToConsole)
				Console_Print("CASValidateIdleForm: IDLE %08X has no model path", idle->refID);
			++issues;
		}
		else
		{
			if (!IsSafeRelativePath(modelPath))
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: unsafe idle model path \"%s\"", modelPath);
				++issues;
			}

			if (HasMeshesPrefix(modelPath))
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: idle model path should be relative under Meshes, not start with \"Meshes\\\": \"%s\"", modelPath);
				++issues;
			}

			if (!exactKF)
			{
				if (printToConsole)
				{
					if (highBitSet)
						Console_Print("CASValidateIdleForm: ANAM high bit is set but model path extension is not exactly .kf, so PickIdle rejects it");
					else
						Console_Print("CASValidateIdleForm: model path extension is not exactly .kf, so this IDLE is not a custom KF candidate");
				}
				++issues;
			}
		}

		if (animType > TESIdleForm::kFlags_UpperBody)
		{
			if (printToConsole)
				Console_Print("CASValidateIdleForm: ANAM type %u is outside the decoded idle type range 0-6", animType);
			++issues;
		}

		UInt32 parentGuard = 0;
		for (TESIdleForm* cur = idle; cur && parentGuard < 128; cur = cur->parent, ++parentGuard)
		{
			if (cur->flags & kTESFormFlagDisabled)
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: idle chain form %08X is disabled; recursive idle search skips it", cur->refID);
				++issues;
			}
		}
		if (parentGuard >= 128)
		{
			if (printToConsole)
				Console_Print("CASValidateIdleForm: parent chain exceeded guard limit");
			++issues;
		}

		if (actorBase)
		{
			bool hitLimit = false;
			TESIdleForm* topIdle = GetTopIdleForm(idle, &hitLimit);
			if (hitLimit)
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: parent chain did not reach a top-level idle");
				++issues;
			}

			const char* actorModelPath = GetActorBaseModelPath(actorBase);
			const char* topIdleModelPath = topIdle ? topIdle->animModel.GetModelPath() : NULL;
			char actorIdleRoot[260];
			char idleRoot[260];
			if (!DeriveIdleAnimsRoot(actorModelPath, actorIdleRoot, sizeof(actorIdleRoot)))
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: actor base %08X has no usable IdleAnims root", actorBase->refID);
				++issues;
			}
			else if (!DeriveIdleAnimsRoot(topIdleModelPath, idleRoot, sizeof(idleRoot)))
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: top-level idle %08X has no usable IdleAnims root", topIdle ? topIdle->refID : 0);
				++issues;
			}
			else if (std::strcmp(actorIdleRoot, idleRoot) != 0)
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: idle root \"%s\" does not match actor root \"%s\"", idleRoot, actorIdleRoot);
				++issues;
			}
		}

		if (modelPath && modelPath[0] && IsSafeRelativePath(modelPath) && exactKF && !HasMeshesPrefix(modelPath))
		{
			char loaderPath[1024];
			if (sprintf_s(loaderPath, sizeof(loaderPath), "Meshes\\%s", modelPath) <= 0)
			{
				if (printToConsole)
					Console_Print("CASValidateIdleForm: idle loader path too long \"%s\"", modelPath);
				++issues;
			}
			else
			{
				UInt32 kfModel = LoadKFModel(loaderPath);
				if (!kfModel)
				{
					if (printToConsole)
						Console_Print("CASValidateIdleForm: native loader failed for %s", loaderPath);
					++issues;
				}
				else
				{
					issues += ValidateParsedKFModel(NULL, kfModel, loaderPath, GetValidationSkeletonRoot(ref, actorBase), printToConsole);
					ReleaseKFModel(kfModel);
				}
			}
		}

		return issues;
	}

	static MiddleHighProcess* ExtractMiddleHighProcess(TESObjectREFR* ref)
	{
		MiddleHighProcess* proc = NULL;
		MobileObject* mob = OBLIVION_CAST(ref, TESObjectREFR, MobileObject);
		if (mob && mob->process)
			proc = (MiddleHighProcess*)Oblivion_DynamicCast(mob->process, 0, RTTI_BaseProcess, RTTI_MiddleHighProcess, 0);

		return proc;
	}

	static ActorAnimData* GetActorAnimData(TESObjectREFR* ref)
	{
		if (!ref)
			return NULL;

		if (g_thePlayer && ref == *g_thePlayer && (*g_thePlayer)->isThirdPerson == 0)
			return (*g_thePlayer)->firstPersonAnimData;

		MiddleHighProcess* proc = ExtractMiddleHighProcess(ref);
		return proc ? proc->animData : NULL;
	}

	static ActorAnimData* GetActorBaseSpecialAnimData(TESObjectREFR* ref)
	{
		MiddleHighProcess* proc = ExtractMiddleHighProcess(ref);
		return proc ? proc->animData : NULL;
	}

	static bool IsPlayerFirstPersonActive(TESObjectREFR* ref)
	{
		return g_thePlayer && ref == *g_thePlayer && (*g_thePlayer)->isThirdPerson == 0;
	}

	static bool IsPlayerFirstPersonAnimData(TESObjectREFR* ref, ActorAnimData* animData)
	{
		return g_thePlayer && ref == *g_thePlayer && animData && (*g_thePlayer)->firstPersonAnimData == animData;
	}

	static bool ShouldLogActorBaseSpecialAnimFirstPersonBypass()
	{
		++s_actorBaseSpecialAnimFirstPersonBypassLogCount;
		return s_actorBaseSpecialAnimFirstPersonBypassLogCount <= 8 || (s_actorBaseSpecialAnimFirstPersonBypassLogCount % 64) == 0;
	}

	static NiNode* GetActorSkeletonRoot(TESObjectREFR* ref)
	{
		ActorAnimData* animData = GetActorAnimData(ref);
		return animData ? animData->niNode04 : NULL;
	}

	static NiNode* GetActorBaseSkeletonRoot(TESObjectREFR* ref)
	{
		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		return animData ? animData->niNode04 : NULL;
	}

	static NiNode* GetValidationSkeletonRoot(TESObjectREFR* ref, TESActorBase* actorBase)
	{
		if (!ref || !actorBase || GetActorBaseFromRef(ref) != actorBase)
			return NULL;

		return GetActorBaseSkeletonRoot(ref);
	}

	static bool IsActorReference(TESObjectREFR* ref)
	{
		if (!ref)
			return false;

		return ref->typeID == kFormType_ACHR || ref->typeID == kFormType_ACRE;
	}

	enum EquippedWeaponLookupSource
	{
		kEquippedWeaponSource_None = 0,
		kEquippedWeaponSource_ProcessVirtual = 1,
		kEquippedWeaponSource_MiddleHighCache = 2,
		kEquippedWeaponSource_WornExtraList = 3
	};

	static const char* GetEquippedWeaponSourceName(UInt32 source)
	{
		switch (source)
		{
			case kEquippedWeaponSource_ProcessVirtual:
				return "process-virtual";
			case kEquippedWeaponSource_MiddleHighCache:
				return "middle-high-cache";
			case kEquippedWeaponSource_WornExtraList:
				return "worn-extra-list";
			default:
				return "none";
		}
	}

	static BaseProcess* GetBaseProcessForRef(TESObjectREFR* ref)
	{
		MobileObject* mob = OBLIVION_CAST(ref, TESObjectREFR, MobileObject);
		return mob ? mob->process : NULL;
	}

	static TESObjectWEAP* GetWeaponFromEntryData(ExtraContainerChanges::EntryData* weaponData)
	{
		if (weaponData && weaponData->type && weaponData->type->typeID == kFormType_Weapon)
			return (TESObjectWEAP*)weaponData->type;

		return NULL;
	}

	static bool QueueIdleFormForRef(TESObjectREFR* ref, TESIdleForm* idle, TESActorBase* validationBase, bool force, bool validate, bool printToConsole)
	{
		if (!IsActorReference(ref) || !idle)
			return false;

		TESActorBase* actualBase = GetActorBaseFromRef(ref);
		if (validationBase && actualBase != validationBase)
		{
			if (printToConsole)
				Console_Print("CASPlayIdleForm: explicit actor base does not match calling actor");
			return false;
		}

		if (!validationBase)
			validationBase = actualBase;

		const char* modelPath = idle->animModel.GetModelPath();
		if (!modelPath || !std::strstr(modelPath, ".kf"))
		{
			if (printToConsole)
				Console_Print("CASPlayIdleForm: IDLE %08X has no .kf model path", idle->refID);
			return false;
		}

		if (validate && ValidateIdleForm(ref, idle, validationBase, printToConsole) != 0)
		{
			if (printToConsole)
				Console_Print("CASPlayIdleForm: validation failed for IDLE %08X", idle->refID);
			return false;
		}

		ActorAnimData* animData = GetActorAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("CASPlayIdleForm: calling actor has no ActorAnimData");
			return false;
		}

		if (!force && !IsNativePickIdleGateOpen(animData, idle, printToConsole))
			return false;

		UInt32 animType = idle->animFlags & 0x7F;
		return ThisStdCall(kActorAnimDataQueueIdle, animData, idle, ref, animType, 3) != 0;
	}

	static bool LoadSpecialAnimationsForRef(TESObjectREFR* ref, TESActorBase* explicitBase)
	{
		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		TESActorBase* actorBase = ResolveActorBase(ref, explicitBase);
		TESAnimation* anim = GetAnimationList(actorBase);

		if (explicitBase && GetActorBaseFromRef(ref) != explicitBase)
			return false;

		if (!animData || !anim || !anim->data.animationName)
			return false;

		char modelDir[260];
		if (!GetModelDirectory(actorBase, modelDir, sizeof(modelDir)))
			return false;

		if (IsPlayerFirstPersonActive(ref) && ShouldLogActorBaseSpecialAnimFirstPersonBypass())
		{
			_MESSAGE("CustomAnimSupport actor-base SpecialAnims load using process animData=%08X while player first-person is active firstPersonAnimData=%08X actor=%08X actorBase=%08X; IDA sub_667BE0 builds first-person data from _1stPerson skeleton",
				(UInt32)animData,
				g_thePlayer && *g_thePlayer ? (UInt32)(*g_thePlayer)->firstPersonAnimData : 0,
				ref ? ref->refID : 0,
				actorBase ? actorBase->refID : 0);
		}

		ThisStdCall(kActorAnimDataLoadKFFZSpecialAnims, animData, &anim->data, modelDir);
		return true;
	}

	static WeaponAnimAttempt* FindWeaponAnimAttempt(UInt32 actorRefID, UInt32 weaponRefID, UInt32 animDataID)
	{
		for (std::vector<WeaponAnimAttempt>::iterator it = s_weaponAnimAttempts.begin(); it != s_weaponAnimAttempts.end(); ++it)
		{
			if (it->actorRefID == actorRefID && it->weaponRefID == weaponRefID && it->animDataID == animDataID)
				return &(*it);
		}

		return NULL;
	}

	static void RecordWeaponAnimAttempt(UInt32 actorRefID, UInt32 weaponRefID, UInt32 actorBaseRefID, UInt32 animDataID, UInt32 actorTypeID, bool loaded, const std::vector<ScopedInstalledAnim>& installed)
	{
		WeaponAnimAttempt attempt;
		attempt.actorRefID = actorRefID;
		attempt.weaponRefID = weaponRefID;
		attempt.actorBaseRefID = actorBaseRefID;
		attempt.animDataID = animDataID;
		attempt.actorTypeID = actorTypeID;
		attempt.generation = s_weaponAnimGeneration;
		attempt.loaded = loaded;
		attempt.installed = installed;
		s_weaponAnimAttempts.push_back(attempt);
	}

	static TargetAnimAttempt* FindTargetAnimAttempt(UInt32 actorRefID, UInt32 targetFormID, UInt32 scope, UInt32 animDataID)
	{
		for (std::vector<TargetAnimAttempt>::iterator it = s_targetAnimAttempts.begin(); it != s_targetAnimAttempts.end(); ++it)
		{
			if (it->actorRefID == actorRefID && it->targetFormID == targetFormID && it->scope == scope && it->animDataID == animDataID)
				return &(*it);
		}

		return NULL;
	}

	static void RecordTargetAnimAttempt(UInt32 actorRefID, UInt32 targetFormID, UInt32 scope, UInt32 actorBaseRefID, UInt32 animDataID, UInt32 actorTypeID, bool loaded)
	{
		TargetAnimAttempt attempt = { actorRefID, targetFormID, scope, actorBaseRefID, animDataID, actorTypeID, s_targetAnimGeneration, loaded };
		s_targetAnimAttempts.push_back(attempt);
	}

	static bool ShouldLogWeaponScopedPowerReassert(bool loaded)
	{
		++s_weaponScopedPowerReassertLogCount;
		return !loaded || s_weaponScopedPowerReassertLogCount <= 16 || (s_weaponScopedPowerReassertLogCount % 64) == 0;
	}

	static bool ShouldLogWeaponScopedProbe()
	{
		++s_weaponScopedProbeLogCount;
		return s_weaponScopedProbeLogCount <= 16 || (s_weaponScopedProbeLogCount % 128) == 0;
	}

	static bool ShouldLogWeaponScopedAttackPrefer(UInt32 played)
	{
		++s_weaponScopedAttackPreferLogCount;
		return !played || s_weaponScopedAttackPreferLogCount <= 16 || (s_weaponScopedAttackPreferLogCount % 64) == 0;
	}

	static bool ShouldLogWeaponScopedAttackFallback()
	{
		++s_weaponScopedAttackFallbackLogCount;
		return s_weaponScopedAttackFallbackLogCount <= 16 || (s_weaponScopedAttackFallbackLogCount % 64) == 0;
	}

	static bool ShouldLogTargetScopedPowerReassert(bool loaded)
	{
		++s_targetScopedPowerReassertLogCount;
		return !loaded || s_targetScopedPowerReassertLogCount <= 16 || (s_targetScopedPowerReassertLogCount % 64) == 0;
	}

	static bool ShouldLogTargetScopedAttackPrefer(UInt32 played)
	{
		++s_targetScopedAttackPreferLogCount;
		return !played || s_targetScopedAttackPreferLogCount <= 16 || (s_targetScopedAttackPreferLogCount % 64) == 0;
	}

	static bool ShouldLogTargetScopedAttackFallback()
	{
		++s_targetScopedAttackFallbackLogCount;
		return s_targetScopedAttackFallbackLogCount <= 16 || (s_targetScopedAttackFallbackLogCount % 64) == 0;
	}

	static bool IsWeaponScopedPreferenceGroup(UInt32 groupID)
	{
		return groupID == TESAnimGroup::kAnimGroup_AttackLeft ||
			groupID == TESAnimGroup::kAnimGroup_AttackRight ||
			groupID == TESAnimGroup::kAnimGroup_AttackPower ||
			groupID == TESAnimGroup::kAnimGroup_AttackForwardPower ||
			groupID == TESAnimGroup::kAnimGroup_AttackBackPower ||
			groupID == TESAnimGroup::kAnimGroup_AttackLeftPower ||
			groupID == TESAnimGroup::kAnimGroup_AttackRightPower ||
			groupID == TESAnimGroup::kAnimGroup_BlockAttack;
	}

	static bool SequenceMatchesScopedAnimPath(const char* sequencePath, const char* animPath)
	{
		if (!sequencePath || !sequencePath[0] || !animPath || !animPath[0])
			return false;

		if (IsKFPath(animPath))
			return SequencePathMatchesSpecialAnimPath(sequencePath, animPath, animPath);

		return SequencePathIsUnderSpecialAnimFolder(sequencePath, animPath);
	}

	static bool SequenceMatchesWeaponAnimEntry(const char* sequencePath, const WeaponAnimEntry& entry)
	{
		return !entry.animPath.empty() && SequenceMatchesScopedAnimPath(sequencePath, entry.animPath.c_str());
	}

	static bool SequenceMatchesTargetAnimEntry(const char* sequencePath, const TargetAnimEntry& entry)
	{
		return !entry.animPath.empty() && SequenceMatchesScopedAnimPath(sequencePath, entry.animPath.c_str());
	}

	static TESRace* GetActorRace(TESActorBase* actorBase)
	{
		if (!actorBase || actorBase->typeID != kFormType_NPC)
			return NULL;

		TESNPC* npc = (TESNPC*)actorBase;
		return npc->race.race;
	}

	static bool TargetAnimEntryMatchesActor(TESObjectREFR* ref, TESActorBase* actorBase, const TargetAnimEntry& entry)
	{
		if (!ref || !actorBase || !entry.targetFormID || entry.scope == kTargetAnimScope_None)
			return false;

		TESForm* target = LookupFormByID(entry.targetFormID);
		if (!target || GetTargetAnimScopeForForm(target) != entry.scope)
			return false;

		if (entry.scope == kTargetAnimScope_Race)
		{
			TESRace* actorRace = GetActorRace(actorBase);
			return actorRace && actorRace == target;
		}

		if (entry.scope == kTargetAnimScope_Global)
		{
			TESGlobal* global = (TESGlobal*)target;
			return global->data != 0.0f;
		}

		return false;
	}

	static UInt32 FindWeaponScopedSequenceInMapEntry(UInt32 entry, UInt32 weaponFormID, UInt32* outIndex, UInt32* outCount, const char** outMatchedPath)
	{
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = CountMapEntrySequences(entry);
		if (outMatchedPath)
			*outMatchedPath = NULL;

		if (!entry || !weaponFormID)
			return 0;

		if (IsAnimSequenceSingleEntry(entry))
		{
			UInt32 sequence = *(UInt32*)(entry + kAnimSequenceEntrySequenceOffset);
			const char* sequencePath = GetBSAnimGroupSequencePath(sequence);
			for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
			{
				if (it->weaponFormID == weaponFormID && SequenceMatchesWeaponAnimEntry(sequencePath, *it))
				{
					if (outIndex)
						*outIndex = 0;
					if (outMatchedPath)
						*outMatchedPath = it->animPath.c_str();
					return sequence;
				}
			}
			return 0;
		}

		if (!IsAnimSequenceMultipleEntry(entry))
			return 0;

		UInt32 list = *(UInt32*)(entry + kAnimSequenceMultipleListOffset);
		UInt32 node = list ? *(UInt32*)(list + kAnimSequenceMultipleListHeadOffset) : 0;
		UInt32 index = 0;
		while (node && index < kSequenceListGuard)
		{
			UInt32 sequence = *(UInt32*)(node + kListNodeDataOffset);
			const char* sequencePath = GetBSAnimGroupSequencePath(sequence);
			for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
			{
				if (it->weaponFormID == weaponFormID && SequenceMatchesWeaponAnimEntry(sequencePath, *it))
				{
					if (outIndex)
						*outIndex = index;
					if (outMatchedPath)
						*outMatchedPath = it->animPath.c_str();
					return sequence;
				}
			}

			node = *(UInt32*)(node + kListNodeNextOffset);
			++index;
		}

		return 0;
	}

	static UInt32 FindWeaponScopedSequenceForKey(ActorAnimData* animData, UInt32 weaponFormID, UInt16 key, UInt32* outMapEntry, UInt32* outIndex, UInt32* outCount, const char** outMatchedPath)
	{
		if (outMapEntry)
			*outMapEntry = 0;
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = 0;
		if (outMatchedPath)
			*outMatchedPath = NULL;

		UInt32 mapEntry = 0;
		if (!LookupAnimationMapEntry(animData, key, &mapEntry))
			return 0;

		if (outMapEntry)
			*outMapEntry = mapEntry;
		return FindWeaponScopedSequenceInMapEntry(mapEntry, weaponFormID, outIndex, outCount, outMatchedPath);
	}

	static UInt32 FindTargetScopedSequenceInMapEntry(UInt32 entry, const TargetAnimAttempt& attempt, UInt32* outIndex, UInt32* outCount, const char** outMatchedPath)
	{
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = CountMapEntrySequences(entry);
		if (outMatchedPath)
			*outMatchedPath = NULL;

		if (!entry || !attempt.targetFormID || attempt.scope == kTargetAnimScope_None)
			return 0;

		if (IsAnimSequenceSingleEntry(entry))
		{
			UInt32 sequence = *(UInt32*)(entry + kAnimSequenceEntrySequenceOffset);
			const char* sequencePath = GetBSAnimGroupSequencePath(sequence);
			for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
			{
				if (it->targetFormID == attempt.targetFormID && it->scope == attempt.scope && SequenceMatchesTargetAnimEntry(sequencePath, *it))
				{
					if (outIndex)
						*outIndex = 0;
					if (outMatchedPath)
						*outMatchedPath = it->animPath.c_str();
					return sequence;
				}
			}
			return 0;
		}

		if (!IsAnimSequenceMultipleEntry(entry))
			return 0;

		UInt32 list = *(UInt32*)(entry + kAnimSequenceMultipleListOffset);
		UInt32 node = list ? *(UInt32*)(list + kAnimSequenceMultipleListHeadOffset) : 0;
		UInt32 index = 0;
		while (node && index < kSequenceListGuard)
		{
			UInt32 sequence = *(UInt32*)(node + kListNodeDataOffset);
			const char* sequencePath = GetBSAnimGroupSequencePath(sequence);
			for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
			{
				if (it->targetFormID == attempt.targetFormID && it->scope == attempt.scope && SequenceMatchesTargetAnimEntry(sequencePath, *it))
				{
					if (outIndex)
						*outIndex = index;
					if (outMatchedPath)
						*outMatchedPath = it->animPath.c_str();
					return sequence;
				}
			}

			node = *(UInt32*)(node + kListNodeNextOffset);
			++index;
		}

		return 0;
	}

	static UInt32 FindTargetScopedSequenceForKey(ActorAnimData* animData, UInt16 key, UInt32* outMapEntry, UInt32* outIndex, UInt32* outCount, const char** outMatchedPath, TargetAnimAttempt** outAttempt)
	{
		if (outMapEntry)
			*outMapEntry = 0;
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = 0;
		if (outMatchedPath)
			*outMatchedPath = NULL;
		if (outAttempt)
			*outAttempt = NULL;

		UInt32 mapEntry = 0;
		if (!LookupAnimationMapEntry(animData, key, &mapEntry))
			return 0;

		if (outMapEntry)
			*outMapEntry = mapEntry;

		UInt32 animDataID = (UInt32)animData;
		for (std::vector<TargetAnimAttempt>::iterator it = s_targetAnimAttempts.begin(); it != s_targetAnimAttempts.end(); ++it)
		{
			if (it->animDataID != animDataID || !it->loaded || it->generation != s_targetAnimGeneration)
				continue;

			TESObjectREFR* ref = (TESObjectREFR*)LookupFormByID(it->actorRefID);
			if (!RefOwnsActorBaseSpecialAnimData(ref, animData))
				continue;

			TESActorBase* actorBase = GetActorBaseFromRef(ref);
			TESForm* target = LookupFormByID(it->targetFormID);
			if (!actorBase || !target || GetTargetAnimScopeForForm(target) != it->scope)
				continue;

			TargetAnimEntry probe(it->targetFormID, it->scope, false, "");
			if (!TargetAnimEntryMatchesActor(ref, actorBase, probe) || !HasTargetAnimationMapping(it->targetFormID, it->scope))
				continue;

			UInt32 sequence = FindTargetScopedSequenceInMapEntry(mapEntry, *it, outIndex, outCount, outMatchedPath);
			if (sequence)
			{
				if (outAttempt)
					*outAttempt = &(*it);
				return sequence;
			}
		}

		return 0;
	}

	static bool IsWeaponScopedAnimData(ActorAnimData* animData, TESObjectWEAP** outWeapon, WeaponAnimAttempt** outAttempt)
	{
		if (outWeapon)
			*outWeapon = NULL;
		if (outAttempt)
			*outAttempt = NULL;

		if (!animData)
			return false;

		UInt32 animDataID = (UInt32)animData;
		for (std::vector<WeaponAnimAttempt>::iterator it = s_weaponAnimAttempts.begin(); it != s_weaponAnimAttempts.end(); ++it)
		{
			if (it->animDataID != animDataID || !it->loaded || it->generation != s_weaponAnimGeneration)
				continue;

			TESObjectREFR* ref = (TESObjectREFR*)LookupFormByID(it->actorRefID);
			if (!RefOwnsActorBaseSpecialAnimData(ref, animData))
				continue;

			TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
			if (!currentWeapon || currentWeapon->refID != it->weaponRefID || !HasWeaponAnimationMapping(currentWeapon->refID))
				continue;

			if (outWeapon)
				*outWeapon = currentWeapon;
			if (outAttempt)
				*outAttempt = &(*it);
			return true;
		}

		return false;
	}

	static ScopedRevertResult RevertWeaponAnimAttempt(TESObjectREFR* ref, ActorAnimData* animData, TESActorBase* actorBase, WeaponAnimAttempt* attempt)
	{
		ScopedRevertResult result;
		if (!ref || !actorBase || !attempt)
			return result;

		result = RevertInstalledScopedAnimations(ref, animData, actorBase, &attempt->installed);
		if (result.paths || attempt->loaded)
			result.attempts = 1;

		attempt->actorBaseRefID = actorBase->refID;
		attempt->actorTypeID = ref->typeID;
		attempt->generation = s_weaponAnimGeneration;
		attempt->loaded = false;
		return result;
	}

	static bool WeaponAnimAttemptStillMatches(TESObjectREFR* ref, ActorAnimData* animData, const WeaponAnimAttempt& attempt)
	{
		if (!ref || !animData || !RefOwnsActorBaseSpecialAnimData(ref, animData))
			return false;

		TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
		return currentWeapon &&
			currentWeapon->refID == attempt.weaponRefID &&
			HasWeaponAnimationMapping(currentWeapon->refID) &&
			attempt.generation == s_weaponAnimGeneration;
	}

	static bool WeaponAnimAttemptScopeStillMatches(TESObjectREFR* ref, const WeaponAnimAttempt& attempt)
	{
		if (!ref)
			return false;

		TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
		return currentWeapon &&
			currentWeapon->refID == attempt.weaponRefID &&
			HasWeaponAnimationMapping(currentWeapon->refID) &&
			attempt.generation == s_weaponAnimGeneration;
	}

	static ScopedRevertResult RevertInvalidWeaponScopedAnimationsForAnimData(ActorAnimData* animData)
	{
		ScopedRevertResult aggregate;
		if (!animData)
			return aggregate;

		UInt32 animDataID = (UInt32)animData;
		for (std::vector<WeaponAnimAttempt>::iterator it = s_weaponAnimAttempts.begin(); it != s_weaponAnimAttempts.end(); ++it)
		{
			if (it->animDataID != animDataID || (!it->loaded && it->installed.empty()))
				continue;

			TESObjectREFR* ref = (TESObjectREFR*)LookupFormByID(it->actorRefID);
			if (!RefOwnsActorBaseSpecialAnimData(ref, animData))
				continue;

			if (WeaponAnimAttemptStillMatches(ref, animData, *it))
				continue;

			TESActorBase* actorBase = GetActorBaseFromRef(ref);
			ScopedRevertResult reverted = RevertWeaponAnimAttempt(ref, animData, actorBase, &(*it));
			aggregate.Add(reverted);
			if (reverted.attempts)
			{
				TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
				_MESSAGE("CustomAnimSupport weapon-scope revert actor=%08X actorBase=%08X type=%02X animData=%08X oldWeapon=%08X currentWeapon=%08X paths=%u removedKFFZ=%u removedLive=%u clearedSlots=%u restoredVanilla=%u errors=%u generation=%u",
					ref ? ref->refID : 0,
					actorBase ? actorBase->refID : 0,
					ref ? ref->typeID : 0,
					animDataID,
					it->weaponRefID,
					currentWeapon ? currentWeapon->refID : 0,
					reverted.paths,
					reverted.removedKFFZ,
					reverted.removedLive,
					reverted.clearedSlots,
					reverted.restoredVanilla,
					reverted.errors,
					s_weaponAnimGeneration);
			}
		}

		return aggregate;
	}

	static ScopedRevertResult RevertInvalidWeaponScopedAnimationsForActor(Actor* actor)
	{
		ScopedRevertResult aggregate;
		TESObjectREFR* ref = actor;
		if (!ref)
			return aggregate;

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		ActorAnimData* liveAnimData = GetActorBaseSpecialAnimData(ref);
		UInt32 liveAnimDataID = (UInt32)liveAnimData;
		for (std::vector<WeaponAnimAttempt>::iterator it = s_weaponAnimAttempts.begin(); it != s_weaponAnimAttempts.end(); ++it)
		{
			if (it->actorRefID != ref->refID || (!it->loaded && it->installed.empty()))
				continue;

			if (WeaponAnimAttemptScopeStillMatches(ref, *it))
				continue;

			ScopedRevertResult reverted = RevertWeaponAnimAttempt(ref, liveAnimData, actorBase, &(*it));
			aggregate.Add(reverted);
			if (reverted.attempts)
			{
				TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
				_MESSAGE("CustomAnimSupport weapon-scope revert actor=%08X actorBase=%08X type=%02X animData=%08X attemptAnimData=%08X oldWeapon=%08X currentWeapon=%08X paths=%u removedKFFZ=%u removedLive=%u clearedSlots=%u restoredVanilla=%u errors=%u generation=%u",
					ref->refID,
					actorBase ? actorBase->refID : 0,
					ref->typeID,
					liveAnimDataID,
					it->animDataID,
					it->weaponRefID,
					currentWeapon ? currentWeapon->refID : 0,
					reverted.paths,
					reverted.removedKFFZ,
					reverted.removedLive,
					reverted.clearedSlots,
					reverted.restoredVanilla,
					reverted.errors,
					s_weaponAnimGeneration);
			}
		}

		return aggregate;
	}

	static bool ApplyWeaponAnimEntryForActor(TESObjectREFR* ref, TESActorBase* actorBase, const WeaponAnimEntry& entry, WeaponScopedApplyResult* result, std::vector<ScopedInstalledAnim>* installed)
	{
		if (!actorBase || entry.animPath.empty())
			return false;

		if (result)
			++result->mappings;

		if (IsKFPath(entry.animPath.c_str()))
		{
			UInt16 parsedKey = 0xFFFF;
			UInt32 parsedGroupID = TESAnimGroup::kAnimGroup_Max;
			char loaderPath[1024] = { 0 };
			if (!ReadSpecialAnimParsedKey(actorBase, entry.animPath.c_str(), &parsedKey, &parsedGroupID, loaderPath, sizeof(loaderPath), false))
			{
				if (result)
				{
					++result->invalid;
					++result->errors;
				}
				return false;
			}

			TESAnimation* anim = GetAnimationList(actorBase);
			if (!anim)
			{
				if (result)
					++result->errors;
				return false;
			}

			bool added = AddAnimation(anim, entry.animPath.c_str());
			if (added)
				AddInstalledAnimPath(installed, entry.animPath.c_str(), parsedKey, parsedGroupID);
			else if (HasExactAnimation(anim, entry.animPath.c_str()))
				AddParsedInstalledAnimPath(installed, actorBase, entry.animPath.c_str(), false);
			if (result)
			{
				++result->found;
				if (added)
					++result->added;
				else if (HasExactAnimation(anim, entry.animPath.c_str()))
					++result->existing;
				else
					++result->errors;
			}
			return added || HasExactAnimation(anim, entry.animPath.c_str());
		}

		std::vector<std::string> addedPaths;
		DiscoverResult discovered = DiscoverSpecialAnimations(actorBase, entry.animPath.c_str(), false, &addedPaths);
		AddParsedInstalledAnimPaths(installed, actorBase, addedPaths);
		AddRegisteredScopedAnimPaths(installed, actorBase, entry.animPath.c_str(), false);
		if (result)
		{
			result->found += discovered.found;
			result->added += discovered.added;
			result->existing += discovered.skipped;
			result->errors += discovered.errors;
		}

		return discovered.found != 0 && discovered.errors == 0;
	}

	static bool ApplyTargetAnimEntryForActor(TESObjectREFR* ref, TESActorBase* actorBase, const TargetAnimEntry& entry, TargetScopedApplyResult* result)
	{
		if (!actorBase || entry.animPath.empty())
			return false;

		if (result)
			++result->mappings;

		if (IsKFPath(entry.animPath.c_str()))
		{
			UInt16 parsedKey = 0xFFFF;
			UInt32 parsedGroupID = TESAnimGroup::kAnimGroup_Max;
			char loaderPath[1024] = { 0 };
			if (!ReadSpecialAnimParsedKey(actorBase, entry.animPath.c_str(), &parsedKey, &parsedGroupID, loaderPath, sizeof(loaderPath), false))
			{
				if (result)
				{
					++result->invalid;
					++result->errors;
				}
				return false;
			}

			TESAnimation* anim = GetAnimationList(actorBase);
			if (!anim)
			{
				if (result)
					++result->errors;
				return false;
			}

			bool added = AddAnimation(anim, entry.animPath.c_str());
			if (result)
			{
				++result->found;
				if (added)
					++result->added;
				else if (HasExactAnimation(anim, entry.animPath.c_str()))
					++result->existing;
				else
					++result->errors;
			}
			return added || HasExactAnimation(anim, entry.animPath.c_str());
		}

		DiscoverResult discovered = DiscoverSpecialAnimations(actorBase, entry.animPath.c_str(), false);
		if (result)
		{
			result->found += discovered.found;
			result->added += discovered.added;
			result->existing += discovered.skipped;
			result->errors += discovered.errors;
		}

		return discovered.found != 0 && discovered.errors == 0;
	}

	struct MatchedTargetScope
	{
		UInt32 targetFormID;
		UInt32 scope;
		bool found;
		bool loaded;

		MatchedTargetScope(UInt32 id, UInt32 targetScope) : targetFormID(id), scope(targetScope), found(false), loaded(false) {}
	};

	static MatchedTargetScope* FindMatchedTarget(std::vector<MatchedTargetScope>* targets, UInt32 targetFormID, UInt32 scope)
	{
		if (!targets || !targetFormID || scope == kTargetAnimScope_None)
			return NULL;

		for (std::vector<MatchedTargetScope>::iterator it = targets->begin(); it != targets->end(); ++it)
		{
			if (it->targetFormID == targetFormID && it->scope == scope)
				return &(*it);
		}

		return NULL;
	}

	static bool AddUniqueMatchedTarget(std::vector<MatchedTargetScope>* targets, UInt32 targetFormID, UInt32 scope)
	{
		if (!targets || !targetFormID || scope == kTargetAnimScope_None)
			return false;

		if (FindMatchedTarget(targets, targetFormID, scope))
			return false;

		targets->push_back(MatchedTargetScope(targetFormID, scope));
		return true;
	}

	static void MarkMatchedTargetFound(std::vector<MatchedTargetScope>* targets, UInt32 targetFormID, UInt32 scope)
	{
		MatchedTargetScope* target = FindMatchedTarget(targets, targetFormID, scope);
		if (target)
			target->found = true;
	}

	static void MarkMatchedTargetLoaded(std::vector<MatchedTargetScope>* targets, UInt32 targetFormID, UInt32 scope)
	{
		MatchedTargetScope* target = FindMatchedTarget(targets, targetFormID, scope);
		if (target)
			target->loaded = true;
	}

	static TargetMatchDiagnostics GetTargetAnimationMatchDiagnostics(TESObjectREFR* ref, TESForm* target)
	{
		TargetMatchDiagnostics diagnostics;
		if (!ref || !IsActorReference(ref))
			return diagnostics;

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!actorBase)
			return diagnostics;

		diagnostics.actorBaseFormID = actorBase->refID;
		TESRace* actorRace = GetActorRace(actorBase);
		diagnostics.actorRaceFormID = actorRace ? actorRace->refID : 0;

		UInt32 targetFormID = target ? target->refID : 0;
		UInt32 targetScope = target ? GetTargetAnimScopeForForm(target) : kTargetAnimScope_None;
		std::vector<MatchedTargetScope> matchedTargets;

		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (targetFormID && (it->targetFormID != targetFormID || it->scope != targetScope))
				continue;

			++diagnostics.configured;

			TESForm* entryTarget = LookupFormByID(it->targetFormID);
			if (!entryTarget || GetTargetAnimScopeForForm(entryTarget) != it->scope)
			{
				++diagnostics.invalid;
				continue;
			}

			if (it->scope == kTargetAnimScope_Race)
				++diagnostics.race;
			else if (it->scope == kTargetAnimScope_Global)
				++diagnostics.global;
			else
			{
				++diagnostics.invalid;
				continue;
			}

			if (TargetAnimEntryMatchesActor(ref, actorBase, *it))
			{
				++diagnostics.matchedMappings;
				if (AddUniqueMatchedTarget(&matchedTargets, it->targetFormID, it->scope))
					++diagnostics.matchedTargets;
			}
		}

		return diagnostics;
	}

	static TargetValidationDiagnostics ValidateTargetAnimationMappingsForActor(TESObjectREFR* ref, TESForm* target, bool printToConsole)
	{
		TargetValidationDiagnostics diagnostics;
		if (!ref || !IsActorReference(ref))
			return diagnostics;

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!actorBase)
			return diagnostics;

		diagnostics.actorBaseFormID = actorBase->refID;
		TESRace* actorRace = GetActorRace(actorBase);
		diagnostics.actorRaceFormID = actorRace ? actorRace->refID : 0;

		UInt32 targetFormID = target ? target->refID : 0;
		UInt32 targetScope = target ? GetTargetAnimScopeForForm(target) : kTargetAnimScope_None;
		NiNode* skeletonRoot = GetValidationSkeletonRoot(ref, actorBase);
		std::vector<MatchedTargetScope> matchedTargets;

		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (targetFormID && (it->targetFormID != targetFormID || it->scope != targetScope))
				continue;

			++diagnostics.configured;

			TESForm* entryTarget = LookupFormByID(it->targetFormID);
			if (!entryTarget || GetTargetAnimScopeForForm(entryTarget) != it->scope)
			{
				++diagnostics.invalidMappings;
				++diagnostics.errors;
				continue;
			}

			if (it->scope == kTargetAnimScope_Race)
				++diagnostics.race;
			else if (it->scope == kTargetAnimScope_Global)
				++diagnostics.global;
			else
			{
				++diagnostics.invalidMappings;
				++diagnostics.errors;
				continue;
			}

			if (!TargetAnimEntryMatchesActor(ref, actorBase, *it))
				continue;

			++diagnostics.matchedMappings;
			if (AddUniqueMatchedTarget(&matchedTargets, it->targetFormID, it->scope))
				++diagnostics.matchedTargets;

			++diagnostics.checkedMappings;
			if (it->animPath.empty())
			{
				++diagnostics.invalidMappings;
				++diagnostics.errors;
				continue;
			}

			if (IsKFPath(it->animPath.c_str()))
			{
				UInt32 issues = ValidateSpecialAnimAsset(actorBase, it->animPath.c_str(), skeletonRoot, printToConsole);
				if (issues)
				{
					++diagnostics.invalidMappings;
					diagnostics.errors += issues;
				}
				else
				{
					++diagnostics.validMappings;
					++diagnostics.foundKfs;
				}
				continue;
			}

			DiscoverResult validated = ValidateSpecialAnimFolder(actorBase, it->animPath.c_str(), skeletonRoot, printToConsole);
			diagnostics.foundKfs += validated.found;
			diagnostics.errors += validated.errors;
			if (validated.found && !validated.errors)
				++diagnostics.validMappings;
			else
				++diagnostics.invalidMappings;
		}

		return diagnostics;
	}

	static WeaponScopedApplyResult ApplyWeaponScopedAnimationsForActor(Actor* actor, UInt32 requestedGroupID, bool forceReload)
	{
		WeaponScopedApplyResult result;
		if (s_weaponScopedApplyGuard || !actor)
			return result;

		TESObjectREFR* ref = actor;
		if (!IsActorReference(ref))
			return result;

		UInt32 weaponSource = kEquippedWeaponSource_None;
		UInt32 weaponProcess = 0;
		UInt32 weaponEntryData = 0;
		TESObjectWEAP* weapon = GetEquippedWeaponForRef(ref, &weaponSource, &weaponProcess, &weaponEntryData);
		if (!weapon)
		{
			if (!s_weaponAnimEntries.empty() && ShouldLogWeaponScopedProbe())
			{
				_MESSAGE("CustomAnimSupport weapon-scope probe skipped reason=no weapon actor=%08X type=%02X process=%08X entryData=%08X source=%s mappings=%u force=%u logCount=%u",
					ref->refID,
					ref->typeID,
					weaponProcess,
					weaponEntryData,
					GetEquippedWeaponSourceName(weaponSource),
					(UInt32)s_weaponAnimEntries.size(),
					forceReload ? 1 : 0,
					s_weaponScopedProbeLogCount);
			}
			return result;
		}

		if (!HasWeaponAnimationMapping(weapon->refID))
		{
			if (!s_weaponAnimEntries.empty() && ShouldLogWeaponScopedProbe())
			{
				_MESSAGE("CustomAnimSupport weapon-scope probe skipped reason=no mapping weapon=\"%s\" (%08X) actor=%08X type=%02X process=%08X entryData=%08X source=%s mappings=%u force=%u logCount=%u",
					GetFullName(weapon) ? GetFullName(weapon) : "",
					weapon->refID,
					ref->refID,
					ref->typeID,
					weaponProcess,
					weaponEntryData,
					GetEquippedWeaponSourceName(weaponSource),
					(UInt32)s_weaponAnimEntries.size(),
					forceReload ? 1 : 0,
					s_weaponScopedProbeLogCount);
			}
			return result;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!animData || !actorBase)
		{
			if (ShouldLogWeaponScopedProbe())
			{
				_MESSAGE("CustomAnimSupport weapon-scope probe skipped reason=%s weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X process=%08X entryData=%08X source=%s force=%u logCount=%u",
					!animData ? "no animData" : "no actorBase",
					GetFullName(weapon) ? GetFullName(weapon) : "",
					weapon->refID,
					ref->refID,
					actorBase ? actorBase->refID : 0,
					ref->typeID,
					(UInt32)animData,
					weaponProcess,
					weaponEntryData,
					GetEquippedWeaponSourceName(weaponSource),
					forceReload ? 1 : 0,
					s_weaponScopedProbeLogCount);
			}
			return result;
		}

		UInt32 actorRefID = ref->refID;
		UInt32 weaponRefID = weapon->refID;
		UInt32 animDataID = (UInt32)animData;
		WeaponAnimAttempt* attempt = FindWeaponAnimAttempt(actorRefID, weaponRefID, animDataID);
		bool attempted = attempt != NULL;
		if (attempted && attempt->generation != s_weaponAnimGeneration && (!attempt->installed.empty() || attempt->loaded))
			RevertWeaponAnimAttempt(ref, animData, actorBase, attempt);

		if (attempted && attempt->loaded && attempt->generation == s_weaponAnimGeneration && !forceReload)
		{
			result.loaded = true;
			if (ShouldLogWeaponScopedProbe())
			{
				_MESSAGE("CustomAnimSupport weapon-scope probe skipped reason=already loaded weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X source=%s generation=%u installed=%u logCount=%u",
					GetFullName(weapon) ? GetFullName(weapon) : "",
					weaponRefID,
					actorRefID,
					actorBase->refID,
					ref->typeID,
					animDataID,
					GetEquippedWeaponSourceName(weaponSource),
					s_weaponAnimGeneration,
					(UInt32)attempt->installed.size(),
					s_weaponScopedProbeLogCount);
			}
			return result;
		}

		if (attempted && attempt->loaded && attempt->generation == s_weaponAnimGeneration && forceReload && requestedGroupID < TESAnimGroup::kAnimGroup_Max)
		{
			UInt16 existingKey = ResolveActorLoadAnimGroupKeyNoAutoHook(actor, requestedGroupID);
			UInt32 resolvedGroupID = GetAnimKeyGroupID(existingKey);
			if (!IsNativeSavedSlotKey(existingKey) || resolvedGroupID != requestedGroupID)
				return result;

			UInt32 existingMapEntry = 0;
			UInt32 existingIndex = 0xFFFFFFFF;
			UInt32 existingCount = 0;
			const char* matchedPath = NULL;
			UInt32 existingSequence = FindWeaponScopedSequenceForKey(animData, weaponRefID, existingKey, &existingMapEntry, &existingIndex, &existingCount, &matchedPath);
			if (existingSequence)
			{
				result.loaded = true;
				if (ShouldLogWeaponScopedPowerReassert(true))
				{
					const char* groupName = requestedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(requestedGroupID) : "";
					_MESSAGE("CustomAnimSupport weapon-scope power reassert skipped reload group=%s (%u) key=%04X weapon=\"%s\" (%08X) actor=%08X actorBase=%08X animData=%08X sequence=%08X index=%u count=%u mapping=\"%s\" path=\"%s\" logCount=%u",
						groupName ? groupName : "",
						requestedGroupID,
						existingKey,
						GetFullName(weapon) ? GetFullName(weapon) : "",
						weaponRefID,
						actorRefID,
						actorBase->refID,
						animDataID,
						existingSequence,
						existingIndex,
						existingCount,
						matchedPath ? matchedPath : "",
						GetBSAnimGroupSequencePath(existingSequence) ? GetBSAnimGroupSequencePath(existingSequence) : "",
						s_weaponScopedPowerReassertLogCount);
				}
				return result;
			}
		}

		s_weaponScopedApplyGuard = true;
		std::vector<ScopedInstalledAnim> installed = attempted ? attempt->installed : std::vector<ScopedInstalledAnim>();
		for (std::vector<WeaponAnimEntry>::const_iterator it = s_weaponAnimEntries.begin(); it != s_weaponAnimEntries.end(); ++it)
		{
			if (it->weaponFormID != weaponRefID)
				continue;

			ApplyWeaponAnimEntryForActor(ref, actorBase, *it, &result, &installed);
		}

		if (result.found)
			result.loaded = LoadSpecialAnimationsForRef(ref, actorBase);

		if (!attempted)
		{
			RecordWeaponAnimAttempt(actorRefID, weaponRefID, actorBase->refID, animDataID, ref->typeID, result.loaded, installed);
		}
		else
		{
			attempt->actorBaseRefID = actorBase->refID;
			attempt->actorTypeID = ref->typeID;
			attempt->generation = s_weaponAnimGeneration;
			attempt->loaded = result.loaded;
			attempt->installed = installed;
		}

		s_weaponScopedApplyGuard = false;

		if (forceReload && ShouldLogWeaponScopedPowerReassert(result.loaded))
		{
			const char* groupName = requestedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(requestedGroupID) : "";
			_MESSAGE("CustomAnimSupport weapon-scope power reassert group=%s (%u) weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
				groupName ? groupName : "",
				requestedGroupID,
				GetFullName(weapon) ? GetFullName(weapon) : "",
				weaponRefID,
				actorRefID,
				actorBase->refID,
				ref->typeID,
				animDataID,
				result.mappings,
				result.found,
				result.added,
				result.existing,
				result.invalid,
				result.errors,
				result.loaded ? 1 : 0,
				s_weaponAnimGeneration);
		}
		else
		{
			_MESSAGE("CustomAnimSupport weapon-scope apply weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
				GetFullName(weapon) ? GetFullName(weapon) : "",
				weaponRefID,
				actorRefID,
				actorBase->refID,
				ref->typeID,
				animDataID,
				result.mappings,
				result.found,
				result.added,
				result.existing,
				result.invalid,
				result.errors,
				result.loaded ? 1 : 0,
				s_weaponAnimGeneration);
		}

		return result;
	}

	static TargetScopedApplyResult ApplyTargetScopedAnimationsForActor(Actor* actor, UInt32 requestedGroupID, bool forceReload)
	{
		TargetScopedApplyResult result;
		if (s_targetScopedApplyGuard || !actor || s_targetAnimEntries.empty())
			return result;

		TESObjectREFR* ref = actor;
		if (!IsActorReference(ref))
			return result;

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!animData || !actorBase)
			return result;

		UInt32 actorRefID = ref->refID;
		UInt32 animDataID = (UInt32)animData;
		std::vector<MatchedTargetScope> matchedTargets;

		s_targetScopedApplyGuard = true;
		for (std::vector<TargetAnimEntry>::const_iterator it = s_targetAnimEntries.begin(); it != s_targetAnimEntries.end(); ++it)
		{
			if (!TargetAnimEntryMatchesActor(ref, actorBase, *it))
				continue;

			if (AddUniqueMatchedTarget(&matchedTargets, it->targetFormID, it->scope))
				++result.matched;

			TargetAnimAttempt* attempt = FindTargetAnimAttempt(actorRefID, it->targetFormID, it->scope, animDataID);
			if (attempt && attempt->loaded && attempt->generation == s_targetAnimGeneration && !forceReload)
			{
				result.loaded = true;
				MarkMatchedTargetLoaded(&matchedTargets, it->targetFormID, it->scope);
				continue;
			}

			if (attempt && attempt->loaded && attempt->generation == s_targetAnimGeneration && forceReload && requestedGroupID < TESAnimGroup::kAnimGroup_Max)
			{
				UInt16 existingKey = ResolveActorLoadAnimGroupKeyNoAutoHook(actor, requestedGroupID);
				UInt32 resolvedGroupID = GetAnimKeyGroupID(existingKey);
				if (!IsNativeSavedSlotKey(existingKey) || resolvedGroupID != requestedGroupID)
					continue;

				UInt32 existingMapEntry = 0;
				if (LookupAnimationMapEntry(animData, existingKey, &existingMapEntry))
				{
					UInt32 existingIndex = 0xFFFFFFFF;
					UInt32 existingCount = 0;
					const char* matchedPath = NULL;
					UInt32 existingSequence = FindTargetScopedSequenceInMapEntry(existingMapEntry, *attempt, &existingIndex, &existingCount, &matchedPath);
					if (existingSequence)
						{
							result.loaded = true;
							MarkMatchedTargetLoaded(&matchedTargets, it->targetFormID, it->scope);
							if (ShouldLogTargetScopedPowerReassert(true))
							{
							const char* groupName = requestedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(requestedGroupID) : "";
							_MESSAGE("CustomAnimSupport target-scope power reassert skipped reload group=%s (%u) key=%04X target=%08X scope=%s actor=%08X actorBase=%08X animData=%08X sequence=%08X index=%u count=%u mapping=\"%s\" path=\"%s\" logCount=%u",
								groupName ? groupName : "",
								requestedGroupID,
								existingKey,
								it->targetFormID,
								GetTargetAnimScopeName(it->scope),
								actorRefID,
								actorBase->refID,
								animDataID,
								existingSequence,
								existingIndex,
								existingCount,
								matchedPath ? matchedPath : "",
								GetBSAnimGroupSequencePath(existingSequence) ? GetBSAnimGroupSequencePath(existingSequence) : "",
								s_targetScopedPowerReassertLogCount);
						}
						continue;
					}
				}
			}

			if (ApplyTargetAnimEntryForActor(ref, actorBase, *it, &result))
				MarkMatchedTargetFound(&matchedTargets, it->targetFormID, it->scope);
		}

		if (result.found)
			result.loaded = LoadSpecialAnimationsForRef(ref, actorBase);

		for (std::vector<MatchedTargetScope>::iterator it = matchedTargets.begin(); it != matchedTargets.end(); ++it)
		{
			if (it->found)
			{
				++result.foundTargets;
				if (result.loaded)
					it->loaded = true;
			}

			if (it->loaded)
				++result.loadedTargets;
		}

		if (result.loadedTargets)
			result.loaded = true;

		for (std::vector<MatchedTargetScope>::const_iterator it = matchedTargets.begin(); it != matchedTargets.end(); ++it)
		{
			TargetAnimAttempt* attempt = FindTargetAnimAttempt(actorRefID, it->targetFormID, it->scope, animDataID);
			if (!attempt)
				RecordTargetAnimAttempt(actorRefID, it->targetFormID, it->scope, actorBase->refID, animDataID, ref->typeID, it->loaded);
			else
			{
				attempt->actorBaseRefID = actorBase->refID;
				attempt->actorTypeID = ref->typeID;
				attempt->generation = s_targetAnimGeneration;
				attempt->loaded = it->loaded;
			}
		}

		s_targetScopedApplyGuard = false;

		if (!matchedTargets.empty())
		{
			if (forceReload && ShouldLogTargetScopedPowerReassert(result.loaded))
			{
				const char* groupName = requestedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(requestedGroupID) : "";
				_MESSAGE("CustomAnimSupport target-scope power reassert group=%s (%u) actor=%08X actorBase=%08X type=%02X animData=%08X targets=%u foundTargets=%u loadedTargets=%u mappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
					groupName ? groupName : "",
					requestedGroupID,
					actorRefID,
					actorBase->refID,
					ref->typeID,
					animDataID,
					result.matched,
					result.foundTargets,
					result.loadedTargets,
					result.mappings,
					result.found,
					result.added,
					result.existing,
					result.invalid,
					result.errors,
					result.loaded ? 1 : 0,
					s_targetAnimGeneration);
			}
			else
			{
				_MESSAGE("CustomAnimSupport target-scope apply actor=%08X actorBase=%08X type=%02X animData=%08X targets=%u foundTargets=%u loadedTargets=%u mappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
					actorRefID,
					actorBase->refID,
					ref->typeID,
					animDataID,
					result.matched,
					result.foundTargets,
					result.loadedTargets,
					result.mappings,
					result.found,
					result.added,
					result.existing,
					result.invalid,
					result.errors,
					result.loaded ? 1 : 0,
					s_targetAnimGeneration);
			}
		}

		return result;
	}

	static bool ContainsCaseInsensitive(const char* haystack, const char* needle)
	{
		if (!haystack || !needle || !needle[0])
			return false;

		UInt32 needleLen = std::strlen(needle);
		for (const char* cur = haystack; *cur; ++cur)
		{
			if (_strnicmp(cur, needle, needleLen) == 0)
				return true;
		}

		return false;
	}

	static TESObjectWEAP* GetEquippedWeaponForRef(TESObjectREFR* ref, UInt32* outSource, UInt32* outProcess, UInt32* outEntryData)
	{
		if (outSource)
			*outSource = kEquippedWeaponSource_None;
		if (outProcess)
			*outProcess = 0;
		if (outEntryData)
			*outEntryData = 0;

		if (!ref || !IsActorReference(ref))
			return NULL;

		BaseProcess* baseProcess = GetBaseProcessForRef(ref);
		if (outProcess)
			*outProcess = (UInt32)baseProcess;

		if (baseProcess)
		{
			ExtraContainerChanges::EntryData* weaponData = baseProcess->GetEquippedWeaponData(true);
			if (outEntryData)
				*outEntryData = (UInt32)weaponData;

			TESObjectWEAP* weapon = GetWeaponFromEntryData(weaponData);
			if (weapon)
			{
				if (outSource)
					*outSource = kEquippedWeaponSource_ProcessVirtual;
				return weapon;
			}
		}

		MiddleHighProcess* proc = ExtractMiddleHighProcess(ref);
		ExtraContainerChanges::EntryData* weaponData = proc ? proc->equippedWeaponData : NULL;
		if (outEntryData && weaponData)
			*outEntryData = (UInt32)weaponData;

		TESObjectWEAP* cachedWeapon = GetWeaponFromEntryData(weaponData);
		if (cachedWeapon)
		{
			if (outSource)
				*outSource = kEquippedWeaponSource_MiddleHighCache;
			return cachedWeapon;
		}

		Actor* actor = OBLIVION_CAST(ref, TESObjectREFR, Actor);
		if (!actor)
			return NULL;

		EquippedItemsList equipped = actor->GetEquippedItems();
		for (EquippedItemsList::const_iterator it = equipped.begin(); it != equipped.end(); ++it)
		{
			TESForm* form = *it;
			if (form && form->typeID == kFormType_Weapon)
			{
				if (outSource)
					*outSource = kEquippedWeaponSource_WornExtraList;
				return (TESObjectWEAP*)form;
			}
		}

		return NULL;
	}

	static AutoWeaponRuleValidationDiagnostics ValidateAutoWeaponAnimationRulesForActor(TESObjectREFR* ref, const char* nameContainsFilter, bool printToConsole)
	{
		AutoWeaponRuleValidationDiagnostics diagnostics;
		if (!ref || !IsActorReference(ref))
			return diagnostics;

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!actorBase)
		{
			++diagnostics.errors;
			return diagnostics;
		}

		diagnostics.actorBaseFormID = actorBase->refID;

		UInt32 weaponSource = kEquippedWeaponSource_None;
		UInt32 weaponProcess = 0;
		UInt32 weaponEntryData = 0;
		TESObjectWEAP* weapon = GetEquippedWeaponForRef(ref, &weaponSource, &weaponProcess, &weaponEntryData);
		diagnostics.weaponSource = weaponSource;
		diagnostics.weaponProcess = weaponProcess;
		diagnostics.weaponEntryData = weaponEntryData;
		diagnostics.weaponFormID = weapon ? weapon->refID : 0;

		if (!weapon)
		{
			++diagnostics.errors;
			if (printToConsole)
				Console_Print("CASValidateAutoWeaponAnimationRules: actor %08X has no equipped weapon exposed by the decoded process path process=%08X entryData=%08X source=%s", ref->refID, weaponProcess, weaponEntryData, GetEquippedWeaponSourceName(weaponSource));
			return diagnostics;
		}

		const char* weaponName = GetFullName(weapon);
		NiNode* skeletonRoot = GetValidationSkeletonRoot(ref, actorBase);
		EnsureDefaultAutoWeaponAnimationRules();

		for (std::vector<AutoWeaponAnimRule>::const_iterator it = s_autoWeaponAnimRules.begin(); it != s_autoWeaponAnimRules.end(); ++it)
		{
			if (nameContainsFilter && nameContainsFilter[0] && _stricmp(it->nameContains.c_str(), nameContainsFilter) != 0)
				continue;

			++diagnostics.configuredRules;
			if (it->nameContains.empty() || it->animPath.empty())
			{
				++diagnostics.invalidRules;
				++diagnostics.errors;
				continue;
			}

			if (!ContainsCaseInsensitive(weaponName, it->nameContains.c_str()))
				continue;

			++diagnostics.matchedRules;
			++diagnostics.checkedRules;
			if (printToConsole)
				Console_Print("CASValidateAutoWeaponAnimationRules: matched weapon=\"%s\" (%08X) nameContains=\"%s\" path=\"%s\"", weaponName ? weaponName : "", weapon->refID, it->nameContains.c_str(), it->animPath.c_str());

			if (IsKFPath(it->animPath.c_str()))
			{
				UInt32 issues = ValidateSpecialAnimAsset(actorBase, it->animPath.c_str(), skeletonRoot, printToConsole);
				if (issues)
				{
					++diagnostics.invalidRules;
					diagnostics.errors += issues;
				}
				else
				{
					++diagnostics.validRules;
					++diagnostics.foundKfs;
				}
				continue;
			}

			DiscoverResult validated = ValidateSpecialAnimFolder(actorBase, it->animPath.c_str(), skeletonRoot, printToConsole);
			diagnostics.foundKfs += validated.found;
			diagnostics.errors += validated.errors;
			if (validated.found && !validated.errors)
				++diagnostics.validRules;
			else
				++diagnostics.invalidRules;
		}

		if (!diagnostics.configuredRules)
		{
			++diagnostics.errors;
			if (printToConsole)
				Console_Print("CASValidateAutoWeaponAnimationRules: no configured rules for weaponNameContains=\"%s\"", nameContainsFilter ? nameContainsFilter : "");
		}
		else if (!diagnostics.matchedRules)
		{
			++diagnostics.errors;
			if (printToConsole)
				Console_Print("CASValidateAutoWeaponAnimationRules: equipped weapon \"%s\" (%08X) source=%s matched no configured auto-weapon rule for filter=\"%s\"", weaponName ? weaponName : "", weapon->refID, GetEquippedWeaponSourceName(weaponSource), nameContainsFilter ? nameContainsFilter : "");
		}

		return diagnostics;
	}

	static AutoWeaponAnimAttempt* FindAutoWeaponAnimationAttempt(UInt32 actorRefID, UInt32 weaponRefID, UInt32 animDataID)
	{
		for (std::vector<AutoWeaponAnimAttempt>::iterator it = s_autoWeaponAnimAttempts.begin(); it != s_autoWeaponAnimAttempts.end(); ++it)
		{
			if (it->actorRefID == actorRefID && it->weaponRefID == weaponRefID && it->animDataID == animDataID)
				return &(*it);
		}

		return NULL;
	}

	static void AddUniqueString(std::vector<std::string>* values, const char* value)
	{
		if (!values || !value || !value[0])
			return;

		for (std::vector<std::string>::const_iterator it = values->begin(); it != values->end(); ++it)
		{
			if (_stricmp(it->c_str(), value) == 0)
				return;
		}

		values->push_back(value);
	}

	static void RecordAutoWeaponAnimationAttempt(UInt32 actorRefID, UInt32 weaponRefID, UInt32 actorBaseRefID, UInt32 animDataID, UInt32 actorTypeID, bool loaded, const std::vector<std::string>& nameContains, const std::vector<std::string>& animPaths, const std::vector<ScopedInstalledAnim>& installed)
	{
		AutoWeaponAnimAttempt attempt;
		attempt.actorRefID = actorRefID;
		attempt.weaponRefID = weaponRefID;
		attempt.actorBaseRefID = actorBaseRefID;
		attempt.animDataID = animDataID;
		attempt.actorTypeID = actorTypeID;
		attempt.generation = s_autoWeaponAnimGeneration;
		attempt.loaded = loaded;
		attempt.nameContains = nameContains;
		attempt.animPaths = animPaths;
		attempt.installed = installed;
		s_autoWeaponAnimAttempts.push_back(attempt);
	}

	static void ClearAutoWeaponAnimationAttempts()
	{
		s_autoWeaponAnimAttempts.clear();
		s_autoWeaponProbeLogCount = 0;
		s_autoWeaponPowerReassertLogCount = 0;
		s_autoWeaponAttackPreferLogCount = 0;
		s_autoWeaponAttackFallbackLogCount = 0;
	}

	static bool ShouldLogAutoWeaponProbe()
	{
		++s_autoWeaponProbeLogCount;
		return s_autoWeaponProbeLogCount <= 16 || (s_autoWeaponProbeLogCount % 128) == 0;
	}

	static bool ShouldLogAutoWeaponPowerReassert(bool loaded)
	{
		++s_autoWeaponPowerReassertLogCount;
		return !loaded || s_autoWeaponPowerReassertLogCount <= 16 || (s_autoWeaponPowerReassertLogCount % 64) == 0;
	}

	static bool ShouldLogAutoWeaponAttackPrefer(UInt32 played)
	{
		++s_autoWeaponAttackPreferLogCount;
		return !played || s_autoWeaponAttackPreferLogCount <= 16 || (s_autoWeaponAttackPreferLogCount % 64) == 0;
	}

	static bool ShouldLogAutoWeaponAttackFallback()
	{
		++s_autoWeaponAttackFallbackLogCount;
		return s_autoWeaponAttackFallbackLogCount <= 16 || (s_autoWeaponAttackFallbackLogCount % 64) == 0;
	}

	static bool RefOwnsActorBaseSpecialAnimData(TESObjectREFR* ref, ActorAnimData* animData)
	{
		if (!ref || !animData || !IsActorReference(ref))
			return false;

		MiddleHighProcess* proc = ExtractMiddleHighProcess(ref);
		return proc && proc->animData == animData;
	}

	static bool AutoWeaponAttemptWeaponNameMatches(TESObjectWEAP* weapon, const AutoWeaponAnimAttempt& attempt)
	{
		const char* weaponName = weapon ? GetFullName(weapon) : NULL;
		for (std::vector<std::string>::const_iterator it = attempt.nameContains.begin(); it != attempt.nameContains.end(); ++it)
		{
			if (ContainsCaseInsensitive(weaponName, it->c_str()))
				return true;
		}

		return false;
	}

	static bool IsAutoWeaponAnimationData(ActorAnimData* animData, TESObjectWEAP** outWeapon, AutoWeaponAnimAttempt** outAttempt)
	{
		if (outWeapon)
			*outWeapon = NULL;
		if (outAttempt)
			*outAttempt = NULL;

		if (!animData)
			return false;

		UInt32 animDataID = (UInt32)animData;
		for (std::vector<AutoWeaponAnimAttempt>::iterator it = s_autoWeaponAnimAttempts.begin(); it != s_autoWeaponAnimAttempts.end(); ++it)
		{
			if (it->animDataID != animDataID || !it->loaded || it->generation != s_autoWeaponAnimGeneration)
				continue;

			TESObjectREFR* ref = (TESObjectREFR*)LookupFormByID(it->actorRefID);
			if (!RefOwnsActorBaseSpecialAnimData(ref, animData))
				continue;

			TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
			if (!currentWeapon || currentWeapon->refID != it->weaponRefID || !AutoWeaponAttemptWeaponNameMatches(currentWeapon, *it) || !HasAutoWeaponAnimationRuleForAttempt(*it))
				continue;

			if (outWeapon)
				*outWeapon = currentWeapon;
			if (outAttempt)
				*outAttempt = &(*it);
			return true;
		}

		return false;
	}

	static ScopedRevertResult RevertAutoWeaponAnimationAttempt(TESObjectREFR* ref, ActorAnimData* animData, TESActorBase* actorBase, AutoWeaponAnimAttempt* attempt)
	{
		ScopedRevertResult result;
		if (!ref || !actorBase || !attempt)
			return result;

		result = RevertInstalledScopedAnimations(ref, animData, actorBase, &attempt->installed);
		if (result.paths || attempt->loaded)
			result.attempts = 1;

		attempt->actorBaseRefID = actorBase->refID;
		attempt->actorTypeID = ref->typeID;
		attempt->generation = s_autoWeaponAnimGeneration;
		attempt->loaded = false;
		return result;
	}

	static bool AutoWeaponAnimationAttemptStillMatches(TESObjectREFR* ref, ActorAnimData* animData, const AutoWeaponAnimAttempt& attempt)
	{
		if (!ref || !animData || !RefOwnsActorBaseSpecialAnimData(ref, animData))
			return false;

		TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
		return currentWeapon &&
			currentWeapon->refID == attempt.weaponRefID &&
			AutoWeaponAttemptWeaponNameMatches(currentWeapon, attempt) &&
			HasAutoWeaponAnimationRuleForAttempt(attempt) &&
			attempt.generation == s_autoWeaponAnimGeneration;
	}

	static bool AutoWeaponAnimationAttemptScopeStillMatches(TESObjectREFR* ref, const AutoWeaponAnimAttempt& attempt)
	{
		if (!ref)
			return false;

		TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
		return currentWeapon &&
			currentWeapon->refID == attempt.weaponRefID &&
			AutoWeaponAttemptWeaponNameMatches(currentWeapon, attempt) &&
			HasAutoWeaponAnimationRuleForAttempt(attempt) &&
			attempt.generation == s_autoWeaponAnimGeneration;
	}

	static ScopedRevertResult RevertInvalidAutoWeaponAnimationsForAnimData(ActorAnimData* animData)
	{
		ScopedRevertResult aggregate;
		if (!animData)
			return aggregate;

		UInt32 animDataID = (UInt32)animData;
		for (std::vector<AutoWeaponAnimAttempt>::iterator it = s_autoWeaponAnimAttempts.begin(); it != s_autoWeaponAnimAttempts.end(); ++it)
		{
			if (it->animDataID != animDataID || (!it->loaded && it->installed.empty()))
				continue;

			TESObjectREFR* ref = (TESObjectREFR*)LookupFormByID(it->actorRefID);
			if (!RefOwnsActorBaseSpecialAnimData(ref, animData))
				continue;

			if (AutoWeaponAnimationAttemptStillMatches(ref, animData, *it))
				continue;

			TESActorBase* actorBase = GetActorBaseFromRef(ref);
			ScopedRevertResult reverted = RevertAutoWeaponAnimationAttempt(ref, animData, actorBase, &(*it));
			aggregate.Add(reverted);
			if (reverted.attempts)
			{
				TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
				_MESSAGE("CustomAnimSupport auto-weapon revert actor=%08X actorBase=%08X type=%02X animData=%08X oldWeapon=%08X currentWeapon=%08X paths=%u removedKFFZ=%u removedLive=%u clearedSlots=%u restoredVanilla=%u errors=%u generation=%u",
					ref ? ref->refID : 0,
					actorBase ? actorBase->refID : 0,
					ref ? ref->typeID : 0,
					animDataID,
					it->weaponRefID,
					currentWeapon ? currentWeapon->refID : 0,
					reverted.paths,
					reverted.removedKFFZ,
					reverted.removedLive,
					reverted.clearedSlots,
					reverted.restoredVanilla,
					reverted.errors,
					s_autoWeaponAnimGeneration);
			}
		}

		return aggregate;
	}

	static ScopedRevertResult RevertInvalidAutoWeaponAnimationsForActor(Actor* actor)
	{
		ScopedRevertResult aggregate;
		TESObjectREFR* ref = actor;
		if (!ref)
			return aggregate;

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		ActorAnimData* liveAnimData = GetActorBaseSpecialAnimData(ref);
		UInt32 liveAnimDataID = (UInt32)liveAnimData;
		for (std::vector<AutoWeaponAnimAttempt>::iterator it = s_autoWeaponAnimAttempts.begin(); it != s_autoWeaponAnimAttempts.end(); ++it)
		{
			if (it->actorRefID != ref->refID || (!it->loaded && it->installed.empty()))
				continue;

			if (AutoWeaponAnimationAttemptScopeStillMatches(ref, *it))
				continue;

			ScopedRevertResult reverted = RevertAutoWeaponAnimationAttempt(ref, liveAnimData, actorBase, &(*it));
			aggregate.Add(reverted);
			if (reverted.attempts)
			{
				TESObjectWEAP* currentWeapon = GetEquippedWeaponForRef(ref);
				_MESSAGE("CustomAnimSupport auto-weapon revert actor=%08X actorBase=%08X type=%02X animData=%08X attemptAnimData=%08X oldWeapon=%08X currentWeapon=%08X paths=%u removedKFFZ=%u removedLive=%u clearedSlots=%u restoredVanilla=%u errors=%u generation=%u",
					ref->refID,
					actorBase ? actorBase->refID : 0,
					ref->typeID,
					liveAnimDataID,
					it->animDataID,
					it->weaponRefID,
					currentWeapon ? currentWeapon->refID : 0,
					reverted.paths,
					reverted.removedKFFZ,
					reverted.removedLive,
					reverted.clearedSlots,
					reverted.restoredVanilla,
					reverted.errors,
					s_autoWeaponAnimGeneration);
			}
		}

		return aggregate;
	}

	static UInt32 FindAutoWeaponAnimationSequenceForKey(ActorAnimData* animData, const AutoWeaponAnimAttempt& attempt, UInt16 key, UInt32* outMapEntry, UInt32* outIndex, UInt32* outCount, const char** outMatchedPath)
	{
		if (outMapEntry)
			*outMapEntry = 0;
		if (outIndex)
			*outIndex = 0xFFFFFFFF;
		if (outCount)
			*outCount = 0;
		if (outMatchedPath)
			*outMatchedPath = NULL;

		UInt32 mapEntry = 0;
		if (!LookupAnimationMapEntry(animData, key, &mapEntry))
			return 0;

		if (outMapEntry)
			*outMapEntry = mapEntry;
		return FindAutoWeaponAnimationSequenceInMapEntry(mapEntry, attempt, outIndex, outCount, outMatchedPath);
	}

	static bool AutoApplyWeaponAnimationsForActor(Actor* actor, UInt32 requestedGroupID, bool forceReload)
	{
		if (s_autoWeaponAnimApplyGuard || !actor)
			return false;

		TESObjectREFR* ref = actor;
		if (!IsActorReference(ref))
			return false;

		UInt32 weaponSource = kEquippedWeaponSource_None;
		UInt32 weaponProcess = 0;
		UInt32 weaponEntryData = 0;
		TESObjectWEAP* weapon = GetEquippedWeaponForRef(ref, &weaponSource, &weaponProcess, &weaponEntryData);
		if (!weapon)
		{
			if (ShouldLogAutoWeaponProbe())
			{
				_MESSAGE("CustomAnimSupport auto-weapon probe skipped reason=no weapon actor=%08X type=%02X process=%08X entryData=%08X source=%s group=%u force=%u rules=%u logCount=%u",
					ref->refID,
					ref->typeID,
					weaponProcess,
					weaponEntryData,
					GetEquippedWeaponSourceName(weaponSource),
					requestedGroupID,
					forceReload ? 1 : 0,
					CountAutoWeaponAnimationRules(NULL),
					s_autoWeaponProbeLogCount);
			}
			return false;
		}

		const char* weaponName = GetFullName(weapon);
		std::vector<AutoWeaponAnimRule> matchedRules;
		FindAutoWeaponAnimationRulesForWeapon(weapon, &matchedRules);
		if (matchedRules.empty())
		{
			if (ShouldLogAutoWeaponProbe())
			{
				_MESSAGE("CustomAnimSupport auto-weapon probe skipped reason=no rule weapon=\"%s\" (%08X) actor=%08X type=%02X process=%08X entryData=%08X source=%s group=%u force=%u rules=%u logCount=%u",
					weaponName ? weaponName : "",
					weapon->refID,
					ref->refID,
					ref->typeID,
					weaponProcess,
					weaponEntryData,
					GetEquippedWeaponSourceName(weaponSource),
					requestedGroupID,
					forceReload ? 1 : 0,
					CountAutoWeaponAnimationRules(NULL),
					s_autoWeaponProbeLogCount);
			}
			return false;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		if (!animData)
		{
			if (ShouldLogAutoWeaponProbe())
			{
				_MESSAGE("CustomAnimSupport auto-weapon probe skipped reason=no animData weapon=\"%s\" (%08X) actor=%08X type=%02X process=%08X entryData=%08X source=%s group=%u force=%u matchedRules=%u logCount=%u",
					weaponName ? weaponName : "",
					weapon->refID,
					ref->refID,
					ref->typeID,
					weaponProcess,
					weaponEntryData,
					GetEquippedWeaponSourceName(weaponSource),
					requestedGroupID,
					forceReload ? 1 : 0,
					(UInt32)matchedRules.size(),
					s_autoWeaponProbeLogCount);
			}
			return false;
		}

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!actorBase)
		{
			_MESSAGE("CustomAnimSupport auto-weapon skipped: actor %08X has no actor base for weapon \"%s\" (%08X)", ref->refID, weaponName ? weaponName : "", weapon->refID);
			return false;
		}

		UInt32 actorRefID = ref->refID;
		UInt32 weaponRefID = weapon->refID;
		UInt32 animDataID = (UInt32)animData;
		AutoWeaponAnimAttempt* attempt = FindAutoWeaponAnimationAttempt(actorRefID, weaponRefID, animDataID);
		bool attempted = attempt != NULL;
		if (attempted && attempt->generation != s_autoWeaponAnimGeneration && (!attempt->installed.empty() || attempt->loaded))
			RevertAutoWeaponAnimationAttempt(ref, animData, actorBase, attempt);

		if (attempted && attempt->loaded && attempt->generation == s_autoWeaponAnimGeneration && !forceReload)
		{
			if (ShouldLogAutoWeaponProbe())
			{
				_MESSAGE("CustomAnimSupport auto-weapon probe skipped reason=already loaded weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X source=%s rules=%u generation=%u installed=%u logCount=%u",
					weaponName ? weaponName : "",
					weaponRefID,
					actorRefID,
					attempt->actorBaseRefID,
					ref->typeID,
					animDataID,
					GetEquippedWeaponSourceName(weaponSource),
					(UInt32)attempt->animPaths.size(),
					s_autoWeaponAnimGeneration,
					(UInt32)attempt->installed.size(),
					s_autoWeaponProbeLogCount);
			}
			return true;
		}

		if (attempted && attempt->loaded && attempt->generation == s_autoWeaponAnimGeneration && forceReload && requestedGroupID < TESAnimGroup::kAnimGroup_Max)
		{
			UInt16 existingKey = ResolveActorLoadAnimGroupKeyNoAutoHook(actor, requestedGroupID);
			UInt32 resolvedGroupID = GetAnimKeyGroupID(existingKey);
			if (!IsNativeSavedSlotKey(existingKey) || resolvedGroupID != requestedGroupID)
			{
				if (ShouldLogAutoWeaponPowerReassert(true))
				{
					const char* requestedName = TESAnimGroup::StringForAnimGroupCode(requestedGroupID);
					const char* resolvedName = resolvedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(resolvedGroupID) : "";
					_MESSAGE("CustomAnimSupport auto-weapon power reassert skipped reload resolved mismatch requested=%s (%u) resolved=%s (%u) key=%04X weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X logCount=%u",
						requestedName ? requestedName : "",
						requestedGroupID,
						resolvedName ? resolvedName : "",
						resolvedGroupID,
						existingKey,
						weaponName ? weaponName : "",
						weaponRefID,
						actorRefID,
						attempt->actorBaseRefID,
						ref->typeID,
						animDataID,
						s_autoWeaponPowerReassertLogCount);
				}
				return true;
			}

			UInt32 existingMapEntry = 0;
			UInt32 existingIndex = 0xFFFFFFFF;
			UInt32 existingCount = 0;
			const char* matchedPath = NULL;
			UInt32 existingSequence = FindAutoWeaponAnimationSequenceForKey(animData, *attempt, existingKey, &existingMapEntry, &existingIndex, &existingCount, &matchedPath);
			if (existingSequence)
			{
				if (ShouldLogAutoWeaponPowerReassert(true))
				{
					const char* groupName = requestedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(requestedGroupID) : "";
					_MESSAGE("CustomAnimSupport auto-weapon power reassert skipped reload group=%s (%u) key=%04X weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mapEntry=%08X sequence=%08X index=%u count=%u mapping=\"%s\" path=\"%s\" logCount=%u",
						groupName ? groupName : "",
						requestedGroupID,
						existingKey,
						weaponName ? weaponName : "",
						weaponRefID,
						actorRefID,
						attempt->actorBaseRefID,
						ref->typeID,
						animDataID,
						existingMapEntry,
						existingSequence,
						existingIndex,
						existingCount,
						matchedPath ? matchedPath : "",
						GetBSAnimGroupSequencePath(existingSequence) ? GetBSAnimGroupSequencePath(existingSequence) : "",
						s_autoWeaponPowerReassertLogCount);
				}
				return true;
			}
		}

		s_autoWeaponAnimApplyGuard = true;

		WeaponScopedApplyResult applied;
		std::vector<ScopedInstalledAnim> installed = attempted ? attempt->installed : std::vector<ScopedInstalledAnim>();
		std::vector<std::string> ruleNames;
		std::vector<std::string> rulePaths;
		for (std::vector<AutoWeaponAnimRule>::const_iterator it = matchedRules.begin(); it != matchedRules.end(); ++it)
		{
			AddUniqueString(&ruleNames, it->nameContains.c_str());
			AddUniqueString(&rulePaths, it->animPath.c_str());
			WeaponAnimEntry tempEntry(weaponRefID, 0, it->animPath.c_str());
			ApplyWeaponAnimEntryForActor(ref, actorBase, tempEntry, &applied, &installed);
		}

		if (applied.found)
			applied.loaded = LoadSpecialAnimationsForRef(ref, actorBase);

		if (!attempted)
		{
			RecordAutoWeaponAnimationAttempt(actorRefID, weaponRefID, actorBase->refID, animDataID, ref->typeID, applied.loaded, ruleNames, rulePaths, installed);
		}
		else
		{
			attempt->actorBaseRefID = actorBase->refID;
			attempt->actorTypeID = ref->typeID;
			attempt->generation = s_autoWeaponAnimGeneration;
			attempt->loaded = applied.loaded;
			attempt->nameContains = ruleNames;
			attempt->animPaths = rulePaths;
			attempt->installed = installed;
		}

		s_autoWeaponAnimApplyGuard = false;

		if (forceReload && ShouldLogAutoWeaponPowerReassert(applied.loaded))
		{
			const char* groupName = requestedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(requestedGroupID) : "";
			_MESSAGE("CustomAnimSupport auto-weapon power reassert group=%s (%u) weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X rules=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
				groupName ? groupName : "",
				requestedGroupID,
				weaponName ? weaponName : "",
				weaponRefID,
				actorRefID,
				actorBase->refID,
				ref->typeID,
				animDataID,
				applied.mappings,
				applied.found,
				applied.added,
				applied.existing,
				applied.invalid,
				applied.errors,
				applied.loaded ? 1 : 0,
				s_autoWeaponAnimGeneration);
		}
		else
		{
			_MESSAGE("CustomAnimSupport auto-weapon apply weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X rules=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
				weaponName ? weaponName : "",
				weaponRefID,
				actorRefID,
				actorBase->refID,
				ref->typeID,
				animDataID,
				applied.mappings,
				applied.found,
				applied.added,
				applied.existing,
				applied.invalid,
				applied.errors,
				applied.loaded ? 1 : 0,
				s_autoWeaponAnimGeneration);
		}

		return applied.loaded;
	}

	static bool ApplyAutoWeaponAnimationsForRef(TESObjectREFR* ref, bool forceReload, bool printToConsole, const char* commandName)
	{
		const char* safeCommandName = commandName ? commandName : "CASApplyAutoWeaponAnimations";
		if (!ref || !IsActorReference(ref))
		{
			if (printToConsole)
				Console_Print("%s: command requires a live actor reference", safeCommandName);
			return false;
		}

		Actor* actor = OBLIVION_CAST(ref, TESObjectREFR, Actor);
		if (!actor)
		{
			if (printToConsole)
				Console_Print("%s: reference %08X is not an Actor", safeCommandName, ref->refID);
			return false;
		}

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("%s: actor %08X has no actor base", safeCommandName, ref->refID);
			return false;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("%s: actor %08X has no live ActorAnimData; move the actor into an active loaded process and retry", safeCommandName, ref->refID);
			_MESSAGE("CustomAnimSupport %s skipped: actor=%08X actorBase=%08X has no live ActorAnimData", safeCommandName, ref->refID, actorBase->refID);
			return false;
		}

		UInt32 weaponSource = kEquippedWeaponSource_None;
		UInt32 weaponProcess = 0;
		UInt32 weaponEntryData = 0;
		TESObjectWEAP* weapon = GetEquippedWeaponForRef(ref, &weaponSource, &weaponProcess, &weaponEntryData);
		if (!weapon)
		{
			RevertInvalidAutoWeaponAnimationsForActor(actor);
			if (printToConsole)
				Console_Print("%s: actor %08X has no equipped weapon exposed by the decoded process path process=%08X entryData=%08X source=%s", safeCommandName, ref->refID, weaponProcess, weaponEntryData, GetEquippedWeaponSourceName(weaponSource));
			_MESSAGE("CustomAnimSupport %s skipped: actor=%08X actorBase=%08X animData=%08X process=%08X entryData=%08X source=%s has no equipped weapon", safeCommandName, ref->refID, actorBase->refID, (UInt32)animData, weaponProcess, weaponEntryData, GetEquippedWeaponSourceName(weaponSource));
			return false;
		}

		std::vector<AutoWeaponAnimRule> matchedRules;
		FindAutoWeaponAnimationRulesForWeapon(weapon, &matchedRules);
		const char* weaponName = GetFullName(weapon);
		if (matchedRules.empty())
		{
			RevertInvalidAutoWeaponAnimationsForActor(actor);
			if (printToConsole)
				Console_Print("%s: equipped weapon \"%s\" (%08X) source=%s matched no auto-weapon animation rule; configuredRules=%u", safeCommandName, weaponName ? weaponName : "", weapon->refID, GetEquippedWeaponSourceName(weaponSource), CountAutoWeaponAnimationRules(NULL));
			_MESSAGE("CustomAnimSupport %s skipped: actor=%08X actorBase=%08X animData=%08X weapon=\"%s\" (%08X) process=%08X entryData=%08X source=%s matched no auto-weapon rule configuredRules=%u",
				safeCommandName, ref->refID, actorBase->refID, (UInt32)animData, weaponName ? weaponName : "", weapon->refID, weaponProcess, weaponEntryData, GetEquippedWeaponSourceName(weaponSource), CountAutoWeaponAnimationRules(NULL));
			return false;
		}

		AutoWeaponAnimAttempt* before = FindAutoWeaponAnimationAttempt(ref->refID, weapon->refID, (UInt32)animData);
		if (before && before->loaded && before->generation == s_autoWeaponAnimGeneration && !forceReload)
		{
			if (printToConsole)
				Console_Print("%s: already loaded actor=%08X actorBase=%08X weapon=\"%s\" (%08X) source=%s animData=%08X rules=%u generation=%u",
					safeCommandName, ref->refID, actorBase->refID, weaponName ? weaponName : "", weapon->refID, GetEquippedWeaponSourceName(weaponSource), (UInt32)animData, (UInt32)before->animPaths.size(), s_autoWeaponAnimGeneration);
			_MESSAGE("CustomAnimSupport %s already loaded: actor=%08X actorBase=%08X weapon=\"%s\" (%08X) source=%s animData=%08X rules=%u generation=%u",
				safeCommandName, ref->refID, actorBase->refID, weaponName ? weaponName : "", weapon->refID, GetEquippedWeaponSourceName(weaponSource), (UInt32)animData, (UInt32)before->animPaths.size(), s_autoWeaponAnimGeneration);
			return true;
		}

		bool loaded = AutoApplyWeaponAnimationsForActor(actor, TESAnimGroup::kAnimGroup_Max, forceReload);
		AutoWeaponAnimAttempt* after = FindAutoWeaponAnimationAttempt(ref->refID, weapon->refID, (UInt32)animData);
		if (after)
			loaded = after->loaded;

		if (printToConsole)
			Console_Print("%s forceReload=%u actor=%08X actorBase=%08X weapon=\"%s\" (%08X) source=%s animData=%08X matchedRules=%u loaded=%u generation=%u",
				safeCommandName,
				forceReload ? 1 : 0,
				ref->refID,
				actorBase->refID,
				weaponName ? weaponName : "",
				weapon->refID,
				GetEquippedWeaponSourceName(weaponSource),
				(UInt32)animData,
				(UInt32)matchedRules.size(),
				loaded ? 1 : 0,
				s_autoWeaponAnimGeneration);

		_MESSAGE("CustomAnimSupport %s forceReload=%u actor=%08X actorBase=%08X type=%02X weapon=\"%s\" (%08X) process=%08X entryData=%08X source=%s animData=%08X matchedRules=%u loaded=%u attempted=%u generation=%u",
			safeCommandName,
			forceReload ? 1 : 0,
			ref->refID,
			actorBase->refID,
			ref->typeID,
			weaponName ? weaponName : "",
			weapon->refID,
			weaponProcess,
			weaponEntryData,
			GetEquippedWeaponSourceName(weaponSource),
			(UInt32)animData,
			(UInt32)matchedRules.size(),
			loaded ? 1 : 0,
			after ? 1 : 0,
			s_autoWeaponAnimGeneration);

		return loaded;
	}

	static bool SetAutoWeaponAnimationPathForRef(TESObjectREFR* ref, const char* nameContains, bool enable, const char* path, bool applyNow, const char* commandName, bool printToConsole)
	{
		const char* safeCommandName = commandName ? commandName : "CASSetAutoWeaponAnimationPath";
		if (!nameContains || !nameContains[0])
		{
			if (printToConsole)
				Console_Print("%s failed: weaponNameContains is empty", safeCommandName);
			return false;
		}

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		bool folderMode = false;
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), &folderMode))
		{
			if (printToConsole)
				Console_Print("%s failed: weaponNameContains=\"%s\" path=\"%s\" is not a safe native SpecialAnims folder or .kf path",
					safeCommandName,
					nameContains,
					path ? path : "");
			return false;
		}

		UInt32 changed = 0;
		if (enable)
			changed = AddAutoWeaponAnimationRule(nameContains, cleanPath, true, false) ? 1 : 0;
		else
			changed = RemoveAutoWeaponAnimationRule(nameContains, cleanPath);

		if (applyNow && ref && IsActorReference(ref))
		{
			Actor* actor = OBLIVION_CAST(ref, TESObjectREFR, Actor);
			if (actor)
			{
				RevertInvalidAutoWeaponAnimationsForActor(actor);
				if (enable)
					AutoApplyWeaponAnimationsForActor(actor, TESAnimGroup::kAnimGroup_Max, true);
			}
		}

		UInt32 rules = CountAutoWeaponAnimationRules(NULL);
		UInt32 matchingRules = CountAutoWeaponAnimationRules(nameContains);
		_MESSAGE("CustomAnimSupport %s weaponNameContains=\"%s\" enable=%u mode=%s path=\"%s\" changed=%u matchingRules=%u rules=%u applyNow=%u generation=%u",
			safeCommandName,
			nameContains,
			enable ? 1 : 0,
			folderMode ? "folder" : "kf",
			cleanPath,
			changed,
			matchingRules,
			rules,
			applyNow ? 1 : 0,
			s_autoWeaponAnimGeneration);

		if (printToConsole)
		{
			Console_Print("%s weaponNameContains=\"%s\" enable=%u mode=%s path=\"%s\" changed=%u matchingRules=%u rules=%u applyNow=%u generation=%u",
				safeCommandName,
				nameContains,
				enable ? 1 : 0,
				folderMode ? "folder" : "kf",
				cleanPath,
				changed,
				matchingRules,
				rules,
				applyNow ? 1 : 0,
				s_autoWeaponAnimGeneration);
			Console_Print("%s: this is a plugin-managed weapon-name rule over actor-base SpecialAnims, not a decoded native weapon-type override map", safeCommandName);
		}

		return changed != 0;
	}

	static bool ApplyTargetMappingsForRef(TESObjectREFR* ref, bool forceReload, bool printToConsole)
	{
		if (!ref || !IsActorReference(ref))
		{
			if (printToConsole)
				Console_Print("CASApplyTargetMappings: command requires a live actor reference");
			return false;
		}

		Actor* actor = OBLIVION_CAST(ref, TESObjectREFR, Actor);
		if (!actor)
		{
			if (printToConsole)
				Console_Print("CASApplyTargetMappings: reference %08X is not an Actor", ref->refID);
			return false;
		}

		TESActorBase* actorBase = GetActorBaseFromRef(ref);
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("CASApplyTargetMappings: actor %08X has no actor base", ref->refID);
			return false;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("CASApplyTargetMappings: actor %08X has no live ActorAnimData; move the actor into an active loaded process and retry", ref->refID);
			_MESSAGE("CustomAnimSupport CASApplyTargetMappings skipped: actor=%08X actorBase=%08X has no live ActorAnimData", ref->refID, actorBase->refID);
			return false;
		}

		UInt32 configuredMappings = CountTargetAnimationMappings(0);
		if (!configuredMappings)
		{
			if (printToConsole)
				Console_Print("CASApplyTargetMappings: no race/global manifest target mappings are loaded; run CASLoadSpecialAnimManifests or place JSON manifests in the default CustomAnimSupport directory");
			_MESSAGE("CustomAnimSupport CASApplyTargetMappings skipped: actor=%08X actorBase=%08X animData=%08X no configured race/global target mappings", ref->refID, actorBase->refID, (UInt32)animData);
			return false;
		}

		TargetScopedApplyResult applied = ApplyTargetScopedAnimationsForActor(actor, TESAnimGroup::kAnimGroup_Max, forceReload);

		if (printToConsole)
			Console_Print("CASApplyTargetMappings forceReload=%u actor=%08X actorBase=%08X animData=%08X configured=%u matched=%u foundTargets=%u loadedTargets=%u mappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u",
				forceReload ? 1 : 0,
				ref->refID,
				actorBase->refID,
				(UInt32)animData,
				configuredMappings,
				applied.matched,
				applied.foundTargets,
				applied.loadedTargets,
				applied.mappings,
				applied.found,
				applied.added,
				applied.existing,
				applied.invalid,
				applied.errors,
				applied.loaded ? 1 : 0);

		_MESSAGE("CustomAnimSupport CASApplyTargetMappings forceReload=%u actor=%08X actorBase=%08X type=%02X animData=%08X configured=%u matched=%u foundTargets=%u loadedTargets=%u mappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
			forceReload ? 1 : 0,
			ref->refID,
			actorBase->refID,
			ref->typeID,
			(UInt32)animData,
			configuredMappings,
			applied.matched,
			applied.foundTargets,
			applied.loadedTargets,
			applied.mappings,
			applied.found,
			applied.added,
			applied.existing,
			applied.invalid,
			applied.errors,
			applied.loaded ? 1 : 0,
			s_targetAnimGeneration);

		return applied.loaded;
	}

	static bool SetTargetAnimationPathForRef(TESObjectREFR* ref, TESForm* target, bool enable, const char* path, bool applyNow, const char* commandName, bool printToConsole)
	{
		const char* safeCommandName = commandName ? commandName : "CASSetTargetAnimationPath";
		UInt32 scope = GetTargetAnimScopeForForm(target);
		if (!target || scope == kTargetAnimScope_None)
		{
			if (printToConsole)
				Console_Print("%s failed: target=%08X type=%02X (%s) is not a TESRace or TESGlobal target",
					safeCommandName,
					target ? target->refID : 0,
					target ? target->typeID : 0,
					target ? GetFormTypeName(target->typeID) : "Missing");
			return false;
		}

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		bool folderMode = false;
		if (!NormalizeWeaponAnimationPath(path, cleanPath, sizeof(cleanPath), &folderMode))
		{
			if (printToConsole)
				Console_Print("%s failed: target=%08X scope=%s path=\"%s\" is not a safe native SpecialAnims folder or .kf path",
					safeCommandName,
					target->refID,
					GetTargetAnimScopeName(scope),
					path ? path : "");
			return false;
		}

		UInt32 changed = 0;
		if (enable)
			changed = AddTargetAnimationMapping(target, cleanPath, true) ? 1 : 0;
		else
			changed = RemoveTargetAnimationMapping(target, cleanPath);

		if (changed)
			ClearTargetAnimationAttempts();

		TargetScopedApplyResult applied;
		if (enable && applyNow && ref && IsActorReference(ref))
		{
			Actor* actor = OBLIVION_CAST(ref, TESObjectREFR, Actor);
			if (actor)
				applied = ApplyTargetScopedAnimationsForActor(actor, TESAnimGroup::kAnimGroup_Max, true);
		}

		UInt32 mappings = CountTargetAnimationMappings(target->refID);
		_MESSAGE("CustomAnimSupport %s target=%08X scope=%s enable=%u mode=%s path=\"%s\" changed=%u mappings=%u applyNow=%u matched=%u foundTargets=%u loadedTargets=%u appliedMappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
			safeCommandName,
			target->refID,
			GetTargetAnimScopeName(scope),
			enable ? 1 : 0,
			folderMode ? "folder" : "kf",
			cleanPath,
			changed,
			mappings,
			applyNow ? 1 : 0,
			applied.matched,
			applied.foundTargets,
			applied.loadedTargets,
			applied.mappings,
			applied.found,
			applied.added,
			applied.existing,
			applied.invalid,
			applied.errors,
			applied.loaded ? 1 : 0,
			s_targetAnimGeneration);

		if (printToConsole)
		{
			Console_Print("%s target=%08X scope=%s enable=%u mode=%s path=\"%s\" changed=%u mappings=%u applyNow=%u matched=%u foundTargets=%u loadedTargets=%u appliedFound=%u appliedAdded=%u appliedExisting=%u appliedInvalid=%u appliedErrors=%u loaded=%u",
				safeCommandName,
				target->refID,
				GetTargetAnimScopeName(scope),
				enable ? 1 : 0,
				folderMode ? "folder" : "kf",
				cleanPath,
				changed,
				mappings,
				applyNow ? 1 : 0,
				applied.matched,
				applied.foundTargets,
				applied.loadedTargets,
				applied.found,
				applied.added,
				applied.existing,
				applied.invalid,
				applied.errors,
				applied.loaded ? 1 : 0);
			Console_Print("%s: this is a plugin-managed race/global mapping over actor-base SpecialAnims, not a decoded native race/global override map", safeCommandName);
		}

		return changed != 0;
	}

	static void TryAutoApplyWeaponAnimationsForPlayer()
	{
		if (g_thePlayer && *g_thePlayer)
			AutoApplyWeaponAnimationsForActor((Actor*)(*g_thePlayer), TESAnimGroup::kAnimGroup_Max, false);
	}

	static void TryApplyTargetScopedAnimationsForPlayer()
	{
		if (g_thePlayer && *g_thePlayer)
			ApplyTargetScopedAnimationsForActor((Actor*)(*g_thePlayer), TESAnimGroup::kAnimGroup_Max, false);
	}

	static UInt16 __fastcall Hook_ActorLoadAnimGroup(Actor* actor, void*, UInt32 groupID, UInt32 arg2, UInt32 arg3)
	{
		RevertInvalidWeaponScopedAnimationsForActor(actor);
		RevertInvalidAutoWeaponAnimationsForActor(actor);
		ApplyWeaponScopedAnimationsForActor(actor, groupID, IsVolatilePowerAttackGroup(groupID));
		ApplyTargetScopedAnimationsForActor(actor, groupID, IsVolatilePowerAttackGroup(groupID));
		AutoApplyWeaponAnimationsForActor(actor, groupID, IsVolatilePowerAttackGroup(groupID));
		return s_actorLoadAnimGroupOriginal ? s_actorLoadAnimGroupOriginal(actor, groupID, arg2, arg3) : 0x00FF;
	}

	static UInt32 __fastcall Hook_ActorAnimDataPlayEncodedGroup(ActorAnimData* animData, void*, UInt32 encodedGroup, UInt32 slotArg)
	{
		UInt16 key = encodedGroup & 0xFFFF;
		UInt32 groupID = GetAnimKeyGroupID(key);
		TESObjectWEAP* weapon = NULL;
		AutoWeaponAnimAttempt* autoWeaponAttempt = NULL;
		WeaponAnimAttempt* weaponAttempt = NULL;
		TargetAnimAttempt* targetAttempt = NULL;

		RevertInvalidWeaponScopedAnimationsForAnimData(animData);
		RevertInvalidAutoWeaponAnimationsForAnimData(animData);

		if (IsWeaponScopedPreferenceGroup(groupID) && IsWeaponScopedAnimData(animData, &weapon, &weaponAttempt))
		{
			UInt32 mapEntry = 0;
			UInt32 sequenceIndex = 0xFFFFFFFF;
			UInt32 sequenceCount = 0;
			const char* matchedPath = NULL;
			UInt32 sequence = FindWeaponScopedSequenceForKey(animData, weapon ? weapon->refID : 0, key, &mapEntry, &sequenceIndex, &sequenceCount, &matchedPath);
			if (sequence)
			{
				UInt32 played = ThisStdCall(kActorAnimDataPlaySequence, animData, sequence, (UInt32)key, slotArg);
				if (ShouldLogWeaponScopedAttackPrefer(played))
				{
					const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
					_MESSAGE("CustomAnimSupport weapon-scope attack prefer key=%04X group=%s weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mapEntry=%08X sequence=%08X index=%u count=%u mapping=\"%s\" path=\"%s\" played=%08X logCount=%u",
						key,
						groupName ? groupName : "",
						weapon ? GetFullName(weapon) : "",
						weapon ? weapon->refID : 0,
						weaponAttempt ? weaponAttempt->actorRefID : 0,
						weaponAttempt ? weaponAttempt->actorBaseRefID : 0,
						weaponAttempt ? weaponAttempt->actorTypeID : 0,
						(UInt32)animData,
						mapEntry,
						sequence,
						sequenceIndex,
						sequenceCount,
						matchedPath ? matchedPath : "",
						GetBSAnimGroupSequencePath(sequence) ? GetBSAnimGroupSequencePath(sequence) : "",
						played,
						s_weaponScopedAttackPreferLogCount);
				}
				return played;
			}
			else if (ShouldLogWeaponScopedAttackFallback())
			{
				const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
				_MESSAGE("CustomAnimSupport weapon-scope attack fallback key=%04X group=%s weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mapEntry=%08X count=%u reason=no mapped sequence path logCount=%u",
					key,
					groupName ? groupName : "",
					weapon ? GetFullName(weapon) : "",
					weapon ? weapon->refID : 0,
					weaponAttempt ? weaponAttempt->actorRefID : 0,
					weaponAttempt ? weaponAttempt->actorBaseRefID : 0,
					weaponAttempt ? weaponAttempt->actorTypeID : 0,
					(UInt32)animData,
					mapEntry,
					sequenceCount,
					s_weaponScopedAttackFallbackLogCount);
			}
		}

		if (IsWeaponScopedPreferenceGroup(groupID))
		{
			UInt32 mapEntry = 0;
			UInt32 sequenceIndex = 0xFFFFFFFF;
			UInt32 sequenceCount = 0;
			const char* matchedPath = NULL;
			UInt32 sequence = FindTargetScopedSequenceForKey(animData, key, &mapEntry, &sequenceIndex, &sequenceCount, &matchedPath, &targetAttempt);
			if (sequence)
			{
				UInt32 played = ThisStdCall(kActorAnimDataPlaySequence, animData, sequence, (UInt32)key, slotArg);
				if (ShouldLogTargetScopedAttackPrefer(played))
				{
					const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
					_MESSAGE("CustomAnimSupport target-scope attack prefer key=%04X group=%s target=%08X scope=%s actor=%08X actorBase=%08X type=%02X animData=%08X mapEntry=%08X sequence=%08X index=%u count=%u mapping=\"%s\" path=\"%s\" played=%08X logCount=%u",
						key,
						groupName ? groupName : "",
						targetAttempt ? targetAttempt->targetFormID : 0,
						targetAttempt ? GetTargetAnimScopeName(targetAttempt->scope) : "",
						targetAttempt ? targetAttempt->actorRefID : 0,
						targetAttempt ? targetAttempt->actorBaseRefID : 0,
						targetAttempt ? targetAttempt->actorTypeID : 0,
						(UInt32)animData,
						mapEntry,
						sequence,
						sequenceIndex,
						sequenceCount,
						matchedPath ? matchedPath : "",
						GetBSAnimGroupSequencePath(sequence) ? GetBSAnimGroupSequencePath(sequence) : "",
						played,
						s_targetScopedAttackPreferLogCount);
				}
				return played;
			}
			else if (ShouldLogTargetScopedAttackFallback() && !s_targetAnimAttempts.empty())
			{
				const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
				_MESSAGE("CustomAnimSupport target-scope attack fallback key=%04X group=%s animData=%08X mapEntry=%08X count=%u reason=no mapped sequence path logCount=%u",
					key,
					groupName ? groupName : "",
					(UInt32)animData,
					mapEntry,
					sequenceCount,
					s_targetScopedAttackFallbackLogCount);
			}
		}

		if (IsWeaponScopedPreferenceGroup(groupID) && IsAutoWeaponAnimationData(animData, &weapon, &autoWeaponAttempt))
		{
			UInt32 mapEntry = 0;
			UInt32 sequenceIndex = 0xFFFFFFFF;
			UInt32 sequenceCount = 0;
			const char* matchedPath = NULL;
			UInt32 sequence = autoWeaponAttempt ? FindAutoWeaponAnimationSequenceForKey(animData, *autoWeaponAttempt, key, &mapEntry, &sequenceIndex, &sequenceCount, &matchedPath) : 0;
			if (sequence)
			{
				UInt32 played = ThisStdCall(kActorAnimDataPlaySequence, animData, sequence, (UInt32)key, slotArg);
				if (ShouldLogAutoWeaponAttackPrefer(played))
				{
					const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
					_MESSAGE("CustomAnimSupport auto-weapon attack prefer key=%04X group=%s weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mapEntry=%08X sequence=%08X index=%u count=%u mapping=\"%s\" path=\"%s\" played=%08X logCount=%u",
						key,
						groupName ? groupName : "",
						weapon ? GetFullName(weapon) : "",
						weapon ? weapon->refID : 0,
						autoWeaponAttempt ? autoWeaponAttempt->actorRefID : 0,
						autoWeaponAttempt ? autoWeaponAttempt->actorBaseRefID : 0,
						autoWeaponAttempt ? autoWeaponAttempt->actorTypeID : 0,
						(UInt32)animData,
						mapEntry,
						sequence,
						sequenceIndex,
						sequenceCount,
						matchedPath ? matchedPath : "",
						GetBSAnimGroupSequencePath(sequence) ? GetBSAnimGroupSequencePath(sequence) : "",
						played,
						s_autoWeaponAttackPreferLogCount);
				}
				return played;
			}
			else if (ShouldLogAutoWeaponAttackFallback())
			{
				const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
				_MESSAGE("CustomAnimSupport auto-weapon attack fallback key=%04X group=%s weapon=\"%s\" (%08X) actor=%08X actorBase=%08X type=%02X animData=%08X mapEntry=%08X count=%u reason=no matching auto-weapon sequence path logCount=%u",
					key,
					groupName ? groupName : "",
					weapon ? GetFullName(weapon) : "",
					weapon ? weapon->refID : 0,
					autoWeaponAttempt ? autoWeaponAttempt->actorRefID : 0,
					autoWeaponAttempt ? autoWeaponAttempt->actorBaseRefID : 0,
					autoWeaponAttempt ? autoWeaponAttempt->actorTypeID : 0,
					(UInt32)animData,
					mapEntry,
					sequenceCount,
					s_autoWeaponAttackFallbackLogCount);
			}
		}

		return s_actorAnimDataPlayEncodedGroupOriginal ? s_actorAnimDataPlayEncodedGroupOriginal(animData, encodedGroup, slotArg) : 0;
	}

	static bool InstallActorLoadAnimGroupHook()
	{
		if (s_actorLoadAnimGroupHookInstalled)
			return true;

		const UInt8 expected[kActorLoadAnimGroupPatchSize] = { 0x83, 0xEC, 0x08, 0x55, 0x56 };
		if (std::memcmp((void*)kActorLoadAnimGroup, expected, sizeof(expected)) != 0)
		{
			UInt8* found = (UInt8*)kActorLoadAnimGroup;
			_MESSAGE("CustomAnimSupport auto-weapon hook skipped: Actor_LoadAnimGroup prologue mismatch at %08X found=%02X %02X %02X %02X %02X",
				kActorLoadAnimGroup, found[0], found[1], found[2], found[3], found[4]);
			return false;
		}

		UInt8* trampoline = (UInt8*)VirtualAlloc(NULL, kActorLoadAnimGroupPatchSize + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!trampoline)
		{
			_MESSAGE("CustomAnimSupport auto-weapon hook skipped: failed to allocate Actor_LoadAnimGroup trampoline");
			return false;
		}

		std::memcpy(trampoline, (void*)kActorLoadAnimGroup, kActorLoadAnimGroupPatchSize);
		trampoline[kActorLoadAnimGroupPatchSize] = 0xE9;
		*(UInt32*)(trampoline + kActorLoadAnimGroupPatchSize + 1) = (kActorLoadAnimGroup + kActorLoadAnimGroupPatchSize) - ((UInt32)trampoline + kActorLoadAnimGroupPatchSize + 5);

		s_actorLoadAnimGroupOriginal = (ActorLoadAnimGroupFn)trampoline;
		WriteRelJump(kActorLoadAnimGroup, (UInt32)&Hook_ActorLoadAnimGroup);
		s_actorLoadAnimGroupHookInstalled = true;
		_MESSAGE("CustomAnimSupport auto-weapon hook installed: Actor_LoadAnimGroup=%08X trampoline=%08X rules=%u defaultFolder=\"%s\" defaultWeaponNameContains=\"%s\"",
			kActorLoadAnimGroup, trampoline, CountAutoWeaponAnimationRules(NULL), kDefaultAutoWeaponSpecialAnimFolder, kDefaultAutoWeaponNameNeedle);
		return true;
	}

	static bool InstallActorAnimDataPlayEncodedGroupHook()
	{
		if (s_actorAnimDataPlayEncodedGroupHookInstalled)
			return true;

		const UInt8 expected[kActorAnimDataPlayEncodedGroupPatchSize] = { 0x56, 0x57, 0x8B, 0x7C, 0x24, 0x0C };
		if (std::memcmp((void*)kActorAnimDataPlayEncodedGroup, expected, sizeof(expected)) != 0)
		{
			UInt8* found = (UInt8*)kActorAnimDataPlayEncodedGroup;
			_MESSAGE("CustomAnimSupport auto-weapon attack-prefer hook skipped: ActorAnimData_PlayEncodedGroup prologue mismatch at %08X found=%02X %02X %02X %02X %02X %02X",
				kActorAnimDataPlayEncodedGroup, found[0], found[1], found[2], found[3], found[4], found[5]);
			return false;
		}

		UInt8* trampoline = (UInt8*)VirtualAlloc(NULL, kActorAnimDataPlayEncodedGroupPatchSize + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!trampoline)
		{
			_MESSAGE("CustomAnimSupport auto-weapon attack-prefer hook skipped: failed to allocate ActorAnimData_PlayEncodedGroup trampoline");
			return false;
		}

		std::memcpy(trampoline, (void*)kActorAnimDataPlayEncodedGroup, kActorAnimDataPlayEncodedGroupPatchSize);
		trampoline[kActorAnimDataPlayEncodedGroupPatchSize] = 0xE9;
		*(UInt32*)(trampoline + kActorAnimDataPlayEncodedGroupPatchSize + 1) = (kActorAnimDataPlayEncodedGroup + kActorAnimDataPlayEncodedGroupPatchSize) - ((UInt32)trampoline + kActorAnimDataPlayEncodedGroupPatchSize + 5);

		s_actorAnimDataPlayEncodedGroupOriginal = (ActorAnimDataPlayEncodedGroupFn)trampoline;
		WriteRelJump(kActorAnimDataPlayEncodedGroup, (UInt32)&Hook_ActorAnimDataPlayEncodedGroup);
		SafeWrite8(kActorAnimDataPlayEncodedGroup + 5, 0x90);
		s_actorAnimDataPlayEncodedGroupHookInstalled = true;
		_MESSAGE("CustomAnimSupport auto-weapon attack-prefer hook installed: ActorAnimData_PlayEncodedGroup=%08X trampoline=%08X rules=%u defaultFolder=\"%s\" defaultWeaponNameContains=\"%s\"",
			kActorAnimDataPlayEncodedGroup, trampoline, CountAutoWeaponAnimationRules(NULL), kDefaultAutoWeaponSpecialAnimFolder, kDefaultAutoWeaponNameNeedle);
		return true;
	}

	static UInt32 CountActiveAnimationSlots(ActorAnimData* animData)
	{
		if (!animData)
			return 0;

		UInt32 count = 0;
		for (UInt32 i = 0; i < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++i)
		{
			if (animData->animSequences[i])
				++count;
		}

		return count;
	}

	static void PrintAnimationKey(UInt32 slot, const char* prefix, UInt16 key)
	{
		UInt32 groupID = GetAnimKeyGroupID(key);
		const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
		const char* movementName = GetPointerTableString(kAnimMovementPrefixTable, GetAnimKeyMovementPrefix(key), 16);
		const char* weaponName = GetPointerTableString(kAnimWeaponPrefixTable, GetAnimKeyWeaponPrefix(key), 16);
		const char* slotName = GetPointerTableString(kAnimSlotNameTable, slot, 5);

		Console_Print("%s%s -> %s/%s/%s (%04X)", prefix ? prefix : "", slotName, movementName, weaponName, groupName, key);
	}

	static UInt32 DumpAnimationState(TESObjectREFR* ref, bool includeQueued, bool printToConsole)
	{
		ActorAnimData* animData = GetActorAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("CASDumpAnimationState: calling actor has no ActorAnimData");
			return 0;
		}

		UInt32 activeSlots = 0;
		if (printToConsole)
		{
			Console_Print("--- CAS Animation State -------------------------");
			Console_Print("Anims Loading: %u", ThisStdCall(kActorAnimDataAnimsLoading, animData));
			Console_Print("Idle Playing: %u", IsIdleInactive(animData) ? 0 : 1);
		}

		for (UInt32 slot = 0; slot < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++slot)
		{
			UInt16 currentKey = GetActorAnimDataKey(animData, slot, false);
			UInt16 queuedKey = GetActorAnimDataKey(animData, slot, true);
			bool currentActive = animData->animSequences[slot] != NULL;
			bool queuedActive = IsActiveAnimKey(queuedKey);

			if (currentActive)
			{
				++activeSlots;
				if (printToConsole)
					PrintAnimationKey(slot, "", currentKey);
			}

			if (includeQueued && queuedActive && printToConsole)
				PrintAnimationKey(slot, "Queued ", queuedKey);
		}

		if (printToConsole && !activeSlots)
			Console_Print("CASDumpAnimationState: no active animation slots");

		if (printToConsole)
		{
			TESForm* currentIdle = GetAnimIdleForm(animData->unkC8[1]);
			TESForm* queuedIdle = GetAnimIdleForm(animData->unkC8[2]);
			if (currentIdle)
				Console_Print("IdleAnim: %s (%08X)", GetFullName(currentIdle), currentIdle->refID);
			if (queuedIdle)
				Console_Print("IdleAnim Queued: %s (%08X)", GetFullName(queuedIdle), queuedIdle->refID);
			Console_Print("Deferred: count=%u head=%08X tail=%08X", CountDeferredInstallModels(animData), animData->unkB4, animData->unkB8);
		}

		return activeSlots;
	}

	static UInt32 ValidateSaveLoadState(TESObjectREFR* ref, bool printToConsole)
	{
		ActorAnimData* animData = GetActorAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("CASValidateSaveLoadState: calling actor has no ActorAnimData");
			return 1;
		}

		UInt32 issues = 0;
		UInt32 savedSlots = 0;

		if (printToConsole)
			Console_Print("--- CAS Save/Load Animation State ----------------");

		if (!animData->map9C)
		{
			if (printToConsole)
				Console_Print("CASValidateSaveLoadState: live ActorAnimData has no animation map");
			++issues;
		}

		for (UInt32 slot = 0; slot < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++slot)
		{
			UInt16 key = GetActorAnimDataKey(animData, slot, false);
			if (!IsNativeSavedSlotKey(key))
				continue;

			++savedSlots;
			UInt32 entry = 0;
			bool mapFound = LookupAnimationMapEntry(animData, key, &entry);
			bool hasSequence = animData->animSequences[slot] != NULL;

			if (printToConsole)
			{
				const char* slotName = GetPointerTableString(kAnimSlotNameTable, slot, 5);
				const char* groupName = GetAnimKeyGroupID(key) < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(GetAnimKeyGroupID(key)) : "";
				Console_Print("Save slot %s key=%04X group=%s mapEntry=%08X sequence=%08X", slotName, key, groupName, entry, (UInt32)animData->animSequences[slot]);
			}

			if (!mapFound)
			{
				if (printToConsole)
					Console_Print("CASValidateSaveLoadState: slot %u key %04X has no anim-map entry; native load restore clears the slot", slot, key);
				++issues;
			}

			if (!hasSequence)
			{
				if (printToConsole)
					Console_Print("CASValidateSaveLoadState: slot %u key %04X has no live sequence; native save writes the null-sequence marker", slot, key);
				++issues;
			}
		}

		UInt32 idleSource = animData->unkC8[2] ? animData->unkC8[2] : animData->unkC8[1];
		TESForm* savedIdle = GetAnimIdleForm(idleSource);
		if (printToConsole)
		{
			if (savedIdle)
				Console_Print("Save idle source=%s form=%s (%08X)", animData->unkC8[2] ? "queued" : "current", GetFullName(savedIdle), savedIdle->refID);
			else
				Console_Print("Save idle source=%s form=%08X", idleSource ? (animData->unkC8[2] ? "queued" : "current") : "none", 0);
		}

		if (idleSource && !savedIdle)
		{
			if (printToConsole)
				Console_Print("CASValidateSaveLoadState: idle object %08X has no form pointer at +0x24 for native idle save", idleSource);
			++issues;
		}

		if (printToConsole)
			Console_Print("CASValidateSaveLoadState savedSlots=%u issues=%u", savedSlots, issues);

		return issues;
	}

	static UInt32 ValidateDeferredAnimations(TESObjectREFR* ref, bool printToConsole)
	{
		ActorAnimData* animData = GetActorAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("CASValidateDeferredAnims: calling actor has no ActorAnimData");
			return 1;
		}

		UInt32 issues = 0;
		UInt32 pending = CountDeferredInstallModels(animData);

		if (printToConsole)
			Console_Print("CASValidateDeferredAnims: pending=%u head=%08X tail=%08X", pending, animData->unkB4, animData->unkB8);

		if (!animData->unkB4 && animData->unkB8)
		{
			if (printToConsole)
				Console_Print("CASValidateDeferredAnims: deferred list has a tail node without a head KFModel");
			++issues;
		}

		if (pending >= kDeferredInstallGuard && printToConsole)
			Console_Print("CASValidateDeferredAnims: deferred list reached guard limit %u", kDeferredInstallGuard);
		if (pending >= kDeferredInstallGuard)
			++issues;

		for (UInt32 i = 0; i < pending && i < kDeferredInstallGuard; ++i)
		{
			UInt32 kfModel = GetDeferredInstallKFModel(animData, i);
			if (!kfModel)
			{
				if (printToConsole)
					Console_Print("CASValidateDeferredAnims: deferred[%u] has no KFModel", i);
				++issues;
				continue;
			}

			TESAnimGroup* animGroup = GetKFModelAnimGroup(kfModel);
			NiControllerSequence* sequence = GetKFModelControllerSequence(kfModel);
			UInt32 groupID = GetParsedAnimGroupID(animGroup);
			UInt32 groupKey = GetParsedAnimGroupKey(animGroup);
			const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";

			if (printToConsole)
				Console_Print("CASValidateDeferredAnims: deferred[%u] kfModel=%08X group=%s (%04X) sequence=%08X refs=%u", i, kfModel, groupName, groupKey, (UInt32)sequence, GetKFModelRefCount(kfModel));

			if (!animGroup)
			{
				if (printToConsole)
					Console_Print("CASValidateDeferredAnims: deferred[%u] has no parsed TESAnimGroup", i);
				++issues;
			}
			else if (groupID >= TESAnimGroup::kAnimGroup_Max)
			{
				if (printToConsole)
					Console_Print("CASValidateDeferredAnims: deferred[%u] has invalid native group %04X", i, groupKey);
				++issues;
			}
			else
			{
				if (groupID == TESAnimGroup::kAnimGroup_Idle || groupID == TESAnimGroup::kAnimGroup_Death)
				{
					if (printToConsole)
						Console_Print("CASValidateDeferredAnims: deferred[%u] group %s is not deferred by native AnimSequenceSingle", i, groupName);
					++issues;
				}

				if (IsVolatilePowerAttackGroup(groupID))
				{
					if (printToConsole)
						Console_Print("CASValidateDeferredAnims: deferred[%u] group %s is volatile; native power-attack rebuild can remove it", i, groupName);
					++issues;
				}
			}

			if (!sequence)
			{
				if (printToConsole)
					Console_Print("CASValidateDeferredAnims: deferred[%u] has no controller sequence", i);
				++issues;
			}
		}

		if (printToConsole)
			Console_Print("CASValidateDeferredAnims issues=%u", issues);

		return issues;
	}

	static UInt32 ValidateAnimGroupLoaded(TESObjectREFR* ref, UInt32 groupID, bool printToConsole)
	{
		UInt32 issues = 0;
		ActorAnimData* animData = GetActorAnimData(ref);
		if (!IsActorReference(ref) || !animData)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimGroupLoaded: calling reference is not an actor with ActorAnimData");
			return 1;
		}

		if (groupID >= TESAnimGroup::kAnimGroup_Max)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimGroupLoaded: invalid native group %u", groupID);
			return 1;
		}

		UInt16 encodedGroup = 0xFFFF;
		UInt32 mapEntry = 0;
		bool loaded = ResolveLoadedAnimGroup(ref, groupID, &encodedGroup, &mapEntry);
		UInt32 encodedGroupID = GetAnimKeyGroupID(encodedGroup);
		const char* requestedName = TESAnimGroup::StringForAnimGroupCode(groupID);
		const char* resolvedName = encodedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(encodedGroupID) : "";

		if (printToConsole)
		{
			Console_Print("CASValidateAnimGroupLoaded: requested=%s (%u) resolvedKey=%04X mapEntry=%08X", requestedName, groupID, encodedGroup, mapEntry);
			if (IsNativeSavedSlotKey(encodedGroup))
				Console_Print("CASValidateAnimGroupLoaded: resolved=%s movement=%s weapon=%s", resolvedName, GetPointerTableString(kAnimMovementPrefixTable, GetAnimKeyMovementPrefix(encodedGroup), 16), GetPointerTableString(kAnimWeaponPrefixTable, GetAnimKeyWeaponPrefix(encodedGroup), 16));
		}

		if (!IsNativeSavedSlotKey(encodedGroup))
		{
			if (printToConsole)
				Console_Print("CASValidateAnimGroupLoaded: Actor_LoadAnimGroup_ returned native sentinel %04X", encodedGroup);
			++issues;
		}
		else if (!loaded)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimGroupLoaded: resolved key %04X is absent from ActorAnimData.animsMap; ActorAnimData_PlayEncodedGroup would fail", encodedGroup);
			++issues;
		}

		UInt32 activeMatches = 0;
		UInt32 queuedMatches = 0;
		for (UInt32 slot = 0; slot < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++slot)
		{
			if (GetActorAnimDataKey(animData, slot, false) == encodedGroup && animData->animSequences[slot])
				++activeMatches;
			if (GetActorAnimDataKey(animData, slot, true) == encodedGroup)
				++queuedMatches;
		}

		if (printToConsole)
			Console_Print("CASValidateAnimGroupLoaded: activeMatches=%u queuedMatches=%u issues=%u", activeMatches, queuedMatches, issues);

		return issues;
	}

	static UInt32 ValidateSpecialAnimLoadState(TESObjectREFR* ref, TESActorBase* explicitBase, const char* folder, bool printToConsole)
	{
		UInt32 issues = 0;
		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: unsafe folder \"%s\"", folder ? folder : "");
			return 1;
		}

		if (!ref || !IsActorReference(ref))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: calling reference is not an actor");
			return 1;
		}

		TESActorBase* actorBase = ResolveActorBase(ref, explicitBase);
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: no actor base");
			return 1;
		}

		TESActorBase* actualBase = GetActorBaseFromRef(ref);
		if (actualBase != actorBase)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: calling actor base %08X does not match validation base %08X", actualBase ? actualBase->refID : 0, actorBase->refID);
			++issues;
		}

		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: form %08X has no TESAnimation component", actorBase->refID);
			return issues + 1;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		if (!animData || !animData->map9C)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: live ActorAnimData has no animation map");
			return issues + 1;
		}

		UInt32 matched = 0;
		UInt32 checked = 0;
		UInt32 mapLoaded = 0;
		UInt32 deferred = 0;
		UInt32 missing = 0;
		UInt32 parseErrors = 0;
		UInt32 skipped = 0;

		if (printToConsole)
			Console_Print("CASValidateSpecialAnimLoadState: actorBase=%08X folder=\"%s\" deferred=%u", actorBase->refID, cleanFolder, CountDeferredInstallModels(animData));

		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			const char* entry = cur->animationName;
			if (!PathMatchesOptionalFolder(entry, cleanFolder))
				continue;

			++matched;

			if (!IsSafeRelativePath(entry))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimLoadState: unsafe KFFZ entry \"%s\"", entry ? entry : "");
				++skipped;
				++issues;
				continue;
			}

			if (!IsKFPath(entry))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimLoadState: non-KF KFFZ entry \"%s\"", entry);
				++skipped;
				++issues;
				continue;
			}

			UInt16 key = 0xFFFF;
			UInt32 groupID = TESAnimGroup::kAnimGroup_Max;
			char loaderPath[1024] = { 0 };
			if (!ReadSpecialAnimParsedKey(actorBase, entry, &key, &groupID, loaderPath, sizeof(loaderPath), printToConsole))
			{
				++parseErrors;
				++issues;
				continue;
			}

			++checked;
			UInt32 mapEntry = 0;
			const char* groupName = TESAnimGroup::StringForAnimGroupCode(groupID);
			if (LookupAnimationMapEntry(animData, key, &mapEntry))
			{
				++mapLoaded;
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimLoadState: loaded %s key=%04X mapEntry=%08X path=\"%s\"", groupName, key, mapEntry, entry);
			}
			else
			{
				UInt32 deferredModel = 0;
				if (FindDeferredInstallKey(animData, key, &deferredModel))
				{
					++deferred;
					if (printToConsole)
						Console_Print("CASValidateSpecialAnimLoadState: deferred %s key=%04X kfModel=%08X path=\"%s\"", groupName, key, deferredModel, entry);
				}
				else
				{
					++missing;
					++issues;
					if (printToConsole)
						Console_Print("CASValidateSpecialAnimLoadState: missing %s key=%04X path=\"%s\"; not in anim map or deferred install list", groupName, key, entry);
				}
			}

			if (printToConsole && IsVolatilePowerAttackGroup(groupID))
				Console_Print("CASValidateSpecialAnimLoadState: %s is a volatile native power-attack group", groupName);
		}

		if (cleanFolder[0] && !matched)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimLoadState: no registered KFFZ entries under folder \"%s\"", cleanFolder);
			++issues;
		}

		if (printToConsole)
			Console_Print("CASValidateSpecialAnimLoadState: matched=%u checked=%u mapLoaded=%u deferred=%u missing=%u parseErrors=%u skipped=%u issues=%u", matched, checked, mapLoaded, deferred, missing, parseErrors, skipped, issues);

		return issues;
	}

	struct SpecialAnimVariantSummary
	{
		UInt16 key;
		UInt32 groupID;
		UInt32 count;
		bool allowsMultiple;

		SpecialAnimVariantSummary(UInt16 parsedKey, UInt32 parsedGroupID) :
			key(parsedKey),
			groupID(parsedGroupID),
			count(1),
			allowsMultiple(AnimGroupAllowsMultiple(parsedGroupID))
		{
		}
	};

	static SpecialAnimVariantSummary* FindVariantSummary(std::vector<SpecialAnimVariantSummary>* summaries, UInt16 key)
	{
		if (!summaries)
			return NULL;

		for (std::vector<SpecialAnimVariantSummary>::iterator it = summaries->begin(); it != summaries->end(); ++it)
		{
			if (it->key == key)
				return &(*it);
		}

		return NULL;
	}

	static UInt32 ValidateSpecialAnimVariants(TESObjectREFR* ref, TESActorBase* explicitBase, const char* folder, bool printToConsole)
	{
		UInt32 issues = 0;
		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(folder, cleanFolder, sizeof(cleanFolder)))
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimVariants: unsafe folder \"%s\"", folder ? folder : "");
			return 1;
		}

		TESActorBase* actorBase = ResolveActorBase(ref, explicitBase);
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimVariants: no actor base");
			return 1;
		}

		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimVariants: form %08X has no TESAnimation component", actorBase->refID);
			return 1;
		}

		ActorAnimData* animData = NULL;
		if (ref && IsActorReference(ref) && GetActorBaseFromRef(ref) == actorBase)
			animData = GetActorAnimData(ref);

		std::vector<SpecialAnimVariantSummary> summaries;
		UInt32 matched = 0;
		UInt32 checked = 0;
		UInt32 parseErrors = 0;
		UInt32 skipped = 0;

		if (printToConsole)
			Console_Print("CASValidateSpecialAnimVariants: actorBase=%08X folder=\"%s\" live=%u", actorBase->refID, cleanFolder, animData ? 1 : 0);

		for (TESAnimation::AnimationNode* cur = &anim->data; cur && cur->animationName; cur = cur->next)
		{
			const char* entry = cur->animationName;
			if (!PathMatchesOptionalFolder(entry, cleanFolder))
				continue;

			++matched;
			if (!IsSafeRelativePath(entry))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimVariants: unsafe KFFZ entry \"%s\"", entry ? entry : "");
				++skipped;
				++issues;
				continue;
			}

			if (!IsKFPath(entry))
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimVariants: non-KF KFFZ entry \"%s\"", entry);
				++skipped;
				++issues;
				continue;
			}

			UInt16 key = 0xFFFF;
			UInt32 groupID = TESAnimGroup::kAnimGroup_Max;
			if (!ReadSpecialAnimParsedKey(actorBase, entry, &key, &groupID, NULL, 0, printToConsole))
			{
				++parseErrors;
				++issues;
				continue;
			}

			++checked;
			SpecialAnimVariantSummary* summary = FindVariantSummary(&summaries, key);
			if (summary)
			{
				++summary->count;
			}
			else
			{
				summaries.push_back(SpecialAnimVariantSummary(key, groupID));
			}
		}

		if (cleanFolder[0] && !matched)
		{
			if (printToConsole)
				Console_Print("CASValidateSpecialAnimVariants: no registered KFFZ entries under folder \"%s\"", cleanFolder);
			++issues;
		}

		UInt32 duplicateKeys = 0;
		UInt32 blockedDuplicateKeys = 0;
		for (std::vector<SpecialAnimVariantSummary>::const_iterator it = summaries.begin(); it != summaries.end(); ++it)
		{
			if (it->count <= 1)
				continue;

			++duplicateKeys;
			const char* groupName = it->groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(it->groupID) : "";
			UInt32 mapEntry = 0;
			bool mapLoaded = animData && LookupAnimationMapEntry(animData, it->key, &mapEntry);
			UInt32 mapCount = mapLoaded ? CountMapEntrySequences(mapEntry) : 0;
			UInt32 deferredCount = animData ? CountDeferredInstallKey(animData, it->key) : 0;

			if (it->allowsMultiple)
			{
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimVariants: variant key=%04X group=%s movement=%s weapon=%s registered=%u mapKind=%s mapCount=%u deferred=%u", it->key, groupName, GetPointerTableString(kAnimMovementPrefixTable, GetAnimKeyMovementPrefix(it->key), 16), GetPointerTableString(kAnimWeaponPrefixTable, GetAnimKeyWeaponPrefix(it->key), 16), it->count, GetAnimSequenceEntryKind(mapEntry), mapCount, deferredCount);

				if (animData && (mapCount + deferredCount) < it->count)
				{
					if (printToConsole)
						Console_Print("CASValidateSpecialAnimVariants: allow-multiple key %04X has fewer live/deferred sequences than registered variants", it->key);
					++issues;
				}
			}
			else
			{
				++blockedDuplicateKeys;
				++issues;
				if (printToConsole)
					Console_Print("CASValidateSpecialAnimVariants: duplicate key=%04X group=%s registered=%u but native group-info allow-multiple flag is 0; later installs replace the single sequence", it->key, groupName, it->count);
			}
		}

		if (printToConsole)
			Console_Print("CASValidateSpecialAnimVariants: matched=%u checked=%u uniqueKeys=%u duplicateKeys=%u blockedDuplicateKeys=%u parseErrors=%u skipped=%u issues=%u", matched, checked, summaries.size(), duplicateKeys, blockedDuplicateKeys, parseErrors, skipped, issues);

		return issues;
	}

	static UInt32 ValidateAnimationTarget(TESObjectREFR* ref, TESForm* targetForm, bool printToConsole)
	{
		TESForm* target = targetForm ? targetForm : ref;
		if (!target)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationTarget: no target form and no calling reference");
			return 1;
		}

		TESActorBase* actorBase = GetActorBaseFromForm(target);
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationTarget: target=%08X type=%02X (%s) is not an actor base or actor reference; decoded Oblivion KFFZ/SpecialAnims target scope is actor-base only",
					target->refID,
					target->typeID,
					GetFormTypeName(target->typeID));
			return 1;
		}

		UInt32 issues = 0;
		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
			++issues;

		const char* modelPath = GetActorBaseModelPath(actorBase);
		if (!modelPath || !modelPath[0])
			++issues;

		char root[520] = { 0 };
		bool hasRoot = GetSpecialAnimsRoot(actorBase, root, sizeof(root));
		if (!hasRoot)
			++issues;

		bool rootExists = hasRoot && DirectoryExists(root);
		UInt32 entryCount = CountAnimations(anim);
		bool targetIsLiveRef = ref && target == ref;
		bool targetBaseMatchesCallingRef = ref && GetActorBaseFromRef(ref) == actorBase;

		if (printToConsole)
			Console_Print("CASValidateAnimationTarget: target=%08X type=%02X (%s) actorBase=%08X animList=%u entries=%u model=\"%s\" specialAnims=\"%s\" exists=%u liveRef=%u matchesCallingRef=%u issues=%u",
				target->refID,
				target->typeID,
				GetFormTypeName(target->typeID),
				actorBase->refID,
				anim ? 1 : 0,
				entryCount,
				modelPath ? modelPath : "",
				hasRoot ? root : "",
				rootExists ? 1 : 0,
				targetIsLiveRef ? 1 : 0,
				targetBaseMatchesCallingRef ? 1 : 0,
				issues);

		_MESSAGE("CustomAnimSupport CASValidateAnimationTarget target=%08X type=%02X actorBase=%08X animList=%u entries=%u model=\"%s\" specialAnims=\"%s\" exists=%u liveRef=%u matchesCallingRef=%u issues=%u",
			target->refID,
			target->typeID,
			actorBase->refID,
			anim ? 1 : 0,
			entryCount,
			modelPath ? modelPath : "",
			hasRoot ? root : "",
			rootExists ? 1 : 0,
			targetIsLiveRef ? 1 : 0,
			targetBaseMatchesCallingRef ? 1 : 0,
			issues);

		return issues;
	}

	static UInt32 ValidateAnimationPipeline(TESObjectREFR* ref, TESActorBase* explicitBase, bool reload, bool printToConsole)
	{
		UInt32 issues = 0;
		TESActorBase* actorBase = ResolveActorBase(ref, explicitBase);
		if (!actorBase)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationPipeline: no actor base");
			return 1;
		}

		TESAnimation* anim = GetAnimationList(actorBase);
		if (!anim)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationPipeline: form %08X has no TESAnimation component", actorBase->refID);
			return 1;
		}

		UInt32 entryCount = CountAnimations(anim);
		UInt32 persistedCount = CountPersistentAnimations(actorBase);
		if (printToConsole)
			Console_Print("CASValidateAnimationPipeline: actorBase=%08X entries=%u persisted=%u reload=%u", actorBase->refID, entryCount, persistedCount, reload ? 1 : 0);

		if (!entryCount)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationPipeline: actor base has no KFFZ/SpecialAnims entries");
			++issues;
		}

		char modelDir[260];
		if (!GetModelDirectory(actorBase, modelDir, sizeof(modelDir)))
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationPipeline: actor base has no usable model directory");
			++issues;
		}
		else if (printToConsole)
		{
			Console_Print("CASValidateAnimationPipeline: model directory \"%s\"", modelDir);
		}

		issues += ValidateSpecialAnimations(ref, actorBase, printToConsole);

		if (ref)
		{
			if (!IsActorReference(ref))
			{
				if (printToConsole)
					Console_Print("CASValidateAnimationPipeline: calling reference is not an actor");
				if (reload)
					++issues;
			}
			else
			{
				TESActorBase* actualBase = GetActorBaseFromRef(ref);
				if (actualBase != actorBase)
				{
					if (printToConsole)
						Console_Print("CASValidateAnimationPipeline: calling actor base %08X does not match validation base %08X", actualBase ? actualBase->refID : 0, actorBase->refID);
					++issues;
				}

				ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
				if (!animData)
				{
					if (printToConsole)
						Console_Print("CASValidateAnimationPipeline: live actor has no ActorAnimData");
					++issues;
				}
				else
				{
					if (!animData->manager)
					{
						if (printToConsole)
							Console_Print("CASValidateAnimationPipeline: live ActorAnimData has no controller manager");
						++issues;
					}

					if (!animData->map9C)
					{
						if (printToConsole)
							Console_Print("CASValidateAnimationPipeline: live ActorAnimData has no animation map");
						++issues;
					}

					if (!GetActorBaseSkeletonRoot(ref))
					{
						if (printToConsole)
							Console_Print("CASValidateAnimationPipeline: live actor has no skeleton root");
						++issues;
					}

					if (printToConsole)
						Console_Print("CASValidateAnimationPipeline: activeSlots=%u idlePlaying=%u deferred=%u deferredHead=%08X deferredTail=%08X currentIdle=%08X queuedIdle=%08X", CountActiveAnimationSlots(animData), IsIdleInactive(animData) ? 0 : 1, CountDeferredInstallModels(animData), animData->unkB4, animData->unkB8, animData->unkC8[1], animData->unkC8[2]);
				}

				if (reload)
				{
					bool loaded = actualBase == actorBase && LoadSpecialAnimationsForRef(ref, actorBase);
					if (printToConsole)
						Console_Print("CASValidateAnimationPipeline: reload >> %u", loaded ? 1 : 0);
					if (!loaded)
						++issues;
				}
			}
		}
		else if (reload)
		{
			if (printToConsole)
				Console_Print("CASValidateAnimationPipeline: reload requested without a calling actor reference");
			++issues;
		}

		if (printToConsole)
			Console_Print("CASValidateAnimationPipeline issues=%u", issues);

		return issues;
	}

	static ActorPathChangeResult SetActorAnimationPathDetailed(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, bool enable, bool loadNow)
	{
		ActorPathChangeResult result;
		TESActorBase* resolvedBase = ResolveActorBase(ref, actorBase);
		TESAnimation* anim = GetAnimationList(resolvedBase);
		if (!anim)
		{
			++result.errors;
			return result;
		}

		char cleanPath[kMaxSpecialAnimPath] = { 0 };
		strncpy_s(cleanPath, sizeof(cleanPath), path ? path : "", _TRUNCATE);
		NormalizeSlashes(cleanPath);

		if (IsKFPath(cleanPath))
		{
			if (enable && !IsPersistableAnimationPath(cleanPath))
			{
				++result.errors;
				return result;
			}

			result.changed = enable ? AddAnimation(anim, cleanPath) : RemoveAnimation(anim, cleanPath);
			if (enable)
			{
				if (result.changed)
					result.added = 1;
				if (result.changed || HasExactAnimation(anim, cleanPath))
					AddPersistentAnimation(resolvedBase, cleanPath);
				if (loadNow && ref)
					result.loaded = LoadSpecialAnimationsForRef(ref, resolvedBase);
			}
			else
			{
				ScopedRevertResult reverted = RemoveLiveRegisteredSequencesForPath(ref, resolvedBase, cleanPath);
				result.removedLive += reverted.removedLive;
				result.clearedSlots += reverted.clearedSlots;
				result.errors += reverted.errors;
				if (result.changed)
					result.removed = 1;
				result.persistedRemoved = RemovePersistentAnimation(resolvedBase, cleanPath);
				result.changed = result.changed || result.removedLive != 0 || result.persistedRemoved != 0;
			}

			return result;
		}

		char cleanFolder[kMaxSpecialAnimPath] = { 0 };
		if (!NormalizeRelativeFolder(cleanPath, cleanFolder, sizeof(cleanFolder)) || !cleanFolder[0])
		{
			++result.errors;
			return result;
		}

		result.folderMode = true;
		if (enable)
		{
			DiscoverResult discovered = DiscoverSpecialAnimations(resolvedBase, cleanFolder, true);
			result.found = discovered.found;
			result.added = discovered.added;
			result.skipped = discovered.skipped;
			result.errors = discovered.errors;
			result.changed = discovered.added != 0;
			if (loadNow && ref && discovered.found && !discovered.errors)
				result.loaded = LoadSpecialAnimationsForRef(ref, resolvedBase);
		}
		else
		{
			ScopedRevertResult reverted = RemoveLiveRegisteredSequencesForPath(ref, resolvedBase, cleanFolder);
			result.removedLive += reverted.removedLive;
			result.clearedSlots += reverted.clearedSlots;
			result.errors += reverted.errors;
			result.removed = RemoveAnimationsInFolder(anim, cleanFolder);
			result.persistedRemoved = RemovePersistentAnimationsInFolder(resolvedBase, cleanFolder);
			result.changed = result.removed != 0 || result.removedLive != 0 || result.persistedRemoved != 0;
		}

		return result;
	}

	static bool SetActorAnimationPath(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, bool enable, bool loadNow)
	{
		return SetActorAnimationPathDetailed(ref, actorBase, path, enable, loadNow).changed;
	}

	static bool RegisterAndLoadAnimationPath(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, bool trackPersistent)
	{
		if (!ref || !path || !path[0] || !IsSafeRelativePath(path) || !IsKFPath(path))
			return false;

		TESActorBase* resolvedBase = ResolveActorBase(ref, actorBase);
		TESAnimation* anim = GetAnimationList(resolvedBase);
		if (!anim)
			return false;

		bool added = AddAnimation(anim, path);
		if (!added && !HasExactAnimation(anim, path))
			return false;

		bool loaded = LoadSpecialAnimationsForRef(ref, resolvedBase);
		if (trackPersistent && loaded && (added || HasExactAnimation(anim, path)))
			AddPersistentAnimation(resolvedBase, path);

		return loaded;
	}

	static bool PlayNativeAnimGroup(TESObjectREFR* ref, UInt32 groupID, bool playImmediately)
	{
		UInt16 encodedGroup = 0xFFFF;
		if (!ResolveLoadedAnimGroup(ref, groupID, &encodedGroup, NULL))
			return false;

		ActorAnimData* animData = GetActorAnimData(ref);
		if (!animData)
			return false;

		ThisStdCall(kActorAnimDataPlayAnimGroup, animData, (UInt32)encodedGroup, playImmediately ? 1 : 0, 0xFFFFFFFF);
		return true;
	}

	static bool PlayAnimationPathSequence(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, UInt32 expectedGroupID, bool requireExpectedGroup, bool playImmediately, const char* commandName, bool printToConsole)
	{
		if (!RegisterAndLoadAnimationPath(ref, actorBase, path, false))
			return false;

		TESActorBase* resolvedBase = ResolveActorBase(ref, actorBase);
		UInt16 parsedKey = 0xFFFF;
		UInt32 groupID = TESAnimGroup::kAnimGroup_Max;
		char loaderPath[1024];
		if (!ReadSpecialAnimParsedKey(resolvedBase, path, &parsedKey, &groupID, loaderPath, sizeof(loaderPath), false))
		{
			if (printToConsole)
				Console_Print("%s: native KF parser could not resolve a playable group for \"%s\"", commandName, path);
			return false;
		}

		const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";
		if (requireExpectedGroup && expectedGroupID != groupID)
		{
			if (printToConsole)
				Console_Print("%s: supplied group %s (%u) does not match parsed KF group %s (%u) for \"%s\"",
					commandName,
					expectedGroupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(expectedGroupID) : "",
					expectedGroupID,
					groupName,
					groupID,
					path);
			return false;
		}

		UInt32 noteClass = GetAnimGroupNoteClass(groupID);
		if (!playImmediately && noteClass <= 1)
		{
			if (printToConsole)
				Console_Print("%s: queued path-specific playback is not decoded for group %s; native queue stores only key=%04X, not a BSAnimGroupSequence path", commandName, groupName, parsedKey);
			return false;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		if (!animData)
			return false;

		UInt32 mapEntry = 0;
		if (!LookupAnimationMapEntry(animData, parsedKey, &mapEntry))
		{
			if (printToConsole)
				Console_Print("%s: parsed key %04X from %s is not loaded in ActorAnimData", commandName, parsedKey, loaderPath);
			return false;
		}

		UInt32 sequenceIndex = 0xFFFFFFFF;
		UInt32 sequenceCount = 0;
		UInt32 sequence = FindSequenceMatchingSpecialAnimPathInMapEntry(mapEntry, loaderPath, path, &sequenceIndex, &sequenceCount);
		if (!sequence)
		{
			if (printToConsole)
				Console_Print("%s: parsed key %04X from %s is loaded, but no BSAnimGroupSequence path matched the requested SpecialAnims path; mapEntry=%08X count=%u", commandName, parsedKey, loaderPath, mapEntry, sequenceCount);
			return false;
		}

		UInt32 played = ThisStdCall(kActorAnimDataPlaySequence, animData, sequence, (UInt32)parsedKey, 0xFFFFFFFF);
		if (printToConsole)
			Console_Print("%s: %s parsed key=%04X group=%s mapEntry=%08X sequence=%08X index=%u count=%u path=\"%s\" played=%u",
				commandName,
				loaderPath,
				parsedKey,
				groupName,
				mapEntry,
				sequence,
				sequenceIndex,
				sequenceCount,
				GetBSAnimGroupSequencePath(sequence) ? GetBSAnimGroupSequencePath(sequence) : "",
				played ? 1 : 0);

		return played != 0;
	}

	static bool PlayAnimationPath(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, UInt32 groupID, bool playImmediately, bool printToConsole)
	{
		return PlayAnimationPathSequence(ref, actorBase, path, groupID, true, playImmediately, "CASPlayAnimationPath", printToConsole);
	}

	static bool PlayAnimationPathParsedKey(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, bool printToConsole)
	{
		return PlayAnimationPathSequence(ref, actorBase, path, TESAnimGroup::kAnimGroup_Max, false, true, "PlayAnimationPath", printToConsole);
	}

	static UInt32 CountActiveAnimationKeyMatches(ActorAnimData* animData, UInt16 key, UInt32* outQueuedMatches)
	{
		if (outQueuedMatches)
			*outQueuedMatches = 0;

		if (!animData || !IsNativeSavedSlotKey(key))
			return 0;

		UInt32 activeMatches = 0;
		UInt32 queuedMatches = 0;
		for (UInt32 slot = 0; slot < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++slot)
		{
			if (GetActorAnimDataKey(animData, slot, false) == key && animData->animSequences[slot])
				++activeMatches;
			if (GetActorAnimDataKey(animData, slot, true) == key)
				++queuedMatches;
		}

		if (outQueuedMatches)
			*outQueuedMatches = queuedMatches;
		return activeMatches;
	}

	static UInt32 CountActiveAnimationPathMatches(ActorAnimData* animData, UInt16 key, const char* loaderPath, const char* animPath, UInt32* outActiveKeyMatches, UInt32* outQueuedMatches, UInt32* outPathMismatches, UInt32* outMissingPaths)
	{
		if (outActiveKeyMatches)
			*outActiveKeyMatches = 0;
		if (outQueuedMatches)
			*outQueuedMatches = 0;
		if (outPathMismatches)
			*outPathMismatches = 0;
		if (outMissingPaths)
			*outMissingPaths = 0;

		if (!animData || !IsNativeSavedSlotKey(key))
			return 0;

		UInt32 activeKeyMatches = 0;
		UInt32 activePathMatches = 0;
		UInt32 queuedMatches = 0;
		UInt32 pathMismatches = 0;
		UInt32 missingPaths = 0;
		for (UInt32 slot = 0; slot < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++slot)
		{
			if (GetActorAnimDataKey(animData, slot, true) == key)
				++queuedMatches;

			if (GetActorAnimDataKey(animData, slot, false) != key || !animData->animSequences[slot])
				continue;

			++activeKeyMatches;
			const char* sequencePath = GetBSAnimGroupSequencePath((UInt32)animData->animSequences[slot]);
			if (SequencePathMatchesSpecialAnimPath(sequencePath, loaderPath, animPath))
				++activePathMatches;
			else if (sequencePath && sequencePath[0])
				++pathMismatches;
			else
				++missingPaths;
		}

		if (outActiveKeyMatches)
			*outActiveKeyMatches = activeKeyMatches;
		if (outQueuedMatches)
			*outQueuedMatches = queuedMatches;
		if (outPathMismatches)
			*outPathMismatches = pathMismatches;
		if (outMissingPaths)
			*outMissingPaths = missingPaths;
		return activePathMatches;
	}

	static void PrintActiveAnimationPathMatches(ActorAnimData* animData, UInt16 key, const char* loaderPath, const char* animPath)
	{
		if (!animData || !IsNativeSavedSlotKey(key))
			return;

		for (UInt32 slot = 0; slot < SIZEOF_ARRAY(animData->animSequences, BSAnimGroupSequence*); ++slot)
		{
			if (GetActorAnimDataKey(animData, slot, false) != key || !animData->animSequences[slot])
				continue;

			const char* slotName = GetPointerTableString(kAnimSlotNameTable, slot, 5);
			const char* sequencePath = GetBSAnimGroupSequencePath((UInt32)animData->animSequences[slot]);
			bool pathMatch = SequencePathMatchesSpecialAnimPath(sequencePath, loaderPath, animPath);
			Console_Print("IsAnimSequencePlaying: active slot %s sequence=%08X pathMatch=%u path=\"%s\"",
				slotName,
				(UInt32)animData->animSequences[slot],
				pathMatch ? 1 : 0,
				sequencePath ? sequencePath : "");
		}
	}

	static UInt32 IsAnimationPathPlaying(TESObjectREFR* ref, TESActorBase* actorBase, const char* path, bool printToConsole)
	{
		if (!ref || !IsActorReference(ref))
		{
			if (printToConsole)
				Console_Print("IsAnimSequencePlaying: calling reference is not an actor");
			return 0;
		}

		if (!IsPersistableAnimationPath(path))
		{
			if (printToConsole)
				Console_Print("IsAnimSequencePlaying: unsafe or non-KF path \"%s\"", path ? path : "");
			return 0;
		}

		TESActorBase* resolvedBase = ResolveActorBase(ref, actorBase);
		TESActorBase* actualBase = GetActorBaseFromRef(ref);
		if (!resolvedBase || !actualBase || resolvedBase != actualBase)
		{
			if (printToConsole)
				Console_Print("IsAnimSequencePlaying: actor base mismatch or missing actor base");
			return 0;
		}

		TESAnimation* anim = GetAnimationList(resolvedBase);
		if (!anim || !HasExactAnimation(anim, path))
		{
			if (printToConsole)
				Console_Print("IsAnimSequencePlaying: path is not registered in actor-base KFFZ \"%s\"", path);
			return 0;
		}

		UInt16 parsedKey = 0xFFFF;
		UInt32 groupID = TESAnimGroup::kAnimGroup_Max;
		char loaderPath[1024];
		if (!ReadSpecialAnimParsedKey(resolvedBase, path, &parsedKey, &groupID, loaderPath, sizeof(loaderPath), false))
		{
			if (printToConsole)
				Console_Print("IsAnimSequencePlaying: native KF parser could not resolve a playable group for \"%s\"", path);
			return 0;
		}

		ActorAnimData* animData = GetActorBaseSpecialAnimData(ref);
		if (!animData)
		{
			if (printToConsole)
				Console_Print("IsAnimSequencePlaying: calling actor has no ActorAnimData");
			return 0;
		}

		UInt32 mapEntry = 0;
		bool mapLoaded = LookupAnimationMapEntry(animData, parsedKey, &mapEntry);
		UInt32 activeKeyMatches = 0;
		UInt32 queuedMatches = 0;
		UInt32 pathMismatches = 0;
		UInt32 missingPaths = 0;
		UInt32 activePathMatches = CountActiveAnimationPathMatches(animData, parsedKey, loaderPath, path, &activeKeyMatches, &queuedMatches, &pathMismatches, &missingPaths);
		UInt32 mapCount = mapLoaded ? CountMapEntrySequences(mapEntry) : 0;
		const char* groupName = groupID < TESAnimGroup::kAnimGroup_Max ? TESAnimGroup::StringForAnimGroupCode(groupID) : "";

		if (printToConsole)
		{
			Console_Print("IsAnimSequencePlaying: %s parsed key=%04X group=%s mapEntry=%08X mapCount=%u activeKeyMatches=%u activePathMatches=%u queuedMatches=%u pathMismatches=%u missingPaths=%u",
				loaderPath, parsedKey, groupName, mapEntry, mapCount, activeKeyMatches, activePathMatches, queuedMatches, pathMismatches, missingPaths);
			if (activeKeyMatches)
				PrintActiveAnimationPathMatches(animData, parsedKey, loaderPath, path);
			if (activeKeyMatches && !activePathMatches)
				Console_Print("IsAnimSequencePlaying: active native key matched, but no active BSAnimGroupSequence path matched the requested SpecialAnims path");
		}

		return activePathMatches ? 1 : 0;
	}
}

static void CAS_OBSEMessageHandler(OBSEMessagingInterface::Message* message)
{
	if (!message)
		return;

	switch (message->type)
	{
	case OBSEMessagingInterface::kMessage_PostPostLoad:
		CAS::LoadDefaultManifests("OBSE PostPostLoad");
		CAS::TryApplyTargetScopedAnimationsForPlayer();
		CAS::TryAutoApplyWeaponAnimationsForPlayer();
		break;

	case OBSEMessagingInterface::kMessage_PostLoadGame:
		if (message->data)
		{
			CAS::LoadDefaultManifests("OBSE PostLoadGame");
			CAS::TryApplyTargetScopedAnimationsForPlayer();
			CAS::TryAutoApplyWeaponAnimationsForPlayer();
		}
		else
			_MESSAGE("CustomAnimSupport manifest load skipped: OBSE PostLoadGame reported failed load");
		break;
	}
}

static void CAS_SaveCallback(void* reserved)
{
	CAS::SavePersistentAnimations();
	CAS::SaveWeaponAnimationMappings();
	CAS::SaveTargetAnimationMappings();
	CAS::SaveAutoWeaponAnimationRules();
}

static void CAS_LoadCallback(void* reserved)
{
	CAS::LoadPersistentAnimations();
}

static void CAS_PreloadCallback(void* reserved)
{
	CAS::ClearAutoWeaponAnimationAttempts();
	CAS::LoadPersistentAnimations();
}

static void CAS_NewGameCallback(void* reserved)
{
	CAS::ClearAutoWeaponAnimationAttempts();
	CAS::ClearWeaponAnimationAttempts();
	CAS::ClearTargetAnimationAttempts();
	CAS::ClearPersistentAnimations(NULL);
	CAS::ClearWeaponAnimationMappings();
	CAS::ClearTargetAnimationMappings();
	CAS::ClearPersistentAutoWeaponAnimationRules();
	CAS::ClearConfigAutoWeaponAnimationRules();
	CAS::LoadDefaultManifests("OBSE NewGame");
	CAS::TryApplyTargetScopedAnimationsForPlayer();
	CAS::TryAutoApplyWeaponAnimationsForPlayer();
}

// ================================
// Command implementations
// ================================

#ifdef RUNTIME

bool Cmd_CASGetVersion_Execute(COMMAND_ARGS)
{
	*result = (PLUGIN_VERSION_MAJOR * 10000) + (PLUGIN_VERSION_MINOR * 100) + PLUGIN_VERSION_BUILD;
	return true;
}

bool Cmd_CASGetSpecialAnimCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &actorBase))
		return true;

	TESAnimation* anim = CAS::GetAnimationList(CAS::ResolveActorBase(thisObj, actorBase));
	*result = CAS::CountAnimations(anim);
	return true;
}

bool Cmd_CASGetSpecialAnimPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 index = 0;
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &index, &actorBase))
		return true;

	const char* path = CAS::GetAnimationAt(CAS::GetAnimationList(CAS::ResolveActorBase(thisObj, actorBase)), index);
	if (!path)
		path = "";

	if (g_stringVar)
		g_stringVar->Assign(PASS_COMMAND_ARGS, path);
	else if (IsConsoleMode())
		Console_Print("CASGetSpecialAnimPath: OBSE string interface unavailable");

	return true;
}

bool Cmd_CASGetPersistedSpecialAnimCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &actorBase))
		return true;

	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	*result = CAS::CountPersistentAnimations(resolvedBase);
	return true;
}

bool Cmd_CASClearPersistedSpecialAnims_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &actorBase))
		return true;

	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	*result = CAS::ClearPersistentAnimations(resolvedBase);

	if (IsConsoleMode())
		Console_Print("CASClearPersistedSpecialAnims removed=%.0f", *result);

	return true;
}

bool Cmd_CASPruneMissingSpecialAnims_Execute(COMMAND_ARGS)
{
	*result = 0;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::NormalizeSlashes(folder);
	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	CAS::PruneResult pruned = CAS::PruneMissingSpecialAnimations(resolvedBase, folder, IsConsoleMode());
	*result = pruned.removed;

	_MESSAGE("CustomAnimSupport CASPruneMissingSpecialAnims folder=\"%s\" checked=%u missing=%u removed=%u skipped=%u errors=%u", folder, pruned.checked, pruned.missing, pruned.removed, pruned.skipped, pruned.errors);

	if (IsConsoleMode())
		Console_Print("CASPruneMissingSpecialAnims \"%s\" checked=%u missing=%u removed=%u skipped=%u errors=%u", folder, pruned.checked, pruned.missing, pruned.removed, pruned.skipped, pruned.errors);

	return true;
}

bool Cmd_CASValidateKNVSELayout_Execute(COMMAND_ARGS)
{
	*result = 0;

	char directory[CAS::kMaxManifestPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, directory, &actorBase))
		return true;

	CAS::KNVSELayoutResult layout = CAS::ValidateKNVSELayout(thisObj, directory, actorBase, IsConsoleMode());
	UInt32 issues = layout.errors + layout.unsupported + layout.requiresStaging;
	*result = issues;

	const char* scannedDirectory = directory[0] ? directory : CAS::kDefaultKNVSEAnimGroupOverrideDirectory;
	_MESSAGE("CustomAnimSupport CASValidateKNVSELayout directory=\"%s\" files=%u entries=%u targets=%u folders=%u kfs=%u firstPerson=%u nativeReady=%u requiresStaging=%u unsupported=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u errors=%u issues=%u",
		scannedDirectory,
		layout.files,
		layout.entries,
		layout.targets,
		layout.folders,
		layout.kfs,
		layout.firstPersonKfs,
		layout.nativeReady,
		layout.requiresStaging,
		layout.unsupported,
		layout.ignoredFields,
		layout.conditionFields,
		layout.conditionalEntries,
		layout.errors,
		issues);

	if (IsConsoleMode())
		Console_Print("CASValidateKNVSELayout \"%s\" files=%u entries=%u targets=%u folders=%u kfs=%u firstPerson=%u nativeReady=%u requiresStaging=%u unsupported=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u errors=%u issues=%u",
			scannedDirectory,
			layout.files,
			layout.entries,
			layout.targets,
			layout.folders,
			layout.kfs,
			layout.firstPersonKfs,
			layout.nativeReady,
			layout.requiresStaging,
			layout.unsupported,
			layout.ignoredFields,
			layout.conditionFields,
			layout.conditionalEntries,
			layout.errors,
			issues);

	return true;
}

bool Cmd_CASStageKNVSELayout_Execute(COMMAND_ARGS)
{
	*result = 0;

	char directory[CAS::kMaxManifestPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, directory, &actorBase))
		return true;

	CAS::KNVSEStageResult staged = CAS::StageKNVSELayout(thisObj, directory, actorBase, IsConsoleMode());
	*result = staged.validated;

	const char* scannedDirectory = directory[0] ? directory : CAS::kDefaultKNVSEAnimGroupOverrideDirectory;
	_MESSAGE("CustomAnimSupport CASStageKNVSELayout directory=\"%s\" files=%u entries=%u targets=%u found=%u copied=%u existing=%u validated=%u registered=%u invalid=%u skipped=%u unsupported=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u errors=%u",
		scannedDirectory,
		staged.files,
		staged.entries,
		staged.targets,
		staged.found,
		staged.copied,
		staged.existing,
		staged.validated,
		staged.registered,
		staged.invalid,
		staged.skipped,
		staged.unsupported,
		staged.ignoredFields,
		staged.conditionFields,
		staged.conditionalEntries,
		staged.errors);

	if (IsConsoleMode())
		Console_Print("CASStageKNVSELayout \"%s\" files=%u entries=%u targets=%u found=%u copied=%u existing=%u validated=%u registered=%u invalid=%u skipped=%u unsupported=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u errors=%u",
			scannedDirectory,
			staged.files,
			staged.entries,
			staged.targets,
			staged.found,
			staged.copied,
			staged.existing,
			staged.validated,
			staged.registered,
			staged.invalid,
			staged.skipped,
			staged.unsupported,
			staged.ignoredFields,
			staged.conditionFields,
			staged.conditionalEntries,
			staged.errors);

	return true;
}

bool Cmd_CASStageAnimGroupOverride_Execute(COMMAND_ARGS)
{
	*result = 0;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::KNVSEStageResult staged = CAS::StageAnimGroupOverrideFolder(thisObj, actorBase, folder, IsConsoleMode());
	*result = staged.validated;

	_MESSAGE("CustomAnimSupport CASStageAnimGroupOverride folder=\"%s\" targets=%u found=%u copied=%u existing=%u validated=%u registered=%u invalid=%u skipped=%u unsupported=%u errors=%u",
		folder,
		staged.targets,
		staged.found,
		staged.copied,
		staged.existing,
		staged.validated,
		staged.registered,
		staged.invalid,
		staged.skipped,
		staged.unsupported,
		staged.errors);

	if (IsConsoleMode())
		Console_Print("CASStageAnimGroupOverride \"%s\" targets=%u found=%u copied=%u existing=%u validated=%u registered=%u invalid=%u skipped=%u unsupported=%u errors=%u",
			folder,
			staged.targets,
			staged.found,
			staged.copied,
			staged.existing,
			staged.validated,
			staged.registered,
			staged.invalid,
			staged.skipped,
			staged.unsupported,
			staged.errors);

	return true;
}

bool Cmd_CASHasSpecialAnim_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, animPath, &actorBase))
		return true;

	CAS::NormalizeSlashes(animPath);
	TESAnimation* anim = CAS::GetAnimationList(CAS::ResolveActorBase(thisObj, actorBase));
	*result = CAS::HasExactAnimation(anim, animPath) ? 1 : 0;
	return true;
}

bool Cmd_CASAddSpecialAnim_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, animPath, &actorBase))
		return true;

	CAS::NormalizeSlashes(animPath);
	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	TESAnimation* anim = CAS::GetAnimationList(resolvedBase);
	if (CAS::IsPersistableAnimationPath(animPath))
	{
		bool added = CAS::AddAnimation(anim, animPath);
		*result = added ? 1 : 0;
		if (added || CAS::HasExactAnimation(anim, animPath))
			CAS::AddPersistentAnimation(resolvedBase, animPath);
	}

	if (IsConsoleMode())
		Console_Print("CASAddSpecialAnim \"%s\" >> %.0f", animPath, *result);

	return true;
}

bool Cmd_CASRemoveSpecialAnim_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, animPath, &actorBase))
		return true;

	CAS::NormalizeSlashes(animPath);
	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	TESAnimation* anim = CAS::GetAnimationList(resolvedBase);
	*result = CAS::RemoveAnimation(anim, animPath) ? 1 : 0;
	CAS::RemovePersistentAnimation(resolvedBase, animPath);

	if (IsConsoleMode())
		Console_Print("CASRemoveSpecialAnim \"%s\" >> %.0f", animPath, *result);

	return true;
}

bool Cmd_CASLoadSpecialAnims_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &actorBase))
		return true;

	*result = CAS::LoadSpecialAnimationsForRef(thisObj, actorBase) ? 1 : 0;

	if (IsConsoleMode())
		Console_Print("CASLoadSpecialAnims >> %.0f", *result);

	return true;
}

bool Cmd_CASApplyAutoSpear_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 forceReload = 0;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &forceReload))
		return true;

	*result = CAS::ApplyAutoWeaponAnimationsForRef(thisObj, forceReload != 0, IsConsoleMode(), "CASApplyAutoSpear") ? 1 : 0;
	return true;
}

bool Cmd_CASApplyAutoWeaponAnimations_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 forceReload = 0;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &forceReload))
		return true;

	*result = CAS::ApplyAutoWeaponAnimationsForRef(thisObj, forceReload != 0, IsConsoleMode(), "CASApplyAutoWeaponAnimations") ? 1 : 0;
	return true;
}

bool Cmd_CASSetAutoWeaponAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	char nameContains[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 enable = 0;
	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 applyNow = 1;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, nameContains, &enable, animPath, &applyNow))
		return true;

	CAS::NormalizeSlashes(animPath);
	*result = CAS::SetAutoWeaponAnimationPathForRef(thisObj, nameContains, enable != 0, animPath, applyNow != 0, "CASSetAutoWeaponAnimationPath", IsConsoleMode()) ? 1 : 0;
	return true;
}

bool Cmd_CASGetAutoWeaponAnimationRuleCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	char nameContains[CAS::kMaxSpecialAnimPath] = { 0 };
	if (!ExtractArgs(PASS_EXTRACT_ARGS, nameContains))
		return true;

	*result = CAS::CountAutoWeaponAnimationRules(nameContains);
	if (IsConsoleMode())
		Console_Print("CASGetAutoWeaponAnimationRuleCount weaponNameContains=\"%s\" rules=%.0f generation=%u", nameContains, *result, CAS::s_autoWeaponAnimGeneration);

	return true;
}

bool Cmd_CASValidateAutoWeaponAnimationRules_Execute(COMMAND_ARGS)
{
	*result = 0;

	char nameContains[CAS::kMaxSpecialAnimPath] = { 0 };
	if (!ExtractArgs(PASS_EXTRACT_ARGS, nameContains))
		return true;

	if (!thisObj || !CAS::IsActorReference(thisObj))
	{
		*result = 1;
		if (IsConsoleMode())
			Console_Print("CASValidateAutoWeaponAnimationRules: command requires a live actor reference");
		return true;
	}

	CAS::AutoWeaponRuleValidationDiagnostics diagnostics = CAS::ValidateAutoWeaponAnimationRulesForActor(thisObj, nameContains, IsConsoleMode());
	*result = diagnostics.errors;

	_MESSAGE("CustomAnimSupport CASValidateAutoWeaponAnimationRules actor=%08X actorBase=%08X weapon=%08X source=%s process=%08X entryData=%08X filter=\"%s\" configuredRules=%u matchedRules=%u checkedRules=%u validRules=%u foundKfs=%u invalidRules=%u errors=%u generation=%u",
		thisObj->refID,
		diagnostics.actorBaseFormID,
		diagnostics.weaponFormID,
		CAS::GetEquippedWeaponSourceName(diagnostics.weaponSource),
		diagnostics.weaponProcess,
		diagnostics.weaponEntryData,
		nameContains,
		diagnostics.configuredRules,
		diagnostics.matchedRules,
		diagnostics.checkedRules,
		diagnostics.validRules,
		diagnostics.foundKfs,
		diagnostics.invalidRules,
		diagnostics.errors,
		CAS::s_autoWeaponAnimGeneration);

	if (IsConsoleMode())
		Console_Print("CASValidateAutoWeaponAnimationRules actor=%08X actorBase=%08X weapon=%08X source=%s filter=\"%s\" configuredRules=%u matchedRules=%u checkedRules=%u validRules=%u foundKfs=%u invalidRules=%u errors=%u",
			thisObj->refID,
			diagnostics.actorBaseFormID,
			diagnostics.weaponFormID,
			CAS::GetEquippedWeaponSourceName(diagnostics.weaponSource),
			nameContains,
			diagnostics.configuredRules,
			diagnostics.matchedRules,
			diagnostics.checkedRules,
			diagnostics.validRules,
			diagnostics.foundKfs,
			diagnostics.invalidRules,
			diagnostics.errors);

	return true;
}

bool Cmd_CASApplyTargetMappings_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 forceReload = 0;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &forceReload))
		return true;

	*result = CAS::ApplyTargetMappingsForRef(thisObj, forceReload != 0, IsConsoleMode()) ? 1 : 0;
	return true;
}

bool Cmd_CASGetTargetAnimationMappingCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* targetForm = NULL;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs())
		return true;

	if (eval.NumArgs() > 0)
		targetForm = eval.Arg(0)->GetTESForm();

	if (targetForm && !CAS::IsTargetAnimForm(targetForm))
	{
		if (IsConsoleMode())
			Console_Print("CASGetTargetAnimationMappingCount: target=%08X type=%02X (%s) is not a TESRace or TESGlobal target",
				targetForm->refID,
				targetForm->typeID,
				CAS::GetFormTypeName(targetForm->typeID));
		_MESSAGE("CustomAnimSupport CASGetTargetAnimationMappingCount unsupported target=%08X type=%02X (%s)",
			targetForm->refID,
			targetForm->typeID,
			CAS::GetFormTypeName(targetForm->typeID));
		return true;
	}

	CAS::TargetMappingDiagnostics diagnostics = CAS::GetTargetAnimationMappingDiagnostics(targetForm);
	*result = diagnostics.total;

	const char* scopeName = targetForm ? CAS::GetTargetAnimScopeName(CAS::GetTargetAnimScopeForForm(targetForm)) : "All";
	_MESSAGE("CustomAnimSupport CASGetTargetAnimationMappingCount target=%08X scope=%s total=%u race=%u global=%u persisted=%u config=%u invalid=%u generation=%u",
		targetForm ? targetForm->refID : 0,
		scopeName,
		diagnostics.total,
		diagnostics.race,
		diagnostics.global,
		diagnostics.persisted,
		diagnostics.config,
		diagnostics.invalid,
		CAS::s_targetAnimGeneration);

	if (IsConsoleMode())
		Console_Print("CASGetTargetAnimationMappingCount target=%08X scope=%s total=%u race=%u global=%u persisted=%u config=%u invalid=%u generation=%u",
			targetForm ? targetForm->refID : 0,
			scopeName,
			diagnostics.total,
			diagnostics.race,
			diagnostics.global,
			diagnostics.persisted,
			diagnostics.config,
			diagnostics.invalid,
			CAS::s_targetAnimGeneration);

	return true;
}

bool Cmd_CASGetTargetAnimationMatchCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* targetForm = NULL;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs())
		return true;

	if (eval.NumArgs() > 0)
		targetForm = eval.Arg(0)->GetTESForm();

	if (!thisObj || !CAS::IsActorReference(thisObj))
	{
		if (IsConsoleMode())
			Console_Print("CASGetTargetAnimationMatchCount: command requires a live actor reference");
		return true;
	}

	TESActorBase* actorBase = CAS::GetActorBaseFromRef(thisObj);
	if (!actorBase)
	{
		if (IsConsoleMode())
			Console_Print("CASGetTargetAnimationMatchCount: actor %08X has no actor base", thisObj->refID);
		return true;
	}

	if (targetForm && !CAS::IsTargetAnimForm(targetForm))
	{
		if (IsConsoleMode())
			Console_Print("CASGetTargetAnimationMatchCount: target=%08X type=%02X (%s) is not a TESRace or TESGlobal target",
				targetForm->refID,
				targetForm->typeID,
				CAS::GetFormTypeName(targetForm->typeID));
		_MESSAGE("CustomAnimSupport CASGetTargetAnimationMatchCount unsupported target=%08X type=%02X (%s)",
			targetForm->refID,
			targetForm->typeID,
			CAS::GetFormTypeName(targetForm->typeID));
		return true;
	}

	CAS::TargetMatchDiagnostics diagnostics = CAS::GetTargetAnimationMatchDiagnostics(thisObj, targetForm);
	*result = diagnostics.matchedMappings;

	const char* scopeName = targetForm ? CAS::GetTargetAnimScopeName(CAS::GetTargetAnimScopeForForm(targetForm)) : "All";
	_MESSAGE("CustomAnimSupport CASGetTargetAnimationMatchCount actor=%08X actorBase=%08X actorRace=%08X target=%08X scope=%s configured=%u matchedMappings=%u matchedTargets=%u race=%u global=%u invalid=%u generation=%u",
		thisObj->refID,
		diagnostics.actorBaseFormID,
		diagnostics.actorRaceFormID,
		targetForm ? targetForm->refID : 0,
		scopeName,
		diagnostics.configured,
		diagnostics.matchedMappings,
		diagnostics.matchedTargets,
		diagnostics.race,
		diagnostics.global,
		diagnostics.invalid,
		CAS::s_targetAnimGeneration);

	if (IsConsoleMode())
		Console_Print("CASGetTargetAnimationMatchCount actor=%08X actorBase=%08X actorRace=%08X target=%08X scope=%s configured=%u matchedMappings=%u matchedTargets=%u race=%u global=%u invalid=%u generation=%u",
			thisObj->refID,
			diagnostics.actorBaseFormID,
			diagnostics.actorRaceFormID,
			targetForm ? targetForm->refID : 0,
			scopeName,
			diagnostics.configured,
			diagnostics.matchedMappings,
			diagnostics.matchedTargets,
			diagnostics.race,
			diagnostics.global,
			diagnostics.invalid,
			CAS::s_targetAnimGeneration);

	return true;
}

bool Cmd_CASValidateTargetAnimationMappings_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* targetForm = NULL;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs())
		return true;

	if (eval.NumArgs() > 0)
		targetForm = eval.Arg(0)->GetTESForm();

	if (!thisObj || !CAS::IsActorReference(thisObj))
	{
		if (IsConsoleMode())
			Console_Print("CASValidateTargetAnimationMappings: command requires an actor reference");
		return true;
	}

	TESActorBase* actorBase = CAS::GetActorBaseFromRef(thisObj);
	if (!actorBase)
	{
		if (IsConsoleMode())
			Console_Print("CASValidateTargetAnimationMappings: actor %08X has no actor base", thisObj->refID);
		return true;
	}

	if (targetForm && !CAS::IsTargetAnimForm(targetForm))
	{
		if (IsConsoleMode())
			Console_Print("CASValidateTargetAnimationMappings: target=%08X type=%02X (%s) is not a TESRace or TESGlobal target",
				targetForm->refID,
				targetForm->typeID,
				CAS::GetFormTypeName(targetForm->typeID));
		_MESSAGE("CustomAnimSupport CASValidateTargetAnimationMappings unsupported target=%08X type=%02X (%s)",
			targetForm->refID,
			targetForm->typeID,
			CAS::GetFormTypeName(targetForm->typeID));
		return true;
	}

	CAS::TargetValidationDiagnostics diagnostics = CAS::ValidateTargetAnimationMappingsForActor(thisObj, targetForm, IsConsoleMode());
	*result = diagnostics.errors;

	const char* scopeName = targetForm ? CAS::GetTargetAnimScopeName(CAS::GetTargetAnimScopeForForm(targetForm)) : "All";
	_MESSAGE("CustomAnimSupport CASValidateTargetAnimationMappings actor=%08X actorBase=%08X actorRace=%08X target=%08X scope=%s configured=%u matchedMappings=%u matchedTargets=%u checkedMappings=%u validMappings=%u foundKfs=%u invalidMappings=%u race=%u global=%u errors=%u generation=%u",
		thisObj->refID,
		diagnostics.actorBaseFormID,
		diagnostics.actorRaceFormID,
		targetForm ? targetForm->refID : 0,
		scopeName,
		diagnostics.configured,
		diagnostics.matchedMappings,
		diagnostics.matchedTargets,
		diagnostics.checkedMappings,
		diagnostics.validMappings,
		diagnostics.foundKfs,
		diagnostics.invalidMappings,
		diagnostics.race,
		diagnostics.global,
		diagnostics.errors,
		CAS::s_targetAnimGeneration);

	if (IsConsoleMode())
		Console_Print("CASValidateTargetAnimationMappings actor=%08X actorBase=%08X actorRace=%08X target=%08X scope=%s configured=%u matchedMappings=%u matchedTargets=%u checkedMappings=%u validMappings=%u foundKfs=%u invalidMappings=%u race=%u global=%u errors=%u generation=%u",
			thisObj->refID,
			diagnostics.actorBaseFormID,
			diagnostics.actorRaceFormID,
			targetForm ? targetForm->refID : 0,
			scopeName,
			diagnostics.configured,
			diagnostics.matchedMappings,
			diagnostics.matchedTargets,
			diagnostics.checkedMappings,
			diagnostics.validMappings,
			diagnostics.foundKfs,
			diagnostics.invalidMappings,
			diagnostics.race,
			diagnostics.global,
			diagnostics.errors,
			CAS::s_targetAnimGeneration);

	return true;
}

bool Cmd_CASSetRaceAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* race = NULL;
	UInt32 enable = 0;
	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 applyNow = 1;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &race, &enable, animPath, &applyNow))
		return true;

	CAS::NormalizeSlashes(animPath);
	*result = CAS::SetTargetAnimationPathForRef(thisObj, race, enable != 0, animPath, applyNow != 0, "CASSetRaceAnimationPath", IsConsoleMode()) ? 1 : 0;
	return true;
}

bool Cmd_CASSetGlobalAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* global = NULL;
	UInt32 enable = 0;
	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 applyNow = 1;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &global, &enable, animPath, &applyNow))
		return true;

	CAS::NormalizeSlashes(animPath);
	*result = CAS::SetTargetAnimationPathForRef(thisObj, global, enable != 0, animPath, applyNow != 0, "CASSetGlobalAnimationPath", IsConsoleMode()) ? 1 : 0;
	return true;
}

bool Cmd_CASSetActorAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 enable = 0;
	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &enable, animPath, &actorBase))
		return true;

	CAS::NormalizeSlashes(animPath);
	CAS::ActorPathChangeResult changed = CAS::SetActorAnimationPathDetailed(thisObj, actorBase, animPath, enable != 0, true);
	*result = changed.changed ? 1 : 0;

	_MESSAGE("CustomAnimSupport CASSetActorAnimationPath enable=%u mode=%s path=\"%s\" changed=%u loaded=%u found=%u added=%u removed=%u persistedRemoved=%u removedLive=%u clearedSlots=%u existing=%u errors=%u",
		enable,
		changed.folderMode ? "folder" : "kf",
		animPath,
		changed.changed ? 1 : 0,
		changed.loaded ? 1 : 0,
		changed.found,
		changed.added,
		changed.removed,
		changed.persistedRemoved,
		changed.removedLive,
		changed.clearedSlots,
		changed.skipped,
		changed.errors);

	if (IsConsoleMode())
		Console_Print("CASSetActorAnimationPath %d \"%s\" mode=%s changed=%.0f loaded=%u found=%u added=%u removed=%u persistedRemoved=%u removedLive=%u clearedSlots=%u existing=%u errors=%u",
			enable,
			animPath,
			changed.folderMode ? "folder" : "kf",
			*result,
			changed.loaded ? 1 : 0,
			changed.found,
			changed.added,
			changed.removed,
			changed.persistedRemoved,
			changed.removedLive,
			changed.clearedSlots,
			changed.skipped,
			changed.errors);

	return true;
}

bool Cmd_CASDiscoverSpecialAnims_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &actorBase))
		return true;

	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	CAS::DiscoverResult discovered = CAS::DiscoverSpecialAnimations(resolvedBase, NULL, true);
	*result = discovered.added;

	if (thisObj && discovered.added)
		CAS::LoadSpecialAnimationsForRef(thisObj, resolvedBase);

	_MESSAGE("CustomAnimSupport CASDiscoverSpecialAnims found=%u added=%u existing=%u errors=%u", discovered.found, discovered.added, discovered.skipped, discovered.errors);

	if (IsConsoleMode())
		Console_Print("CASDiscoverSpecialAnims found=%u added=%u existing=%u errors=%u", discovered.found, discovered.added, discovered.skipped, discovered.errors);

	return true;
}

bool Cmd_CASDiscoverSpecialAnimFolder_Execute(COMMAND_ARGS)
{
	*result = 0;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::NormalizeSlashes(folder);
	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	CAS::DiscoverResult discovered = CAS::DiscoverSpecialAnimations(resolvedBase, folder, true);
	*result = discovered.added;

	if (thisObj && discovered.added)
		CAS::LoadSpecialAnimationsForRef(thisObj, resolvedBase);

	_MESSAGE("CustomAnimSupport CASDiscoverSpecialAnimFolder folder=\"%s\" found=%u added=%u existing=%u errors=%u", folder, discovered.found, discovered.added, discovered.skipped, discovered.errors);

	if (IsConsoleMode())
		Console_Print("CASDiscoverSpecialAnimFolder \"%s\" found=%u added=%u existing=%u errors=%u", folder, discovered.found, discovered.added, discovered.skipped, discovered.errors);

	return true;
}

bool Cmd_CASLoadAnimGroupOverride_Execute(COMMAND_ARGS)
{
	*result = 0;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::NormalizeSlashes(folder);
	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	CAS::DiscoverResult discovered = CAS::DiscoverSpecialAnimations(resolvedBase, folder, true);
	*result = discovered.added;

	if (thisObj && discovered.added)
		CAS::LoadSpecialAnimationsForRef(thisObj, resolvedBase);

	if (IsConsoleMode())
		Console_Print("CASLoadAnimGroupOverride \"%s\" found=%u added=%u existing=%u errors=%u", folder, discovered.found, discovered.added, discovered.skipped, discovered.errors);

	return true;
}

bool Cmd_CASLoadSpecialAnimManifest_Execute(COMMAND_ARGS)
{
	*result = 0;

	char manifestPath[CAS::kMaxManifestPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, manifestPath, &actorBase))
		return true;

	CAS::ManifestResult loaded = CAS::LoadSpecialAnimManifest(thisObj, actorBase, manifestPath, true);
	*result = loaded.added;

	if (IsConsoleMode())
		Console_Print("CASLoadSpecialAnimManifest entries=%u targets=%u found=%u added=%u existing=%u unsupported=%u errors=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u", loaded.entries, loaded.targets, loaded.found, loaded.added, loaded.skipped, loaded.unsupported, loaded.errors, loaded.ignoredFields, loaded.conditionFields, loaded.conditionalEntries);

	return true;
}

bool Cmd_CASLoadSpecialAnimManifests_Execute(COMMAND_ARGS)
{
	*result = 0;

	char directory[CAS::kMaxManifestPath] = { 0 };
	if (!ExtractArgs(PASS_EXTRACT_ARGS, directory))
		return true;

	CAS::ManifestResult loaded = CAS::LoadSpecialAnimManifestDirectory(directory, true);
	*result = loaded.added;

	if (IsConsoleMode())
		Console_Print("CASLoadSpecialAnimManifests files=%u entries=%u targets=%u found=%u added=%u existing=%u unsupported=%u errors=%u ignoredFields=%u conditionFields=%u conditionalEntries=%u", loaded.files, loaded.entries, loaded.targets, loaded.found, loaded.added, loaded.skipped, loaded.unsupported, loaded.errors, loaded.ignoredFields, loaded.conditionFields, loaded.conditionalEntries);

	return true;
}

bool Cmd_CASValidateSpecialAnimFolder_Execute(COMMAND_ARGS)
{
	*result = 0;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::NormalizeSlashes(folder);
	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	CAS::DiscoverResult validated = CAS::ValidateSpecialAnimFolder(resolvedBase, folder, CAS::GetValidationSkeletonRoot(thisObj, resolvedBase), IsConsoleMode());
	*result = validated.errors;

	_MESSAGE("CustomAnimSupport CASValidateSpecialAnimFolder folder=\"%s\" found=%u issues=%u", folder, validated.found, validated.errors);

	if (IsConsoleMode())
		Console_Print("CASValidateSpecialAnimFolder \"%s\" found=%u issues=%u", folder, validated.found, validated.errors);

	return true;
}

bool Cmd_CASValidateSpecialAnimManifest_Execute(COMMAND_ARGS)
{
	*result = 0;

	char manifestPath[CAS::kMaxManifestPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, manifestPath, &actorBase))
		return true;

	CAS::ManifestResult validated = CAS::ValidateSpecialAnimManifest(thisObj, actorBase, manifestPath, IsConsoleMode());
	*result = validated.errors + validated.unsupported;

	if (IsConsoleMode())
		Console_Print("CASValidateSpecialAnimManifest files=%u entries=%u targets=%u found=%u unsupported=%u errors=%u issues=%.0f ignoredFields=%u conditionFields=%u conditionalEntries=%u", validated.files, validated.entries, validated.targets, validated.found, validated.unsupported, validated.errors, *result, validated.ignoredFields, validated.conditionFields, validated.conditionalEntries);

	return true;
}

bool Cmd_CASValidateSpecialAnimManifests_Execute(COMMAND_ARGS)
{
	*result = 0;

	char directory[CAS::kMaxManifestPath] = { 0 };
	if (!ExtractArgs(PASS_EXTRACT_ARGS, directory))
		return true;

	CAS::ManifestResult validated = CAS::ValidateSpecialAnimManifestDirectory(thisObj, directory, IsConsoleMode());
	*result = validated.errors + validated.unsupported;

	if (IsConsoleMode())
		Console_Print("CASValidateSpecialAnimManifests files=%u entries=%u targets=%u found=%u unsupported=%u errors=%u issues=%.0f ignoredFields=%u conditionFields=%u conditionalEntries=%u", validated.files, validated.entries, validated.targets, validated.found, validated.unsupported, validated.errors, *result, validated.ignoredFields, validated.conditionFields, validated.conditionalEntries);

	return true;
}

bool Cmd_CASPlayAnimGroup_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 animGroup = 0;
	UInt32 playImmediately = 1;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &animGroup, &playImmediately))
		return true;

	*result = CAS::PlayNativeAnimGroup(thisObj, animGroup, playImmediately != 0) ? 1 : 0;

	_MESSAGE("CustomAnimSupport CASPlayAnimGroup group=%u playImmediately=%u result=%.0f", animGroup, playImmediately, *result);

	if (IsConsoleMode())
		Console_Print("CASPlayAnimGroup group=%u playImmediately=%u >> %.0f", animGroup, playImmediately, *result);

	return true;
}

bool Cmd_CASPlayAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 animGroup = 0;
	UInt32 playImmediately = 1;
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, animPath, &animGroup, &playImmediately, &actorBase))
		return true;

	CAS::NormalizeSlashes(animPath);
	*result = CAS::PlayAnimationPath(thisObj, actorBase, animPath, animGroup, playImmediately != 0, IsConsoleMode()) ? 1 : 0;

	if (IsConsoleMode())
		Console_Print("CASPlayAnimationPath \"%s\" group=%u playImmediately=%u >> %.0f", animPath, animGroup, playImmediately, *result);

	return true;
}

bool Cmd_CASValidateSpecialAnims_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &actorBase))
		return true;

	TESActorBase* resolvedBase = CAS::ResolveActorBase(thisObj, actorBase);
	*result = CAS::ValidateSpecialAnimations(thisObj, resolvedBase, IsConsoleMode());

	if (IsConsoleMode())
		Console_Print("CASValidateSpecialAnims issues=%.0f", *result);

	return true;
}

bool Cmd_CASValidateAnimationTarget_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* targetForm = NULL;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs())
		return true;

	if (eval.NumArgs() > 0)
		targetForm = eval.Arg(0)->GetTESForm();

	*result = CAS::ValidateAnimationTarget(thisObj, targetForm, IsConsoleMode());
	return true;
}

bool Cmd_CASValidateAnimationPipeline_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 reload = 0;
	TESForm* actorBaseForm = NULL;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs())
		return true;

	if (eval.NumArgs() > 0)
		reload = eval.Arg(0)->GetNumber();
	if (eval.NumArgs() > 1)
		actorBaseForm = eval.Arg(1)->GetTESForm();

	TESActorBase* actorBase = NULL;
	if (actorBaseForm)
	{
		actorBase = CAS::GetActorBaseFromForm(actorBaseForm);
		if (!actorBase)
		{
			if (IsConsoleMode())
				Console_Print("CASValidateAnimationPipeline: actor base argument is not an actor base or actor reference");
			*result = 1;
			return true;
		}
	}

	*result = CAS::ValidateAnimationPipeline(thisObj, actorBase, reload != 0, IsConsoleMode());
	return true;
}

bool Cmd_CASDumpAnimationState_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32 includeQueued = 1;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &includeQueued))
		return true;

	*result = CAS::DumpAnimationState(thisObj, includeQueued != 0, IsConsoleMode());
	return true;
}

bool Cmd_CASValidateSaveLoadState_Execute(COMMAND_ARGS)
{
	*result = CAS::ValidateSaveLoadState(thisObj, IsConsoleMode());
	return true;
}

bool Cmd_CASValidateDeferredAnims_Execute(COMMAND_ARGS)
{
	*result = CAS::ValidateDeferredAnimations(thisObj, IsConsoleMode());
	return true;
}

bool Cmd_CASValidateAnimGroupLoaded_Execute(COMMAND_ARGS)
{
	*result = 1;

	UInt32 animGroup = 0;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &animGroup))
		return true;

	*result = CAS::ValidateAnimGroupLoaded(thisObj, animGroup, IsConsoleMode());
	_MESSAGE("CustomAnimSupport CASValidateAnimGroupLoaded group=%u issues=%.0f", animGroup, *result);
	return true;
}

bool Cmd_CASValidateSpecialAnimLoadState_Execute(COMMAND_ARGS)
{
	*result = 1;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::NormalizeSlashes(folder);
	*result = CAS::ValidateSpecialAnimLoadState(thisObj, actorBase, folder, IsConsoleMode());
	_MESSAGE("CustomAnimSupport CASValidateSpecialAnimLoadState folder=\"%s\" issues=%.0f", folder, *result);
	return true;
}

bool Cmd_CASValidateSpecialAnimVariants_Execute(COMMAND_ARGS)
{
	*result = 1;

	char folder[CAS::kMaxSpecialAnimPath] = { 0 };
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, folder, &actorBase))
		return true;

	CAS::NormalizeSlashes(folder);
	*result = CAS::ValidateSpecialAnimVariants(thisObj, actorBase, folder, IsConsoleMode());
	_MESSAGE("CustomAnimSupport CASValidateSpecialAnimVariants folder=\"%s\" issues=%.0f", folder, *result);
	return true;
}

bool Cmd_CASValidateIdleForm_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* idleForm = NULL;
	TESForm* actorBaseForm = NULL;
	if (!ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &idleForm, &actorBaseForm))
		return true;

	TESIdleForm* idle = idleForm ? OBLIVION_CAST(idleForm, TESForm, TESIdleForm) : NULL;
	TESActorBase* actorBase = actorBaseForm ? CAS::GetActorBaseFromForm(actorBaseForm) : CAS::ResolveActorBase(thisObj, NULL);
	*result = CAS::ValidateIdleForm(thisObj, idle, actorBase, IsConsoleMode());

	if (IsConsoleMode())
		Console_Print("CASValidateIdleForm issues=%.0f", *result);

	return true;
}

bool Cmd_CASPlayIdleForm_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* idleForm = NULL;
	UInt32 force = 0;
	UInt32 validate = 1;
	TESForm* actorBaseForm = NULL;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs() || eval.NumArgs() < 1)
		return true;

	idleForm = eval.Arg(0)->GetTESForm();
	if (eval.NumArgs() > 1)
		force = eval.Arg(1)->GetNumber();
	if (eval.NumArgs() > 2)
		validate = eval.Arg(2)->GetNumber();
	if (eval.NumArgs() > 3)
		actorBaseForm = eval.Arg(3)->GetTESForm();

	TESIdleForm* idle = idleForm ? OBLIVION_CAST(idleForm, TESForm, TESIdleForm) : NULL;
	TESActorBase* actorBase = NULL;
	if (actorBaseForm)
	{
		actorBase = CAS::GetActorBaseFromForm(actorBaseForm);
		if (!actorBase)
		{
			if (IsConsoleMode())
				Console_Print("CASPlayIdleForm: actor base argument is not an actor base or actor reference");
			return true;
		}
	}
	else
	{
		actorBase = CAS::ResolveActorBase(thisObj, NULL);
	}

	*result = CAS::QueueIdleFormForRef(thisObj, idle, actorBase, force != 0, validate != 0, IsConsoleMode()) ? 1 : 0;

	if (IsConsoleMode())
		Console_Print("CASPlayIdleForm idle=%08X force=%u validate=%u >> %.0f", idle ? idle->refID : 0, force, validate, *result);

	return true;
}

bool Cmd_PlayAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 firstPerson = 0;
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, animPath, &firstPerson, &actorBase))
		return true;

	TESObjectREFR* targetRef = thisObj;
	if (!targetRef && firstPerson && g_thePlayer)
		targetRef = *g_thePlayer;

	CAS::NormalizeSlashes(animPath);
	*result = CAS::PlayAnimationPathParsedKey(targetRef, actorBase, animPath, IsConsoleMode()) ? 1 : 0;

	if (IsConsoleMode())
		Console_Print("PlayAnimationPath \"%s\" firstPerson=%u >> %.0f", animPath, firstPerson, *result);

	return true;
}

bool Cmd_IsAnimSequencePlaying_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	UInt32 firstPerson = 0;
	TESActorBase* actorBase = NULL;
	if (!ExtractArgs(PASS_EXTRACT_ARGS, animPath, &firstPerson, &actorBase))
		return true;

	TESObjectREFR* targetRef = thisObj;
	if (!targetRef && firstPerson && g_thePlayer)
		targetRef = *g_thePlayer;

	CAS::NormalizeSlashes(animPath);
	*result = CAS::IsAnimationPathPlaying(targetRef, actorBase, animPath, IsConsoleMode()) ? 1 : 0;

	if (IsConsoleMode())
		Console_Print("IsAnimSequencePlaying \"%s\" firstPerson=%u >> %.0f", animPath, firstPerson, *result);

	return true;
}

bool Cmd_SetWeaponAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESForm* weapon = NULL;
	UInt32 firstPerson = 0;
	UInt32 enable = 0;
	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	if (!ExtractArgs(PASS_EXTRACT_ARGS, &weapon, &firstPerson, &enable, animPath))
		return true;

	CAS::NormalizeSlashes(animPath);
	char cleanPath[CAS::kMaxSpecialAnimPath] = { 0 };
	bool folderMode = false;
	if (!CAS::IsWeaponForm(weapon) || !CAS::NormalizeWeaponAnimationPath(animPath, cleanPath, sizeof(cleanPath), &folderMode))
	{
		if (IsConsoleMode())
			Console_Print("SetWeaponAnimationPath failed: weapon=%08X type=%02X path=\"%s\" is not a weapon target or safe native SpecialAnims path",
				weapon ? weapon->refID : 0,
				weapon ? weapon->typeID : 0,
				animPath);
		return true;
	}

	UInt32 changed = 0;
	if (enable)
		changed = CAS::AddWeaponAnimationMapping(weapon, firstPerson, cleanPath) ? 1 : 0;
	else
		changed = CAS::RemoveWeaponAnimationMapping(weapon, cleanPath);

	*result = changed ? 1 : 0;

	CAS::WeaponScopedApplyResult applied;
	TESObjectWEAP* currentWeapon = thisObj ? CAS::GetEquippedWeaponForRef(thisObj) : NULL;
	if (currentWeapon && currentWeapon->refID == weapon->refID)
	{
		Actor* actor = OBLIVION_CAST(thisObj, TESObjectREFR, Actor);
		if (actor)
		{
			CAS::RevertInvalidWeaponScopedAnimationsForActor(actor);
			if (enable || CAS::HasWeaponAnimationMapping(weapon->refID))
				applied = CAS::ApplyWeaponScopedAnimationsForActor(actor, TESAnimGroup::kAnimGroup_Max, true);
		}
	}

	_MESSAGE("CustomAnimSupport SetWeaponAnimationPath weapon=%08X firstPerson=%u enable=%u mode=%s path=\"%s\" changed=%u mappings=%u appliedMappings=%u found=%u added=%u existing=%u invalid=%u errors=%u loaded=%u generation=%u",
		weapon ? weapon->refID : 0,
		firstPerson ? 1 : 0,
		enable ? 1 : 0,
		folderMode ? "folder" : "kf",
		cleanPath,
		changed,
		CAS::CountWeaponAnimationMappings(weapon ? weapon->refID : 0),
		applied.mappings,
		applied.found,
		applied.added,
		applied.existing,
		applied.invalid,
		applied.errors,
		applied.loaded ? 1 : 0,
		CAS::s_weaponAnimGeneration);

	if (IsConsoleMode())
	{
		if (firstPerson)
			Console_Print("SetWeaponAnimationPath: firstPerson is accepted for kNVSE call-shape only; decoded Oblivion KFFZ loading remains actor-base SpecialAnims");
		Console_Print("SetWeaponAnimationPath weapon=%08X firstPerson=%u enable=%u mode=%s path=\"%s\" changed=%.0f mappings=%u appliedFound=%u appliedAdded=%u appliedExisting=%u appliedInvalid=%u appliedErrors=%u loaded=%u",
			weapon ? weapon->refID : 0,
			firstPerson ? 1 : 0,
			enable ? 1 : 0,
			folderMode ? "folder" : "kf",
			cleanPath,
			*result,
			CAS::CountWeaponAnimationMappings(weapon ? weapon->refID : 0),
			applied.found,
			applied.added,
			applied.existing,
			applied.invalid,
			applied.errors,
			applied.loaded ? 1 : 0);
	}

	return true;
}

bool Cmd_SetActorAnimationPath_Execute(COMMAND_ARGS)
{
	*result = 0;

	char animPath[CAS::kMaxSpecialAnimPath] = { 0 };
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs() || eval.NumArgs() < 3)
		return true;

	UInt32 firstPerson = eval.Arg(0)->GetNumber() != 0;
	UInt32 enable = eval.Arg(1)->GetNumber() != 0;
	strncpy_s(animPath, sizeof(animPath), eval.Arg(2)->GetString(), _TRUNCATE);
	UInt32 pollCondition = eval.NumArgs() > 3 ? (eval.Arg(3)->GetNumber() != 0) : 0;
	bool hasConditionArg = eval.NumArgs() > 4;
	UInt32 matchBaseAnimGroup = eval.NumArgs() > 5 ? (eval.Arg(5)->GetNumber() != 0) : 0;

	TESObjectREFR* targetRef = thisObj;
	if (!targetRef && firstPerson && g_thePlayer)
		targetRef = *g_thePlayer;

	CAS::NormalizeSlashes(animPath);
	if (pollCondition || hasConditionArg)
	{
		_MESSAGE("CustomAnimSupport SetActorAnimationPath skipped conditional call firstPerson=%u enable=%u path=\"%s\" pollCondition=%u hasCondition=%u; no verified Oblivion KFFZ/SpecialAnims condition polling map",
			firstPerson,
			enable,
			animPath,
			pollCondition ? 1 : 0,
			hasConditionArg ? 1 : 0);
		if (IsConsoleMode())
			Console_Print("SetActorAnimationPath: condition/pollCondition is unsupported and the path was not registered; use an IDLE form for native conditional idle behavior");
		return true;
	}

	CAS::ActorPathChangeResult changed = CAS::SetActorAnimationPathDetailed(targetRef, NULL, animPath, enable != 0, true);
	*result = changed.changed ? 1 : 0;

	_MESSAGE("CustomAnimSupport SetActorAnimationPath firstPerson=%u enable=%u mode=%s path=\"%s\" changed=%u loaded=%u found=%u added=%u removed=%u persistedRemoved=%u removedLive=%u clearedSlots=%u existing=%u errors=%u",
		firstPerson,
		enable,
		changed.folderMode ? "folder" : "kf",
		animPath,
		changed.changed ? 1 : 0,
		changed.loaded ? 1 : 0,
		changed.found,
		changed.added,
		changed.removed,
		changed.persistedRemoved,
		changed.removedLive,
		changed.clearedSlots,
		changed.skipped,
		changed.errors);

	if (IsConsoleMode())
	{
		if (matchBaseAnimGroup)
			Console_Print("SetActorAnimationPath: matchBaseAnimGroup is accepted for kNVSE call-shape only; Oblivion native KFFZ has no verified base-group override map");
		Console_Print("SetActorAnimationPath firstPerson=%u enable=%u \"%s\" mode=%s changed=%.0f loaded=%u found=%u added=%u removed=%u persistedRemoved=%u removedLive=%u clearedSlots=%u existing=%u errors=%u",
			firstPerson,
			enable,
			animPath,
			changed.folderMode ? "folder" : "kf",
			*result,
			changed.loaded ? 1 : 0,
			changed.found,
			changed.added,
			changed.removed,
			changed.persistedRemoved,
			changed.removedLive,
			changed.clearedSlots,
			changed.skipped,
			changed.errors);
	}

	return true;
}

bool Cmd_SetAnimationPathCondition_Execute(COMMAND_ARGS)
{
	*result = 0;

	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	if (!eval.ExtractArgs() || eval.NumArgs() < 2)
		return true;

	const char* animPath = eval.Arg(0)->GetString();
	_MESSAGE("CustomAnimSupport SetAnimationPathCondition path=\"%s\" unsupported; IDA condition evaluator is not connected to KFFZ/SpecialAnims", animPath ? animPath : "");
	if (IsConsoleMode())
		Console_Print("SetAnimationPathCondition unsupported for \"%s\": Oblivion native KFFZ has no verified condition override map", animPath ? animPath : "");

	return true;
}

bool Cmd_CASGetConditionPollingSupport_Execute(COMMAND_ARGS)
{
	*result = 0;

	_MESSAGE("CustomAnimSupport CASGetConditionPollingSupport kffz=0 nativeSelectionConditions=1 conditionEval=%08X conditionWrapper=%08X loadCondition=%08X conditionLoad=%08X packageChooser=%08X pickIdle=%08X idleCandidate=%08X idleRoot=%08X idleQueue=%08X idleLoader=%08X idleReady=%08X kffzReader=%08X kffzLoader=%08X actorLoadGroup=%08X playEncoded=%08X",
		CAS::kConditionListEvaluate,
		CAS::kConditionListEvaluateWrapper,
		CAS::kConditionListLoadCondition,
		CAS::kConditionLoad,
		CAS::kPackageChooser,
		CAS::kPickIdleCommand,
		CAS::kIdleCandidateSearch,
		CAS::kIdleRootSearch,
		CAS::kActorAnimDataQueueIdle,
		CAS::kQueuedIdleKFLoader,
		CAS::kQueuedIdleReadyHandler,
		CAS::kTESAnimationLoadKFFZChunk,
		CAS::kActorAnimDataLoadKFFZSpecialAnims,
		CAS::kActorLoadAnimGroup,
		CAS::kActorAnimDataPlayEncodedGroup);

	if (IsConsoleMode())
	{
		Console_Print("CASGetConditionPollingSupport: KFFZ/SpecialAnims condition polling support=0");
		Console_Print("CASGetConditionPollingSupport: native conditions are observed at selection time: eval=%08X wrapper=%08X packageChooser=%08X pickIdle=%08X idleCandidate=%08X idleRoot=%08X", CAS::kConditionListEvaluate, CAS::kConditionListEvaluateWrapper, CAS::kPackageChooser, CAS::kPickIdleCommand, CAS::kIdleCandidateSearch, CAS::kIdleRootSearch);
		Console_Print("CASGetConditionPollingSupport: queued IDLE playback checked after selection: queue=%08X loader=%08X ready=%08X; no condition evaluator call is used there", CAS::kActorAnimDataQueueIdle, CAS::kQueuedIdleKFLoader, CAS::kQueuedIdleReadyHandler);
		Console_Print("CASGetConditionPollingSupport: checked KFFZ/playback path kffzReader=%08X loader=%08X actorLoadGroup=%08X playEncoded=%08X", CAS::kTESAnimationLoadKFFZChunk, CAS::kActorAnimDataLoadKFFZSpecialAnims, CAS::kActorLoadAnimGroup, CAS::kActorAnimDataPlayEncodedGroup);
	}

	return true;
}

#endif // RUNTIME

// ================================
// Command definitions
// ================================

static ParamInfo kParams_OptionalActorBase[] =
{
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_Index_OptionalActorBase[] =
{
	{ "index", kParamType_Integer, 0 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_AnimPath_OptionalActorBase[] =
{
	{ "animation path", kParamType_String, 0 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_EnableAnimPath_OptionalActorBase[] =
{
	{ "enable", kParamType_Integer, 0 },
	{ "animation path", kParamType_String, 0 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_kNVSESetWeaponAnimationPath[] =
{
	{ "weapon", kParamType_InventoryObject, 0 },
	{ "first person", kParamType_Integer, 0 },
	{ "enable", kParamType_Integer, 0 },
	{ "animation path", kParamType_String, 0 },
};

static ParamInfo kParams_Folder_OptionalActorBase[] =
{
	{ "folder", kParamType_String, 0 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_OptionalFolder_OptionalActorBase[] =
{
	{ "folder", kParamType_String, 1 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_Manifest_OptionalActorBase[] =
{
	{ "manifest path", kParamType_String, 0 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_OptionalManifestDirectory[] =
{
	{ "directory", kParamType_String, 1 },
};

static ParamInfo kParams_OptionalDirectory_OptionalActorBase[] =
{
	{ "directory", kParamType_String, 1 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_AnimGroup_OptionalPlayNow[] =
{
	{ "anim group", kParamType_AnimationGroup, 0 },
	{ "play immediately", kParamType_Integer, 1 },
};

static ParamInfo kParams_AnimGroup[] =
{
	{ "anim group", kParamType_AnimationGroup, 0 },
};

static ParamInfo kParams_OptionalIncludeQueued[] =
{
	{ "include queued", kParamType_Integer, 1 },
};

static ParamInfo kParams_OptionalForceReload[] =
{
	{ "force reload", kParamType_Integer, 1 },
};

static ParamInfo kParams_OptionalWeaponNameContains[] =
{
	{ "weapon name contains", kParamType_String, 1 },
};

static ParamInfo kParams_WeaponName_EnableAnimPath_OptionalApplyNow[] =
{
	{ "weapon name contains", kParamType_String, 0 },
	{ "enable", kParamType_Integer, 0 },
	{ "animation path", kParamType_String, 0 },
	{ "apply now", kParamType_Integer, 1 },
};

static ParamInfo kParams_Race_EnableAnimPath_OptionalApplyNow[] =
{
	{ "race", kParamType_Race, 0 },
	{ "enable", kParamType_Integer, 0 },
	{ "animation path", kParamType_String, 0 },
	{ "apply now", kParamType_Integer, 1 },
};

static ParamInfo kParams_Global_EnableAnimPath_OptionalApplyNow[] =
{
	{ "global", kParamType_Global, 0 },
	{ "enable", kParamType_Integer, 0 },
	{ "animation path", kParamType_String, 0 },
	{ "apply now", kParamType_Integer, 1 },
};

static ParamInfo kParams_AnimPath_AnimGroup_OptionalPlayNow_ActorBase[] =
{
	{ "animation path", kParamType_String, 0 },
	{ "anim group", kParamType_AnimationGroup, 0 },
	{ "play immediately", kParamType_Integer, 1 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kParams_kNVSEPlayAnimationPath[] =
{
	{ "animation path", kParamType_String, 0 },
	{ "first person", kParamType_Integer, 1 },
	{ "actor base", kParamType_ActorBase, 1 },
};

static ParamInfo kOBSEParams_IdleForm_OptionalActorBase[] =
{
	{ "idle form", kOBSEParamType_Form, 0 },
	{ "actor base", kOBSEParamType_Form, 1 },
};

static ParamInfo kOBSEParams_IdleForm_OptionalForceValidateActorBase[] =
{
	{ "idle form", kOBSEParamType_Form, 0 },
	{ "force", kOBSEParamType_Number, 1 },
	{ "validate", kOBSEParamType_Number, 1 },
	{ "actor base", kOBSEParamType_Form, 1 },
};

static ParamInfo kOBSEParams_OptionalReloadActorBase[] =
{
	{ "reload", kOBSEParamType_Number, 1 },
	{ "actor base", kOBSEParamType_Form, 1 },
};

static ParamInfo kOBSEParams_OptionalTargetForm[] =
{
	{ "target form", kOBSEParamType_Form, 1 },
};

static ParamInfo kOBSEParams_kNVSESetActorAnimationPath[] =
{
	{ "first person", kOBSEParamType_Number, 0 },
	{ "enable", kOBSEParamType_Number, 0 },
	{ "animation path", kOBSEParamType_String, 0 },
	{ "poll condition", kOBSEParamType_Number, 1 },
	{ "condition", kOBSEParamType_NoTypeCheck, 1 },
	{ "match base anim group id", kOBSEParamType_Number, 1 },
};

static ParamInfo kOBSEParams_SetAnimationPathCondition[] =
{
	{ "animation path", kOBSEParamType_String, 0 },
	{ "condition", kOBSEParamType_NoTypeCheck, 0 },
};

DEFINE_COMMAND_PLUGIN(CASGetVersion, "Returns Custom Animation Support version as major*10000 + minor*100 + build.", 0, 0, NULL)
DEFINE_COMMAND_PLUGIN(CASGetSpecialAnimCount, "Returns the number of KFFZ/SpecialAnims entries on an actor base.", 0, SIZEOF_ARRAY(kParams_OptionalActorBase, ParamInfo), kParams_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASGetSpecialAnimPath, "Returns the indexed KFFZ/SpecialAnims path on an actor base.", 0, SIZEOF_ARRAY(kParams_Index_OptionalActorBase, ParamInfo), kParams_Index_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASGetPersistedSpecialAnimCount, "Returns the number of CustomAnimSupport co-save persisted SpecialAnims entries.", 0, SIZEOF_ARRAY(kParams_OptionalActorBase, ParamInfo), kParams_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASClearPersistedSpecialAnims, "Clears CustomAnimSupport co-save persistence for one actor base, or all entries when no target is supplied.", 0, SIZEOF_ARRAY(kParams_OptionalActorBase, ParamInfo), kParams_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASPruneMissingSpecialAnims, "Removes registered KFFZ/SpecialAnims entries whose backing .kf files are no longer on disk.", 0, SIZEOF_ARRAY(kParams_OptionalFolder_OptionalActorBase, ParamInfo), kParams_OptionalFolder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASHasSpecialAnim, "Returns whether an actor base has the exact KFFZ/SpecialAnims entry.", 0, SIZEOF_ARRAY(kParams_AnimPath_OptionalActorBase, ParamInfo), kParams_AnimPath_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASAddSpecialAnim, "Adds an exact KFFZ/SpecialAnims entry to an actor base.", 0, SIZEOF_ARRAY(kParams_AnimPath_OptionalActorBase, ParamInfo), kParams_AnimPath_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASRemoveSpecialAnim, "Removes an exact KFFZ/SpecialAnims entry from an actor base.", 0, SIZEOF_ARRAY(kParams_AnimPath_OptionalActorBase, ParamInfo), kParams_AnimPath_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASLoadSpecialAnims, "Loads current actor-base KFFZ/SpecialAnims entries into the calling actor's ActorAnimData.", 1, SIZEOF_ARRAY(kParams_OptionalActorBase, ParamInfo), kParams_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASApplyAutoSpear, "Legacy alias for CASApplyAutoWeaponAnimations; applies configured weapon-name SpecialAnims rules on the calling actor.", 1, SIZEOF_ARRAY(kParams_OptionalForceReload, ParamInfo), kParams_OptionalForceReload)
DEFINE_COMMAND_PLUGIN(CASApplyAutoWeaponAnimations, "Forces configured weapon-name SpecialAnims rules on the calling actor and reports live diagnostics.", 1, SIZEOF_ARRAY(kParams_OptionalForceReload, ParamInfo), kParams_OptionalForceReload)
DEFINE_COMMAND_PLUGIN(CASSetAutoWeaponAnimationPath, "Records or removes a plugin-managed weapon-name-to-SpecialAnims rule and optionally applies it to the calling actor.", 0, SIZEOF_ARRAY(kParams_WeaponName_EnableAnimPath_OptionalApplyNow, ParamInfo), kParams_WeaponName_EnableAnimPath_OptionalApplyNow)
DEFINE_COMMAND_PLUGIN(CASGetAutoWeaponAnimationRuleCount, "Returns the number of configured weapon-name SpecialAnims rules, optionally filtered by exact name-match text.", 0, SIZEOF_ARRAY(kParams_OptionalWeaponNameContains, ParamInfo), kParams_OptionalWeaponNameContains)
DEFINE_COMMAND_PLUGIN(CASValidateAutoWeaponAnimationRules, "Read-only validation for configured weapon-name SpecialAnims rules that match the calling actor's equipped weapon.", 1, SIZEOF_ARRAY(kParams_OptionalWeaponNameContains, ParamInfo), kParams_OptionalWeaponNameContains)
DEFINE_COMMAND_PLUGIN(CASApplyTargetMappings, "Forces plugin-managed race/global manifest target mappings on the calling actor and reports live diagnostics.", 1, SIZEOF_ARRAY(kParams_OptionalForceReload, ParamInfo), kParams_OptionalForceReload)
DEFINE_COMMAND_PLUGIN(CASSetRaceAnimationPath, "Records or removes a plugin-managed TESRace-to-SpecialAnims mapping and optionally applies it to the calling actor.", 0, SIZEOF_ARRAY(kParams_Race_EnableAnimPath_OptionalApplyNow, ParamInfo), kParams_Race_EnableAnimPath_OptionalApplyNow)
DEFINE_COMMAND_PLUGIN(CASSetGlobalAnimationPath, "Records or removes a plugin-managed TESGlobal-to-SpecialAnims mapping and optionally applies it to the calling actor.", 0, SIZEOF_ARRAY(kParams_Global_EnableAnimPath_OptionalApplyNow, ParamInfo), kParams_Global_EnableAnimPath_OptionalApplyNow)
DEFINE_COMMAND_PLUGIN(CASSetActorAnimationPath, "Adds/removes an actor-base special animation path or native SpecialAnims folder and loads it for the calling actor when enabled.", 0, SIZEOF_ARRAY(kParams_EnableAnimPath_OptionalActorBase, ParamInfo), kParams_EnableAnimPath_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASDiscoverSpecialAnims, "Recursively registers every .kf under the actor model's SpecialAnims folder.", 0, SIZEOF_ARRAY(kParams_OptionalActorBase, ParamInfo), kParams_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASDiscoverSpecialAnimFolder, "Recursively registers every .kf under a SpecialAnims subfolder.", 0, SIZEOF_ARRAY(kParams_Folder_OptionalActorBase, ParamInfo), kParams_Folder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASLoadAnimGroupOverride, "kNVSE-style folder alias that scans a native SpecialAnims subfolder.", 0, SIZEOF_ARRAY(kParams_Folder_OptionalActorBase, ParamInfo), kParams_Folder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASLoadSpecialAnimManifest, "Loads kNVSE-shaped JSON entries into native actor-base SpecialAnims KFFZ entries.", 0, SIZEOF_ARRAY(kParams_Manifest_OptionalActorBase, ParamInfo), kParams_Manifest_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASLoadSpecialAnimManifests, "Loads every JSON manifest from the default CustomAnimSupport directory or a supplied relative directory.", 0, SIZEOF_ARRAY(kParams_OptionalManifestDirectory, ParamInfo), kParams_OptionalManifestDirectory)
DEFINE_COMMAND_PLUGIN(CASValidateSpecialAnimFolder, "Preflights a native SpecialAnims subfolder without mutating actor-base KFFZ entries.", 0, SIZEOF_ARRAY(kParams_Folder_OptionalActorBase, ParamInfo), kParams_Folder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASValidateSpecialAnimManifest, "Preflights one CustomAnimSupport JSON manifest without mutating actor-base KFFZ entries.", 0, SIZEOF_ARRAY(kParams_Manifest_OptionalActorBase, ParamInfo), kParams_Manifest_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASValidateSpecialAnimManifests, "Preflights every JSON manifest from the default CustomAnimSupport directory or a supplied relative directory.", 0, SIZEOF_ARRAY(kParams_OptionalManifestDirectory, ParamInfo), kParams_OptionalManifestDirectory)
DEFINE_COMMAND_PLUGIN(CASValidateKNVSELayout, "Diagnoses kNVSE-style AnimGroupOverride package layout without importing or mutating KFFZ entries.", 0, SIZEOF_ARRAY(kParams_OptionalDirectory_OptionalActorBase, ParamInfo), kParams_OptionalDirectory_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASStageKNVSELayout, "Stages kNVSE-style AnimGroupOverride package files into native SpecialAnims without activating actor-base KFFZ entries.", 0, SIZEOF_ARRAY(kParams_OptionalDirectory_OptionalActorBase, ParamInfo), kParams_OptionalDirectory_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASStageAnimGroupOverride, "Stages one AnimGroupOverride folder into native SpecialAnims without activating actor-base KFFZ entries.", 0, SIZEOF_ARRAY(kParams_Folder_OptionalActorBase, ParamInfo), kParams_Folder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASGetConditionPollingSupport, "Returns 0 because no Oblivion KFFZ/SpecialAnims condition polling path has been verified; logs package/IDLE selection evidence and playback boundaries.", 0, 0, NULL)
DEFINE_COMMAND_PLUGIN(CASDumpAnimationState, "Prints decoded native ActorAnimData slot and idle state for the calling actor.", 1, SIZEOF_ARRAY(kParams_OptionalIncludeQueued, ParamInfo), kParams_OptionalIncludeQueued)
DEFINE_COMMAND_PLUGIN(CASValidateSaveLoadState, "Checks active native animation state against decoded save/load restore prerequisites.", 1, 0, NULL)
DEFINE_COMMAND_PLUGIN(CASValidateDeferredAnims, "Checks native deferred KF installs pending at ActorAnimData +0xB4/+0xB8.", 1, 0, NULL)
DEFINE_COMMAND_PLUGIN(CASValidateAnimGroupLoaded, "Checks whether the native resolved animation group key is loaded in ActorAnimData.", 1, SIZEOF_ARRAY(kParams_AnimGroup, ParamInfo), kParams_AnimGroup)
DEFINE_COMMAND_PLUGIN(CASValidateSpecialAnimLoadState, "Checks registered KFFZ entries against live ActorAnimData map and deferred install state.", 1, SIZEOF_ARRAY(kParams_OptionalFolder_OptionalActorBase, ParamInfo), kParams_OptionalFolder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASValidateSpecialAnimVariants, "Checks duplicate KFFZ group keys against native allow-multiple variant behavior.", 1, SIZEOF_ARRAY(kParams_OptionalFolder_OptionalActorBase, ParamInfo), kParams_OptionalFolder_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(CASPlayAnimGroup, "Plays a native Oblivion animation group on the calling actor.", 1, SIZEOF_ARRAY(kParams_AnimGroup_OptionalPlayNow, ParamInfo), kParams_AnimGroup_OptionalPlayNow)
DEFINE_COMMAND_PLUGIN(CASPlayAnimationPath, "Registers, loads, and plays a matching SpecialAnims sequence path when the supplied native group matches the parsed KF.", 1, SIZEOF_ARRAY(kParams_AnimPath_AnimGroup_OptionalPlayNow_ActorBase, ParamInfo), kParams_AnimPath_AnimGroup_OptionalPlayNow_ActorBase)
DEFINE_COMMAND_PLUGIN(CASValidateSpecialAnims, "Returns the number of unsafe, non-KF, or missing KFFZ/SpecialAnims entries.", 0, SIZEOF_ARRAY(kParams_OptionalActorBase, ParamInfo), kParams_OptionalActorBase)
DEFINE_COMMAND_PLUGIN(PlayAnimationPath, "kNVSE-style shim: registers, loads, and plays the matching decoded BSAnimGroupSequence path.", 0, SIZEOF_ARRAY(kParams_kNVSEPlayAnimationPath, ParamInfo), kParams_kNVSEPlayAnimationPath)
DEFINE_COMMAND_PLUGIN(IsAnimSequencePlaying, "kNVSE-style shim: checks whether the path's parsed native group key and active sequence path match the calling actor.", 0, SIZEOF_ARRAY(kParams_kNVSEPlayAnimationPath, ParamInfo), kParams_kNVSEPlayAnimationPath)
DEFINE_COMMAND_PLUGIN(SetWeaponAnimationPath, "kNVSE-style shim: records a plugin-managed weapon-to-SpecialAnims mapping and applies it through the decoded actor-base KFFZ loader.", 0, SIZEOF_ARRAY(kParams_kNVSESetWeaponAnimationPath, ParamInfo), kParams_kNVSESetWeaponAnimationPath)

CommandInfo kCommandInfo_CASValidateIdleForm =
{
	"CASValidateIdleForm",
	"",
	0,
	"Preflights an IDLE form's decoded ANAM/model-path custom KF behavior.",
	0,
	SIZEOF_ARRAY(kOBSEParams_IdleForm_OptionalActorBase, ParamInfo),
	kOBSEParams_IdleForm_OptionalActorBase,
	HANDLER(Cmd_CASValidateIdleForm_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_SetActorAnimationPath =
{
	"SetActorAnimationPath",
	"",
	0,
	"kNVSE-style shim: accepts .kf paths or native SpecialAnims folders; condition-shaped args are rejected without KFFZ mutation.",
	0,
	SIZEOF_ARRAY(kOBSEParams_kNVSESetActorAnimationPath, ParamInfo),
	kOBSEParams_kNVSESetActorAnimationPath,
	HANDLER(Cmd_SetActorAnimationPath_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_SetAnimationPathCondition =
{
	"SetAnimationPathCondition",
	"",
	0,
	"kNVSE-style unsupported shim for condition attachment; Oblivion KFFZ has no verified condition override map.",
	0,
	SIZEOF_ARRAY(kOBSEParams_SetAnimationPathCondition, ParamInfo),
	kOBSEParams_SetAnimationPathCondition,
	HANDLER(Cmd_SetAnimationPathCondition_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_CASValidateAnimationPipeline =
{
	"CASValidateAnimationPipeline",
	"",
	0,
	"Runs end-to-end diagnostics for actor-base KFFZ, assets, live ActorAnimData, and optional reload.",
	0,
	SIZEOF_ARRAY(kOBSEParams_OptionalReloadActorBase, ParamInfo),
	kOBSEParams_OptionalReloadActorBase,
	HANDLER(Cmd_CASValidateAnimationPipeline_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_CASValidateAnimationTarget =
{
	"CASValidateAnimationTarget",
	"",
	0,
	"Reports whether a form is a supported native actor-base KFFZ/SpecialAnims target.",
	0,
	SIZEOF_ARRAY(kOBSEParams_OptionalTargetForm, ParamInfo),
	kOBSEParams_OptionalTargetForm,
	HANDLER(Cmd_CASValidateAnimationTarget_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_CASGetTargetAnimationMappingCount =
{
	"CASGetTargetAnimationMappingCount",
	"",
	0,
	"Returns loaded plugin-managed race/global target mapping count, optionally filtered to one TESRace or TESGlobal.",
	0,
	SIZEOF_ARRAY(kOBSEParams_OptionalTargetForm, ParamInfo),
	kOBSEParams_OptionalTargetForm,
	HANDLER(Cmd_CASGetTargetAnimationMappingCount_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_CASGetTargetAnimationMatchCount =
{
	"CASGetTargetAnimationMatchCount",
	"",
	0,
	"Returns the number of loaded plugin-managed race/global mappings that currently match the calling actor, optionally filtered to one TESRace or TESGlobal.",
	1,
	SIZEOF_ARRAY(kOBSEParams_OptionalTargetForm, ParamInfo),
	kOBSEParams_OptionalTargetForm,
	HANDLER(Cmd_CASGetTargetAnimationMatchCount_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_CASValidateTargetAnimationMappings =
{
	"CASValidateTargetAnimationMappings",
	"",
	0,
	"Read-only validation for plugin-managed race/global mappings that currently match the calling actor.",
	1,
	SIZEOF_ARRAY(kOBSEParams_OptionalTargetForm, ParamInfo),
	kOBSEParams_OptionalTargetForm,
	HANDLER(Cmd_CASValidateTargetAnimationMappings_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

CommandInfo kCommandInfo_CASPlayIdleForm =
{
	"CASPlayIdleForm",
	"",
	0,
	"Queues a native IDLE form on the calling actor through decoded ActorAnimData_QueueIdle.",
	1,
	SIZEOF_ARRAY(kOBSEParams_IdleForm_OptionalForceValidateActorBase, ParamInfo),
	kOBSEParams_IdleForm_OptionalForceValidateActorBase,
	HANDLER(Cmd_CASPlayIdleForm_Execute),
	Cmd_Expression_Parse,
	NULL,
	0
};

// ================================
// Plugin Compatibility Check
// ================================

const bool IsCompatible(const OBSEInterface* obse)
{
	if (obse->isEditor)
	{
		if (obse->editorVersion < SUPPORTED_RUNTIME_VERSION_CS)
		{
			_MESSAGE("ERROR::IsCompatible: Editor incorrect editor version (got %08X need at least %08X)", obse->editorVersion, SUPPORTED_RUNTIME_VERSION_CS);
			_ERROR("ERROR::IsCompatible: Editor incorrect editor version (got %08X need at least %08X)", obse->editorVersion, SUPPORTED_RUNTIME_VERSION_CS);
			return false;
		}
	}
	else if (!IVersionCheck::IsCompatibleVersion(obse->oblivionVersion, MINIMUM_RUNTIME_VERSION, SUPPORTED_RUNTIME_VERSION, SUPPORTED_RUNTIME_VERSION_STRICT))
	{
		_MESSAGE("ERROR::IsCompatible: Plugin is not compatible with runtime version, disabling");
		_FATALERROR("ERROR::IsCompatible: Plugin is not compatible with runtime version, disabling");
		return false;
	}

	return true;
}

// ================================
// Plugin Export, Query and Load - Start Extern-C
// ================================

extern "C" {

bool OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, PLUGIN_LOG_FILE);

	_MESSAGE(PLUGIN_VERSION_INFO);
	_MESSAGE("Plugin_Query: Querying");

	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = PLUGIN_NAME_LONG;
	info->version = PLUGIN_VERSION_DLL;

	if (!IsCompatible(obse))
	{
		_MESSAGE("ERROR::Plugin_Query: Incompatible | Disabling Plugin");
		_FATALERROR("ERROR::Plugin_Query: Incompatible | Disabling Plugin");
		return false;
	}

	_MESSAGE("Plugin_Query: Queried Successfully");
	return true;
}

bool OBSEPlugin_Load(const OBSEInterface* obse)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, PLUGIN_LOG_FILE);

	_MESSAGE(PLUGIN_VERSION_INFO);
	_MESSAGE("Plugin_Load: Loading");

	if (!IsCompatible(obse))
	{
		_MESSAGE("ERROR::Plugin_Load: Incompatible | Disabling Plugin");
		_FATALERROR("ERROR::Plugin_Load: Incompatible | Disabling Plugin");
		return false;
	}

	g_pluginHandle = obse->GetPluginHandle();

	/***************************************************************************
	 *
	 * This package currently registers commands in the 0x70C0-0x70F3 slice.
	 *
	 **************************************************************************/
	obse->SetOpcodeBase(CAS::kOpcodeBase);

	bool registered = true;
	registered &= obse->RegisterCommand(&kCommandInfo_CASGetVersion);
	registered &= obse->RegisterCommand(&kCommandInfo_CASGetSpecialAnimCount);
	registered &= obse->RegisterCommand(&kCommandInfo_CASGetPersistedSpecialAnimCount);
	registered &= obse->RegisterCommand(&kCommandInfo_CASClearPersistedSpecialAnims);
	registered &= obse->RegisterCommand(&kCommandInfo_CASHasSpecialAnim);
	registered &= obse->RegisterCommand(&kCommandInfo_CASAddSpecialAnim);
	registered &= obse->RegisterCommand(&kCommandInfo_CASRemoveSpecialAnim);
	registered &= obse->RegisterCommand(&kCommandInfo_CASLoadSpecialAnims);
	registered &= obse->RegisterCommand(&kCommandInfo_CASSetActorAnimationPath);
	registered &= obse->RegisterCommand(&kCommandInfo_CASDiscoverSpecialAnims);
	registered &= obse->RegisterCommand(&kCommandInfo_CASDiscoverSpecialAnimFolder);
	registered &= obse->RegisterCommand(&kCommandInfo_CASLoadAnimGroupOverride);
	registered &= obse->RegisterCommand(&kCommandInfo_CASLoadSpecialAnimManifest);
	registered &= obse->RegisterCommand(&kCommandInfo_CASLoadSpecialAnimManifests);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSpecialAnimFolder);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSpecialAnimManifest);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSpecialAnimManifests);
	registered &= obse->RegisterCommand(&kCommandInfo_CASPlayAnimGroup);
	registered &= obse->RegisterCommand(&kCommandInfo_CASPlayAnimationPath);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSpecialAnims);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateIdleForm);
	registered &= obse->RegisterCommand(&kCommandInfo_CASPlayIdleForm);
	registered &= obse->RegisterCommand(&kCommandInfo_PlayAnimationPath);
	registered &= obse->RegisterCommand(&kCommandInfo_SetWeaponAnimationPath);
	registered &= obse->RegisterCommand(&kCommandInfo_SetActorAnimationPath);
	registered &= obse->RegisterTypedCommand(&kCommandInfo_CASGetSpecialAnimPath, kRetnType_String);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateAnimationPipeline);
	registered &= obse->RegisterCommand(&kCommandInfo_CASDumpAnimationState);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSaveLoadState);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateDeferredAnims);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateAnimGroupLoaded);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSpecialAnimLoadState);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateSpecialAnimVariants);
	registered &= obse->RegisterCommand(&kCommandInfo_CASPruneMissingSpecialAnims);
	registered &= obse->RegisterCommand(&kCommandInfo_CASValidateKNVSELayout);
	registered &= obse->RegisterCommand(&kCommandInfo_CASStageKNVSELayout);
	registered &= obse->RegisterCommand(&kCommandInfo_CASStageAnimGroupOverride);
		registered &= obse->RegisterCommand(&kCommandInfo_SetAnimationPathCondition);
		registered &= obse->RegisterCommand(&kCommandInfo_IsAnimSequencePlaying);
		registered &= obse->RegisterCommand(&kCommandInfo_CASApplyAutoSpear);
		registered &= obse->RegisterCommand(&kCommandInfo_CASValidateAnimationTarget);
	registered &= obse->RegisterCommand(&kCommandInfo_CASApplyTargetMappings);
	registered &= obse->RegisterCommand(&kCommandInfo_CASSetRaceAnimationPath);
	registered &= obse->RegisterCommand(&kCommandInfo_CASSetGlobalAnimationPath);
		registered &= obse->RegisterCommand(&kCommandInfo_CASGetTargetAnimationMappingCount);
		registered &= obse->RegisterCommand(&kCommandInfo_CASGetTargetAnimationMatchCount);
		registered &= obse->RegisterCommand(&kCommandInfo_CASValidateTargetAnimationMappings);
		registered &= obse->RegisterCommand(&kCommandInfo_CASGetConditionPollingSupport);
		registered &= obse->RegisterCommand(&kCommandInfo_CASApplyAutoWeaponAnimations);
		registered &= obse->RegisterCommand(&kCommandInfo_CASSetAutoWeaponAnimationPath);
		registered &= obse->RegisterCommand(&kCommandInfo_CASGetAutoWeaponAnimationRuleCount);
		registered &= obse->RegisterCommand(&kCommandInfo_CASValidateAutoWeaponAnimationRules);

	g_stringVar = (OBSEStringVarInterface*)obse->QueryInterface(kInterface_StringVar);
	if (g_stringVar && g_stringVar->version >= OBSEStringVarInterface::kVersion)
		RegisterStringVarInterface(g_stringVar);
	else
		_MESSAGE("Plugin_Load: OBSE string interface unavailable");

	g_messaging = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
	if (g_messaging && g_messaging->version >= OBSEMessagingInterface::kVersion)
	{
		bool listenerRegistered = g_messaging->RegisterListener(g_pluginHandle, "OBSE", CAS_OBSEMessageHandler);
		_MESSAGE("Plugin_Load: OBSE message listener registered = %d", listenerRegistered ? 1 : 0);
	}
	else
	{
		_MESSAGE("Plugin_Load: OBSE messaging interface unavailable");
	}

	if (!obse->isEditor)
	{
		CAS::InstallActorLoadAnimGroupHook();
		CAS::InstallActorAnimDataPlayEncodedGroupHook();

		g_serialization = (OBSESerializationInterface*)obse->QueryInterface(kInterface_Serialization);
		if (g_serialization && g_serialization->version >= OBSESerializationInterface::kVersion)
		{
			g_serialization->SetSaveCallback(g_pluginHandle, CAS_SaveCallback);
			g_serialization->SetPreloadCallback(g_pluginHandle, CAS_PreloadCallback);
			g_serialization->SetLoadCallback(g_pluginHandle, CAS_LoadCallback);
			g_serialization->SetNewGameCallback(g_pluginHandle, CAS_NewGameCallback);
			_MESSAGE("Plugin_Load: OBSE serialization callbacks registered");
		}
		else
		{
			_MESSAGE("Plugin_Load: OBSE serialization interface unavailable");
		}
	}

	_MESSAGE("Plugin_Load: Functions Registered = %d", registered ? 1 : 0);
	return registered;
}

} // extern "C"
