#include "stdafx.h"
#include "Weapon_Shared.h"

/**********************************/
/***** Ядро оружейного класса *****/ //--#SM+#--
/**********************************/

BOOL    b_toggle_weapon_aim = FALSE;
CUIXml* pWpnScopeXml        = NULL;

ENGINE_API bool g_dedicated_server;

// Конструктор
CWeapon::CWeapon() : CShellLauncher(this)
{
    SetState(eHidden);
    SetNextState(eHidden);
    SetDefaults();

    m_sub_state            = eSubstateReloadBegin;
    m_bTriStateReload_main = false;
    m_bTriStateReload_gl   = false;
    m_bTriStateReload_ab   = false;
    m_bTriStateReload_frab = false;

    m_Offset.identity();
    m_StrapOffset.identity();

    m_iAmmoCurrentTotal   = 0;
    m_BriefInfo_CalcFrame = 0;

    iAmmoElapsed   = 0;
    iMagazineSize  = 0;
    iAmmoElapsed2  = 0;
    iMagazineSize2 = 0;
    m_ammoType     = 0;
    m_ammoType2    = 0;

    m_magazine.clear();
    m_magazine2.clear();
    m_ammoTypes.clear();
    m_ammoTypes2.clear();
    m_AmmoCartidges.clear();
    m_AmmoCartidges2.clear();

    m_bGrenadeMode = false;

    eHandDependence = hdNone;

    m_zoom_params.m_fCurrentZoomFactor  = g_fov;
    m_zoom_params.m_fZoomRotationFactor = 0.f;
    m_zoom_params.m_pVision             = NULL;
    m_zoom_params.m_pNight_vision       = NULL;
    m_bIdleFromZoomOut                  = false;

    m_pCurrentAmmo = NULL;

    m_pFlameParticles2 = NULL;
    m_sFlameParticles2 = NULL;

    m_fCurrentCartirdgeDisp = 1.f;

    m_strap_bone0 = 0;
    m_strap_bone1 = 0;
    m_StrapOffset.identity();
    m_strapped_mode                 = false;
    m_can_be_strapped               = false;
    m_ef_main_weapon_type           = u32(-1);
    m_ef_weapon_type                = u32(-1);
    m_UIScope                       = NULL;
    m_set_next_ammoType_on_reload   = undefined_ammo_type;
    m_crosshair_inertion            = 0.f;
    m_activation_speed_is_overriden = false;

    m_bRememberActorNVisnStatus = false;
    m_bUIShowAmmo               = true;
    m_bDisableFire              = false;

    m_bKickAtRun          = false;
    m_dw_last_kick_at_run = 0;
    m_kicker_main         = NULL;
    m_kicker_alt          = NULL;

    m_bKnifeMode    = false;
    m_first_attack  = NULL;
    m_second_attack = NULL;

    m_dwAddons_last_upd_time = 0;

    m_sounds_enabled = true;

    m_sSndShotCurrent         = NULL;
    m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

    m_sOverridedRocketSection = NULL;

    m_bFireSingleShot = false;
    m_iShotNum        = 0;
    m_fOldBulletSpeed = 0;
    m_iQueueSize      = WEAPON_DEFAULT_QUEUE;
    m_bLockType       = false;

    m_bUsePumpMode = false;
    m_bNeed2Pump   = false;

    m_ZoomAnimState            = eZANone;
    m_bDisableFireWhileZooming = false;

    m_bAllowAutoReload          = true;
    m_overridenAmmoForReloadAnm = -1;
    m_bAllowUnload              = true;

    m_bUseMagazines            = false;
    m_set_next_magaz_on_reload = empty_addon_idx;
    m_set_next_magaz_by_id     = u16(-1);

    m_bUseAmmoBeltMode = false;
    m_AmmoBeltSlot     = eNotExist;
    m_sAB_hud          = NULL;
    m_sAB_vis          = NULL;

    m_ForegripSlot = eNotExist;

    m_ForendSlot = eNotExist;

    m_sBulletVisual      = NULL;
    m_sShellVisual       = NULL;
    m_sCurrentShellModel = NULL;

    m_bNeed2StopTriStateReload = false;
    m_bIsReloadFromAB          = false;
    m_bSwitchAddAnimation      = false;

    m_dwHideBulletVisual = 0;
    m_dwShowShellVisual  = 0;

    m_fLR_MovingFactor = 0.f;

    m_bDisableMovEffAtZoom = false;
}

// Деструктор
CWeapon::~CWeapon()
{
    xr_delete(m_UIScope);
    xr_delete(m_kicker_main);
    xr_delete(m_kicker_alt);
    xr_delete(m_first_attack);
    xr_delete(m_second_attack);
    delete_data(m_AmmoCartidges);
    delete_data(m_AmmoCartidges2);
    delete_data(m_AmmoBeltData);
}

// Установка дефолтного состояния у оружия
void CWeapon::SetDefaults()
{
    m_flags.set(FUsingCondition, TRUE);
    SetPending(FALSE);
    bMisfire                       = false;
    m_zoom_params.m_bIsZoomModeNow = false;
    m_bIdleFromZoomOut             = false;
}

// Появление в онлайне
BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
    CSE_ALifeItemWeapon* const E = smart_cast<CSE_ALifeItemWeapon*>(DC);
    R_ASSERT(E);

    if (IsGameTypeSingle())
    {
        inherited::net_Spawn_install_upgrades(E->m_upgrades);
    }

    // Разное
    m_fRTZoomFactor    = m_zoom_params.m_fScopeZoomFactor;
    m_bIdleFromZoomOut = false;

    SetState(E->wpn_state);
    SetNextState(E->wpn_state);

    m_dwWeaponIndependencyTime = 0;
    m_bAmmoWasSpawned          = false;
    SetPending(FALSE);

    // Аддоны
    InstallAddon(eScope, E->m_scope_idx, true);
    InstallAddon(eMuzzle, E->m_muzzle_idx, true);
    InstallAddon(eLauncher, E->m_launcher_idx, true);
    InstallAddon(eMagaz, E->m_magaz_idx, true);
    InstallAddon(eSpec_1, E->m_spec_1_idx, true);
    InstallAddon(eSpec_2, E->m_spec_2_idx, true);
    InstallAddon(eSpec_3, E->m_spec_3_idx, true);
    InstallAddon(eSpec_4, E->m_spec_4_idx, true);

    UpdateAddons();

    // Патроны - Основной ствол
    SetAmmoElapsedFor(E->a_elapsed, false);
    SetAmmoTypeSafeFor(E->ammo_type, false);
    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    // Патроны - Подствол
    SetAmmoElapsedFor(E->a_elapsed_2, true);
    SetAmmoTypeSafeFor(E->ammo_type_2, true);
    VERIFY((u32)iAmmoElapsed2 == m_magazine2.size());

    // TODO: Переделать SM_TODO
    if (pSettings->line_exist(this->cNameSect_str(), "anm_world_idle"))
    {
        IKinematicsAnimated* pWeaponVisual = Visual()->dcast_PKinematicsAnimated();
        if (pWeaponVisual)
        {
            // Msg("Animation [%s] played for [%s]", pSettings->r_string(this->cNameSect_str(), "anm_world_idle"), this->cNameSect_str());
            pWeaponVisual->PlayCycle(pSettings->r_string(this->cNameSect_str(), "anm_world_idle"), TRUE);
        }
    }

    return inherited::net_Spawn(DC);
}

// Уход из онлайна
void CWeapon::net_Destroy()
{
    inherited::net_Destroy();

    // Удалить объекты партиклов
    StopFlameParticles();
    StopFlameParticles2();
    StopLight();
    Light_Destroy();

    m_overridenAmmoForReloadAnm = -1;
    m_bNeed2Pump                = false;

    while (m_magazine.size())
        m_magazine.pop_back();
    while (m_magazine2.size())
        m_magazine2.pop_back();
}

// Экспорт данных в серверный объект и по сети
void CWeapon::net_Export(NET_Packet& P)
{
    inherited::net_Export(P);

    // Состояние оружия
    P.w_float_q8(GetCondition(), 0.0f, 1.0f);

    // Флаги
    u8 need_upd = IsUpdating() ? 1 : 0;
    P.w_u8(need_upd);

    // Режим подствольника
    P.w_u8(m_bGrenadeMode ? 1 : 0);

    // Патроны в основном стволе и подстволе
    if (!m_bGrenadeMode)
    {
        P.w_u16(u16(iAmmoElapsed));
        P.w_u8(m_ammoType);
        P.w_u16(u16(iAmmoElapsed2));
        P.w_u8(m_ammoType2);
    }
    else
    {
        P.w_u16(u16(iAmmoElapsed2));
        P.w_u8(m_ammoType2);
        P.w_u16(u16(iAmmoElapsed));
        P.w_u8(m_ammoType);
    }

    // Стэйт и зум
    P.w_u8((u8)GetState());
    P.w_u8((u8)IsZoomed());

    // Текущий режим стрельбы
    P.w_u8(u8(m_iCurFireMode & 0x00ff));

    // clang-format off
	// Аддоны
	P.w_u8 (m_addons[eScope].addon_idx);
	P.w_u8 (m_addons[eMuzzle].addon_idx);
	P.w_u8 (m_addons[eLauncher].addon_idx);
	P.w_u8 (m_addons[eMagaz].addon_idx);
	P.w_u8 (m_addons[eSpec_1].addon_idx);
	P.w_u8 (m_addons[eSpec_2].addon_idx);
	P.w_u8 (m_addons[eSpec_3].addon_idx);
	P.w_u8 (m_addons[eSpec_4].addon_idx);
    // clang-format on
}

// Импорт данных из серверного объекта и сети
void CWeapon::net_Import(NET_Packet& P)
{
    inherited::net_Import(P);

    // Состояние оружия
    float _cond;
    P.r_float_q8(_cond, 0.0f, 1.0f);
    SetCondition(_cond);

    // Флаги
    u8 flags = 0;
    P.r_u8(flags);

    // Режим подствольника
    bool NewMode = FALSE;
    NewMode      = !!P.r_u8();

    // Патроны в основном стволе и подстволе
    u16 ammo_elapsed, ammo_elapsed2 = 0;
    u8  ammoType, ammoType2;

    P.r_u16(ammo_elapsed);
    P.r_u8(ammoType);
    P.r_u16(ammo_elapsed2);
    P.r_u8(ammoType2);

    // Стэйт и зум (1)
    u8 wstate;
    P.r_u8(wstate);

    u8 Zoom;
    P.r_u8((u8)Zoom);

    // Текущий режим стрельбы (1)
    m_iCurFireMode = P.r_u8();
    if (m_iCurFireMode >= m_aFireModes.size())
        m_iCurFireMode = 0;

    // clang-format off
	// Аддоны
	u8 idx = u8(-1);
	P.r_u8(idx); InstallAddon(eScope,	idx, true);
	P.r_u8(idx); InstallAddon(eMuzzle,	idx, true);
	P.r_u8(idx); InstallAddon(eLauncher, idx, true);
	P.r_u8(idx); InstallAddon(eMagaz,	idx, true);
	P.r_u8(idx); InstallAddon(eSpec_1,	idx, true);
	P.r_u8(idx); InstallAddon(eSpec_2,	idx, true);
	P.r_u8(idx); InstallAddon(eSpec_3,	idx, true);
	P.r_u8(idx); InstallAddon(eSpec_4,	idx, true);
    // clang-format on

    UpdateAddons();

    // Стэйт и зум (2)
    if (H_Parent() && H_Parent()->Remote())
    {
        if (Zoom)
            OnZoomIn();
        else
            OnZoomOut();
    };
    switch (wstate)
    {
    case eFire:
    case eFire2:
    case eSwitch:
    case eReload: {
    }
    break;
    default:
    {
        SetAmmoElapsedFor(ammo_elapsed, false);
        SetAmmoTypeSafeFor(ammoType, false);
        SetAmmoElapsedFor(ammo_elapsed2, true);
        SetAmmoTypeSafeFor(ammoType2, true);
    }
    break;
    }

    // Переключаем подствольник
    if (NewMode != m_bGrenadeMode)
        SwitchMode();

    // Текущий режим стрельбы (2)
    SetQueueSize(GetCurrentFireMode());
}

// Ре-инициализация
void CWeapon::reinit()
{
    CShootingObject::reinit();
    CHudItemObject::reinit();
}

// Сохранение данных клиентского объекта
void CWeapon::save(NET_Packet& output_packet)
{
    inherited::save(output_packet);

    save_data(m_zoom_params.m_bIsZoomModeNow, output_packet);
    save_data(m_bRememberActorNVisnStatus, output_packet);
    save_data(m_iQueueSize, output_packet);
    save_data(m_iShotNum, output_packet);
    save_data(m_iCurFireMode, output_packet);
    save_data(m_bGrenadeMode, output_packet);
    save_data(bMisfire, output_packet);
}

// Загрузка данных клиентского объекта
void CWeapon::load(IReader& input_packet)
{
    inherited::load(input_packet);

    bool bGL = false;

    load_data(m_zoom_params.m_bIsZoomModeNow, input_packet);
    load_data(m_bRememberActorNVisnStatus, input_packet);
    load_data(m_iQueueSize, input_packet);
    load_data(m_iShotNum, input_packet);
    load_data(m_iCurFireMode, input_packet);
    load_data(bGL, input_packet);
    load_data(bMisfire, input_packet);

    if (bGL != m_bGrenadeMode)
        PerformSwitchGL();

    SetQueueSize(m_iQueueSize);

    if (m_iCurFireMode >= m_aFireModes.size())
        m_iCurFireMode = 0;

    if (m_zoom_params.m_bIsZoomModeNow)
        OnZoomIn();
    else
        OnZoomOut();
}