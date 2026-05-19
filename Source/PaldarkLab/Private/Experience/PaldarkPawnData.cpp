#include "Experience/PaldarkPawnData.h"

UPaldarkPawnData::UPaldarkPawnData() = default;

FPrimaryAssetId UPaldarkPawnData::GetPrimaryAssetId() const
{
	// Primary asset type "PaldarkPawnData" matches the type declared in
	// UPaldarkAssetManager::StartInitialLoading and in DefaultGame.ini under
	// [/Script/Engine.AssetManagerSettings].
	return FPrimaryAssetId(TEXT("PaldarkPawnData"), GetFName());
}
