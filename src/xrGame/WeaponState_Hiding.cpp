/***************************************/
/***** Состояние "Прятанье оружия" *****/ //--#SM+#--
/***************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Переключение стэйта на "Прятанье"
void CWeapon::switch2_Hiding()
{
    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(this->H_Parent());
    if (owner)
        m_sounds_enabled = owner->CanPlayShHdRldSounds();

    OnZoomOut();

    if (m_sounds_enabled)
    {
        if (!m_bGrenadeMode && iAmmoElapsed == 0 && m_sounds.FindSoundItem("sndCloseEmpty", false))
        {
            PlaySound("sndCloseEmpty", get_LastFP());
        }
    }

    PlaySound("sndHide", get_LastFP());
    PlayAnimHide();
    SetPending(TRUE);
}

// Переключение на другой стэйт из стэйта "Прятанье"
void CWeapon::switchFrom_Hiding(u32 newS) {}

// Обновление оружия в состоянии "Прятанье"
void CWeapon::state_Hiding(float dt) {}
