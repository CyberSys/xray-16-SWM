#pragma once

#include "WeaponMagazinedWGrenade.h"

class CWeaponKnife : public CWeaponMagazinedWGrenade //--#SM+#--
{
private:
    typedef CWeaponMagazinedWGrenade inherited;

public:
    CWeaponKnife();
    virtual ~CWeaponKnife();

    void Load(LPCSTR section);

    virtual void ReloadAllSounds();

    virtual bool IsMagazineCanBeUnload() { return false; }
};
