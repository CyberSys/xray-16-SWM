/***************************************/
/***** Состояние "Прятанье оружия" *****/ //--#SM+#--
/***************************************/

#include "StdAfx.h"
#include "Weapon.h"

// Переключение стэйта на "Прятанье"
void CWeapon::switch2_Hiding()
{
    bool bSoundsEnabled = true;

    CInventoryOwner* owner = smart_cast<CInventoryOwner*>(this->H_Parent());
    if (owner)
        bSoundsEnabled = owner->CanPlayShHdRldSounds();

    UndeployBipods();
    OnZoomOut();

    if (bSoundsEnabled)
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
