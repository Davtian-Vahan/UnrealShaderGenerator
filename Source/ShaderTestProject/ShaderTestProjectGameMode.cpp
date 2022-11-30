// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShaderTestProjectGameMode.h"
#include "ShaderTestProjectCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShaderTestProjectGameMode::AShaderTestProjectGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
