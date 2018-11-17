#include "StdAfx.h"
#include "WeaponBinoculars.h"

//--#SM+#--
CWeaponBinoculars::CWeaponBinoculars() {}
CWeaponBinoculars::~CWeaponBinoculars() {}

void CWeaponBinoculars::Load(LPCSTR section)
{
    if (!pSettings->line_exist(section, "scope_alive_detector"))
        pSettings->w_string(section, "scope_alive_detector", section);

    if (!pSettings->line_exist(section, "scope_dynamic_zoom"))
        pSettings->w_bool(section, "scope_dynamic_zoom", true);

    inherited::Load(section);

    m_bUIShowAmmo = false;
    m_bDisableFire = true;
}
