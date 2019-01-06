#include "StdAfx.h"
#include "Weapon.h"
#include "WeaponKnifeHit.h"

/************************************/
/***** Загрузка конфигов оружия *****/ //--#SM+#--
/************************************/

void CWeapon::Load(LPCSTR section)
{
    // Загрузка данных предков
    inherited::Load(section);
    CShootingObject::Load(section);

    // Загрузка параметров отдачи
    LoadRecoilParams(section);

    // Загрузка параметров зума
    LoadZoomParams(section);

    // Загрузка патронов для основного магазина
    LoadMainAmmoParams(section, true);

    // Параметры перезарядки
    m_bTriStateReload_main      = READ_IF_EXISTS(pSettings, r_bool, section, "tri_state_reload", false);
    m_bTriStateReload_anim_hack = READ_IF_EXISTS(pSettings, r_bool, section, "tri_state_rld_anim_hack", false);

    // Параметры износа
    misfireStartCondition   = pSettings->r_float(section, "misfire_start_condition");
    misfireEndCondition     = READ_IF_EXISTS(pSettings, r_float, section, "misfire_end_condition", 0.f);
    misfireStartProbability = READ_IF_EXISTS(pSettings, r_float, section, "misfire_start_prob", 0.f);
    misfireEndProbability   = pSettings->r_float(section, "misfire_end_prob");

    //--> Увеличение износа за каждый выстрел
    conditionDecreasePerShot      = pSettings->r_float(section, "condition_shot_dec");
    conditionDecreasePerQueueShot = READ_IF_EXISTS(pSettings, r_float, section, "condition_queue_shot_dec", conditionDecreasePerShot);

    // Контроллер первого выстрела (повышенная точность и скорость пули)
    m_first_bullet_controller.load(section);
    m_iBaseDispersionedBulletsCount = READ_IF_EXISTS(pSettings, r_u8, section, "base_dispersioned_bullets_count", 0);
    m_fBaseDispersionedBulletsSpeed = READ_IF_EXISTS(pSettings, r_float, section, "base_dispersioned_bullets_speed", m_fStartBulletSpeed);

    // Радиус стрельбы для АИ (не работает)
    m_fMinRadius = pSettings->r_float(section, "min_radius");
    m_fMaxRadius = pSettings->r_float(section, "max_radius");

    // Время удаления бесхозного оружия в МП
    if (pSettings->line_exist(section, "weapon_remove_time"))
        m_dwWeaponRemoveTime = pSettings->r_u32(section, "weapon_remove_time");
    else
        m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;

    // Спавнить оружие с патронами
    if (pSettings->line_exist(section, "auto_spawn_ammo"))
        m_bAutoSpawnAmmo = pSettings->r_bool(section, "auto_spawn_ammo");
    else
        m_bAutoSpawnAmmo = TRUE;

    // Параметры трассеров пуль
    m_bHasTracers     = !!READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
    m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

    // Вероятность прохождения хита по ГГ
    string256 temp;
    for (int i = egdNovice; i < egdCount; ++i)
    {
        strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
        m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
    }

    // Режимы стрельбы
    if (pSettings->line_exist(section, "fire_modes"))
    {
        m_iPrefferedFireMode     = READ_IF_EXISTS(pSettings, r_s16, section, "preffered_fire_mode", -1);
        m_bHasDifferentFireModes = true;
        m_iCurFireMode           = -1;

        shared_str FireModesList = pSettings->r_string(section, "fire_modes");
        int        ModesCount    = _GetItemCount(FireModesList.c_str());
        m_aFireModes.clear();

        for (int i = 0; i < ModesCount; i++)
        {
            string16 sItem;
            _GetItem(FireModesList.c_str(), i, sItem);
            s8 iFireMode = (s8)atoi(sItem);
            m_aFireModes.push_back(iFireMode);

            if (m_iPrefferedFireMode != -1 && iFireMode == (s8)m_iPrefferedFireMode)
            {
                m_iCurFireMode = i;
            }
        }

        if (m_iCurFireMode == -1)
            m_iCurFireMode = ModesCount - 1;
    }
    else
    {
        m_iCurFireMode           = 0;
        m_bHasDifferentFireModes = false;
    }
    SetQueueSize(GetCurrentFireMode());

    // Режим болтовки или помпы
    m_bUsePumpMode = READ_IF_EXISTS(pSettings, r_bool, section, "use_pump_mode", false);

    // Атаки ножом
    if (m_bKnifeMode != true) //--> for CWeaponKnife
        m_bKnifeMode = READ_IF_EXISTS(pSettings, r_bool, section, "use_knife_mode", false);

    if (m_bKnifeMode == true)
    {
        R_ASSERT(m_first_attack == NULL);
        R_ASSERT(m_second_attack == NULL);

        LPCSTR sFirstAttack = READ_IF_EXISTS(pSettings, r_string, section, "knife_attack_1", NULL);
        R_ASSERT2(sFirstAttack != NULL, "Use new Knife config !!! (knife_attack_1 = <section>)");
        m_first_attack = new CWeaponKnifeHit(sFirstAttack, this);

        LPCSTR sSecondAttack = READ_IF_EXISTS(pSettings, r_string, section, "knife_attack_2", NULL);
        if (sSecondAttack)
            m_second_attack = new CWeaponKnifeHit(sSecondAttack, this);
    }

    // Режим патронташа
    m_bUseAmmoBeltMode = READ_IF_EXISTS(pSettings, r_bool, section, "use_ammo_belt_mode", false);

    // Загружаем параметры магазинного питания
    LoadMagazinesParams(section);

    // Визуализация патрона во время перезарядки и гильзы после выстрела
    m_sBulletHUDVisual = READ_IF_EXISTS(pSettings, r_string, section, "hud_bullet_visual", NULL);
    m_sAnimatedShellVisData = READ_IF_EXISTS(pSettings, r_string, section, "hud_shell_visual", NULL);

    // Аддоны
    LoadAddons(section);

    // Параметры DOF-а
    Fvector def_dof;
    def_dof.set(-1, -1, -1);
    //	m_zoom_params.m_ZoomDof		= READ_IF_EXISTS(pSettings, r_fvector3, section, "zoom_dof", Fvector().set(-1,-1,-1));
    //	m_zoom_params.m_bZoomDofEnabled	= !def_dof.similar(m_zoom_params.m_ZoomDof);
    //	m_zoom_params.m_ReloadDof	= READ_IF_EXISTS(pSettings, r_fvector4, section, "reload_dof", Fvector4().set(-1,-1,-1,-1));

    // Параметры рук
    eHandDependence   = EHandDependence(pSettings->r_s32(section, "hand_dependence"));
    m_bIsSingleHanded = true;
    if (pSettings->line_exist(section, "single_handed"))
        m_bIsSingleHanded = !!pSettings->r_bool(section, "single_handed");

    // Инерция перекрестия (скорость изменения его размеров в зависимости от отдачи)
    m_crosshair_inertion = READ_IF_EXISTS(pSettings, r_float, section, "crosshair_inertion", 5.91f);

    // Партикл альтернативного выстрела от 3-го лица
    m_sFlameParticles2 = READ_IF_EXISTS(pSettings, r_string, section, "grenade_flame_particles", m_sFlameParticles);
    if (pSettings->line_exist(section, "flame_particles_2"))
        m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

    // Позиция выстрела от 3-го лица
    vLoadedFirePoint  = pSettings->r_fvector3(section, "fire_point");
    vLoadedFirePoint2 = READ_IF_EXISTS(pSettings, r_fvector3, section, "fire_point2", vLoadedFirePoint);

    // Текущий звук выстрела
    m_sSndShotCurrent = "sndShot";

    // Параметры подствольника
    LoadGLParams();

    // Параметры ракетницы
    m_sOverridedRocketSection = READ_IF_EXISTS(pSettings, r_string, section, "rocket_class", NULL);
    CRocketLauncher::Load(section);

    // Параметры гильз
    CShellLauncher::ReLoadShellData(section, hud_sect);

    // Модификатор для HUD FOV от бедра
    m_HudFovAddition = READ_IF_EXISTS(pSettings, r_float, section, "hud_fov_addition_modifier", 0.f);

    // Параметры изменения HUD FOV когда игрок стоит вплотную к стене
    m_nearwall_dist_min       = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_dist_min", 0.5f);
    m_nearwall_dist_max       = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_dist_max", 1.f);
    m_nearwall_target_hud_fov = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_target_hud_fov", 0.27f);
    m_nearwall_speed_mod      = READ_IF_EXISTS(pSettings, r_float, section, "nearwall_speed_mod", 10.f);

    // Прочее
    m_bUIShowAmmo              = READ_IF_EXISTS(pSettings, r_bool, section, "show_ammo", true);
    m_bAllowAutoReload         = READ_IF_EXISTS(pSettings, r_bool, section, "auto_reload", true);
    m_bAllowUnload             = READ_IF_EXISTS(pSettings, r_bool, section, "can_unload_magazine", true);
    m_bDisableFire             = READ_IF_EXISTS(pSettings, r_bool, section, "shooting_disabled", false);
    m_bDisableFireWhileZooming = READ_IF_EXISTS(pSettings, r_bool, section, "disable_fire_at_zooming", false);
    m_bDisableMovEffAtZoom     = READ_IF_EXISTS(pSettings, r_bool, section, "disable_mov_eff_at_zoom", false);
    m_bInvShowAmmoCntInMagaz   = READ_IF_EXISTS(pSettings, r_bool, section, "ui_show_ammo_cnt_for_types", false);

    // Added by Axel, to enable optional condition use on any item
    m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", true));
}

void CWeapon::PostLoad(LPCSTR section)
{
    inherited::PostLoad(section);

    // Загрузка звуков
    ReloadAllSounds();
}

// Загрузить параметры патронов основного магазина
bool CWeapon::LoadMainAmmoParams(LPCSTR section, bool bFromLoad, bool bDontUnload, EFuncUpgrMode upgrMode)
{
    bool bResult = false;

    bool bGMode = m_bGrenadeMode;
    if (bGMode)
        PerformSwitchGL();

    if (!bDontUnload && m_magazine.size() > 0)
        UnloadMagazine(); //--> Разряжаем магазин (если он не был пуст)

    // Грузим параметры из конфига
    if (upgrMode == eUpgrNone || pSettings->line_exist(section, "ammo_class"))
    {
        if (upgrMode != eUpgrTest)
        {
            m_ammoType = 0;
            m_set_next_ammoType_on_reload = undefined_ammo_type;

            m_ammoTypes.clear();
            m_AmmoCartidges.clear();
            {
                LPCSTR S = pSettings->r_string(section, "ammo_class");
                if (S && S[0])
                {
                    string128 _ammoItem;
                    int       count = _GetItemCount(S);
                    for (int it = 0; it < count; ++it)
                    {
                        _GetItem(S, it, _ammoItem);
                        m_ammoTypes.push_back(_ammoItem);

                        CCartridge cartridge;
                        cartridge.Load(_ammoItem, it);
                        m_AmmoCartidges.push_back(cartridge);
                    }
                }
            }
        }

        bResult = true;
    }

    if (upgrMode == eUpgrNone)
    {
        // В обычном режиме - считывем прямое значение размера магазина из конфига
        iMagazineSize = pSettings->r_s32(section, "ammo_mag_size");
        bResult = true;
    }
    else
    {
        if (pSettings->line_exist(section, "ammo_mag_size"))
        {
            if (upgrMode == eUpgrInstall)
            {
                // В режиме апгрейда - добавляем значение к существующему
                iMagazineSize += pSettings->r_s32(section, "ammo_mag_size");
            }

            bResult = true;
        }
    }
 
    if (bGMode)
        PerformSwitchGL();

    return bResult;
}

// Загружаем параметры магазинного питания
void CWeapon::LoadMagazinesParams(LPCSTR section)
{
    // Данное оружие является магазином
    m_bIsMagazine = READ_IF_EXISTS(pSettings, r_bool, section, "is_magazine", false);

    // Использовать магазины для перезарядки (только игрок)
    if (ParentIsActor())
        m_bUseMagazines = READ_IF_EXISTS(pSettings, r_bool, section, "use_magazines", false);
    else
        m_bUseMagazines = false;

    //--> Патронташ и магазины не могут использоваться одновременно
    R_ASSERT2((m_bUseMagazines && m_bUseAmmoBeltMode) == false, "Using ammo belt together with magazines not allowed!");
}

// Загрузить боевые характеристики пули
void CWeapon::LoadFireParams(LPCSTR section)
{
    // Загружаем предка
    CShootingObject::LoadFireParams(section);

    // Фактор зависимости разброса от состояния оружия
    fireDispersionConditionFactor = pSettings->r_float(section, "fire_dispersion_condition_factor");

    // Дисперсия от бедра в разных позах
    m_pdm.m_fPDM_disp_base          = pSettings->r_float(section, "PDM_disp_base");
    m_pdm.m_fPDM_disp_vel_factor    = pSettings->r_float(section, "PDM_disp_vel_factor");
    m_pdm.m_fPDM_disp_accel_factor  = pSettings->r_float(section, "PDM_disp_accel_factor");
    m_pdm.m_fPDM_disp_crouch        = pSettings->r_float(section, "PDM_disp_crouch");
    m_pdm.m_fPDM_disp_crouch_no_acc = pSettings->r_float(section, "PDM_disp_crouch_no_acc");
};

// Загрузка параметров отдачи оружия
void CWeapon::LoadRecoilParams(LPCSTR section)
{
    //** Общие параметры **//
    float temp_f = 0.0f;

    // Режимы возврата камеры
    u8 rm                 = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return", 1);
    cam_recoil.ReturnMode = (rm == 1);

    rm                    = READ_IF_EXISTS(pSettings, r_u8, section, "cam_return_stop", 0);
    cam_recoil.StopReturn = (rm == 1);

    // Максимальный угол отдачи
    temp_f                  = pSettings->r_float(section, "cam_max_angle");
    cam_recoil.MaxAngleVert = _abs(deg2rad(temp_f));
    VERIFY(!fis_zero(cam_recoil.MaxAngleVert));
    if (fis_zero(cam_recoil.MaxAngleVert))
        cam_recoil.MaxAngleVert = EPS;

    temp_f                  = pSettings->r_float(section, "cam_max_angle_horz");
    cam_recoil.MaxAngleHorz = _abs(deg2rad(temp_f));
    VERIFY(!fis_zero(cam_recoil.MaxAngleHorz));
    if (fis_zero(cam_recoil.MaxAngleHorz))
        cam_recoil.MaxAngleHorz = EPS;

    temp_f                   = pSettings->r_float(section, "cam_step_angle_horz");
    cam_recoil.StepAngleHorz = deg2rad(temp_f);

    //** Отдача от бедра **//
    if (pSettings->line_exist(section, "cam_dispersion"))
    {
        cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "cam_dispersion"));
    }
    else
        cam_recoil.Dispersion = 0.0f;

    if (pSettings->line_exist(section, "cam_dispersion_inc"))
    {
        cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "cam_dispersion_inc"));
    }
    else
        cam_recoil.DispersionInc = 0.0f;

    // Коэфицент отдачи
    cam_recoil.DispersionFrac = _abs(READ_IF_EXISTS(pSettings, r_float, section, "cam_dispersion_frac", 0.7f));

    // Скорость возврата камеры
    temp_f                = pSettings->r_float(section, "cam_relax_speed");
    cam_recoil.RelaxSpeed = _abs(deg2rad(temp_f));

    if (fis_zero(cam_recoil.RelaxSpeed))
        cam_recoil.RelaxSpeed = EPS_L;

    cam_recoil.RelaxSpeed_AI = cam_recoil.RelaxSpeed;
    if (pSettings->line_exist(section, "cam_relax_speed_ai"))
    {
        temp_f                   = pSettings->r_float(section, "cam_relax_speed_ai");
        cam_recoil.RelaxSpeed_AI = _abs(deg2rad(temp_f));

        if (fis_zero(cam_recoil.RelaxSpeed_AI))
            cam_recoil.RelaxSpeed_AI = EPS_L;
    }

    //** Отдача при зуме **//
    zoom_cam_recoil.Dispersion    = cam_recoil.Dispersion;
    zoom_cam_recoil.DispersionInc = cam_recoil.DispersionInc;

    if (pSettings->line_exist(section, "zoom_cam_dispersion"))
    {
        zoom_cam_recoil.Dispersion = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion"));
    }
    if (pSettings->line_exist(section, "zoom_cam_dispersion_inc"))
    {
        zoom_cam_recoil.DispersionInc = deg2rad(pSettings->r_float(section, "zoom_cam_dispersion_inc"));
    }

    zoom_cam_recoil.RelaxSpeed     = cam_recoil.RelaxSpeed;
    zoom_cam_recoil.RelaxSpeed_AI  = cam_recoil.RelaxSpeed_AI;
    zoom_cam_recoil.DispersionFrac = cam_recoil.DispersionFrac;
    zoom_cam_recoil.MaxAngleVert   = cam_recoil.MaxAngleVert;
    zoom_cam_recoil.MaxAngleHorz   = cam_recoil.MaxAngleHorz;
    zoom_cam_recoil.StepAngleHorz  = cam_recoil.StepAngleHorz;
    zoom_cam_recoil.ReturnMode     = cam_recoil.ReturnMode;
    zoom_cam_recoil.StopReturn     = cam_recoil.StopReturn;

    // Скорость возврата камеры
    if (pSettings->line_exist(section, "zoom_cam_relax_speed"))
    {
        zoom_cam_recoil.RelaxSpeed = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed")));

        if (fis_zero(zoom_cam_recoil.RelaxSpeed))
            zoom_cam_recoil.RelaxSpeed = EPS_L;
    }
    if (pSettings->line_exist(section, "zoom_cam_relax_speed_ai"))
    {
        zoom_cam_recoil.RelaxSpeed_AI = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_relax_speed_ai")));

        if (fis_zero(zoom_cam_recoil.RelaxSpeed_AI))
            zoom_cam_recoil.RelaxSpeed_AI = EPS_L;
    }
    if (pSettings->line_exist(section, "zoom_cam_max_angle"))
    {
        zoom_cam_recoil.MaxAngleVert = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle")));
        VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleVert));
        if (fis_zero(zoom_cam_recoil.MaxAngleVert))
            zoom_cam_recoil.MaxAngleVert = EPS;
    }
    if (pSettings->line_exist(section, "zoom_cam_max_angle_horz"))
    {
        zoom_cam_recoil.MaxAngleHorz = _abs(deg2rad(pSettings->r_float(section, "zoom_cam_max_angle_horz")));
        VERIFY(!fis_zero(zoom_cam_recoil.MaxAngleHorz));
        if (fis_zero(zoom_cam_recoil.MaxAngleHorz))
            zoom_cam_recoil.MaxAngleHorz = EPS;
    }
    if (pSettings->line_exist(section, "zoom_cam_step_angle_horz"))
    {
        zoom_cam_recoil.StepAngleHorz = deg2rad(pSettings->r_float(section, "zoom_cam_step_angle_horz"));
    }
    if (pSettings->line_exist(section, "zoom_cam_dispersion_frac"))
    {
        zoom_cam_recoil.DispersionFrac = _abs(pSettings->r_float(section, "zoom_cam_dispersion_frac"));
    }
}

// Параметры зума
void CWeapon::LoadZoomParams(LPCSTR section)
{
    // Разрешён-ли зум
    m_bZoomEnabled = pSettings->r_bool(section, "zoom_enabled");

    // Разрешён-ли альтернативный зум
    m_bAltZoomEnabled = READ_IF_EXISTS(pSettings, r_bool, section, "zoom_alt_enabled", false);

    // Скорость зуминга
    m_fZoomRotateTime = pSettings->r_float(section, "zoom_rotate_time");

    // Скрыть прицел (от бедра)
    m_bHideCrosshair = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "hide_crosshair", false);

    // Скрыть прицел (в зуме)
    m_bHideCrosshairInZoom = READ_IF_EXISTS(pSettings, r_bool, hud_sect, "zoom_hide_crosshair", true);

    // Модификатор для всего прицеливания ("совместимость" с конфигами оригинальной игры)
    m_bUseOldZoomFactor = READ_IF_EXISTS(pSettings, r_bool, section, "zoom_old_mode", true);

    // Основной зум
    GetZoomParams(eZoomMain).Initialize(section);

    // Альтернативный зум
    GetZoomParams(eZoomAlt).Initialize(section, "_alt");

    // <!> Текстура прицельной сетки здесь не создаётся в целях оптимизации - у нас ещё не загружены данные аддонов
    // GetZoomParams().UpdateUIScope();
}

// Вызывается при net_Spawn, после Load
void CWeapon::reload(LPCSTR section)
{
    CShootingObject::reload(section);
    CHudItemObject::reload(section);

    // Параметры оружия на плече
    m_can_be_strapped = true;
    m_strapped_mode   = false;

    if (pSettings->line_exist(section, "strap_bone0"))
        m_strap_bone0 = pSettings->r_string(section, "strap_bone0");
    else
        m_can_be_strapped = false;

    if (pSettings->line_exist(section, "strap_bone1"))
        m_strap_bone1 = pSettings->r_string(section, "strap_bone1");
    else
        m_can_be_strapped = false;

    m_StrapOffset = m_Offset;
    if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation"))
    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "strap_position");
        ypr = pSettings->r_fvector3(section, "strap_orientation");
        ypr.mul(PI / 180.f);

        m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
        m_StrapOffset.translate_over(pos);
    }
    else
        m_can_be_strapped = false;

    // Параметры зрения НПС с одетыми аддонами
    m_addon_holder_range_modifier = m_holder_range_modifier;
    m_addon_holder_fov_modifier   = m_holder_fov_modifier;

    // Позиция оружия в руках от 3-го лица
    {
        Fvector pos, ypr;
        pos = pSettings->r_fvector3(section, "position");
        ypr = pSettings->r_fvector3(section, "orientation");
        ypr.mul(PI / 180.f);

        m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
        m_Offset.translate_over(pos);
    }

    // Эффективный тип оружия (для АИ)
    m_ef_main_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_main_weapon_type", u32(-1));
    m_ef_weapon_type      = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", u32(-1));
}
