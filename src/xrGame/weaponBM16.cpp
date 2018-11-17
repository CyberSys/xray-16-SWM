#include "StdAfx.h"
#include "weaponBM16.h"

//--#SM+#--
CWeaponBM16::CWeaponBM16() {}
CWeaponBM16::~CWeaponBM16() {}

void CWeaponBM16::Load(LPCSTR section)
{
    LPCSTR sHUD = pSettings->r_string(section, "hud");

    if (!pSettings->line_exist(sHUD, "anm_reload") && pSettings->line_exist(sHUD, "anm_reload_2"))
    {
        pSettings->w_string(sHUD, "anm_reload", pSettings->r_string(sHUD, "anm_reload_2"));
    }

    if (!pSettings->line_exist(sHUD, "anm_shot") && pSettings->line_exist(sHUD, "anm_shot_1"))
    {
        pSettings->w_string(sHUD, "anm_shot", pSettings->r_string(sHUD, "anm_shot_1"));
    }

    if (!pSettings->line_exist(section, "snd_anm_reload_1") && pSettings->line_exist(section, "snd_reload_1"))
    {
        pSettings->w_string(section, "snd_anm_reload_1", pSettings->r_string(section, "snd_reload_1"));
    }

    inherited::Load(section);
}
