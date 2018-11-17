#pragma once
#include "WeaponMagazinedWGrenade.h"

class CWeaponAutomaticShotgun : public CWeaponMagazinedWGrenade //--#SM+#--
{
    typedef CWeaponMagazined inherited;

public:
    CWeaponAutomaticShotgun();
    virtual ~CWeaponAutomaticShotgun();
};
