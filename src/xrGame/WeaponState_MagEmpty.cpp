/**************************************/
/***** Состояние "Пустой магазин" *****/ //--#SM+#--
/**************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Требуется перейти в состояние пустого магазина
void CWeapon::Need2Empty() { SwitchState(eMagEmpty); }

// Переключение стейта на пустой магазин
void CWeapon::switch2_Empty()
{
    // OnZoomOut();
    OnMagazineEmpty();
}

// Переключение на другой стэйт из стэйта пустого магазина
void CWeapon::switchFrom_Empty(u32 newS) {}

// Обновление оружия в состоянии "Покой"
void CWeapon::state_Empty(float dt) {}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

void CWeapon::OnMagazineEmpty()
{
    if (!m_bNeed2Pump)
        if (!Try2AutoReload())
            Need2Idle();
}
