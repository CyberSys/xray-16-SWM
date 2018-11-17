#pragma once

#include "WeaponMagazinedWGrenade.h"

class CWeaponBinoculars : public CWeapon //--#SM+#--
{
private:
    typedef CWeapon inherited;

public:
    CWeaponBinoculars();
    virtual ~CWeaponBinoculars();

    void Load(LPCSTR section);

    virtual bool IsMagazineCanBeUnload() { return false; }
};
