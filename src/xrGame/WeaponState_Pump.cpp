/*****************************/
/***** Состояние "Помпа" *****/ //--#SM+#--
/*****************************/

#include "StdAfx.h"
#include "Weapon.h"

// Пробуем начать помпу на клиенте
bool CWeapon::Try2Pump(bool bCheckOnlyMode)
{
    if (m_bUsePumpMode == false)
        return false;
    if (m_bNeed2Pump == false)
        return false;
    if (!bCheckOnlyMode && GetState() == ePump)
        return false;
    if (IsPending() == true)
        return false;

    if (!bCheckOnlyMode)
        SwitchState(ePump);

    return true;
}

// Нужно остановить помпу на клиенте
void CWeapon::Need2Stop_Pump()
{
    if (GetState() == ePump)
        Need2Idle();
}

// Переключение стэйта на "Помпа"
void CWeapon::switch2_Pump()
{
    if (!Try2Pump(true)) //--> Повторная проверка для МП, где вызов стэйтов идёт в обход Try-функций
    {
        Need2Idle();
        return;
    }

    PlayAnimPump();
    SetPending(TRUE);
}

// Переключение на другой стэйт из стэйта "Помпа"
void CWeapon::switchFrom_Pump(u32 newS)
{
    m_sounds.StopSound("sndPump");
    m_sounds.StopSound("sndPumpWGL");
    m_sounds.StopSound("sndPumpAim");
    m_sounds.StopSound("sndPumpAimWGL");

    if (iAmmoElapsed == 0)
        Try2AutoReload();
}

// Обновление оружия в состоянии "Помпа"
void CWeapon::state_Pump(float dt) { UpdShellShowTimer(); }
