/**************************************/
/***** Состояние "Удар прикладом" *****/ //--#SM+#--
/**************************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "WeaponKnifeHit.h"

// Пробуем начать приклад на клиенте
bool CWeapon::Try2Kick(bool bCheckOnlyMode)
{
    if (!bCheckOnlyMode && GetState() == eKick)
        return false;
    if ((bool)IsPending() == true)
        return false;

    // При активных сошках нельзя делать удар
    if (IsBipodsDeployed())
        return false;

    // Проверяем, есть-ли такая возможность у оружия.
    if (m_kicker_main == NULL && m_kicker_alt == NULL)
        return false;

    if (!bCheckOnlyMode)
        SwitchState(eKick);

    return true;
}

// Нужно остановить приклад на клиенте
void CWeapon::Need2Stop_Kick()
{
    if (GetState() != eKick)
        return;
    Need2Idle();
}

// Переключение стэйта на "Удар прикладом"
void CWeapon::switch2_Kick()
{
    if (!Try2Kick(true)) //--> Повторная проверка для МП, где вызов стэйтов идёт в обход Try-функций
    {
        Need2Idle();
        return;
    }

    // Сбрасываем прицеливание
    if (IsZoomed())
        OnZoomOut();

    // Если мы бежим, то вместо обычной атаки пробуем запустить атаку на бегу
    if (m_kicker_alt != NULL)
    {
        CActor* pActor = smart_cast<CActor*>(H_Parent());
        if (pActor)
        {
            CEntity::SEntityState st;
            pActor->g_State(st);
            if (st.bSprint)
            {
                m_bKickAtRunActivated = true;
                PlayAnimKickAlt();
                SetPending(TRUE);
                return;
            }
        }
    }

    // Иначе пробуем играть обычную атаку
    if (m_kicker_main != NULL)
    {
        PlaySound("sndKick", get_LastFP());

        m_bKickAtRunActivated = false;
        PlayAnimKick();
        SetPending(TRUE);
        return;
    }

    // У нас вообще нет атаки прикладом - как мы сюда попали?
    R_ASSERT2(false, "switch2_Kick() called without any kicker aviable!");
}

// Переключение на другой стэйт из стэйта "Удар прикладом"
void CWeapon::switchFrom_Kick(u32 newS) { m_bKickAtRunActivated = false; }

// Обновление оружия в состоянии "Удар прикладом"
void CWeapon::state_Kick(float dt)
{
    // Атака на бегу
    if (m_bKickAtRunActivated == true)
    {
        // Проверяем, поддерживает-ли наше оружие такую атаку
        if (m_kicker_alt == NULL)
        {
            Need2Stop_Kick();
            return;
        }

        // Проверяем, что актёр всё ещё бежит, если нет - прерываем атаку.
        CActor* pActor = smart_cast<CActor*>(H_Parent());
        if (pActor)
        {
            CEntity::SEntityState st;
            pActor->g_State(st);
            if (!st.bSprint)
            {
                Need2Stop_Kick();
                return;
            }
        }

        if (Device.dwTimeGlobal - m_dw_last_kick_at_run_upd_time <= WEAPON_ADDONS_KAR_UPD_INTERVAL)
            return;
        m_dw_last_kick_at_run_upd_time = Device.dwTimeGlobal;

        // "Стреляем" до тех пор, пока стэйт не изменится
        KickHit(true);
    }
}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Нанести "хит" прикладом
void CWeapon::KickHit(bool bAltMode)
{
    Fvector p1, d;
    p1.set(Device.vCameraPosition);
    d.set(Device.vCameraDirection);

    bool bDidWeHit = false; //--> Попали-ли мы в кого-нибудь?
    if (bAltMode)
        bDidWeHit = m_kicker_alt->KnifeStrike(p1, d);
    else
        bDidWeHit = m_kicker_main->KnifeStrike(p1, d);

    if (GetState() != eKick)
        return;

    // Если мы в кого-то попали, играем анимацию "выхода"
    if (bDidWeHit)
    {
        // Останавливаем бег
        CActor* pActor = smart_cast<CActor*>(H_Parent());
        if (pActor)
        {
            pActor->StopAnyMove();
        }

        // Запускаем анимацию выхода из удара
        m_bKickAtRunActivated  = false;
        bool bIsExist = false;

        if (bAltMode)
            bIsExist = PlayAnimKickOutAlt();
        else
            bIsExist = PlayAnimKickOut();

        if (!bIsExist && bAltMode) //--> Анимации нет, поэтому сразу останавливаем, если в альтернативном режиме (там нет колбека на конец анимации)
            Need2Stop_Kick();
    }
}
