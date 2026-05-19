#include "Gas/PaldarkGameplayAbility.h"

UPaldarkGameplayAbility::UPaldarkGameplayAbility()
{
	// Default instancing policy mirrors what Aura / Crunch use for input-bound
	// abilities — instanced per actor (so per-character state lives on the
	// ability) and replicated from server -> autonomous proxy.
	InstancingPolicy        = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy      = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy       = EGameplayAbilityReplicationPolicy::ReplicateNo;
	NetSecurityPolicy       = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}
