/***********************************/
/***** Анимации и звуки оружия *****/ //--#SM+#--
/***********************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "WeaponKnifeHit.h"
#include "player_hud.h"

// Колбэк на конец анимации
void CWeapon::OnAnimationEnd(u32 state)
{
    // Сбрасываем флаг анимации зума
    bool bIsFromZoom = (m_ZoomAnimState != eZANone);
    m_ZoomAnimState  = eZANone;

    // В режиме ножа стрельбу обрабатываем отдельно
    if (m_bKnifeMode == true)
    {
        switch (state)
        {
        case eFire:
        case eFire2: Need2Idle(); return;
        }
    }

    // Смена магазина
    if (state == eSwitchMag)
    {
        Need2SwitchMag();
        return;
    }

    // Перезарядку в три стадии обрабатываем отдельно
    if (IsTriStateReload() && state == eReload)
    {
        switch (m_sub_state)
        {
        case eSubstateReloadBegin:
        {
            if (m_bNeed2StopTriStateReload)
            {
                Need2Idle();
                return;
            }
            m_sub_state = eSubstateReloadInProcess;
            Need2Reload();
        }
        break;

        case eSubstateReloadInProcess:
        {
            if (m_bSwitchAddAnimation == false)
            {
                u8 cnt = (m_bIsReloadFromAB ? AddCartridgeFrAB(1) : AddCartridge(1));
                if (m_bNeed2StopTriStateReload || (0 != cnt))
                    m_sub_state = eSubstateReloadEnd;
            }
            Need2Reload();
        }
        break;

        case eSubstateReloadEnd:
        {
            m_sub_state = eSubstateReloadBegin;
            Need2Stop_Reload();
        }
        break;
        };
        return;
    }

    // Обрабатываем всё остальное
    switch (state)
    {
    case eReload: // End of reload animation
    {
        if (!m_bIsReloadFromAB)
            ReloadMagazine();
        else
            ReloadMagazineFrAB();
        Need2Stop_Reload();
        break;
    }
    case eSwitch:
        Need2Idle();
        break; // Switch Main\GL
    case eHiding:
        SwitchState(eHidden);
        break; // End of Hide
    case eShowing:
        Need2Idle();
        break; // End of Show
    case ePump:
        m_bNeed2Pump = false;
        Need2Idle();
        break; // End of Pump
    case eIdle:
        if (GetNextState() == eIdle)
            Need2Idle();
        break; // Keep showing idle
    case eKick:
        if (m_bKickAtRunActivated)
        { // Атака на бегу должна быть зациклена
            PlayAnimKickAlt();
        }
        else
        { // Обычная атака нет
            Need2Stop_Kick();
        }
        break; // Stop kicking
    case eFire:
        Try2Pump();
        break; // Pump
    default: inherited::OnAnimationEnd(state);
    }

    // Для фикса ситуации когда мы стреляем и сразу целимся (idle-анимация не запускается)
    if (bIsFromZoom && state == eFire)
        Need2Idle();
}

// Колбэк на метки в анимация
void CWeapon::OnMotionMark(u32 state, const motion_marks& M)
{
    CWeaponKnifeHit* hitObj = NULL;

    switch (state)
    {
    // Смена магазина
    case eSwitchMag:
        if (m_sub_state != eSubstateMagazFinish)
        {
            // Остановим звук перезарядки
            StopReloadSoundsOnMagazChanged();

            // Передадим управление в стэйт SwitchMag
            Need2SwitchMag();
        }
        break;
    // Перезарядка помпы (вылет гильзы)
    case ePump:
        m_bNeed2Pump = false;
        LaunchShell2D();
        break;
    // Удар прикладом (момент удара)
    case eKick:
        KickHit(false);
        break;
    // Удары ножом
    case eFire: //--> Основной
        hitObj = m_first_attack;
        break;
    case eFire2: //--> Альтернативный
        hitObj = m_second_attack;
        break;
    default: inherited::OnMotionMark(state, M);
    }

    if (m_bKnifeMode == true && hitObj != NULL)
    {
        Fvector p1, d;
        p1.set(get_LastFP());
        d.set(get_LastFD());

        if (H_Parent())
        {
            smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p1, d);
            hitObj->KnifeStrike(p1, d);
            PlaySound("sndKnife", p1);
        }
    }
}

// Вызывается перед проигрыванием любой анимации.
// Возвращает необходимость проиграть анимацию не с начала, а с первой метки внутри неё.
void CWeapon::OnBeforeMotionPlayed(const shared_str& sAnmAlias, motion_params& params)
{
    xr_string sAnmAliasStr = sAnmAlias.c_str();
  
    // Ускоряем анимацию атаки ножом
    if (m_bKnifeMode && sAnmAliasStr.find("anm_attack") != xr_string::npos)
    {
        params.fSpeed = m_fKnifeSpeedMod;
        return; //--> Не модифицируем дальше (оптимизация)
    }

    // Для анимации зума управляем её стартовой секундой и направлением движения
    if (m_ZoomAnimState != eZANone)
    {
        //-> Стартовая секунда анимации зависит от того сколько оружие уже успело повернуться
        params.fStartFromTime = (GetZRotatingFactor() * -1); //--> -1 для указания времени в процентах от всей длины анимации

        //--> Если мы выходим из зума - пускаем анимацию реверсивно
        if (m_ZoomAnimState == eZAOut)
            params.fSpeed = -1.0f;

        return; //--> Не модифицируем дальше (оптимизация)
    }

    // Ускоряем анимации перезарядки и их звук
    if (GetState() == eReload || GetState() == eReloadFrAB || GetState() == eSwitchMag)
    {
        params.fSpeed = (bIsGrenadeMode() ? m_fReloadSpeedModGL : m_fReloadSpeedModMain);
        params.fSndFreq = (bIsGrenadeMode() ? m_fReloadSndFreqModGL : m_fReloadSndFreqModMain);
    }

    // Управляем стартовой секундой анимации смены магазина (для оружия с магазинным питанием)
    if (GetState() == eSwitchMag && m_sub_state == eSubstateMagazFinish)
    {
        //--> При осечке отыгрываем анимацию до конца
        if (IsMisfire())
            return; //--> Не модифицируем дальше (оптимизация)

        //--> Иначе высчитываем время старта анимации из конфига или метки
        if (g_player_hud != NULL)
        {
            SAddonData* pAddonMagaz = GetAddonBySlot(eMagaz);
            if (pAddonMagaz->bActive)
            { //--> Магазин уже установлен
                params.fStartFromTime = READ_IF_EXISTS(pSettings, r_float, pAddonMagaz->GetName(), "insert_anim_start_time", params.fStartFromTime);
                params.bTakeTimeFromMotionMark = READ_IF_EXISTS(pSettings, r_bool, pAddonMagaz->GetName(), "insert_anim_start_time_from_anim", false);
            }
            else
                params.bTakeTimeFromMotionMark = true; //--> Магазина нет, значит мы его только что сняли => проигрываем анимацию с метки
        }

        return; //--> Не модифицируем дальше (оптимизация)
    }

    inherited::OnBeforeMotionPlayed(sAnmAlias, params);
}

// Вызывается при проигрывании первой анимации после каждого появления худа
void CWeapon::OnFirstAnimationPlayed(const shared_str& sAnmAlias)
{
    UpdateHUDAddonsVisibility();

    inherited::OnFirstAnimationPlayed(sAnmAlias);
}

// Проиграть анимацию со звуком (а также учитывая число патронов в основном магазине)
bool CWeapon::PlaySoundMotion(const shared_str& sAnmAlias, BOOL bMixIn, LPCSTR sSndAlias, bool bAssert, int anim_idx)
{
    bool bFound = false;
    bool bIsHUDPresent = (HudItemData() != NULL);
    string256 sAnm;

    // Ищем альтернативные варианты анимации
    do
    {
        int idx;
        if (anim_idx != -1)
            idx = anim_idx;
        else
            idx = GetMainAmmoElapsed();

        // Ищем анимацию с привязкой к точному числу патронов\индексу
        //--> Старый формат (поддержка совместимости для двухстволок из оригинальной игры)
        xr_sprintf(sAnm, "%s_%d", sAnmAlias.c_str(), idx);
        if (isHUDAnimationExist(sAnm, (bIsHUDPresent == false)))
        {
            bFound = true;
            break;
        }
        //--> Новый формат
        xr_sprintf(sAnm, "%s_(%d)", sAnmAlias.c_str(), idx);
        if (isHUDAnimationExist(sAnm, (bIsHUDPresent == false)))
        {
            bFound = true;
            break;
        }

        // Ищем анимацию с привязкой к чётному (%2) \ нечётному (%1) числу патронов (кроме 0)
        if (idx > 0)
        {
            xr_sprintf(sAnm, "%s_(%s)", sAnmAlias.c_str(), (idx % 2 == 0 ? "%2" : "%1"));
            if (isHUDAnimationExist(sAnm, (bIsHUDPresent == false)))
            {
                bFound = true;
                break;
            }
        }
    } while (false);

    // Ищем анимацию дальше, уже без привязки к числу патронов
    if (bFound == false)
    {
        xr_sprintf(sAnm, "%s", sAnmAlias.c_str());
        if (isHUDAnimationExist(sAnm, (bIsHUDPresent == false)))
        {
            bFound = true;
        }
        else
        {
            // Если анимация не найдена, но она обязательна - крашим игру
            if (bAssert == true)
            {
                R_ASSERT2(false,
                    make_string("hudItem model [%s] has no motion with alias [%s] %s",
                        hud_sect.c_str(),
                        sAnm,
                        (bIsHUDPresent ? "": "(third person - weapon addons not included <!>)")
                    ).c_str());
            }
        }
    }

    // Если такая анимация существует...
    if (bFound)
    {
        // Отыгрываем анимацию
        PlayHUDMotion(sAnm, bMixIn, NULL, GetState());

        // Отыгрыаем звук
        LPCSTR sSnd = nullptr;
        string256 sSndAnm;

        xr_sprintf(sSndAnm, "%s%s", "snd_", sAnm);
        if (m_sounds.FindSoundItem(sSndAnm, false))
        { //--> Пробуем подыскать к ней отдельный именной звук
            sSnd = sSndAnm;
        }
        else
        { //--> Иначе пробуем играть стандартный
            sSnd = sSndAlias;
        }

        if (sSnd != NULL)
        {
            PlaySound(sSnd, get_LastFP());
            if (m_fLastAnimStartTime > 0.0f) //--> Корректируем стартовое время звука
                m_sounds.SetCurentTime(sSnd, m_fLastAnimStartTime);
            if (m_fLastAnimSndFreq != 1.0f) //--> Корректируем стартовую частоту звука
                m_sounds.SetFrequency(sSnd, m_fLastAnimSndFreq);
        }
    }

    return bFound;
}

// Проиграть худовую анимацию для NPC \ Третьего лица (игрок её не видит)
// Необходимо т.к из-за аддонов анимаций у игрока много (и они разной длины), а у моделей NPC она всегда одна
void CWeapon::PlaySoundMotionForNPC(LPCSTR sAnmAlias_base, LPCSTR sSndAlias, LPCSTR sAnmAliasDef, LPCSTR sSndAliasDef)
{
    // Прибавляем к названию анимации номер анимационного слота оружия
    string256 sAnmAlias;
    xr_sprintf(sAnmAlias, "%s_%d", sAnmAlias_base, m_animation_slot);

    // Проверяем наличие полученной анимации в главной худовой секции оружия
    bool bAnimExist = pSettings->line_exist(hud_sect, sAnmAlias);

    // Проигрываем найденную худовую анимацию, либо анимацию по умолчанию
    LPCSTR sAnm = (bAnimExist ? sAnmAlias : sAnmAliasDef);
    PlayHUDMotion(sAnm, FALSE, NULL, GetState());

    // Аналогично проигрываем звук к найденной анимации, а если её нет - звук по умолчанию
    LPCSTR sSnd = (bAnimExist ? sSndAlias : sSndAliasDef);
    if (sSnd != NULL && m_sounds.FindSoundItem(sSnd, false))
        PlaySound(sSnd, get_LastFP());
}

// Проиграть мировую анимацию оружия
bool CWeapon::PlayWorldMotion(const shared_str& M, BOOL bMixIn)
{
    if (m_sVisWorldAnmsOverride != nullptr && m_sVisWorldAnmsOverride != M)
    {
        return PlayWorldMotion(m_sVisWorldAnmsOverride, false);
    }

    IKinematicsAnimated* pWeaponVisual = Visual()->dcast_PKinematicsAnimated();
    if (pWeaponVisual != nullptr && pWeaponVisual->ID_Cycle_Safe(M).valid())
    {
        pWeaponVisual->PlayCycle(M.c_str(), bMixIn);
        return true;
    }

    return false;
}

// Возвращает необходимость проиграть анимацию с префиксом _g
bool CWeapon::IsGAnimRequired() const {
    if (m_bUseAmmoBeltMode == true || m_ForceGAnimsMode == eGModeForceOff)
    {
        return false;
    }

    if (m_ForceGAnimsMode == eGModeForceOn)
    {
        return true;
    }

    return m_bGrenadeMode == true;
}

// Возвращает необходимость проиграть анимацию с префиксом _wgl
bool CWeapon::IsWGLAnimRequired() const { return IsGrenadeLauncherAttached() || IsForegripAttached(); }

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Мировая анимация покоя
void CWeapon::PlayWorldAnimIdle()
{
    bool bIsWeaponEmpty = GetMainAmmoElapsed() <= 0;
    bool bEmptyAnmPlayed = false;

    // Мировая модель оружия
    if (bIsWeaponEmpty)
    { //--> Сперва пробуем проиграть анимацию пустого оружия
        bEmptyAnmPlayed = PlayWorldMotion("idle_empty", true);
    }

    if (bEmptyAnmPlayed == false)
    { //--> Иначе играем обычную анимацию
        PlayWorldMotion("idle", true);
    }

    // Её доп. визуалы
    Try2PlayMotionByAliasInAllAdditionalVisuals("anm_world_idle", true, true);
    if (bIsWeaponEmpty)
    {
        Try2PlayMotionByAliasInAllAdditionalVisuals("anm_world_idle_empty", true, true);
    }
}

// Анимация покоя (включая анимации ходьбы и бега)
void CWeapon::PlayAnimIdle()
{
    // Мировая анимация
    PlayWorldAnimIdle();

    // Худовая анимация
    if (m_ZoomAnimState == eZANone)     // Проверяем что мы не играем анимацию зума
        if (TryPlayAnimIdle() == false) // Пробуем сперва проиграть анимации для ходьбы\бега
            PlayAnimIdleOnly();         // Иначе играем анимацию в покое

    m_bIdleFromZoomOut = false;
}

// Анимация покоя (не включая ходьбу и бег)
void CWeapon::PlayAnimIdleOnly()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsZoomed() && !m_bIdleFromZoomOut)
    {
        if (IsGAnimRequired())
        {
            if (iAmmo == 0)
            {
                if (PlaySoundMotion("anm_idle_g_aim_empty", TRUE, NULL, false))
                    return;
            }
            if (PlaySoundMotion("anm_idle_g_aim", TRUE, NULL, false))
                return;
        }

        if (IsWGLAnimRequired())
        {
            if (iAmmo == 0)
            {
                if (PlaySoundMotion("anm_idle_aim_empty_w_gl", TRUE, NULL, false))
                    return;
            }
            if (PlaySoundMotion("anm_idle_aim_w_gl", TRUE, NULL, false))
                return;
        }

        if (true)
        {
            if (iAmmo == 0)
            {
                if (PlaySoundMotion("anm_idle_aim_empty", TRUE, NULL, false))
                    return;
            }
            if (PlaySoundMotion("anm_idle_aim", TRUE, NULL, false))
                return;
        }
    }

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_idle_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_idle_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_empty", TRUE, NULL, false))
                return;
        }
    }

    PlaySoundMotion("anm_idle", TRUE, NULL, true);
}

// Анимация ходьбы
void CWeapon::PlayAnimIdleMoving()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsZoomed() && !m_bIdleFromZoomOut)
    {
        if (IsGAnimRequired())
        {
            if (iAmmo == 0)
            {
                if (PlaySoundMotion("anm_idle_moving_g_aim_empty", TRUE, NULL, false))
                    return;
            }
            if (PlaySoundMotion("anm_idle_moving_g_aim", TRUE, NULL, false))
                return;
        }

        if (IsWGLAnimRequired())
        {
            if (iAmmo == 0)
            {
                if (PlaySoundMotion("anm_idle_moving_aim_empty_w_gl", TRUE, NULL, false))
                    return;
            }
            if (PlaySoundMotion("anm_idle_moving_aim_w_gl", TRUE, NULL, false))
                return;
        }

        if (true)
        {
            if (iAmmo == 0)
            {
                if (PlaySoundMotion("anm_idle_moving_aim_empty", TRUE, NULL, false))
                    return;
            }
            if (PlaySoundMotion("anm_idle_moving_aim", TRUE, NULL, false))
                return;
        }

        PlayAnimIdleOnly();
        return;
    }

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_moving_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_idle_moving_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_moving_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_idle_moving_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_moving_empty", TRUE, NULL, false))
                return;
        }
    }

    if (PlaySoundMotion("anm_idle_moving", TRUE, NULL, false) == false)
        PlayAnimIdleOnly();
}

// Анимация бега
void CWeapon::PlayAnimIdleSprint()
{
    if (IsZoomed())
        return PlayAnimIdleOnly();

    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_sprint_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_idle_sprint_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_sprint_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_idle_sprint_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_idle_sprint_empty", TRUE, NULL, false))
                return;
        }
    }

    if (PlaySoundMotion("anm_idle_sprint", TRUE, NULL, false) == false)
        PlayAnimIdleOnly();
}

// Анимация скуки
void CWeapon::PlayAnimBore()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_bore_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_bore_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_bore_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_bore_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_bore_empty", TRUE, NULL, false))
                return;
        }
    }

    PlaySoundMotion("anm_bore", TRUE, NULL, false);
}

// Анимация прятанья оружия
void CWeapon::PlayAnimHide()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_hide_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_hide_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_hide_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_hide_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_hide_empty", TRUE, NULL, false))
                return;
        }
    }

    PlaySoundMotion("anm_hide", TRUE, NULL, true);
}

// Анимация доставания оружия
void CWeapon::PlayAnimShow()
{
    int iAmmo = GetMainAmmoElapsed();

    // Фикс отсутствия анимации доставания если у игрока два оружия с одинаковым худом
    // [fix for a missing weapon show animation, when player both main weapons have the
    // same HUD section] (by Sin!)
    if (g_player_hud && ParentIsActor())
        g_player_hud->attach_item(this);

    UpdateAddonsVisibility();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_show_g_empty", FALSE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_show_g", FALSE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_show_empty_w_gl", FALSE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_show_w_gl", FALSE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_show_empty", FALSE, NULL, false))
                return;
        }
    }

    PlaySoundMotion("anm_show", FALSE, NULL, true);
}

// Анимация переключения на подствол и обратно
void CWeapon::PlayAnimModeSwitch()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_switch_g_empty", TRUE, "sndSwitch", false))
                return;
        }
        PlaySoundMotion("anm_switch_g", TRUE, "sndSwitch", true);
    }
    else
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_switch_empty", TRUE, "sndSwitch", false))
                return;
        }
        PlaySoundMotion("anm_switch", TRUE, "sndSwitch", true);
    }
}

// Анимация перезарядки оружия
void CWeapon::PlayAnimReload()
{
    UpdBulletHideTimer();

    int iAmmo = m_overridenAmmoForReloadAnm;
    if (iAmmo < 0)
        iAmmo = GetMainAmmoElapsed();

    // Мировая анимация
    if (!IsGAnimRequired())
    {
        bool bPlayAnim = true;
        if (GetState() == eSwitchMag && (
                //--> Если сейчас идёт смена магазина, то проигрываем анимацию только в этих под-состояниях
                m_sub_state != eSubstateReloadBegin &&
                m_sub_state != eSubstateMagazSwitch &&
                m_sub_state != eSubstateMagazDetach_Do &&
                m_sub_state != eSubstateMagazMisfire
            ))
            bPlayAnim = false;

        if (bPlayAnim)
        {
            bool bIsWeaponEmpty = (iAmmo == 0);
            bool bEmptyAnmPlayed = false;

            // Мировая модель оружия
            if (bIsWeaponEmpty)
            { //--> Сперва пробуем проиграть анимацию пустого оружия
                bEmptyAnmPlayed = PlayWorldMotion("reload_world_empty", false);
            }

            if (bEmptyAnmPlayed == false)
            { //--> Иначе играем обычную анимацию
                PlayWorldMotion("reload_world", false);
            }

            // Её доп. визуалы
            Try2PlayMotionByAliasInAllAdditionalVisuals("anm_world_reload", false, true);
            if (bIsWeaponEmpty)
            {
                Try2PlayMotionByAliasInAllAdditionalVisuals("anm_world_reload_empty", false, true);
            }
        }
    }

    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        bool bPlaySnd = (GetState() != eSwitchMag || (IsMisfire() || m_sub_state == eSubstateMagazDetach_Do || m_sub_state == eSubstateMagazSwitch)) ?
                            true :
                            false;

        PlaySoundMotionForNPC("anm_npc_reload", (bPlaySnd ? "sndReloadNPC" : NULL), "anm_reload", (bPlaySnd ? "sndReload" : NULL));
        return;
    }

    // Худовая анимация (Первого лица)
    if (m_bIsReloadFromAB)
        return PlayAnimReloadFrAB();

    if (IsAmmoBeltReloadNow())
        return PlayAnimReloadAB();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_reload_g_empty", TRUE, "sndReloadG", false))
                return;
        }
        if (PlaySoundMotion("anm_reload_g", TRUE, "sndReloadG", false))
            return;
    }

    // Если у нас установлен магазин и подствол\рукоятка\цевье, то требуется использовать особоую анимацию перезарядки под их комбинацию
    LPCSTR    sMagaz   = "";
    LPCSTR    sUBarrel = "";
    string256 combo_wombo;
    bool      bNeed2UseCombinedAnim = false; //--> Флаг необходимости использования комбинированной анимации

    if (IsWGLAnimRequired())
    {
        if ((IsGrenadeLauncherAttached() || IsForegripAttached()) && IsMagazineAttached() == true)
            bNeed2UseCombinedAnim = true; //--> Комбинированная анимация магазин + рукоятка\подствол

        if (bNeed2UseCombinedAnim)
        {
            sMagaz = GetAddonBySlot(eMagaz)->GetName().c_str(); //--> Секция текущего магазина

            do
            { //--> Секция текущего подствольного устройства
                if (IsGrenadeLauncherAttached())
                {
                    sUBarrel = GetAddonBySlot(eLauncher)->GetName().c_str();
                    break;
                }

                if (IsForegripAttached())
                {
                    sUBarrel = GetAddonBySlot(m_ForegripSlot)->GetName().c_str();
                    break;
                }
            } while (0);
        }

        if (iAmmo == 0)
        {
            if (bNeed2UseCombinedAnim)
            {
                if (IsScopeAttached())
                {
                    xr_sprintf(combo_wombo, "anm_reload_empty_w_gl_s_%s_%s", sMagaz, sUBarrel);
                    if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadEmptyWGLS", false))
                        return;
                }
                xr_sprintf(combo_wombo, "anm_reload_empty_w_gl_%s_%s", sMagaz, sUBarrel);
                if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadEmptyWGL", false))
                    return;
            }

            // Иначе играем обычную
            if (IsScopeAttached())
            {
                if (PlaySoundMotion("anm_reload_empty_w_gl_s", TRUE, "sndReloadEmptyWGLS", false))
                    return;
            }
            if (PlaySoundMotion("anm_reload_empty_w_gl", TRUE, "sndReloadEmptyWGL", false))
                return;
        }

        if (bNeed2UseCombinedAnim)
        {
            if (IsScopeAttached())
            {
                xr_sprintf(combo_wombo, "anm_reload_w_gl_s_%s_%s", sMagaz, sUBarrel);
                if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadWGLS", false))
                    return;
            }
            xr_sprintf(combo_wombo, "anm_reload_w_gl_%s_%s", sMagaz, sUBarrel);
            if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadWGL", false))
                return;
        }

        // Иначе играем обычную
        if (IsScopeAttached())
        {
            if (PlaySoundMotion("anm_reload_w_gl_s", TRUE, "sndReloadWGLS", false))
                return;
        }
        if (PlaySoundMotion("anm_reload_w_gl", TRUE, "sndReloadWGL", false))
            return;

        bNeed2UseCombinedAnim = false;
    }

    if (IsForendAttached() == true && IsMagazineAttached() == true)
        bNeed2UseCombinedAnim = true; //--> Комбинированная анимация магазин + цевье

    if (bNeed2UseCombinedAnim)
    {
        sMagaz   = GetAddonBySlot(eMagaz)->GetName().c_str();       //--> Секция текущего магазина
        sUBarrel = GetAddonBySlot(m_ForendSlot)->GetName().c_str(); //--> Секция цевья
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (bNeed2UseCombinedAnim)
            {
                if (IsScopeAttached())
                {
                    xr_sprintf(combo_wombo, "anm_reload_empty_s_%s_%s", sMagaz, sUBarrel);
                    if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadEmptyS", false))
                        return;
                }
                xr_sprintf(combo_wombo, "anm_reload_empty_%s_%s", sMagaz, sUBarrel);
                if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadEmpty", false))
                    return;
            }

            if (IsScopeAttached())
            {
                if (PlaySoundMotion("anm_reload_empty_s", TRUE, "sndReloadEmptyS", false))
                    return;
            }
            if (PlaySoundMotion("anm_reload_empty", TRUE, "sndReloadEmpty", false))
                return;
        }
    }

    if (bNeed2UseCombinedAnim)
    {
        if (IsScopeAttached())
        {
            xr_sprintf(combo_wombo, "anm_reload_s_%s_%s", sMagaz, sUBarrel);
            if (PlaySoundMotion(combo_wombo, TRUE, "sndReloadS", false))
                return;
        }
        xr_sprintf(combo_wombo, "anm_reload_%s_%s", sMagaz, sUBarrel);
        if (PlaySoundMotion(combo_wombo, TRUE, "sndReload", false))
            return;
    }

    if (IsScopeAttached())
    {
        if (PlaySoundMotion("anm_reload_s", TRUE, "sndReloadS", false))
            return;
    }
    PlaySoundMotion("anm_reload", TRUE, "sndReload", true);
}

// Анимация перезарядки патронташа (цельная)
void CWeapon::PlayAnimReloadAB()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_reload_ab_empty_w_gl", TRUE, "sndReloadABEmptyWGL", false))
                return;
        }
        if (PlaySoundMotion("anm_reload_ab_w_gl", TRUE, "sndReloadABWGL", false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_reload_ab_empty", TRUE, "sndReloadABEmpty", false))
                return;
        }
    }

    PlaySoundMotion("anm_reload_ab", TRUE, "sndReloadAB", true);
}

// Анимация перезарядки оружия из патронташа
void CWeapon::PlayAnimReloadFrAB()
{
    int iAmmo     = GetMainAmmoElapsed();
    int iAmmoMain = GetMainAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (iAmmoMain == 0)
        {
            if (PlaySoundMotion("anm_reload_fr_ab_empty_w_gl", TRUE, "sndReloadFrABEmptyWGL", false))
                return;
        }
        if (PlaySoundMotion("anm_reload_fr_ab_w_gl", TRUE, "sndReloadFrABWGL", false))
            return;
    }

    if (true)
    {
        if (iAmmoMain == 0)
        {
            if (PlaySoundMotion("anm_reload_fr_ab_empty", TRUE, "sndReloadFrABEmpty", false))
                return;
        }
    }

    PlaySoundMotion("anm_reload_fr_ab", TRUE, "sndReloadFrAB", true);
}

// Анимация перезарядки оружия в три стадии (открытие)
bool CWeapon::PlayAnimOpenWeapon()
{
    if (PlayAnimOpenWeaponFrAB())
        return true;

    if (IsAmmoBeltReloadNow())
        return PlayAnimOpenWeaponAB();

    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_open_g_empty", TRUE, "sndOpenEmptyG", false))
                return true;
        }
        if (PlaySoundMotion("anm_open_g", TRUE, "sndOpenG", false))
            return true;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_open_empty_w_gl", TRUE, "sndOpenEmptyWGL", false))
                return true;
        }
        if (PlaySoundMotion("anm_open_w_gl", TRUE, "sndOpenWGL", false))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_open_empty", TRUE, "sndOpenEmpty", false))
                return true;
        }
    }

    return PlaySoundMotion("anm_open", TRUE, "sndOpen", false);
}

// Анимация перезарядки оружия в три стадии (открытие из патронташа)
bool CWeapon::PlayAnimOpenWeaponFrAB()
{
    if (m_bIsReloadFromAB == false)
        return false;

    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        PlaySoundMotionForNPC("anm_npc_open_fr_ab", "sndOpenFrAB", "anm_open", "sndOpen");
        return true;
    }

    int iAmmo     = GetGLAmmoElapsed();
    int iAmmoMain = GetMainAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (iAmmoMain == 0)
        {
            if (PlaySoundMotion("anm_open_fr_ab_empty_w_gl", TRUE, "sndOpenFrABEmptyWGL", false, iAmmo))
                return true;
        }
        if (PlaySoundMotion("anm_open_fr_ab_w_gl", TRUE, "sndOpenFrABWGL", false, iAmmo))
            return true;
    }

    if (true)
    {
        if (iAmmoMain == 0)
        {
            if (PlaySoundMotion("anm_open_fr_ab_empty", TRUE, "sndOpenFrABEmpty", false, iAmmo))
                return true;
        }
    }

    return PlaySoundMotion("anm_open_fr_ab", TRUE, "sndOpenFrAB", false, iAmmo);
}

// Анимация перезарядки оружия в три стадии (вставка)
void CWeapon::PlayAnimAddOneCartridgeWeapon()
{
    UpdBulletHideTimer();

    if (m_bIsReloadFromAB)
    {
        PlayAnimAddOneCartridgeWeaponFrAB();
        return;
    }

    if (IsAmmoBeltReloadNow())
    {
        PlayAnimAddOneCartridgeWeaponAB();
        return;
    }

    if (PlayAnimSwitchAddOneCartridge())
        return;

    m_bSwitchAddAnimation = false;

    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_add_cartridge_g_empty", FALSE, "sndAddCartridgeEmptyG", false))
                return;
        }
        if (PlaySoundMotion("anm_add_cartridge_g", FALSE, "sndAddCartridgeG", false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (m_bCanShowLastBulletShell)
        {
            if (PlaySoundMotion("anm_add_cartridge_lbs_w_gl", FALSE, "sndAddCartridgeLBSWGL", false, iAmmo))
                return;
        }
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_add_cartridge_empty_w_gl", FALSE, "sndAddCartridgeEmptyWGL", false))
                return;
        }
        if (PlaySoundMotion("anm_add_cartridge_w_gl", FALSE, "sndAddCartridgeWGL", false))
            return;
    }

    if (m_bCanShowLastBulletShell)
    {
        if (PlaySoundMotion("anm_add_cartridge_lbs", FALSE, "sndAddCartridgeLBS", false, iAmmo))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_add_cartridge_empty", FALSE, "sndAddCartridgeEmpty", false))
                return;
        }
    }

    PlaySoundMotion("anm_add_cartridge", FALSE, "sndAddCartridge", true);
}

// Анимация перехода перезарядки из патронташа в перезарядку из инвентаря
bool CWeapon::PlayAnimSwitchAddOneCartridge()
{
    UpdBulletHideTimer();

    if (m_bSwitchAddAnimation == false)
        return false;

    int iAmmo = GetGLAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_switch_add_cartridge_empty_w_gl", TRUE, "sndSwAddCartridgeEmptyWGL", false, iAmmo))
                return true;
        }
        if (PlaySoundMotion("anm_switch_add_cartridge_w_gl", TRUE, "sndSwAddCartridgeWGL", false, iAmmo))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_switch_add_cartridge_empty", TRUE, "sndSwAddCartridgeEmpty", false, iAmmo))
                return true;
        }
    }

    return PlaySoundMotion("anm_switch_add_cartridge", TRUE, "sndSwAddCartridge", false, iAmmo);
}

// Анимация перезарядки оружия в три стадии (вставка из патронташа)
void CWeapon::PlayAnimAddOneCartridgeWeaponFrAB()
{
    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        PlaySoundMotionForNPC("anm_npc_add_cartridge_fr_ab", "sndAddCartridgeFrAB", "anm_add_cartridge", "sndAddCartridge");
        return;
    }

    int iAmmo     = GetGLAmmoElapsed();
    int iAmmoMain = GetMainAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (m_bCanShowLastBulletShell)
        {
            if (PlaySoundMotion("anm_add_cartridge_lbs_fr_ab_w_gl", FALSE, "sndAddCartridgeLBSFrABWGL", false, iAmmo))
                return;
        }
        if (iAmmoMain == 0)
        {
            if (PlaySoundMotion("anm_add_cartridge_fr_ab_empty_w_gl", FALSE, "sndAddCartridgeFrABEmptyWGL", false, iAmmo))
                return;
        }
        if (PlaySoundMotion("anm_add_cartridge_fr_ab_w_gl", FALSE, "sndAddCartridgeFrABWGL", false, iAmmo))
            return;
    }

    if (m_bCanShowLastBulletShell)
    {
        if (PlaySoundMotion("anm_add_cartridge_lbs_fr_ab", FALSE, "sndAddCartridgeLBSFrAB", false, iAmmo))
            return;
    }

    if (true)
    {
        if (iAmmoMain == 0)
        {
            if (PlaySoundMotion("anm_add_cartridge_fr_ab_empty", FALSE, "sndAddCartridgeFrABEmpty", false, iAmmo))
                return;
        }
    }

    PlaySoundMotion("anm_add_cartridge_fr_ab", FALSE, "sndAddCartridgeFrAB", true, iAmmo);
}

// Анимация перезарядки оружия в три стадии (закрытие)
bool CWeapon::PlayAnimCloseWeapon()
{
    if (PlayAnimCloseWeaponFrAB())
        return true;

    if (IsAmmoBeltReloadNow())
        return PlayAnimCloseWeaponAB();

    if (PlayAnimCloseWeaponFromEmpty())
        return true;

    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_g_empty", FALSE, "sndCloseEmptyG", false))
                return true;
        }
        if (PlaySoundMotion("anm_close_g", FALSE, "sndCloseG", false))
            return true;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_empty_w_gl", FALSE, "sndCloseEmptyWGL", false))
                return true;
        }
        if (PlaySoundMotion("anm_close_w_gl", FALSE, "sndCloseWGL", false))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_empty", FALSE, "sndCloseEmpty", false))
                return true;
        }
    }

    return PlaySoundMotion("anm_close", FALSE, "sndClose", false);
}

// Анимация перезарядки оружия в три стадии (закрытие из патронташа)
bool CWeapon::PlayAnimCloseWeaponFrAB()
{
    if (m_bIsReloadFromAB == false)
        return false;

    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        PlaySoundMotionForNPC("anm_npc_close_fr_ab", "sndCloseFrAB", "anm_close", "sndClose");
        return true;
    }

    if (PlayAnimCloseWeaponFrABFromEmpty())
        return true;

    int iAmmo = GetGLAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fr_ab_empty_w_gl", FALSE, "sndCloseFrABEmptyWGL", false, iAmmo))
                return true;
        }
        if (PlaySoundMotion("anm_close_fr_ab_w_gl", FALSE, "sndCloseFrABWGL", false, iAmmo))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fr_ab_empty", FALSE, "sndCloseFrABEmpty", false, iAmmo))
                return true;
        }
    }

    return PlaySoundMotion("anm_close_fr_ab", FALSE, "sndCloseFrAB", false, iAmmo);
}

// Анимация перезарядки оружия в три стадии (закрытие, если зарядка была с пустого магазина)
bool CWeapon::PlayAnimCloseWeaponFromEmpty()
{
    if (m_overridenAmmoForReloadAnm != 0)
        return false;

    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fempt_g_empty", FALSE, "sndCloseEmptyFEG", false))
                return true;
        }
        if (PlaySoundMotion("anm_close_fempt_g", FALSE, "sndCloseFEG", false))
            return true;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fempt_empty_w_gl", FALSE, "sndCloseEmptyFEWGL", false))
                return true;
        }
        if (PlaySoundMotion("anm_close_fempt_w_gl", FALSE, "sndCloseFEWGL", false))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fempt_empty", FALSE, "sndCloseFEEmpty", false))
                return true;
        }
    }

    return PlaySoundMotion("anm_close_fempt", FALSE, "sndCloseFE", false);
}

// Анимация перезарядки оружия в три стадии (закрытие из патроншата, если зарядка была с пустого магазина)
bool CWeapon::PlayAnimCloseWeaponFrABFromEmpty()
{
    if (m_overridenAmmoForReloadAnm != 0)
        return false;

    int iAmmo = GetGLAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fempt_fr_ab_empty_w_gl", FALSE, "sndCloseEmptyFrABFEWGL", false, iAmmo))
                return true;
        }
        if (PlaySoundMotion("anm_close_fempt_fr_ab_w_gl", FALSE, "sndCloseFrABFEWGL", false, iAmmo))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_close_fempt_fr_ab_empty", FALSE, "sndCloseFrABEmptyFE", false, iAmmo))
                return true;
        }
    }

    return PlaySoundMotion("anm_close_fempt_fr_ab", FALSE, "sndCloseFrABFE", false, iAmmo);
}

// Анимация перезарядки патронташа в три стадии (открытие)
bool CWeapon::PlayAnimOpenWeaponAB()
{
    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        PlaySoundMotionForNPC("anm_npc_open_ab", "sndOpenAB", "anm_open", "sndOpen");
        return true;
    }

    if (IsWGLAnimRequired())
    {
        if (PlaySoundMotion("anm_open_ab_w_gl", TRUE, "sndOpenABWGL", false))
            return true;
    }
    return PlaySoundMotion("anm_open_ab", TRUE, "sndOpenAB", false);
}

// Анимация перезарядки патронташа в три стадии (вставка)
void CWeapon::PlayAnimAddOneCartridgeWeaponAB()
{
    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        PlaySoundMotionForNPC("anm_npc_add_cartridge_ab", "sndAddCartridgeAB", "anm_add_cartridge", "sndAddCartridge");
        return;
    }

    int iAmmo = GetGLAmmoElapsed();

    if (IsWGLAnimRequired())
    {
        if (PlaySoundMotion("anm_add_cartridge_ab_w_gl", FALSE, "sndAddCartridgeABWGL", false, iAmmo))
            return;
    }
    PlaySoundMotion("anm_add_cartridge_ab", FALSE, "sndAddCartridgeAB", true, iAmmo);
}

// Анимация перезарядки патронташа в три стадии (закрытие)
bool CWeapon::PlayAnimCloseWeaponAB()
{
    // Худовая анимация (Для NPC \ Третьего лица)
    if (HudItemData() == NULL)
    {
        PlaySoundMotionForNPC("anm_npc_close_ab", "sndCloseAB", "anm_close", "sndClose");
        return true;
    }

    if (IsWGLAnimRequired())
    {
        if (PlaySoundMotion("anm_close_ab_w_gl", FALSE, "sndCloseABWGL", false))
            return true;
    }
    return PlaySoundMotion("anm_close_ab", FALSE, "sndCloseAB", false);
}

// Анимация помпы \ болтовки
void CWeapon::PlayAnimPump()
{
    if (IsZoomed() && !m_bIdleFromZoomOut)
    {
        if (IsWGLAnimRequired())
        {
            if (PlaySoundMotion("anm_pump_aim_w_gl", TRUE, "sndPumpAimWGL", false))
                return;
        }

        if (PlaySoundMotion("anm_pump_aim", TRUE, "sndPumpAim", false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (PlaySoundMotion("anm_pump_w_gl", TRUE, "sndPumpWGL", false))
            return;
    }

    PlaySoundMotion("anm_pump", TRUE, "sndPump", true);
}

// Анимация прицеливания (реверсивная для входа и выхода)
void CWeapon::PlayAnimZoom()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_zoom_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_zoom_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_zoom_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_zoom_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_zoom_empty", TRUE, NULL, false))
                return;
        }
    }

    if (PlaySoundMotion("anm_zoom", TRUE, NULL, false))
        return;

    m_ZoomAnimState = eZANone; // Не нашли в оружии анимацию для зума
    PlayAnimIdle();
}

// Анимация стрельбы при пустом магазине
void CWeapon::PlayAnimEmptyClick()
{
    int iAmmo = GetMainAmmoElapsed();

    m_bPlayNextIdleAnimFromRandomST = true; //--> Следующая Idle-анимация будет запущена со случайной позиции

    if (IsZoomed())
    {
        if (IsGAnimRequired())
        {
            if (PlaySoundMotion("anm_empty_click_g_aim", TRUE, "sndEmptyClick", false, iAmmo))
                return;
        }

        if (IsWGLAnimRequired())
        {
            if (PlaySoundMotion("anm_empty_click_aim_w_gl", TRUE, "sndEmptyClick", false))
                return;
        }

        if (true)
        {
            if (PlaySoundMotion("anm_empty_click_aim", TRUE, "sndEmptyClick", false))
                return;
        }
    }

    if (IsGAnimRequired())
    {
        if (PlaySoundMotion("anm_empty_click_g", TRUE, "sndEmptyClick", false, iAmmo))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (PlaySoundMotion("anm_empty_click_w_gl", TRUE, "sndEmptyClick", false))
            return;
    }

    PlaySoundMotion("anm_empty_click", TRUE, "sndEmptyClick", false);
}

// Анимация стрельбы
void CWeapon::PlayAnimShoot()
{
    int  iAmmo       = GetMainAmmoElapsed() + 1; //--> Т.к анимация стрельбы играется ПОСЛЕ выстрела
    bool bLastBullet = (iAmmo == 1);
    bool bIsDuplet   = (GetCurrentFireMode() == 2);

    // Мировая анимация
    if (!IsGAnimRequired())
    {
        bool bLastShotAnmPlayed = false;

        // Мировая модель оружия
        if (bLastBullet)
        { //--> Сперва пробуем проиграть анимацию пустого оружия
            bLastShotAnmPlayed = PlayWorldMotion("shoot_last", false);
        }

        if (bLastShotAnmPlayed == false)
        { //--> Иначе играем обычную анимацию
            PlayWorldMotion("shoot", false);
        }

        // Её доп. визуалы
        Try2PlayMotionByAliasInAllAdditionalVisuals("anm_world_shoot", false, true);
        if (bLastBullet)
        {
            Try2PlayMotionByAliasInAllAdditionalVisuals("anm_world_shoot_last", false, true);
        }
    }

    // Худовая анимация
    m_bPlayNextIdleAnimFromRandomST = true; //--> Следующая Idle-анимация будет запущена со случайной позиции

    if (IsZoomed())
    {
        if (GetZoomParams().m_bDisableShotAnimAtZoom == true)
        { //--> Отключаем анимацию стрельбы при прицеливании (для оптики)
            if (ParentIsActor() && H_Parent()->cast_actor()->AnyMove() == false)
            { //--> Но только если игрок не двигается
                do
                {
                    if (IsBipodsDeployed() && IsBipodsZoomed() == false)
                    { //--> В режиме сошек прицеливание работает по другому
                        break;
                    }

                    PlayAnimIdle();
                    return;
                } while (0);
            }
        }

        if (IsGAnimRequired())
        {
            if (bLastBullet)
            {
                if (PlaySoundMotion("anm_shot_l_g_aim", FALSE, NULL, false, iAmmo))
                    return;
            }
            if (PlaySoundMotion("anm_shots_g_aim", FALSE, NULL, false, iAmmo))
                return;
        }

        if (IsWGLAnimRequired())
        {
            if (bIsDuplet && PlaySoundMotion("anm_shot_duplet_aim_w_gl", FALSE, NULL, false, iAmmo))
                return;
            if (bLastBullet)
            {
                if (PlaySoundMotion("anm_shot_l_aim_w_gl", FALSE, NULL, false, iAmmo))
                    return;
            }
            if (PlaySoundMotion("anm_shots_aim_w_gl", FALSE, NULL, false, iAmmo))
                return;
        }

        if (true)
        {
            if (bIsDuplet && PlaySoundMotion("anm_shot_duplet_aim", FALSE, NULL, false, iAmmo))
                return;
            if (bLastBullet)
            {
                if (PlaySoundMotion("anm_shot_l_aim", FALSE, NULL, false, iAmmo))
                    return;
            }
            if (PlaySoundMotion("anm_shots_aim", FALSE, NULL, false, iAmmo))
                return;
        }
    }

    if (IsGAnimRequired())
    {
        if (bLastBullet)
        {
            if (PlaySoundMotion("anm_shot_l_g", FALSE, NULL, false, iAmmo))
                return;
        }
        if (PlaySoundMotion("anm_shots_g", FALSE, NULL, false, iAmmo))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (bIsDuplet && PlaySoundMotion("anm_shot_duplet_w_gl", FALSE, NULL, false, iAmmo))
            return;
        if (bLastBullet)
        {
            if (PlaySoundMotion("anm_shot_l_w_gl", FALSE, NULL, false, iAmmo))
                return;
        }
        if (PlaySoundMotion("anm_shots_w_gl", FALSE, NULL, false, iAmmo))
            return;
    }

    if (bIsDuplet && PlaySoundMotion("anm_shot_duplet", FALSE, NULL, false, iAmmo))
        return;

    if (bLastBullet)
    {
        if (PlaySoundMotion("anm_shot_l", FALSE, NULL, false, iAmmo))
            return;
    }

    if (PlaySoundMotion("anm_shot", FALSE, NULL, false, iAmmo))
        return;
    PlaySoundMotion("anm_shots", FALSE, NULL, true, iAmmo);
}

// Анимация атаки ножом
void CWeapon::PlayAnimKnifeAttack()
{
    int state = GetState();
    switch (state)
    {
    case eFire: PlaySoundMotion("anm_attack", FALSE, NULL, false); return;
    case eFire2: PlaySoundMotion("anm_attack2", FALSE, NULL, false); return;
    }
    Need2Idle();
}

// Анимация удара прикладом (основной)
void CWeapon::PlayAnimKick()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsForegripAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_empty_w_gak", FALSE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_w_gak", FALSE, NULL, false))
            return;
    }

    if (IsForendAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_empty_w_fak", FALSE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_w_fak", FALSE, NULL, false))
            return;
    }

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_g_empty", FALSE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_g", FALSE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_empty_w_gl", FALSE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_w_gl", FALSE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_empty", FALSE, NULL, false))
                return;
        }
    }

    PlaySoundMotion("anm_kick", FALSE, NULL, true);
}

// Анимация удара прикладом (альтернативный)
void CWeapon::PlayAnimKickAlt()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsForegripAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_empty_w_gak", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_alt_w_gak", TRUE, NULL, false))
            return;
    }

    if (IsForendAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_empty_w_fak", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_alt_w_fak", TRUE, NULL, false))
            return;
    }

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_g_empty", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_alt_g", TRUE, NULL, false))
            return;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_empty_w_gl", TRUE, NULL, false))
                return;
        }
        if (PlaySoundMotion("anm_kick_alt_w_gl", TRUE, NULL, false))
            return;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_empty", TRUE, NULL, false))
                return;
        }
    }

    PlaySoundMotion("anm_kick_alt", TRUE, NULL, true);
}

// Анимация выхода из удара прикладом (основной)
bool CWeapon::PlayAnimKickOut()
{
    int iAmmo = GetMainAmmoElapsed();

    if (IsForegripAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_out_empty_w_gak", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_out_w_gak", FALSE, NULL, false))
            return true;
    }

    if (IsForendAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_out_empty_w_fak", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_out_w_fak", FALSE, NULL, false))
            return true;
    }

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_out_g_empty", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_out_g", FALSE, NULL, false))
            return true;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_out_empty_w_gl", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_out_w_gl", FALSE, NULL, false))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_out_empty", FALSE, NULL, false))
                return true;
        }
    }

    if (PlaySoundMotion("anm_kick_out", FALSE, NULL, false))
        return true;

    return false;
}

// Анимация выхода из удара прикладом (альтернативный)
bool CWeapon::PlayAnimKickOutAlt()
{
    int iAmmo = GetMainAmmoElapsed();


    if (IsForegripAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_out_empty_w_gak", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_alt_out_w_gak", FALSE, NULL, false))
            return true;
    }

    if (IsForendAttached() && IsBayonetAttached())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_out_empty_w_fak", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_alt_out_w_fak", FALSE, NULL, false))
            return true;
    }

    if (IsGAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_out_g_empty", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_alt_out_g", FALSE, NULL, false))
            return true;
    }

    if (IsWGLAnimRequired())
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_out_empty_w_gl", FALSE, NULL, false))
                return true;
        }
        if (PlaySoundMotion("anm_kick_alt_out_w_gl", FALSE, NULL, false))
            return true;
    }

    if (true)
    {
        if (iAmmo == 0)
        {
            if (PlaySoundMotion("anm_kick_alt_out_empty", FALSE, NULL, false))
                return true;
        }
    }

    if (PlaySoundMotion("anm_kick_alt_out", FALSE, NULL, false))
        return true;

    return false;
}
