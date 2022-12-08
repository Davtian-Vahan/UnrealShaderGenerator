// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderScripts.h"
#include <Materials/MaterialExpressionAppendVector.h>
#include <Materials/MaterialExpressionTextureCoordinate.h>
#include <Materials/MaterialExpressionScalarParameter.h>
#include <Factories/MaterialFactoryNew.h>
#include <Materials/MaterialFunction.h>
#include <Materials/MaterialFunctionInstance.h>
#include <Materials/MaterialExpressionVectorParameter.h>
#include <Materials/MaterialExpressionConstant.h>
#include <Materials/MaterialExpressionGenericConstant.h>
#include <Materials/MaterialExpressionAdd.h>
#include <Materials/MaterialExpressionSubtract.h>
#include <Materials/MaterialExpressionMultiply.h>
#include <Materials/MaterialExpressionDivide.h>
#include <Materials/MaterialExpressionStaticBoolParameter.h>
#include <Materials/MaterialExpressionStaticSwitchParameter.h>
#include <Materials/MaterialExpressionTime.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <FileHelpers.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <UObject/SavePackage.h>
#include "MaterialGraph/MaterialGraph.h"
#include <Materials/Material.h>
#include <Runtime/Launch/Resources/Version.h>


// Simplify allocation types
#define T_SC_PARAM UMaterialExpressionScalarParameter
#define T_VC_PARAM UMaterialExpressionVectorParameter
#define T_CALLABLE_NODE	 UMaterialExpressionMaterialFunctionCall
#define T_SUBTRACT_NODE	 UMaterialExpressionSubtract
#define T_MULTIPLY_NODE	 UMaterialExpressionMultiply
#define T_ADD_NODE	     UMaterialExpressionAdd
#define T_DIVIDE_NODE	 UMaterialExpressionDivide
#define T_CONST_1D_NODE	 UMaterialExpressionConstant
#define T_SWITCH_NODE    UMaterialExpressionStaticSwitchParameter
#define T_TIME_NODE      UMaterialExpressionTime 

/*
	Simplify allocation calls. NOTE: Macros assume your material variable name is _GenMaterial
	ALLOC_EXPR: Given type and variable name, dynamically creates the desired class instance.
	ALLOC_EXPR_P: A fully automated macro that creates, names, groups, and adds the
				  parameter expression to the material graph.
*/


#if ENGINE_MINOR_VERSION < 1 && WITH_EDITOR

#define ADD_EXPR_PARAM(_VarName) _GenMaterial->Expressions.Add(_VarName); \

#endif

#if ENGINE_MINOR_VERSION >= 1 && WITH_EDITOR

#define ADD_EXPR_PARAM(_VarName) _GenMaterial->AddExpressionParameter(_VarName, _GenMaterial->EditorParameters);

#endif

#define ALLOC_EXPR(_Type, _VarName)  _Type * _VarName = NewObject<_Type>(_GenMaterial);\
									 ADD_EXPR_PARAM(_VarName);\

#define ALLOC_EXPR_P(_Type, _VarName, _ParamName, _Index)  \
															_Type * _VarName = NewObject<_Type>(_GenMaterial);\
															_VarName->ParameterName = IndexedParamName(_ParamName, C_Index);\
															_VarName->Group = IndexedGroupName(C_Index);\
														    ADD_EXPR_PARAM(_VarName); \

#define MAKE_SC_PARAM(_VarName, _ParamName, _Index) ALLOC_EXPR_P(T_SC_PARAM, _VarName, _ParamName, _Index)
#define MAKE_VR_PARAM(_VarName, _ParamName, _Index) ALLOC_EXPR_P(T_VC_PARAM, _VarName, _ParamName, _Index)
#define MAKE_SWITCH(_VarName, _ParamName, _Index)   ALLOC_EXPR_P(T_SWITCH_NODE, _VarName, _ParamName, _Index)
#define MAKE_CALLABLE(_Callable)					ALLOC_EXPR(T_CALLABLE_NODE, _Callable)
#define MAKE_ADD(_VarName)						    ALLOC_EXPR(T_ADD_NODE, _VarName)
#define MAKE_SUBTRACT(_VarName)						ALLOC_EXPR(T_SUBTRACT_NODE, _VarName)
#define MAKE_MULTIPLY(_VarName)					    ALLOC_EXPR(T_MULTIPLY_NODE, _VarName)
#define MAKE_DIVIDE(_VarName)					    ALLOC_EXPR(T_DIVIDE_NODE, _VarName)
#define MAKE_CONST_1D(_VarName)					    ALLOC_EXPR(T_CONST_1D_NODE, _VarName)
#define MAKE_TIME(_VarName)					        ALLOC_EXPR(T_TIME_NODE, _VarName)

void UShaderScripts::GenerateCirclesShader(const int32 Circle_N, const FString MaterialName)
{
	// Create and setup package
	FString MaterialBaseName = MaterialName;
	FString PackageName = "/Game/ShaderUtil/ShaderGen/";
	PackageName += MaterialBaseName;
	UPackage* Package = CreatePackage(nullptr, *PackageName);

	// Create and setup generated material asset 
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UObject* RawMaterial = MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package,
		*MaterialBaseName, RF_Standalone | RF_Public,
		nullptr, GWarn);
	// NOTE: Var name is macro specific, exercise caution when changing it.
	UMaterial* _GenMaterial = Cast<UMaterial>(RawMaterial);
	FAssetRegistryModule::AssetCreated(_GenMaterial);
	Package->FullyLoad();
	Package->SetDirtyFlag(true);

	//// Automated material code below ///
	// Load necessary material functions
	FStringAssetReference MF_RadialSegmentsPath = "/Game/ShaderUtil/MF_UI_RadialSegments";
	FStringAssetReference MF_SegmentedRotationPath = "/Game/ShaderUtil/MF_UI_SegmentedRotation";
	FStringAssetReference MF_RadialGradientExpPath = "/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential";

	// Try to load material functions from asset paths
	UMaterialFunction* MF_SegmentedRotation = Cast<UMaterialFunction>(MF_SegmentedRotationPath.TryLoad());
	UMaterialFunction* MF_RadialSegments = Cast<UMaterialFunction>(MF_RadialSegmentsPath.TryLoad());
	UMaterialFunction* MF_RadialGradientExp = Cast<UMaterialFunction>(MF_RadialGradientExpPath.TryLoad());

	// Exit if MF loading failed
	if (!MF_RadialSegments || !MF_SegmentedRotation || !MF_RadialGradientExp)
	{
		UE_LOG(LogTemp, Error, TEXT("UShaderScripts::GenerateCirclesShader:"
			"Failed to load one or more material functions."));
		return;
	}

	// Cache all expression nodes that are added to final color
	TArray<UMaterialExpression*> FinalExpressions;
	// A sample radius offset for default generation
	float default_radius_offset = 0.1f;

	/* Main loop that creates all the inner nodes in the material */
	for (int32 C_Index = 0; C_Index < Circle_N; ++C_Index)
	{
		// Create and setup scalar parameters
		MAKE_SC_PARAM(Radius, "Radius", C_Index);
		MAKE_SC_PARAM(Thickness, "Thickness", C_Index);
		MAKE_SC_PARAM(Density, "Density", C_Index);
		MAKE_SC_PARAM(Opacity, "Opacity", C_Index);

		// Segment related parameters
		MAKE_SWITCH(bIsSegmented, "bIsSegmented", C_Index);
		MAKE_SC_PARAM(SegmentCount, "SegmentCount", C_Index);
		MAKE_SC_PARAM(SegmentSpacing, "SegmentSpacing", C_Index);
		MAKE_VR_PARAM(CenterOffset, "CenterOffset", C_Index);

		// Rotation related parameters
		MAKE_SWITCH(bAutoRotates, "bAutoRotates", C_Index);
		MAKE_SC_PARAM(InverseRotationSpeed, "InverseRotationSpeed", C_Index);
		MAKE_SC_PARAM(RotationDirection, "RotationDirection", C_Index);

		// Timer
		MAKE_TIME(SineTime);

		// Allocate function call nodes
		MAKE_CALLABLE(RadialGradientCall1);
		MAKE_CALLABLE(RadialGradientCall2);
		MAKE_CALLABLE(RadialSegmentsCall);

		// Allocate math operation nodes
		MAKE_SUBTRACT(RadialsSubtract);
		MAKE_DIVIDE(ThickDivide);
		MAKE_SUBTRACT(ThickSubtract);
		MAKE_CONST_1D(ThickCoeff);
		//MAKE_CONST_1D(ZeroExpress);
		MAKE_CONST_1D(OneExpress);
		MAKE_MULTIPLY(OpacityMultiply);
		MAKE_MULTIPLY(FinalMultiply);
		//
		MAKE_DIVIDE(RotSpeedDivide);
		MAKE_MULTIPLY(RotDirMultiply);

		// Set default values for some of the scalar params
		Radius->DefaultValue = 0.1f + C_Index / 10.f;
		Thickness->DefaultValue = 0.1;
		Density->DefaultValue = 100.f;
		Opacity->DefaultValue = 1.f;
		ThickCoeff->R = 1000.f;
		OneExpress->R = 1.f;

		bIsSegmented->DefaultValue = false;
		bAutoRotates->DefaultValue = false;
		SegmentCount->DefaultValue = 1.f;
		SegmentSpacing->DefaultValue = 0.5f;
		InverseRotationSpeed->DefaultValue = 15.f;
		RotationDirection->DefaultValue = 1.f;
		CenterOffset->DefaultValue = FLinearColor(-0.5f, -0.5f, 0.f, 1.f);

		// Setup gradient function calls
		RadialGradientCall1->SetMaterialFunction(MF_RadialGradientExp);
		RadialGradientCall2->SetMaterialFunction(MF_RadialGradientExp);
		RadialSegmentsCall->SetMaterialFunction(MF_RadialSegments);

		// Connect gradient function call nodes
		Radius->ConnectExpression(RadialGradientCall1->GetInput(2), 0);
		Density->ConnectExpression(RadialGradientCall1->GetInput(3), 0);
		ThickSubtract->ConnectExpression(RadialGradientCall2->GetInput(2), 0);
		Density->ConnectExpression(RadialGradientCall2->GetInput(3), 0);

		// Connect nodes for smaller circle
		Radius->ConnectExpression(ThickSubtract->GetInput(0), 0);
		Thickness->ConnectExpression(ThickDivide->GetInput(0), 0);
		ThickDivide->ConnectExpression(ThickSubtract->GetInput(1), 0);
		ThickCoeff->ConnectExpression(ThickDivide->GetInput(1), 0);

		// Connect the radial call outputs to a subtract node IE: R_Call1 - R_Call2
		RadialGradientCall1->ConnectExpression(RadialsSubtract->GetInput(0), 0);
		RadialGradientCall2->ConnectExpression(RadialsSubtract->GetInput(1), 0);

		// Cost: Opacity_Node * Subtract_Radials
		RadialsSubtract->ConnectExpression(OpacityMultiply->GetInput(0), 0);
		Opacity->ConnectExpression(OpacityMultiply->GetInput(1), 0);

		// Make MF_RadialSegments Repeat (1) input
		UMaterialExpression* RepeatInput = SegmentCount;
		RepeatInput->ConnectExpression(RadialSegmentsCall->GetInput(1), 0);

		// Make MF_RadialSegments CenterOffset (2) input
		UMaterialExpression* CenterOffsetInput = CenterOffset;
		CenterOffsetInput->ConnectExpression(RadialSegmentsCall->GetInput(2), 0);

		// Make MF_RadialSegments Rotation input
		SineTime->ConnectExpression(RotSpeedDivide->GetInput(0), 0);
		InverseRotationSpeed->ConnectExpression(RotSpeedDivide->GetInput(1), 0);
		RotSpeedDivide->ConnectExpression(RotDirMultiply->GetInput(0), 0);
		RotationDirection->ConnectExpression(RotDirMultiply->GetInput(1), 0);
		RotDirMultiply->ConnectExpression(bAutoRotates->GetInput(0), 0);
		OneExpress->ConnectExpression(bAutoRotates->GetInput(1), 0);
		UMaterialExpression* RotationInput = bAutoRotates;
		RotationInput->ConnectExpression(RadialSegmentsCall->GetInput(3), 0);

		// Make MF_RadialSegments SegmentSpacing input
		UMaterialExpression* SegmentSpacingInput = SegmentSpacing;
		SegmentSpacingInput->ConnectExpression(RadialSegmentsCall->GetInput(5), 0);

		// Connect bAutoRotates to bIsSegmented switch
		// if false // if true 
		RadialSegmentsCall->ConnectExpression(bIsSegmented->GetInput(0), 0);
		OneExpress->ConnectExpression(bIsSegmented->GetInput(1), 0);

		// Finally multiply auto rotates with radial segments coeff
		OpacityMultiply->ConnectExpression(FinalMultiply->GetInput(0), 0);
		bIsSegmented->ConnectExpression(FinalMultiply->GetInput(1), 0);

		FinalExpressions.Add(FinalMultiply);
	}

	TArray<UMaterialExpression*> AddNodes;
	for (int32 Iter = 0; Iter < Circle_N && Circle_N > 1; ++Iter)
	{
		MAKE_ADD(AddNode);

		if (AddNodes.IsEmpty() && FinalExpressions.Num() > 1)
		{
			UMaterialExpression* A = FinalExpressions[Iter];
			UMaterialExpression* B = FinalExpressions[Iter + 1];

			A->ConnectExpression(AddNode->GetInput(0), 0);
			B->ConnectExpression(AddNode->GetInput(1), 0);

			AddNodes.Add(AddNode);
			continue;
		}

		UMaterialExpression* A = FinalExpressions[Iter];
		UMaterialExpression* B = (AddNodes.Num() > 1 ? AddNodes.Last() : AddNodes[0]);

		AddNodes.Last()->ConnectExpression(AddNode->GetInput(0), 0);
		A->ConnectExpression(AddNode->GetInput(1), 0);
		AddNodes.Add(AddNode);
	}

	// Emissive color is "final color" for UI result output. Not really sure why??
	UMaterialExpression* FinalNode_ToOutput = (!AddNodes.IsEmpty() ? AddNodes.Last() : FinalExpressions[0]);

	FinalNode_ToOutput->ConnectExpression(_GenMaterial->GetExpressionInputForProperty(MP_EmissiveColor), 0);

	// Final change notifies and state sets
	_GenMaterial->MaterialDomain = EMaterialDomain::MD_UI;
	_GenMaterial->PreEditChange(nullptr);
	_GenMaterial->PostEditChange();

	//// Create a material instance based on the generated material
	//UMaterialInstanceDynamic* _GenMaterialInstance = UMaterialInstanceDynamic::Create(_GenMaterial, this);

	//// Package details
	//FString MI_PackageName = PackageName + "_Instance";
	//UPackage* MI_Package = CreatePackage(nullptr, *MI_PackageName);
	//MI_Package->FullyLoad();

	//_GenMaterialInstance->UpdateCachedData();
	//MI_Package->MarkPackageDirty();
	//FAssetRegistryModule::AssetCreated(_GenMaterialInstance);
	//FString PackageFileName = FPackageName::LongPackageNameToFilename(MI_PackageName, FPackageName::GetAssetPackageExtension());
	//// Save the asset
	//UPackage::SavePackage(MI_Package, _GenMaterialInstance, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
}

FName UShaderScripts::IndexedParamName(const FName ParamName, const int32 Index)
{
	return FName(*FString::Printf(TEXT("Circle_%d_%s"), Index, *ParamName.ToString()));
}

FName UShaderScripts::IndexedGroupName(const int32 Index)
{
	return FName(*FString::Printf(TEXT("Group_Circle_%d"), Index));
}