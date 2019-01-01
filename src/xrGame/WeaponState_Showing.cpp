/*****************************************/
/***** Состояние "Доставание оружия" *****/ //--#SM+#--
/*****************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Переключение стэйта на "Доставание"
void CWeapon::switch2_Showing()
{
    bool bSoundsEnabled = true;

    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(this->H_Parent());
    if (owner)
        bSoundsEnabled = owner->CanPlayShHdRldSounds();

    if (bSoundsEnabled)
        PlaySound("sndShow", get_LastFP());

    SetPending(TRUE);
    PlayAnimShow();
}

// Переключение на другой стэйт из стэйта "Доставание"
void CWeapon::switchFrom_Showing(u32 newS) {}

// Обновление оружия в состоянии "Доставание"
void CWeapon::state_Showing(float dt) {}
