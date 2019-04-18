#include "stdafx.h"
#include "WeaponKnife.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponKnife::CWeaponKnife() {}
CWeaponKnife::~CWeaponKnife() {}

void CWeaponKnife::Load(LPCSTR section)
{
    m_bKnifeMode = true;
    inherited::Load(section);
    m_bAllowUnload = false;
}

void CWeaponKnife::ReloadAllSounds()
{
    inherited::ReloadAllSounds();
    ReloadSound("snd_shoot", "sndKnife", ESndExcl::eExNot, SOUND_TYPE_WEAPON_SHOOTING);
}
