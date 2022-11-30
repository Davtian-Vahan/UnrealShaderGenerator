// Fill out your copyright notice in the Description page of Project Settings.


#include "ShaderScripts.h"
#include <Materials/MaterialExpressionAppendVector.h>
#include <Materials/MaterialExpressionTextureCoordinate.h>
#include <Materials/MaterialExpressionScalarParameter.h>
#include <Factories/MaterialFactoryNew.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <Materials/MaterialExpressionMultiply.h>
#include <Materials/MaterialFunction.h>
#include <Materials/MaterialFunctionInstance.h>
#include <Materials/MaterialExpressionSubtract.h>
#include <Materials/MaterialExpressionVectorParameter.h>
#include <FileHelpers.h>
#include <Materials/MaterialExpressionDivide.h>
#include <Materials/MaterialExpressionConstant.h>
#include <Materials/MaterialExpressionGenericConstant.h>
#include <Materials/MaterialExpressionAdd.h>

// Simplify type code
#define TYPE_SC_PARAM UMaterialExpressionScalarParameter
#define TYPE_VC_PARAM UMaterialExpressionVectorParameter
#define TYPE_CALLABLE UMaterialExpressionMaterialFunctionCall
#define TYPE_SUBTRACT UMaterialExpressionSubtract
#define TYPE_MULTIPLY UMaterialExpressionMultiply
#define TYPE_ADD	  UMaterialExpressionAdd
#define TYPE_DIVIDE   UMaterialExpressionDivide
#define TYPE_CONST_1D UMaterialExpressionConstant

// Simplify allocation calls
#define ALLOC_EXPR(_Type, _VarName, _Owner)  _Type * _VarName = \
											 NewObject<_Type>(_Owner)
#define MAKE_SC_PARAM(_VarName, _Owner)  ALLOC_EXPR(TYPE_SC_PARAM, _VarName, _Owner)
#define MAKE_VR_PARAM(_VarName, _Owner)  ALLOC_EXPR(TYPE_VC_PARAM, _VarName, _Owner)
#define MAKE_SUBTRACT(_VarName, _Owner)  ALLOC_EXPR(TYPE_SUBTRACT, _VarName, _Owner)
#define MAKE_MULTIPLY(_VarName, _Owner)  ALLOC_EXPR(TYPE_MULTIPLY, _VarName, _Owner)
#define MAKE_ADD(_VarName, _Owner)       ALLOC_EXPR(TYPE_ADD, _VarName, _Owner)
#define MAKE_DIVIDE(_VarName, _Owner)    ALLOC_EXPR(TYPE_DIVIDE, _VarName, _Owner)
#define MAKE_CONST(_VarName, _Owner)     ALLOC_EXPR(TYPE_CONST_1D, _VarName, _Owner)
#define MAKE_CALLABLE(_Callable, _Owner) ALLOC_EXPR(TYPE_CALLABLE, _Callable, _Owner)

void UShaderScripts::GenerateCirclesShader(const int32 Circle_N)
{
	// Try loading with this
	// LoadObject<UMaterialInterface>(NULL, TEXT("MaterialInstanceConstant'/Game/Materials/Galaxy/Class/M/3'"), NULL, LOAD_None, NULL);

	// Setup package
	FString MaterialBaseName = "M_Material";
	FString PackageName = "/Game/ShaderUtil/ShaderGen/";
	PackageName += MaterialBaseName;
	UPackage* Package = CreatePackage(NULL, *PackageName);

	// Create and setup a material asset 
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* UnrealMaterial = (UMaterial*)MaterialFactory->FactoryCreateNew(UMaterial::StaticClass(), Package, *MaterialBaseName, RF_Standalone | RF_Public, NULL, GWarn);
	FAssetRegistryModule::AssetCreated(UnrealMaterial);
	Package->FullyLoad();
	Package->SetDirtyFlag(true);

	///////////////////////////////////////////
	//// Automated material code goes here ///
	// 
	// Load necessary material functions
	FStringAssetReference MF_RadialSegmentsPath = "/Game/ShaderUtil/MF_UI_RadialSegments";
	FStringAssetReference MF_SegmentedRotationPath = "/Game/ShaderUtil/MF_UI_SegmentedRotation";
	FStringAssetReference MF_RadialGradientExpPath = "/Engine/Functions/Engine_MaterialFunctions01/Gradient/RadialGradientExponential";

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


	TArray<UMaterialExpression*> FinalExpressions;
	float default_radius_offset = 0.1f;
	// Loop CircleCount times and create that many node sequences
	for (int32 C_Index = 0; C_Index < Circle_N; ++C_Index)
	{
		// Create and setup scalar parameters
		MAKE_SC_PARAM(Radius, UnrealMaterial);
		MAKE_SC_PARAM(Thickness, UnrealMaterial);
		MAKE_SC_PARAM(Density, UnrealMaterial);
		MAKE_SC_PARAM(Opacity, UnrealMaterial);
		// Segment related parameters
		MAKE_SC_PARAM(bIsSegmented, UnrealMaterial);
		MAKE_SC_PARAM(SegmentCount, UnrealMaterial);
		MAKE_SC_PARAM(SegmentSpacing, UnrealMaterial);
		MAKE_VR_PARAM(CenterOffset, UnrealMaterial);
		// Rotation related parameters
		MAKE_SC_PARAM(bAutoRotates, UnrealMaterial);
		MAKE_SC_PARAM(InverseRotationSpeed, UnrealMaterial);
		MAKE_SC_PARAM(RotationDirection, UnrealMaterial);

		// Create function nodes
		MAKE_CALLABLE(RadialGradientCall1, UnrealMaterial);
		MAKE_CALLABLE(RadialGradientCall2, UnrealMaterial);
		MAKE_CALLABLE(RadialSegmentsCall, UnrealMaterial);


		MAKE_SUBTRACT(Subtract_Radials, UnrealMaterial);
		MAKE_MULTIPLY(MultiplyRot, UnrealMaterial);
		MAKE_DIVIDE(DivideThick, UnrealMaterial);
		MAKE_SUBTRACT(SubtractThick, UnrealMaterial);
		MAKE_CONST(ThicknessCoeff, UnrealMaterial);

		Radius->ParameterName = IndexedParamName("Radius", C_Index);
		Thickness->ParameterName = IndexedParamName("Thickness", C_Index);
		Density->ParameterName = IndexedParamName("Density", C_Index);
		Opacity->ParameterName = IndexedParamName("Opacity", C_Index);
		bIsSegmented->ParameterName = IndexedParamName("bIsSegmented", C_Index);
		SegmentCount->ParameterName = IndexedParamName("SegmentCount", C_Index);
		SegmentSpacing->ParameterName = IndexedParamName("SegmentSpacing", C_Index);
		CenterOffset->ParameterName = IndexedParamName("CenterOffset", C_Index);
		bAutoRotates->ParameterName = IndexedParamName("bAutoRotates", C_Index);
		InverseRotationSpeed->ParameterName = IndexedParamName("InverseRotationSpeed", C_Index);
		RotationDirection->ParameterName = IndexedParamName("RotationDirection", C_Index);

		Radius->DefaultValue = 0.1f + C_Index / 10.f;
		Thickness->DefaultValue = 0.1;
		Density->DefaultValue = 100.f;

		Radius->Group = IndexedGroupName(C_Index);
		Thickness->Group = IndexedGroupName(C_Index);
		Density->Group = IndexedGroupName(C_Index);
		Opacity->Group = IndexedGroupName(C_Index);
		bIsSegmented->Group = IndexedGroupName(C_Index);
		SegmentCount->Group = IndexedGroupName(C_Index);
		SegmentSpacing->Group = IndexedGroupName(C_Index);
		CenterOffset->Group = IndexedGroupName(C_Index);
		bAutoRotates->Group = IndexedGroupName(C_Index);
		InverseRotationSpeed->Group = IndexedGroupName(C_Index);
		RotationDirection->Group = IndexedGroupName(C_Index);

		UnrealMaterial->Expressions.Add(Radius);
		UnrealMaterial->Expressions.Add(Thickness);
		UnrealMaterial->Expressions.Add(Density);
		UnrealMaterial->Expressions.Add(Opacity);
		//UnrealMaterial->Expressions.Add(bIsSegmented);
		//UnrealMaterial->Expressions.Add(SegmentCount);
		//UnrealMaterial->Expressions.Add(SegmentSpacing);
		//UnrealMaterial->Expressions.Add(bAutoRotates);
		//UnrealMaterial->Expressions.Add(InverseRotationSpeed);
		//UnrealMaterial->Expressions.Add(RotationDirection);

		UnrealMaterial->Expressions.Add(RadialGradientCall1);
		UnrealMaterial->Expressions.Add(RadialGradientCall2);
		//UnrealMaterial->Expressions.Add(RadialSegmentsCall);
		
		// Math operation functions
		UnrealMaterial->Expressions.Add(Subtract_Radials);
		UnrealMaterial->Expressions.Add(MultiplyRot);
		UnrealMaterial->Expressions.Add(DivideThick);
		UnrealMaterial->Expressions.Add(SubtractThick);
		UnrealMaterial->Expressions.Add(ThicknessCoeff);
		/*
			Connect radial gradient nodes
		*/

		// Create and setup gradient function call 1
		RadialGradientCall1->SetMaterialFunction(MF_RadialGradientExp);
		Radius->ConnectExpression(RadialGradientCall1->GetInput(2), 0);
		Density->ConnectExpression(RadialGradientCall1->GetInput(3), 0);

		// Create and setup gradient function call 2
		RadialGradientCall2->SetMaterialFunction(MF_RadialGradientExp);

		Radius->ConnectExpression(SubtractThick->GetInput(0), 0);
		Thickness->ConnectExpression(DivideThick->GetInput(0), 0);
		DivideThick->ConnectExpression(SubtractThick->GetInput(1), 0);
		// Assign const 10
		ThicknessCoeff->R = 10.f;
		ThicknessCoeff->ConnectExpression(DivideThick->GetInput(1), 0);

		

		SubtractThick->ConnectExpression(RadialGradientCall2->GetInput(2), 0);
		Density->ConnectExpression(RadialGradientCall2->GetInput(3), 0);

		//
	//	RadialSegmentsCall->SetMaterialFunction(MF_RadialSegments);

		RadialGradientCall1->ConnectExpression(Subtract_Radials->GetInput(0), 0);
		RadialGradientCall2->ConnectExpression(Subtract_Radials->GetInput(1), 0);

		FinalExpressions.Add(Subtract_Radials);

		/*
			Connect segmentation and rotation nodes
		*/
		//SegmentCount->ConnectExpression(RadialSegmentsCall->GetInput(1), 0);
		//CenterOffset->ConnectExpression(RadialSegmentsCall->GetInput(2), 0);
		//SegmentSpacing->ConnectExpression(RadialSegmentsCall->GetInput(5), 0);

		/*	InverseRotationSpeed->ConnectExpression(MultiplyRot->GetInput(0), 0);
			RotationDirection->ConnectExpression(MultiplyRot->GetInput(1), 0);
			MultiplyRot->ConnectExpression(RadialSegmentsCall->GetInput(1), 0);*/


		// Test
		//Subtract_Radials->ConnectExpression(UnrealMaterial->BaseColor.Expression->GetInput(0), 0);
		//UnrealMaterial->BaseColor.Expression = Subtract_Radials;
	}

	//MAKE_ADD(AddNode, UnrealMaterial);


	TArray<UMaterialExpression*> AddNodes;
	for (int32 Iter = 0; Iter < Circle_N; ++Iter)
	{
		MAKE_ADD(AddNode, UnrealMaterial);

		if (AddNodes.IsEmpty())
		{
			UMaterialExpression* A = FinalExpressions[Iter];
			UMaterialExpression* B = FinalExpressions[Iter + 1];

			UnrealMaterial->Expressions.Add(AddNode);

			A->ConnectExpression(AddNode->GetInput(0), 0);
			B->ConnectExpression(AddNode->GetInput(1), 0);

			AddNodes.Add(AddNode);
			continue;
		}

		UMaterialExpression* A = FinalExpressions[Iter];

		AddNodes.Last()->ConnectExpression(AddNode->GetInput(0), 0);
		A->ConnectExpression(AddNode->GetInput(1), 0);

		UnrealMaterial->Expressions.Add(AddNode);

		AddNodes.Add(AddNode);
	}
	
	UnrealMaterial->EmissiveColor.Expression = AddNodes.Last();

	//////////////////////////////////////////
    // Final change notifies and state sets
	UnrealMaterial->MaterialDomain = EMaterialDomain::MD_UI;
	UnrealMaterial->PreEditChange(NULL);
	UnrealMaterial->PostEditChange();
}

FName UShaderScripts::IndexedParamName(const FName ParamName, const int32 Index)
{
	return FName(*FString::Printf(TEXT("Circle_%d_%s"), Index, *ParamName.ToString()));
}

FName UShaderScripts::IndexedGroupName(const int32 Index)
{
	return FName(*FString::Printf(TEXT("Group_Circle_%d"), Index));
}

//struct FShaderCircleProperties
//{
//	float ExternalRadius = 0.f;
//	float InternalRadius = 0.f;
//	float Density = 0.f;
//	float Opacity = 0.f;
//	bool  bIsSegmented = 0.f;
//	int32 SegmentCount = 0;
//	float SegmentSpacing = 0.f;
//	float Rotation = 0.f;
//	float RotatoinDirection = 0.f;
//};
