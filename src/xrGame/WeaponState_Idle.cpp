/*****************************/
/***** Состояние "Покоя" *****/ //--#SM+#--
/*****************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Требуется перейти в состояние покоя
void CWeapon::Need2Idle()
{
    // <!> Переход в Idle должен происходить всегда
    SwitchState(eIdle);
}

void CWeapon::switch2_Idle()
{
    SetPending(FALSE);
    PlayAnimIdle();
}

// Переключение на другой стэйт из стэйта "Покой"
void CWeapon::switchFrom_Idle(u32 newS)
{
    if (newS != eIdle)
        m_ZoomAnimState = eZANone; // Сбрасываем флаг анимации зума
}

// Обновление оружия в состоянии "Покой"
void CWeapon::state_Idle(float dt)
{
    // Помпа, если не сработала раньше.
    if (!m_bGrenadeMode)
        Try2Pump();

    // Если зарядка патронташа прервалась некорректно, то страхуем это здесь
    if (IsAmmoBeltReloadNow())
        PerformSwitchGL();

    // У нас теперь не может быть осечки при полном магазине
    if (IsMisfire())
        if (iAmmoElapsed >= iMagazineSize)
            bMisfire = false;
}