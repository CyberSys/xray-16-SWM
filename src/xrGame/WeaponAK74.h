#pragma once

#include "WeaponMagazinedWGrenade.h"

class CWeaponAK74 : public CWeaponMagazinedWGrenade //--#SM+#--
{
public:
    CWeaponAK74(ESoundTypes eSoundType = SOUND_TYPE_WEAPON_SUBMACHINEGUN);
    virtual ~CWeaponAK74();
};