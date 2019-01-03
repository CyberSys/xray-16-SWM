/*************************************************/
/***** Состояние "Перезарядка из патронташа" *****/ //--#SM+#--
/*************************************************/

#include "StdAfx.h"
#include "Weapon.h"

// Пробуем начать перезарядку на клиенте
bool CWeapon::Try2ReloadFrAB(bool bCheckOnlyMode)
{
    if (!bCheckOnlyMode && GetState() == eReloadFrAB)
        return false;
    if (IsPending() == true)
        return false;
    if (!IsAmmoBeltAttached())
        return false;
    if (m_bGrenadeMode)
        return false;
    if (GetGLAmmoElapsed() == 0)
        return false;

    if (!bCheckOnlyMode)
        SwitchState(eReloadFrAB);

    return true;
}

// Переключение стэйта на "Перезарядка из патронташа"
void CWeapon::switch2_ReloadFrAB()
{
    if (!Try2ReloadFrAB(true))
    {
        Need2Idle();
        return;
    }

    m_bIsReloadFromAB = true;

    if (!Try2Reload())
    {
        m_bIsReloadFromAB = false;
        Need2Idle();
    }
}

// Переключение на другой стэйт из стэйта "Перезарядка из патронташа"
void CWeapon::switchFrom_ReloadFrAB(u32 newS) {}

// Обновление оружия в состоянии "Перезарядка из патронташа"
void CWeapon::state_ReloadFrAB(float dt) {}
