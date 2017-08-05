#include "stdafx.h"
#include "Weapon_Shared.h"

/**********************************************/
/***** Различные колбэки, экшены и эвенты *****/ //--#SM+#--
/**********************************************/

// Отлов и реакция на эвенты
void CWeapon::OnEvent(NET_Packet& P, u16 type)
{
    u16 id;

    switch (type)
    {
    case GE_ADDON_CHANGE: // MP
    {
        u8 m_flagsAddOnState;
        P.r_u8(m_flagsAddOnState);
        SetAddonsState(m_flagsAddOnState);
        UpdateAddons();
        UpdateAddonsAnim();
    }
    break;

    case GE_WPN_STATE_CHANGE:
    {
        u8 state;
        P.r_u8(state);
        P.r_u8(m_sub_state);
        //			u8 NewAmmoType =
        P.r_u8();
        u8 AmmoElapsed = P.r_u8();
        P.r_u8(m_set_next_ammoType_on_reload);
        P.r_u8(m_set_next_magaz_on_reload);
        P.r_u16(m_set_next_magaz_by_id);

        if (OnClient())
            SetAmmoElapsed(int(AmmoElapsed));
        OnStateSwitch(u32(state));
    }
    break;

    case GE_OWNERSHIP_TAKE: // <!> Bullet shells, rockets\grenades
    {
        P.r_u16(id);
        CShellLauncher::RegisterShell(id, this);
        CRocketLauncher::AttachRocket(id, this);
    }
    break;

    case GE_OWNERSHIP_REJECT:
    case GE_LAUNCH_ROCKET:
    {
        bool bLaunch = (type == GE_LAUNCH_ROCKET);
        P.r_u16(id);
        CRocketLauncher::DetachRocket(id, bLaunch);
        if (bLaunch)
        {
            OnRocketLaunch(id);
        }
    }
    break;

    default: { inherited::OnEvent(P, type);
    }
    break;
    }
}

// Отлов действий (нажатий клавиш)
bool CWeapon::Action(u16 cmd, u32 flags)
{
    if (inherited::Action(cmd, flags))
        return true;

    // Режим ножа
    if (m_bKnifeMode)
    {
        switch (cmd)
        {
        case kWPN_FIRE: //--> Основной
        {
            if (flags & CMD_START)
                Try2Knife(false);
        }
            return true;
        case kWPN_ZOOM: //--> Альтернативный
        {
            if (flags & CMD_START)
                Try2Knife(true);
        }
            return true;
        }
    }

    // Если оружие не должно стрелять
    if (m_bDisableFire)
        if (cmd == kWPN_FIRE)
            cmd = kWPN_ZOOM;

    switch (cmd)
    {
    ///////////////////////////////////////
    case kWPN_FIRE: //--> Стрельба
    {
        // Если сейчас активна перезарядка в три стадии, то прерываем её
        if (flags & CMD_START)
            if (Try2StopTriStateReload())
                return true;

        // Стреляем
        if (flags & CMD_START)
            return Try2Fire();
        else if (m_bNeed2Pump == false && GetCurrentFireMode() == WEAPON_ININITE_QUEUE)
            Need2Stop_Fire();
    }
        return true;
    ///////////////////////////////////////
    case kWPN_KICK: //--> Удар прикладом
    {
        if (flags & CMD_START)
            Try2Kick();
    }
        return true;
    ///////////////////////////////////////
    case kWPN_BIPODS: //--> Установка\Снятие сошек
    {
        if (flags & CMD_START)
            Try2DeployBipods();
    }
        return true;
    ///////////////////////////////////////
    case kWPN_ZOOM: //--> Прицеливание
    {
        if (IsBipodsDeployed())
        {
            // Зум для сошек
            BipodsZoom(flags);
        }
        else
        {
            // Обычный зум
            if (IsZoomEnabled())
            {
                if (b_toggle_weapon_aim)
                {
                    // Режим "Нажать раз"
                    if (flags & CMD_START)
                    {
                        if (!IsZoomed())
                        {
                            if (!IsPending())
                                OnZoomIn();
                        }
                        else
                            OnZoomOut();
                    }
                }
                else
                {
                    // Режим "Пока зажато"
                    if (flags & CMD_START)
                    {
                        if (!IsZoomed() && !IsPending())
                            OnZoomIn();
                    }
                    else
                        OnZoomOut();
                }
            }
        }
    }
        return true;
    ///////////////////////////////////////
    case kWPN_ZOOM_INC: //--> Увеличение\уменьшение зума
    case kWPN_ZOOM_DEC:
    {
        if (flags & CMD_START)
        {
            if (IsZoomEnabled() && IsZoomed())
            {
                if (cmd == kWPN_ZOOM_INC)
                    ZoomInc();
                else
                    ZoomDec();
            }
        }
    }
        return true;
    ///////////////////////////////////////
    case kWPN_NEXT: //--> Смена типа патронов
    {
        if (flags & CMD_START)
            SwitchAmmoType();
    }
        return true;
    ///////////////////////////////////////
    case kWPN_RELOAD: //--> Перезарядка
    {
        if (flags & CMD_START)
        {
            // Перезарядка патронташа
            if (m_bUseAmmoBeltMode)
            {
                if (GetMainAmmoElapsed() == GetMainMagSize())
                    if (Try2ReloadAmmoBelt())
                        return true;
            }

            // Обычная перезарядка
            Try2Reload();
        }
    }
        return true;
    ///////////////////////////////////////
    case kWPN_FIREMODE_PREV: //--> Следующий режим стрельбы
    {
        if (flags & CMD_START)
            OnPrevFireMode();
    }
        return true;
    ///////////////////////////////////////
    case kWPN_FIREMODE_NEXT: //--> Предыдущий режим стрельбы
    {
        if (flags & CMD_START)
            OnNextFireMode();
    }
        return true;
    ///////////////////////////////////////
    case kWPN_FUNC: //--> Переключение основной ствол\подствол
    {
        if (flags & CMD_START)
        {
            if (!m_bUseAmmoBeltMode)
                Try2Switch();
            else
                Try2ReloadFrAB();
        }
    }
        return true;
    }

    return false;
}

// Колбэк ПЕРЕД детачем объекта от родителя (выкидывание оружия из инвенторя)
void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);

    StopAllEffects();
    Need2Stop_Fire();

    SetPending(FALSE);
    SwitchState(eHidden);

    m_strapped_mode                = false;
    m_zoom_params.m_bIsZoomModeNow = false;

    UpdateXForm();
}

// Колбек ПОСЛЕ детача объекта от старого родителя, новый родитель уже указан
void CWeapon::OnH_A_Independent()
{
    inherited::OnH_A_Independent();

    m_dwWeaponIndependencyTime = Level().timeServer();

    SetQueueSize(WEAPON_ININITE_QUEUE);
    Light_Destroy();
    UpdateAddonsVisibility();
};

// Колбэк ПЕРЕД аттачем объекта к новому родителю
void CWeapon::OnH_B_Chield()
{
    inherited::OnH_B_Chield();

    OnZoomOut();

    m_dwWeaponIndependencyTime    = 0;
    m_set_next_ammoType_on_reload = undefined_ammo_type;
    m_set_next_magaz_on_reload    = empty_addon_idx;
    m_set_next_magaz_by_id        = u16(-1);
}

// Колбэк ПОСЛЕ аттача объекта к новому родителю, новый родитель уже указан
void CWeapon::OnH_A_Chield()
{
    inherited::OnH_A_Chield();

    CActor* actor = smart_cast<CActor*>(H_Parent());
    if (actor)
        SetQueueSize(GetCurrentFireMode());
    else
        SetQueueSize(WEAPON_ININITE_QUEUE); //--> Для НПС оружие всегда стреляет очередью

    // Обновляем видимость аддонов
    UpdateAddonsVisibility();

    // Обновляем параметры магазинного питания
    LoadMagazinesParams(cNameSect_str());
};

// Колбек на активацию оружия (взяли в руки)
void CWeapon::OnActiveItem()
{
    inherited::OnActiveItem();

    m_BriefInfo_CalcFrame = 0;

    UpdateAddonsVisibility();
    SwitchState(eShowing);
}

// Колбек на процесс прятанья оружия
void CWeapon::OnHiddenItem()
{
    inherited::OnHiddenItem();

    m_BriefInfo_CalcFrame = 0;

    if (IsGameTypeSingle())
        SwitchState(eHiding);
    else
        SwitchState(eHidden);

    OnZoomOut();

    m_set_next_ammoType_on_reload = undefined_ammo_type;
    m_set_next_magaz_on_reload    = empty_addon_idx;
    m_set_next_magaz_by_id        = u16(-1);
}

// Колбек "Оружие спрятанно"
void CWeapon::signal_HideComplete()
{
    if (H_Parent())
        setVisible(FALSE);
    SetPending(FALSE);
}

// Колбэк на хит по оружию
void CWeapon::Hit(SHit* pHDS) { inherited::Hit(pHDS); }

// Апдейт с интервалами
void CWeapon::shedule_Update(u32 dT)
{
    // Обновляем перегрев
    UpdateOverheat(dT);

    // Учитываем перегрев при расчёте теплового излучения оружия
    m_common_values.m_fIRNV_value += m_overheat * 1.5;

    // Вызываем обновление у предков
    inherited::shedule_Update(dT);

    // Раз в пол секунды (без учёта частоты вызовов функции) обновляем визуалы установленных аддонов
    if ((Device.dwTimeGlobal - m_dwAddons_last_upd_time) >= 500)
    {
        UpdateAddonsVisibility();
    }

    // Обновляем видимость гранаты
    UpdateGrenadeVisibility();
}

// Быстрый апдейт
void CWeapon::UpdateCL()
{
    inherited::UpdateCL();

    // Подсветка от выстрела
    UpdateLight();

    // Обновляем таймер скорострельности оружия вне стэйта стрельбы
    if (GetState() != eFire)
    {
        fShotTimeCounter -= Device.fTimeDelta;
        clamp(fShotTimeCounter, 0.0f, flt_max);
    }

    // Нарисовать партиклы
    UpdateFlameParticles();
    UpdateFlameParticles2();

    // Интерполяция (MP)
    if (!IsGameTypeSingle())
        make_Interpolation();

    if ((GetNextState() == GetState()) && IsGameTypeSingle() && H_Parent() == Level().CurrentEntity())
    {
        CActor* pActor = smart_cast<CActor*>(H_Parent());
        if (pActor && !pActor->AnyMove() && this == pActor->inventory().ActiveItem())
        {
            if (hud_adj_mode == 0 && GetState() == eIdle && (Device.dwTimeGlobal - m_dw_curr_substate_time > 20000) && !IsZoomed() &&
                g_player_hud->attached_item(1) == NULL)
            {
                Try2Bore();
                ResetSubStateTime();
            }
        }
    }

    if (m_zoom_params.m_pNight_vision && !need_renderable())
    {
        if (!m_zoom_params.m_pNight_vision->IsActive())
        {
            CActor* pA = smart_cast<CActor*>(H_Parent());
            R_ASSERT(pA);
            CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
            if (pTorch && pTorch->GetNightVisionStatus())
            {
                m_bRememberActorNVisnStatus = pTorch->GetNightVisionStatus();
                pTorch->SwitchNightVision(false, false);
            }
            m_zoom_params.m_pNight_vision->Start(m_zoom_params.m_sUseZoomPostprocess, pA, false);
        }
    }
    else if (m_bRememberActorNVisnStatus)
    {
        m_bRememberActorNVisnStatus = false;
        EnableActorNVisnAfterZoom();
    }

    if (m_zoom_params.m_pVision != NULL && IsZoomed() && !IsRotatingToZoom())
        m_zoom_params.m_pVision->Update();

    UpdateStates(Device.fTimeDelta);
    UpdateSounds();

    // Запускаем по одной гранате за каждый апдейт
    if (getRocketCount() > 0)
        LaunchGrenade();

    // Обновляем патронташ
    UpdateAmmoBelt();

    // Обновляем патрон во время перезарядки
    UpdateBulletVisual();

    // Обновляем отрисовку гильзы
    UpdateShellVisual();

    // Обновляем активные визуалы
    UpdateWpnVisuals();

    // Обновляем сошки
    UpdateBipods();
}
