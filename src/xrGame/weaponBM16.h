#pragma once

#include "WeaponMagazinedWGrenade.h"

class CWeaponBM16 : public CWeaponMagazinedWGrenade //--#SM+#--
{
private:
    typedef CWeaponMagazinedWGrenade inherited;

public:
    CWeaponBM16();
    virtual ~CWeaponBM16();
    virtual void Load(LPCSTR section);
};