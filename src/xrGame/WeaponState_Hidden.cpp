/***************************************/
/***** Состояние "Оружие спрятано" *****/ //--#SM+#--
/***************************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "Weapon_AmmoCompress.h"

// Переключение стэйта на "Спрятано"
void CWeapon::switch2_Hidden()
{
    StopCurrentAnimWithoutCallback();
    signal_HideComplete();
}

// Переключение на другой стэйт из стэйта "Спрятано"
void CWeapon::switchFrom_Hidden(u32 newS) {}

// Обновление оружия в состоянии "Спрятано"
void CWeapon::state_Hidden(float dt) {}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Вызывается либо сразу после скрытия, либо всегда когда оружие скрыто
void CWeapon::SendHiddenItem()
{
    if (!CHudItem::object().getDestroy() && m_pInventory)
    {
        // !!! Just single entry for given state !!!
        NET_Packet P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(eHiding));
        P.w_u8(u8(m_sub_state));

        CAmmoCompressUtil::AMMO_VECTOR pVAmmo;
        CAmmoCompressUtil::CompressMagazine(pVAmmo, this, m_bGrenadeMode);
        CAmmoCompressUtil::PackAmmoInPacket(pVAmmo, P);

        P.w_u8(m_set_next_ammoType_on_reload);
        P.w_u8(m_set_next_magaz_on_reload);
        P.w_u16(m_set_next_magaz_by_id);
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
        SetPending(TRUE);
    }
}
