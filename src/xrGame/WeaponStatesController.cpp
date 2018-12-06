/***************************************/
/***** Контроллер состояний оружия *****/ //--#SM+#--
/***************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"
#include "Weapon_AmmoCompress.h"

// "Правильное" переключение стэйта с посылом сигнала по сети
void CWeapon::SwitchState(u32 S)
{
    if (OnClient())
        return;

#ifndef MASTER_GOLD
    if (bDebug)
    {
        Msg("---Server is going to send GE_WPN_STATE_CHANGE to [%d], weapon_section[%s], parent[%s]",
            S,
            cNameSect().c_str(),
            H_Parent() ? H_Parent()->cName().c_str() : "NULL Parent");
    }
#endif // #ifndef MASTER_GOLD

    SetNextState(S);
    if (CHudItem::object().Local() && !CHudItem::object().getDestroy() && m_pInventory && OnServer())
    {
        // !!! Just single entry for given state !!!
        NET_Packet P;
        CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
        P.w_u8(u8(S));
        P.w_u8(u8(m_sub_state));

        CAmmoCompressUtil::AMMO_VECTOR pVAmmo;
        CAmmoCompressUtil::CompressMagazine(pVAmmo, this, m_bGrenadeMode);
        CAmmoCompressUtil::PackAmmoInPacket(pVAmmo, P);

        P.w_u8(m_set_next_ammoType_on_reload);
        P.w_u8(m_set_next_magaz_on_reload);
        P.w_u16(m_set_next_magaz_by_id);
        CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
    }
}

// Колбек после переключения стэйта
void CWeapon::OnStateSwitch(u32 S, u32 oldState)
{
    m_BriefInfo_CalcFrame = 0;

    R_ASSERT(m_magazine.size() == iAmmoElapsed);
    R_ASSERT(m_magazine2.size() == iAmmoElapsed2);

    inherited::OnStateSwitch(S, oldState);

    // clang-format off
	switch (S)
	{
	case eIdle:
		{
		switch2_Idle();
		}break;
	case eFire:
	case eFire2:
		{
		switch2_Fire();
		}break;
	case eMisfire:
		{
		switch2_Misfire();
		}break;
	case eKick:
		{
		switch2_Kick();
		}break;
	case eMagEmpty:
		{
		switch2_Empty();
		}break;
	case eReload:
		{
		switch2_Reload();
		}break;
	case eReloadFrAB:
		{
		switch2_ReloadFrAB();
		}break;
	case eSwitchMag:
		{
		switch2_SwitchMag();
		}break;
	case eShowing:
		{
		switch2_Showing();
		}break;
	case eHiding:
		{
		switch2_Hiding();
		}break;
	case eHidden:
		{
		switch2_Hidden();
		}break;
	case eSwitch:
		{
		switch2_Switch();
		}break;
	case ePump:
		{
		switch2_Pump();
		}break;
	}
    // clang-format on

    UpdateGrenadeVisibility();
}

// До переключения одного стэйта на другой
bool CWeapon::OnBeforeStateSwitch(u32 oldS, u32 newS)
{
    if (!inherited::OnBeforeStateSwitch(oldS, newS))
        return false;

    // clang-format off
	switch (oldS)
	{
	case eIdle:
		switchFrom_Idle	(newS);
		break;
	case eFire:
	case eFire2:
		switchFrom_Fire	(newS);
		break;
	case eMisfire:
		switchFrom_Misfire (newS);
		break;
	case eKick:
		switchFrom_Kick (newS);
		break;
	case eMagEmpty:
		switchFrom_Empty (newS);
		break;
	case eReload:
		switchFrom_Reload (newS);
		break;
	case eReloadFrAB:
		switchFrom_ReloadFrAB (newS);
		break;
	case eSwitchMag:
		switchFrom_SwitchMag (newS);
		break;
	case eBore:
		switchFrom_Bore (newS);
		break;
	case eSwitch:
		switchFrom_Switch (newS);
		break;
	case eShowing:
		switchFrom_Showing (newS);
		break;
	case eHiding:
		switchFrom_Hiding (newS);
		break;
	case eHidden:
		switchFrom_Hidden (newS);
		break;
	case ePump:
		switchFrom_Pump (newS);
		break;
	}
    // clang-format on

    return true;
}

// Апдейт стэйтов
void CWeapon::UpdateStates(float dt)
{
    // Когда происходит апдейт состояния оружия
    // ничего другого не делать

    // clang-format off
	if(GetNextState() == GetState())
	{
		switch (GetState())
		{
		case eIdle:
			{
				state_Idle			(dt);
			}break;
		case eFire:			
		case eFire2:		
			{
				state_Fire			(dt);
			}break;
		case eMisfire:			
			{
				state_Misfire		(dt);
			}break;
		case eKick:			
			{
				state_Kick			(dt);
			}break;
		case eMagEmpty:			
			{
				state_Empty			(dt);
			}break;
		case eReload:
			{
				state_Reload		(dt);
			}break;
		case eReloadFrAB:
			{
				state_ReloadFrAB	(dt);
			}break;
		case eSwitchMag:
			{
				state_SwitchMag		(dt);
			}break;
		case eBore:			
			{
				state_Bore			(dt);
			}break;
		case eSwitch:			
			{
				state_Switch		(dt);
			}break;
		case eShowing:
			{
				state_Showing		(dt);
			}break;
		case eHiding:
			{
				state_Hiding		(dt);
			}break;
		case eHidden:
			{
				state_Hidden		(dt);
			}break;
		case ePump:
			{
				state_Pump			(dt);
			}break;
		}
	}
    // clang-format on
}
