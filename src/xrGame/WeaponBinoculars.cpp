#include "stdafx.h"
#include "WeaponBinoculars.h"
#include "xrScriptEngine/ScriptExporter.hpp"

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

    m_bUIShowAmmo  = false;
    m_bDisableFire = true;
}

using namespace luabind;

SCRIPT_EXPORT(CWeaponBinoculars, (CGameObject), {
    module(luaState)[class_<CWeaponBinoculars, CGameObject>("CWeaponBinoculars").def(constructor<>())];
});