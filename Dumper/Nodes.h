#pragma once
#include "ObjectArray.h"

struct MemberNode
{
	std::string Type;
	std::string Name;

	int32 Offset;
	int32 Size;
	int32 ArrayDim;
	EObjectFlags ObjFlags;
	EPropertyFlags PropertyFlags;
	EClassCastFlags CastFlags;
	bool bIsBitField;
	uint8 BitFieldIndex;
	uint8 BitMask;

	/* Prefer using other members instead of directly accessing UnrealProperty */
	UEProperty* UnrealProperty;
};

struct ParamNode : public MemberNode
{
	bool bIsOutParam;
	bool bIsRefParam;
	bool bIsRetParam;
};

struct UniqueNameBase
{
	/* "unedited" name --> eg. "PlayerController", "Vector", "ENetRole" */
	std::string RawName;

	/* prefixed name --> eg. "Some+Class" -> "ASome_Class" */
	std::string PrefixedName;

	/* unique name --> empty if name is unique, else package name  */
	std::string UniqueNamePrefix;

	/* full name --> "Class Engine.PlayerController" */
	std::string FullName;
};

struct EnumNode : public UniqueNameBase
{
	/* Prefer using other members instead of directly accessing UnrealEnum */
	UEEnum* UnrealEnum;

	std::vector<std::pair<std::string, int32>> NameValuePairs;
	int32 UnderlayingTypeSize;
};

struct StructNodeBase : public UniqueNameBase
{
	int32 Size;
	int32 SuperSize;
};

struct FunctionNode : public StructNodeBase
{
	/* [nullptr for PredefinedFunctions] Prefer using other members instead of directly accessing UnrealFunction */
	UEFunction* UnrealFunction;

	std::vector<ParamNode> Params;
	class ClassNode* OuterClass;
	EFunctionFlags FuncFlags;

	int RetValueIndex = -1;
	bool bIsNative = false;
};

struct StructNode : public StructNodeBase
{
	/* Prefer using other members instead of directly accessing UnrealStruct */
	UEStruct* UnrealStruct;

	/* Field for all Properties or PredefinedMembers */
	std::vector<MemberNode> Members;

	/* Field for all UFunctions or PredefinedFunctions */
	std::vector<FunctionNode> Functions;
};