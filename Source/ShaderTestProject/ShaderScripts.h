// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetActionUtility.h"
#include "ShaderScripts.generated.h"

/**
 * 
 */
UCLASS()
class SHADERTESTPROJECT_API UShaderScripts : public UAssetActionUtility
{
	GENERATED_BODY()

protected:

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Gazan funkcianer")
	void GenerateCirclesShader(const int32 CircleCount, const FString MaterialName = "M_GenMaterial");

	FName IndexedParamName(const FName ParamName, const int32 Index);
	FName IndexedGroupName(const int32 Index);
};
