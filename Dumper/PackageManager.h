#pragma once

#include "Enums.h"

#include "UnrealObjects.h"
#include "DependencyManager.h"
#include "HashStringTable.h"


namespace PackageManagerUtils
{
	std::unordered_set<int32> GetDependencies(UEStruct Struct, int32 StructIndex);
}

class PackageInfoHandle;
class PackageManager;

struct RequirementInfo
{
	int32 PackageIdx;
	bool bShouldIncludeStructs;
	bool bShouldIncludeClasses;
};

struct VisitedNodeInformation
{
	int32 PackageIdx;
	mutable uint64 StructsIterationHitCount = 0x0;
	mutable uint64 ClassesIterationHitCount = 0x0;
};

using DependencyListType = std::unordered_map<int32, RequirementInfo>;


struct DependencyInfo
{
	/* Counter incremented every time this element is hit during iteration, **if** the counter is less than the CurrentIterationIndex */
	mutable uint64 StructsIterationHitCount = 0x0;
	mutable uint64 ClassesIterationHitCount = 0x0;

	/* List of packages required by "ThisPackage_structs.h" */
	DependencyListType StructsDependencies;

	/* List of packages required by "ThisPackage_classes.h" */
	DependencyListType ClassesDependencies;

	/* List of packages required by "ThisPackage_parameters.h" */
	DependencyListType ParametersDependencies;
};

using VisitedNodeContainerType = std::vector<VisitedNodeInformation>;


struct PackageInfo
{
private:
	friend class PackageInfoHandle;
	friend class PackageManager;
	friend class PackageManagerTest;

private:
	/* Name of this Package*/
	HashStringTableIndex Name = HashStringTableIndex::FromInt(-1);

	/* Count to track how many packages with this name already exists at the point this PackageInfos' initialization */
	uint64 CollisionCount = 0x0;

	bool bHasParams;

	DependencyManager StructsSorted;
	DependencyManager ClassesSorted;

	std::vector<int32> Functions;
	std::vector<int32> Enums;

	DependencyInfo PackageDependencies;
};

class PackageInfoHandle
{
private:
	const PackageInfo* Info;

public:
	PackageInfoHandle() = default;
	PackageInfoHandle(std::nullptr_t Nullptr);
	PackageInfoHandle(const PackageInfo& InInfo);

public:
	inline bool IsValidHandle() { return Info != nullptr; }

public:
	/* Returns a pair of name and CollisionCount */
	std::string GetName() const;
	const StringEntry& GetNameEntry() const;
	std::pair<std::string, uint8> GetNameCollisionPair() const;

	bool HasClasses() const;
	bool HasStructs() const;
	bool HasFunctions() const;
	bool HasParameterStructs() const;
	bool HasEnums() const;

	bool IsEmpty() const;

	const DependencyManager& GetSortedStructs() const;
	const DependencyManager& GetSortedClasses() const;

	const std::vector<int32>& GetFunctions() const;
	const std::vector<int32>& GetEnums() const;

	const DependencyInfo& GetPackageDependencies() const;
};

using PackageManagerOverrideMapType = std::unordered_map<int32 /* PackageIndex */, PackageInfo>;;

struct PackageInfoIterator
{
private:
	friend class PackageManager;

private:
	using MapType = PackageManagerOverrideMapType;
	using IteratorType = PackageManagerOverrideMapType::const_iterator;

private:
	const MapType& PackageInfos;
	uint8 CurrentIterationHitCount;
	IteratorType It;

private:
	explicit PackageInfoIterator(const MapType& Infos, uint8 IterationHitCount, IteratorType ItPos)
		: PackageInfos(Infos), CurrentIterationHitCount(IterationHitCount), It(ItPos)
	{
	}

	explicit PackageInfoIterator(const MapType& Infos, uint8 IterationHitCount)
		: PackageInfos(Infos), CurrentIterationHitCount(IterationHitCount), It(Infos.cbegin())
	{
	}

public:
	inline PackageInfoIterator& operator++() { ++It; return *this; }
	inline PackageInfoHandle operator*() const { return { PackageInfoHandle(It->second) }; }

	inline bool operator==(const PackageInfoIterator& Other) const { return It == Other.It; }
	inline bool operator!=(const PackageInfoIterator& Other) const { return It != Other.It; }

public:
	PackageInfoIterator begin() const { return PackageInfoIterator(PackageInfos, CurrentIterationHitCount, PackageInfos.cbegin()); }
	PackageInfoIterator end() const   { return PackageInfoIterator(PackageInfos, CurrentIterationHitCount, PackageInfos.cend());   }
};


struct PackageManagerIterationParams
{
	int32 PrevPackage;
	int32 RequiredPackge;

	bool bWasPrevNodeStructs;
	bool bRequiresClasses;
	bool bRequiresStructs;

	VisitedNodeContainerType& VisitedNodes;
};

class PackageManager
{
private:
	friend class PackageInfoHandle;
	friend class PackageManagerTest;

public:
	using OverrideMaptType = PackageManagerOverrideMapType;

	using IteratePackagesCallbackType = std::function<void(const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct)>;
	using FindCycleCallbackType = std::function<void(const PackageManagerIterationParams& OldParams, const PackageManagerIterationParams& NewParams, bool bIsStruct)>;

private:
	/* NameTable containing names of all Packages as well as information on name-collisions */
	static inline HashStringTable UniquePackageNameTable;

	/* Map containing infos on all Packages. Implemented due to information missing in the Unreal's reflection system (PackageSize). */
	static inline OverrideMaptType PackageInfos;

	/* Count to track how often the PackageInfos was iterated. Allows for up to 2^64 iterations of this list. */
	static inline uint64 CurrentIterationHitCount = 0x0;

	static inline bool bIsInitialized = false;

private:
	static void InitDependencies();
	static void InitNames();

public:
	static void Init();

private:
	static inline const StringEntry& GetPackageName(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name];
	}

private:
	template<bool bCheckForCycle = false>
	static void IterateDependenciesImplementation(const PackageManagerIterationParams& Params, const IteratePackagesCallbackType& CallbackForEachPackage, const FindCycleCallbackType& OnFoundCycle);

public:
	static void IterateDependencies(const IteratePackagesCallbackType& CallbackForEachPackage);
	static void FindCycle(const FindCycleCallbackType& OnFoundCycle);

public:
	static inline const OverrideMaptType& GetPackageInfos()
	{
		return PackageInfos;
	}

	static inline std::string GetName(int32 PackageIndex)
	{
		 return GetInfo(PackageIndex).GetName();
	}

	static inline bool IsPackageNameUnique(const PackageInfo& Info)
	{
		return UniquePackageNameTable[Info.Name].IsUnique();
	}

	static inline PackageInfoHandle GetInfo(int32 PackageIndex)
	{
		return PackageInfos.at(PackageIndex);
	}

	static inline PackageInfoHandle GetInfo(const UEObject Package)
	{
		if (!Package)
			return {};

		return PackageInfos.at(Package.GetIndex());
	}

	static inline PackageInfoIterator IterateOverPackageInfos()
	{
		CurrentIterationHitCount++;

		return PackageInfoIterator(PackageInfos, CurrentIterationHitCount);
	}
};
