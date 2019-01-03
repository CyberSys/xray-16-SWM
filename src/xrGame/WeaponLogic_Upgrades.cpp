/*****************************************/
/***** Апгрейды механиков для оружия *****/ //--#SM+#--
/*****************************************/

#include "StdAfx.h"
#include "Weapon.h"

static bool process_if_exists_deg2rad(LPCSTR section, LPCSTR name, float& value, bool test)
{
    if (!pSettings->line_exist(section, name))
    {
        return false;
    }
    LPCSTR str = pSettings->r_string(section, name);
    if (!str || !xr_strlen(str))
    {
        return false;
    }

    if (!test)
    {
        value += deg2rad(pSettings->r_float(section, name));
    }
    return true;
}

void CWeapon::net_Spawn_install_upgrades(Upgrades_type saved_upgrades)
{
    // do not delete this
    // this is intended behaviour
    // CWeapon::net_Spawn()
}

// Вызывается перед установкой апгрейда
void CWeapon::pre_install_upgrade()
{
    inherited::pre_install_upgrade();

    //=== Разряжаем оружие ===//
    if (m_bUseMagazines == false)
        UnloadMagazineMain();
    UnloadMagazineGL();

    //=== Снимаем все аддоны ===//
    if (ScopeAttachable() && IsScopeAttached())
    {
        Detach(GetScopeName().c_str(), true);
    }
    if (SilencerAttachable() && IsSilencerAttached())
    {
        Detach(GetSilencerName().c_str(), true);
    }
    if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
    {
        Detach(GetGrenadeLauncherName().c_str(), true);
    }
    if (MagazineAttachable() && IsMagazineAttached())
    {
        Detach(GetMagazineName().c_str(), true);
    }
    if (Special_1_Attachable() && IsSpecial_1_Attached())
    {
        Detach(GetSpecial_1_Name().c_str(), true);
    }
    if (Special_2_Attachable() && IsSpecial_2_Attached())
    {
        Detach(GetSpecial_2_Name().c_str(), true);
    }
    if (Special_3_Attachable() && IsSpecial_3_Attached())
    {
        Detach(GetSpecial_3_Name().c_str(), true);
    }
    if (Special_4_Attachable() && IsSpecial_4_Attached())
    {
        Detach(GetSpecial_4_Name().c_str(), true);
    }
}

// Вызывается при установке \ тестировании апгрейда
bool CWeapon::install_upgrade_impl(LPCSTR section, bool test)
{
    bool result = CInventoryItemObject::install_upgrade_impl(section, test);

    result |= install_upgrade_generic(section, test);
    result |= install_upgrade_ammo_class(section, test);
    result |= install_upgrade_addon(section, test);
    result |= install_upgrade_disp(section, test);
    result |= install_upgrade_hit(section, test);
    result |= install_upgrade_sounds(section, test);

    return result;
}

// Апгрейды общего характера
bool CWeapon::install_upgrade_generic(LPCSTR section, bool test)
{
    pcstr str;
    bool result = false, result2 = false;

    //=== Режимы стрельбы (fire_modes = 1, 2, -1) ===//
    result2 = process_if_exists_set(section, "fire_modes", &CInifile::r_string, str, test);
    if (result2 && !test)
    {
        int ModesCount = _GetItemCount(str);
        m_aFireModes.clear();
        for (int i = 0; i < ModesCount; ++i)
        {
            string16 sItem;
            _GetItem(str, i, sItem);
            m_aFireModes.push_back((s8)atoi(sItem));
        }
        m_iCurFireMode = ModesCount - 1;
    }
    result |= result2;

    //=== Износ оружия ===/
    result |= process_if_exists(section, "condition_shot_dec", &CInifile::r_float, conditionDecreasePerShot, test);
    result |= process_if_exists(section, "condition_queue_shot_dec", &CInifile::r_float, conditionDecreasePerQueueShot, test);
    result |= process_if_exists(section, "misfire_start_condition", &CInifile::r_float, misfireStartCondition, test);
    result |= process_if_exists(section, "misfire_end_condition", &CInifile::r_float, misfireEndCondition, test);
    result |= process_if_exists(section, "misfire_start_prob", &CInifile::r_float, misfireStartProbability, test);
    result |= process_if_exists(section, "misfire_end_prob", &CInifile::r_float, misfireEndProbability, test);

    return result;
}

// Апгрейды аммуниции
bool CWeapon::install_upgrade_ammo_class(LPCSTR section, bool test)
{
    pcstr str;
    bool result = false, result2 = false;

    //=== Типы патронов, размер магазина ===//
    result |= LoadMainAmmoParams(section, false, true, (test ? EFuncUpgrMode::eUpgrTest : EFuncUpgrMode::eUpgrInstall));

    return result;
}

// Апгрейды разброса и отдачи оружия
bool CWeapon::install_upgrade_disp(LPCSTR section, bool test)
{
    pcstr str;
    bool result = false, result2 = false;

    //=== Фактор влияния износа на разброс ===/
    result |= process_if_exists(
        section, "fire_dispersion_condition_factor", &CInifile::r_float, fireDispersionConditionFactor, test);

    //=== Разброс пуль ===/
    result |= process_if_exists_deg2rad(section, "fire_dispersion_base", fireDispersionBase, test);

    result |= process_if_exists(section, "PDM_disp_base", &CInifile::r_float, m_pdm.m_fPDM_disp_base, test);
    result |= process_if_exists(section, "PDM_disp_vel_factor", &CInifile::r_float, m_pdm.m_fPDM_disp_vel_factor, test);
    result |= process_if_exists(section, "PDM_disp_accel_factor", &CInifile::r_float, m_pdm.m_fPDM_disp_accel_factor, test);
    result |= process_if_exists(section, "PDM_disp_crouch", &CInifile::r_float, m_pdm.m_fPDM_disp_crouch, test);
    result |= process_if_exists(section, "PDM_disp_crouch_no_acc", &CInifile::r_float, m_pdm.m_fPDM_disp_crouch_no_acc, test);

    //=== Параметры возврата камеры после отдачи ===/
    u8 rm = (cam_recoil.ReturnMode) ? 1 : 0;
    result |= process_if_exists_set(section, "cam_return", &CInifile::r_u8, rm, test);
    cam_recoil.ReturnMode = (rm == 1);

    rm = (cam_recoil.StopReturn) ? 1 : 0;
    result |= process_if_exists_set(section, "cam_return_stop", &CInifile::r_u8, rm, test);
    cam_recoil.StopReturn = (rm == 1);

    //=== Отдача камеры от бедра ===/
    result |= process_if_exists(section, "cam_dispersion_frac", &CInifile::r_float, cam_recoil.DispersionFrac, test);

    result |= process_if_exists_deg2rad(section, "cam_relax_speed", cam_recoil.RelaxSpeed, test);
    result |= process_if_exists_deg2rad(section, "cam_relax_speed_ai", cam_recoil.RelaxSpeed_AI, test);
    result |= process_if_exists_deg2rad(section, "cam_dispersion", cam_recoil.Dispersion, test);
    result |= process_if_exists_deg2rad(section, "cam_dispersion_inc", cam_recoil.DispersionInc, test);

    result |= process_if_exists_deg2rad(section, "cam_max_angle", cam_recoil.MaxAngleVert, test);
    result |= process_if_exists_deg2rad(section, "cam_max_angle_horz", cam_recoil.MaxAngleHorz, test);
    result |= process_if_exists_deg2rad(section, "cam_step_angle_horz", cam_recoil.StepAngleHorz, test);

    VERIFY(!fis_zero(cam_recoil.RelaxSpeed));
    VERIFY(!fis_zero(cam_recoil.RelaxSpeed_AI));
    VERIFY(!fis_zero(cam_recoil.MaxAngleVert));
    VERIFY(!fis_zero(cam_recoil.MaxAngleHorz));

    //=== Отдача камеры при прицеливании ===/
    result |= process_if_exists(
        section, "zoom_cam_dispersion_frac", &CInifile::r_float, zoom_cam_recoil.DispersionFrac, test);

    result |= process_if_exists_deg2rad(section, "zoom_cam_relax_speed", zoom_cam_recoil.RelaxSpeed, test); // zoom_ ...
    result |= process_if_exists_deg2rad(section, "zoom_cam_relax_speed_ai", zoom_cam_recoil.RelaxSpeed_AI, test);
    result |= process_if_exists_deg2rad(section, "zoom_cam_dispersion", zoom_cam_recoil.Dispersion, test);
    result |= process_if_exists_deg2rad(section, "zoom_cam_dispersion_inc", zoom_cam_recoil.DispersionInc, test);

    result |= process_if_exists_deg2rad(section, "zoom_cam_max_angle", zoom_cam_recoil.MaxAngleVert, test);
    result |= process_if_exists_deg2rad(section, "zoom_cam_max_angle_horz", zoom_cam_recoil.MaxAngleHorz, test);
    result |= process_if_exists_deg2rad(section, "zoom_cam_step_angle_horz", zoom_cam_recoil.StepAngleHorz, test);

    VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed));
    VERIFY(!fis_zero(zoom_cam_recoil.RelaxSpeed_AI));
    VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleVert));
    VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleHorz));

    return result;
}

// Апгрейды боевой мощи оружия
bool CWeapon::install_upgrade_hit(LPCSTR section, bool test)
{
    pcstr str;
    bool result = false, result2 = false;
    shared_str s_sHitPower;

    //=== Урон оружия ===/
    result2 = process_if_exists_set(section, "hit_power", &CInifile::r_string_wb, s_sHitPower, test);
    if (result2 && !test)
    {
        string32 buffer;
        fvHitPower[egdMaster] = (float)atof(_GetItem(*s_sHitPower, 0, buffer));
        fvHitPower[egdNovice] = fvHitPower[egdStalker] = fvHitPower[egdVeteran] = fvHitPower[egdMaster];

        int num_game_diff_param = _GetItemCount(*s_sHitPower);
        if (num_game_diff_param > 1)
        {
            fvHitPower[egdVeteran] = (float)atof(_GetItem(*s_sHitPower, 1, buffer));
        }
        if (num_game_diff_param > 2)
        {
            fvHitPower[egdStalker] = (float)atof(_GetItem(*s_sHitPower, 2, buffer));
        }
        if (num_game_diff_param > 3)
        {
            fvHitPower[egdNovice] = (float)atof(_GetItem(*s_sHitPower, 3, buffer));
        }
    }
    result |= result2;

    //=== Урон оружия (критический) ===/
    shared_str s_sHitPowerCritical;
    result2 = process_if_exists_set(section, "hit_power_critical", &CInifile::r_string_wb, s_sHitPower, test);
    if (result2 && !test)
    {
        string32 buffer;
        fvHitPowerCritical[egdMaster] = (float)atof(_GetItem(*s_sHitPowerCritical, 0, buffer));
        fvHitPowerCritical[egdNovice] = fvHitPowerCritical[egdStalker] = fvHitPowerCritical[egdVeteran] =
            fvHitPowerCritical[egdMaster];

        int num_game_diff_param = _GetItemCount(*s_sHitPowerCritical);
        if (num_game_diff_param > 1)
        {
            fvHitPowerCritical[egdVeteran] = (float)atof(_GetItem(*s_sHitPowerCritical, 1, buffer));
        }
        if (num_game_diff_param > 2)
        {
            fvHitPowerCritical[egdStalker] = (float)atof(_GetItem(*s_sHitPowerCritical, 2, buffer));
        }
        if (num_game_diff_param > 3)
        {
            fvHitPowerCritical[egdNovice] = (float)atof(_GetItem(*s_sHitPowerCritical, 3, buffer));
        }
    }
    result |= result2;

    //=== Импульс пули оружия ===/
    result |= process_if_exists(section, "hit_impulse", &CInifile::r_float, fHitImpulse, test);

    //=== Наличие усиленного первого выстрела (игнорирует броню?) ===/
    result |= process_if_exists_set(section, "use_aim_bullet", &CInifile::r_bool, m_bUseAimBullet, test);
    if (m_bUseAimBullet) // first super bullet
    {
        result |= process_if_exists(section, "time_to_aim", &CInifile::r_float, m_fTimeToAim, test);
    }

    //=== Скорость полёта пули ===/
    result |= process_if_exists(section, "bullet_speed", &CInifile::r_float, m_fStartBulletSpeed, test);

    //=== Скорострельность ===/
    float rpm = 60.0f / fOneShotTime;
    result2 = process_if_exists(section, "rpm", &CInifile::r_float, rpm, test);
    if (result2 && !test)
    {
        VERIFY(rpm > 0.0f);
        fOneShotTime = 60.0f / rpm;
    }
    result |= result2;

    //=== Макс. дальность полёта пули ===/
    result |= process_if_exists(section, "fire_distance", &CInifile::r_float, fireDistance, test);

    //=== Динамическая скорость полёта первых пуль (Абакан) ===/
    result |= process_if_exists_set(
        section, "base_dispersioned_bullets_count", &CInifile::r_s32, m_iBaseDispersionedBulletsCount, test);
    result |= process_if_exists_set(
        section, "base_dispersioned_bullets_speed", &CInifile::r_float, m_fBaseDispersionedBulletsSpeed, test);

    //=== Скорость полёта гранаты ===//
    result |= process_if_exists(section, "launch_speed", &CInifile::r_float, m_fLaunchSpeed, test);

    return result;
}

// Апгрейды на возможность установки аддонов на оружие
bool CWeapon::install_upgrade_addon(LPCSTR section, bool test)
{
    pcstr str;
    bool result = false, result2 = false;

    //=== Перегружаем аддоны (эксперементальный функционал <!>) ===//
    // Добавляет новые аддоны к уже существующим, не очищает те что уже есть
    result2 = LoadAddons(section, (test ? EFuncUpgrMode::eUpgrTest : EFuncUpgrMode::eUpgrInstall));
    if (result2 && !test)
    {
        bool bNeed2UpdateZoomTexture = false;

        // Устанавливаем аддоны, если они перманентны
        if (GetAddonBySlot(eScope)->m_attach_status == ALife::eAddonPermanent)
        {
            InstallAddon(eScope, first_addon_idx, true);
            bNeed2UpdateZoomTexture = true;
        }
        if (GetAddonBySlot(eMuzzle)->m_attach_status == ALife::eAddonPermanent)
            InstallAddon(eMuzzle, first_addon_idx, true);
        if (GetAddonBySlot(eLauncher)->m_attach_status == ALife::eAddonPermanent)
        {
            InstallAddon(eLauncher, first_addon_idx, true);
            bNeed2UpdateZoomTexture = true;
        }
        if (GetAddonBySlot(eMagaz)->m_attach_status == ALife::eAddonPermanent)
            InstallAddon(eMagaz, first_addon_idx, true);
        if (GetAddonBySlot(eSpec_1)->m_attach_status == ALife::eAddonPermanent)
            InstallAddon(eSpec_1, first_addon_idx, true);
        if (GetAddonBySlot(eSpec_2)->m_attach_status == ALife::eAddonPermanent)
            InstallAddon(eSpec_2, first_addon_idx, true);
        if (GetAddonBySlot(eSpec_3)->m_attach_status == ALife::eAddonPermanent)
            InstallAddon(eSpec_3, first_addon_idx, true);
        if (GetAddonBySlot(eSpec_4)->m_attach_status == ALife::eAddonPermanent)
            InstallAddon(eSpec_4, first_addon_idx, true);

        // Обновляем состояние аддонов на оружии
        UpdateAddons();

        // Обновляем 2D-Текстуру прицельных сеток
        if (bNeed2UpdateZoomTexture)
        {
            for (int i = 0; i < eZoomTypesCnt; i++)
                GetZoomParams((EZoomTypes)i).UpdateUIScope();
        }
    }
    result |= result2;

    //=== Наличие режима прицеливания ===/
    result |= process_if_exists_set(section, "zoom_enabled", &CInifile::r_bool, m_bZoomEnabled, test);

    //=== Спец-эффекты при прицеливании ===/
    //--> Зум с динамической краткостью
    result |= process_if_exists_set(
        section, "scope_dynamic_zoom", &CInifile::r_bool, GetZoomParams(eZoomMain).m_bUseDynamicZoom, test);

    //--> Пост-эффект прицеливания
    result2 = process_if_exists_set(
        section, "scope_nightvision", &CInifile::r_string_wb, m_sUseZoomPostprocessUpgr, test);
    if (result2)
        xr_delete(GetZoomParams(eZoomMain).m_pNight_vision);
    result |= result2;

    //--> Подсветка целей рамками (как у бинокля)
    result2 = process_if_exists_set(
        section, "scope_alive_detector", &CInifile::r_string_wb, m_sUseBinocularVisionUpgr, test);
    if (result2)
        xr_delete(GetZoomParams(eZoomMain).m_pVision);
    result |= result2;

    //=== Модификатор зума прицела ===/
    result |= process_if_exists(section, "scope_zoom_factor", &CInifile::r_float, m_fZoomFovFactorUpgr, test);

    return result;
}

// Апгрейды звуков оружия
bool CWeapon::install_upgrade_sounds(LPCSTR section, bool test)
{
    // <!> Также вызывается при любых операциях с аддонами [called on addon attach \ detach too]

    if (!test)
        ReloadAllSounds(section);

    return false;
}
