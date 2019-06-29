#include "StdAfx.h"
#include "Weapon.h"
#include "Weapon_AmmoCompress.h"

/**********************************/
/***** Ядро оружейного класса *****/ //--#SM+#--
/**********************************/

BOOL b_toggle_weapon_aim = FALSE;

// Конструктор
CWeapon::CWeapon() : CShellLauncher(this)
{
    m_flags.set(FUsingCondition, true);

    SetState(eHidden);
    SetNextState(eHidden);
    SetPending(FALSE);

    m_sub_state            = eSubstateReloadBegin;
    m_bTriStateReload_main = false;
    m_bTriStateReload_gl   = false;
    m_bTriStateReload_ab   = false;
    m_bTriStateReload_frab = false;

    bMisfire = false;

    m_bTriStateReload_anim_hack = false;

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

    m_fZoomRotationFactor = 0.f;
    m_fZoomFovFactorUpgr  = 0.f;
    m_bIdleFromZoomOut    = false;
    m_bIsZoomModeNow      = false;
    m_iCurZoomType        = eZoomMain;
    m_iPrevZoomType       = m_iCurZoomType;

    m_sUseZoomPostprocessUpgr = NULL;
    m_sUseBinocularVisionUpgr = NULL;

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
    m_set_next_ammoType_on_reload   = undefined_ammo_type;
    m_crosshair_inertion            = 0.f;
    m_activation_speed_is_overriden = false;

    m_bRememberActorNVisnStatus = false;
    m_bUIShowAmmo               = true;
    m_bDisableFire              = false;

    m_bKickAtRunActivated          = false;
    m_dw_last_kick_at_run_upd_time = 0;

    m_kicker_main         = NULL;
    m_kicker_alt          = NULL;

    m_bKnifeMode    = false;
    m_first_attack  = NULL;
    m_second_attack = NULL;

    m_dwLastAddonsVisUpdTime = 0;

    m_sSndShotCurrent         = NULL;
    m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

    m_sOverridedRocketSection = NULL;

    m_bFireSingleShot = false;
    m_iShotNum        = 0;
    m_iQueueSize      = WEAPON_DEFAULT_QUEUE;
    m_bLockType       = false;

    m_fInitialBaseBulletSpeed = 0;

    m_bUsePumpMode = false;
    m_bNeed2Pump   = false;

    m_ZoomAnimState            = eZANone;
    m_bDisableFireWhileZooming = false;

    m_bAllowAutoReload          = true;
    m_overridenAmmoForReloadAnm = -1;
    m_bAllowUnload              = true;
    m_bDisableAnimatedReload    = false;

    m_bUseMagazines            = false;
    m_set_next_magaz_on_reload = empty_addon_idx;
    m_set_next_magaz_by_id     = u16(-1);

    m_bUseAmmoBeltMode = false;
    m_AmmoBeltSlot     = eNotExist;
    m_sAB_hud          = NULL;
    m_sAB_vis          = NULL;

    m_ForegripSlot = eNotExist;

    m_ForendSlot = eNotExist;

    m_sBulletHUDVisSect          = NULL;

    m_sAnimatedShellHUDVisSect   = NULL;
    m_sCurAnimatedShellHudVisual = NULL;

    m_sCurShell3DSect            = NULL;

    m_bNeed2StopTriStateReload = false;
    m_bIsReloadFromAB          = false;
    m_bSwitchAddAnimation      = false;

    m_dwABHideBulletVisual = 0;
    m_dwShowAnimatedShellVisual  = 0;

    m_fLR_MovingFactor = 0.f;
    m_fLR_CameraFactor = 0.f;

    m_fLR_InertiaFactor = 0.f;
    m_fUD_InertiaFactor = 0.f;

    m_fLR_ShootingFactor = 0.f;
    m_fUD_ShootingFactor = 0.f;
    m_fBACKW_ShootingFactor = 0.f;

    m_bDisableMovEffAtZoom = false;

    m_bMagaz3pHideWhileReload = false;
    m_bMagaz3pIsHidden        = false;
    m_iMagaz3pHideStartTime   = 0;
    m_iMagaz3pHideEndTime     = 0;

    m_BayonetSlot = eNotExist;

    m_nearwall_last_hud_fov = psHUD_FOV_def;

    m_sHolographBone           = NULL;
    m_fHolographRotationFactor = 1.f;

    sReloadSndSectOverride = nullptr;

    m_bHideCrosshair = false;

    m_bInvShowAmmoCntInMagaz = false;
    m_bInvShowWeaponStats = false;
    m_bInvShowWeaponAmmo = false;

    m_iMinRequiredAmmoInMag = 0;

    bBipodsUseSavedData = false;
}

// Деструктор
CWeapon::~CWeapon()
{
    xr_delete(m_kicker_main);
    xr_delete(m_kicker_alt);
    xr_delete(m_first_attack);
    xr_delete(m_second_attack);
    delete_data(m_AmmoCartidges);
    delete_data(m_AmmoCartidges2);
    delete_data(m_AmmoBeltData);
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

    // Аддоны
    InstallAddon(eScope, E->m_scope_section, true);
    InstallAddon(eMuzzle, E->m_muzzle_section, true);
    InstallAddon(eLauncher, E->m_launcher_section, true);
    InstallAddon(eMagaz, E->m_magaz_section, true);
    InstallAddon(eSpec_1, E->m_spec_1_section, true);
    InstallAddon(eSpec_2, E->m_spec_2_section, true);
    InstallAddon(eSpec_3, E->m_spec_3_section, true);
    InstallAddon(eSpec_4, E->m_spec_4_section, true);
    UpdateAddons(); // <-- Данные из конфигов аддонов незагружены до этого момента <!>

    // Разное
    m_bIdleFromZoomOut = false;

    SetState(E->wpn_state);
    SetNextState(E->wpn_state);

    m_dwWeaponIndependencyTime = 0;
    m_bAmmoWasSpawned          = false;
    SetPending(FALSE);

    // Патроны - Основной ствол
    CAmmoCompressUtil::DecompressMagazine(E->m_pAmmoMain, this, false);
    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    // Патроны - Подствол
    CAmmoCompressUtil::DecompressMagazine(E->m_pAmmoGL, this, true);
    VERIFY((u32)iAmmoElapsed2 == m_magazine2.size());

    // Запускаем Idle-анимацию (иначе модель будет "поломана")
    PlayWorldAnimIdle();

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

    UpdateMagazine3p();
    BipodsOnDestroy();
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
    CAmmoCompressUtil::AMMO_VECTOR pVAmmoMain;
    CAmmoCompressUtil::CompressMagazine(pVAmmoMain, this, false);
    CAmmoCompressUtil::PackAmmoInPacket(pVAmmoMain, P);

    CAmmoCompressUtil::AMMO_VECTOR pVAmmoGL;
    CAmmoCompressUtil::CompressMagazine(pVAmmoGL, this, true);
    CAmmoCompressUtil::PackAmmoInPacket(pVAmmoGL, P);

    // Стэйт и зум
    P.w_u8((u8)GetState());
    P.w_u8((u8)IsZoomed());

    // Текущий режим стрельбы
    P.w_u8(u8(m_iCurFireMode & 0x00ff));

    // clang-format off
	// Аддоны
	P.w_stringZ (( IsScopeAttached() ? GetAddonBySlot(eScope)->GetName() : nullptr ));
	P.w_stringZ (( IsSilencerAttached() ? GetAddonBySlot(eMuzzle)->GetName() : nullptr ));
	P.w_stringZ (( IsGrenadeLauncherAttached() ? GetAddonBySlot(eLauncher)->GetName() : nullptr ));
	P.w_stringZ (( IsMagazineAttached() ? GetAddonBySlot(eMagaz)->GetName() : nullptr ));
	P.w_stringZ (( IsSpecial_1_Attached() ? GetAddonBySlot(eSpec_1)->GetName() : nullptr ));
	P.w_stringZ (( IsSpecial_2_Attached() ? GetAddonBySlot(eSpec_2)->GetName() : nullptr ));
	P.w_stringZ (( IsSpecial_3_Attached() ? GetAddonBySlot(eSpec_3)->GetName() : nullptr ));
	P.w_stringZ (( IsSpecial_4_Attached() ? GetAddonBySlot(eSpec_4)->GetName() : nullptr ));
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
    CAmmoCompressUtil::AMMO_VECTOR pVAmmoMain;
    CAmmoCompressUtil::UnpackAmmoFromPacket(pVAmmoMain, P);
    CAmmoCompressUtil::DecompressMagazine(pVAmmoMain, this, false);

    CAmmoCompressUtil::AMMO_VECTOR pVAmmoGL;
    CAmmoCompressUtil::UnpackAmmoFromPacket(pVAmmoGL, P);
    CAmmoCompressUtil::DecompressMagazine(pVAmmoGL, this, true);

    // Стэйт и зум (1)
    u8 wstate;
    P.r_u8(wstate);

    u8 Zoom;
    P.r_u8((u8)Zoom);

    // Текущий режим стрельбы (1)
    m_iCurFireMode = P.r_u8();
    if (m_iCurFireMode >= m_aFireModes.size())
        m_iCurFireMode = 0;

    // Аддоны
    shared_str sAddonSetSect = nullptr;

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eScope, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eMuzzle, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eLauncher, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eMagaz, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eSpec_1, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eSpec_2, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eSpec_3, sAddonSetSect, true);

    P.r_stringZ(sAddonSetSect);
    InstallAddon(eSpec_4, sAddonSetSect, true);

    UpdateAddons();

    // Стэйт и зум (2)
    if (H_Parent() && H_Parent()->Remote())
    {
        if (Zoom)
            OnZoomIn();
        else
            OnZoomOut();
    };

    // Переключаем подствольник
    if (NewMode != m_bGrenadeMode)
        SwitchGunMode();

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

    save_data(IsZoomed(), output_packet);
    save_data(GetZoomType(), output_packet);
    save_data(GetPrevZoomType(), output_packet);
    save_data(EZoomTypes::eZoomTypesCnt, output_packet);
    for (int i = 0; i < EZoomTypes::eZoomTypesCnt; i++)
    {
        save_data(GetZoomParams((EZoomTypes)i).m_fRTZoomFactor, output_packet);
    }

    save_data(m_bRememberActorNVisnStatus, output_packet);
    save_data(m_iQueueSize, output_packet);
    save_data(m_iShotNum, output_packet);
    save_data(m_iCurFireMode, output_packet);
    save_data(m_bGrenadeMode, output_packet);
    save_data(bMisfire, output_packet);

    BipodsOnSave(output_packet);
}

// Загрузка данных клиентского объекта
void CWeapon::load(IReader& input_packet)
{
    inherited::load(input_packet);

    bool       bGL           = false;
    EZoomTypes iCurZoomType  = eZoomMain;
    EZoomTypes iPrevZoomType = iCurZoomType;
    int        iZoomTypesCnt = 0;

    load_data(m_bIsZoomModeNow, input_packet);
    load_data(iCurZoomType, input_packet);
    load_data(iPrevZoomType, input_packet);
    load_data(iZoomTypesCnt, input_packet);
    for (int i = 0; i < EZoomTypes::eZoomTypesCnt && i < iZoomTypesCnt; i++)
    {
        load_data(GetZoomParams((EZoomTypes)i).m_fRTZoomFactor, input_packet);
    }

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

    SwitchZoomType(iCurZoomType, iPrevZoomType);

    if (IsZoomed())
        OnZoomIn();
    else
        OnZoomOut();

    BipodsOnLoad(input_packet);
}

// Сброс состояния оружия в дефолт
void CWeapon::ResetWeapon(shared_str const& sCallerName)
{
    /* Текущие возможные места вызова \ значения sCallerName
       - OnH_B_Independent       --> Перед выкидыванием из инвентаря
       - OnH_B_Chield            --> Перед добавлением в инвентарь
       - signal_HideComplete     --> По завершению прятанья оружия
    */

    UndeployBipods(true);
    OnZoomOut(true);
    StopAllEffects(); //--> Light + Particles + Effectors

    m_BriefInfo_CalcFrame = 0;

    m_nearwall_last_hud_fov = psHUD_FOV_def;

    m_fLR_MovingFactor = 0.f;
    m_fLR_CameraFactor = 0.f;

    m_fLR_InertiaFactor = 0.f;
    m_fUD_InertiaFactor = 0.f;

    m_fLR_ShootingFactor = 0.f;
    m_fUD_ShootingFactor = 0.f;
    m_fBACKW_ShootingFactor = 0.f;

    m_fZoomRotationFactor = 0.0f;

    m_set_next_ammoType_on_reload = undefined_ammo_type;
    m_set_next_magaz_on_reload = empty_addon_idx;
    m_set_next_magaz_by_id = u16(-1);

    m_sub_state = eSubstateReloadBegin;

    SetPending(FALSE);

    if (sCallerName.equal("OnH_B_Chield") == false)
    {
        UpdateHUDAddonsVisibility(true); //--> Сбрасываем все аддоны \ кости худовой модели
    }
}
