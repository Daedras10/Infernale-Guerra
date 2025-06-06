
#include "Mass/Army/AmalgamFragments.h"

#include "Mass/Collision/SpatialHashGrid.h"
#include "LD/Buildings/BuildingParent.h"

FAmalgamMovementFragment::FAmalgamMovementFragment(): LocalSpeed(0), LocalRushSpeed(0)
{
}

float FAmalgamTargetFragment::GetTargetRangeOffset(EAmalgamAggro AggroType) const
{
	switch (AggroType)
	{
	case NoAggro:
		return 0.f;
	case Amalgam:
		return ASpatialHashGrid::GetEntityData(TargetEntity).TargetableRadius;
	case Building:
		if (TargetBuilding.IsValid())
			return TargetBuilding->GetTargetableRange();
		else
			return 0.0f;
	case LDElement:
		return 0.f;
	}

	return 0.f;
}

FAmalgamHealthParams::FAmalgamHealthParams(): BaseHealth(0)
{
}

FAmalgamCombatParams::FAmalgamCombatParams(): BaseDamage(0), BaseBuildingDamage(0), BaseAttackDelay(0), BaseRange(0)
{
}

FAmalgamMovementParams::FAmalgamMovementParams(): BaseSpeed(0), BaseRushSpeed(0)
{
}

FAmalgamDetectionParams::FAmalgamDetectionParams(): BaseDetectionRange(0), BaseDetectionAngle(0), TargetableRange(0)
{
}

FAmalgamSightParams::FAmalgamSightParams(): BaseSightRange(0), BaseSightAngle(0), BaseSightType()
{
}

FAmalgamNiagaraParams::FAmalgamNiagaraParams()
{
}

FAmalgamAcceptanceParams::FAmalgamAcceptanceParams(): AcceptancePathfindingRadius(0), AcceptanceRadiusAttack(0)
{
}

FAmalgamInitializeFragment::FAmalgamInitializeFragment(): Health(0), Attack(0), Count(0), Speed(0), RushSpeed(0),
                                                          AggroRadius(0),
                                                          FightRadius(0),
                                                          AttackDelay(0),
                                                          NiagaraSystem(nullptr)
{
}

FAmalgamNiagaraFragment::FAmalgamNiagaraFragment(): HeightOffset(0)
{
}
