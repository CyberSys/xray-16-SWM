/*****************************/
/***** Состояние "Скука" *****/ //--#SM+#--
/*****************************/

#include "StdAfx.h"
#include "Weapon.h"

// Пробуем начать скуку на клиенте
bool CWeapon::Try2Bore(bool bCheckOnlyMode)
{
    if (!bCheckOnlyMode && GetState() == eBore)
        return false;
    if ((bool)IsPending() == true)
        return false;

    if (AllowBore())
    {
        if (!bCheckOnlyMode)
            SwitchState(eBore);

        return true;
    }

    return false;
}

// Нужно остановить скуку на клиенте
void CWeapon::Need2Stop_Bore()
{
    if (GetState() != eBore)
        return;
    Need2Idle();
}

// Переключение стэйта на "Скука"
void CWeapon::switch2_Bore()
{
    if (!Try2Bore(true)) //--> Повторная проверка для МП, где вызов стэйтов идёт в обход Try-функций
    {
        Need2Idle();
        return;
    }

    SetPending(FALSE);
    PlayAnimBore();
}

// Переключение на другой стэйт из стэйта "Скука"
void CWeapon::switchFrom_Bore(u32 newS) {}

// Обновление оружия в состоянии "Скука"
void CWeapon::state_Bore(float dt) {}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Можем-ли мы перейти в состояние скуки
bool CWeapon::AllowBore() { return m_bDisableBoreAnimation == false; }
