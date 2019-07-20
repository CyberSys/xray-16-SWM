/*******************************************/
/***** Различные функции для инвентаря *****/ //--#SM+#--
/*******************************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "Weapon_LuaAdapter.hpp"

// Получить текущий вес оружия
float CWeapon::Weight() const
{
    float res = CInventoryItemObject::Weight();

    // Прибавляем вес аддонов
    if (IsGrenadeLauncherAttached())
    {
        const shared_str sAddonSect = GetGrenadeLauncherName();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsScopeAttached())
    {
        const shared_str sAddonSect = GetScopeName();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSilencerAttached())
    {
        const shared_str sAddonSect = GetSilencerName();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsMagazineAttached())
    {
        const shared_str sAddonSect = GetMagazineName();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSpecial_1_Attached())
    {
        const shared_str sAddonSect = GetSpecial_1_Name();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSpecial_2_Attached())
    {
        const shared_str sAddonSect = GetSpecial_2_Name();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSpecial_3_Attached())
    {
        const shared_str sAddonSect = GetSpecial_3_Name();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSpecial_4_Attached())
    {
        const shared_str sAddonSect = GetSpecial_4_Name();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSpecial_5_Attached())
    {
        const shared_str sAddonSect = GetSpecial_5_Name();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }
    if (IsSpecial_6_Attached())
    {
        const shared_str sAddonSect = GetSpecial_6_Name();
        res += READ_IF_EXISTS(
            pSettings, r_float, sAddonSect, "inv_weight_inst", pSettings->r_float(sAddonSect, "inv_weight"));
    }

    // Прибавляем вес патронов
    if (iAmmoElapsed)
    {
        float w  = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "inv_weight");
        float bs = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "box_size");

        res += w * (iAmmoElapsed / bs);
    }

    if (iAmmoElapsed2)
    {
        float w = pSettings->r_float(m_ammoTypes2[m_ammoType2].c_str(), "inv_weight");
        float bs = pSettings->r_float(m_ammoTypes2[m_ammoType2].c_str(), "box_size");

        res += w * (iAmmoElapsed2 / bs);
    }

    return res;
}

// Получить текущую стоймость оружия
u32 CWeapon::Cost() const
{
    u32 res = CInventoryItem::Cost();

    // Прибавляем стоймость аддонов
    if (IsGrenadeLauncherAttached())
    {
        res += pSettings->r_u32(GetGrenadeLauncherName(), "cost");
    }
    if (IsScopeAttached())
    {
        res += pSettings->r_u32(GetScopeName(), "cost");
    }
    if (IsSilencerAttached())
    {
        res += pSettings->r_u32(GetSilencerName(), "cost");
    }
    if (IsMagazineAttached())
    {
        res += pSettings->r_u32(GetMagazineName(), "cost");
    }
    if (IsSpecial_1_Attached())
    {
        res += pSettings->r_u32(GetSpecial_1_Name(), "cost");
    }
    if (IsSpecial_2_Attached())
    {
        res += pSettings->r_u32(GetSpecial_2_Name(), "cost");
    }
    if (IsSpecial_3_Attached())
    {
        res += pSettings->r_u32(GetSpecial_3_Name(), "cost");
    }
    if (IsSpecial_4_Attached())
    {
        res += pSettings->r_u32(GetSpecial_4_Name(), "cost");
    }
    if (IsSpecial_5_Attached())
    {
        res += pSettings->r_u32(GetSpecial_5_Name(), "cost");
    }
    if (IsSpecial_6_Attached())
    {
        res += pSettings->r_u32(GetSpecial_6_Name(), "cost");
    }

    // Прибавляем стоймость патронов
    if (iAmmoElapsed)
    {
        float w  = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "cost");
        float bs = pSettings->r_float(m_ammoTypes[m_ammoType].c_str(), "box_size");

        res += iFloor(w * (iAmmoElapsed / bs));
    }

    if (iAmmoElapsed2)
    {
        float w = pSettings->r_float(m_ammoTypes2[m_ammoType2].c_str(), "cost");
        float bs = pSettings->r_float(m_ammoTypes2[m_ammoType2].c_str(), "box_size");

        res += iFloor(w * (iAmmoElapsed2 / bs));
    }

    return res;
}

// Проверка является ли переданная секция связанной с данным оружием (имеет значение для него)
bool CWeapon::IsNecessaryItem(const shared_str& item_sect)
{
    return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() ||
            std::find(m_ammoTypes2.begin(), m_ammoTypes2.end(), item_sect) != m_ammoTypes2.end());
}

// Сравнение двух CWeapon (для склеивания ячеек в инвентаре)
bool CWeapon::InventoryEqualTo(CWeapon* pWpnRef) const
{
    // Сравнение двух магазинов
    if (this->IsMagazine() && pWpnRef->IsMagazine())
    {
        // Сверяем макс. размер магазина
        bool bIsMainMagSizeEqual = this->GetMainMagSize() == pWpnRef->GetMainMagSize();
        if (!bIsMainMagSizeEqual)
            return false;

        // Сверяем текущее кол-во патронов
        bool bIsAmmoCntEqual = this->GetMainAmmoElapsed() == pWpnRef->GetMainAmmoElapsed();
        if (!bIsAmmoCntEqual)
            return false;

        // Сверяем текущий тип патронов
        bool bIsAmmoTypeEqual = this->GetMainAmmoType() == pWpnRef->GetMainAmmoType();
        if (!bIsAmmoTypeEqual)
            return false;

        // Сверяем типы патронов в основном магазине
        CartridgesInfoMap AmmoDataRef = pWpnRef->GetAmmoInfo(false);
        if (AmmoDataRef.size() > 1)
            return false; //--> Магазин со смешанным типом патронов внутри никогда не склеиваем
    }

    // Сравнение аддонов
    if (this->IsAddonsEqual(pWpnRef) == false)
        return false;

    return true;
}

// Получить необходимую информацию о предмете для UI
bool CWeapon::GetBriefInfo(II_BriefInfo& info)
{
    VERIFY(m_pInventory);

    if (m_bKnifeMode || m_bUIShowAmmo == false)
    {
        info.clear();
        info.name._set(m_nameShort);
        info.icon._set(cNameSect());
        return true;
    }

    string32 int_str;

    // Обновляем текущее число патронов в магазине
    int ae = GetAmmoElapsed();
    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
        if (m_set_next_ammoType_on_reload != m_ammoType)
            ae = 0;

    xr_sprintf(int_str, "%d", ae);
    info.cur_ammo._set(int_str);

    // Обновляем текущий режим стрельбы
    if (HasFireModes())
    {
        if (m_iQueueSize == WEAPON_ININITE_QUEUE)
            info.fire_mode._set("A");
        else
        {
            xr_sprintf(int_str, "%d", m_iQueueSize);
            info.fire_mode._set(int_str);
        }
    }
    else
        info.fire_mode._set("");

    if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
        return false;

    // Подсчитываем число патронов текущего типа и число патронов других типов в основном магазине
    if (m_bUseMagazines == false)
    {
        u32 at_size = m_bGrenadeMode ? m_ammoTypes2.size() : m_ammoTypes.size(); //--> Возможное кол-во типов патронов в основном магазине
        if (unlimited_ammo() || at_size == 0)
        {
            info.fmj_ammo._set("--");
            info.ap_ammo._set("--");
        }
        else
        {
            u8 ammo_type = m_bGrenadeMode ? m_ammoType2 : m_ammoType; //--> Получаем тип патронов в основном магазине
            if (m_set_next_ammoType_on_reload != undefined_ammo_type)
                ammo_type = m_set_next_ammoType_on_reload;

            int iTotalCurAmmo     = 0;
            int iTotalAviableAmmo = 0;

            for (u8 i = 0; i < at_size; i++)
            {
                int iTotalAmmo = m_bGrenadeMode ? GetAmmoCount2(i) : GetAmmoCount(i);
                iTotalAviableAmmo += iTotalAmmo;

                if (i == ammo_type)
                    iTotalCurAmmo = iTotalAmmo;
            }

            xr_sprintf(int_str, "%d", iTotalCurAmmo);
            info.fmj_ammo._set(int_str); //--> Число патронов текущего типа
            xr_sprintf(int_str, "%d", iTotalAviableAmmo - iTotalCurAmmo);
            info.ap_ammo._set(int_str); //--> Число патронов всех остальных типов
        }
    }
    else
    {
        // При магазинном питании подсчитываем число патронов во всех магазинах
        int iTotalAmmoInMagazines = 0;
        int iTotalMagazines       = 0;

        TIItemContainer::iterator itb = m_pInventory->m_ruck.begin();
        TIItemContainer::iterator ite = m_pInventory->m_ruck.end();
        for (; itb != ite; ++itb)
        {
            CWeapon* pWpn = smart_cast<CWeapon*>(*itb);
            if (pWpn && GetAddonSlot(pWpn) == eMagaz)
            {
                int iAmmo = pWpn->GetMainAmmoElapsed();
                iTotalAmmoInMagazines += iAmmo;
                if (iAmmo > 0)
                    iTotalMagazines++;
            }
        }

        xr_sprintf(int_str, "%d", iTotalAmmoInMagazines);
        info.fmj_ammo._set(int_str);
        xr_sprintf(int_str, "%d", iTotalMagazines);
        info.ap_ammo._set(int_str);
    }

    // Подсчитываем число доступных патронов в подствольнике (все типы)
    if (!IsGrenadeLauncherAttached())
    {
        info.grenade = "";
    }
    else
    {
        if (unlimited_ammo())
            xr_sprintf(int_str, "--");
        else
        {
            u32 at_size2 = m_bGrenadeMode ? m_ammoTypes.size() : m_ammoTypes2.size(); //--> Возможное кол-во типов патронов в подствольном магазине
            int iTotalAviableAmmo = 0;

            for (u8 i = 0; i < at_size2; i++)
                iTotalAviableAmmo += m_bGrenadeMode ? GetAmmoCount(i) : GetAmmoCount2(i);

            if (iTotalAviableAmmo > 0)
                xr_sprintf(int_str, "%d", iTotalAviableAmmo);
            else
                xr_sprintf(int_str, "X");
        }

        info.grenade = int_str;
    }

    // Устанавливаем иконку патронов текущего типа
    if (m_magazine.size() != 0 && m_set_next_ammoType_on_reload == undefined_ammo_type)
    {
        LPCSTR ammo_type = m_ammoTypes[m_magazine.back()->m_LocalAmmoType].c_str();
        info.name._set(StringTable().translate(pSettings->r_string(ammo_type, "inv_name_short")));
        info.icon._set(ammo_type);
    }
    else
    {
        LPCSTR ammo_type = ((m_set_next_ammoType_on_reload == undefined_ammo_type) ? m_ammoTypes[m_ammoType].c_str() :
                                                                                     m_ammoTypes[m_set_next_ammoType_on_reload].c_str());
        info.name._set(StringTable().translate(pSettings->r_string(ammo_type, "inv_name_short")));
        info.icon._set(ammo_type);
    }

    return true;
}

// Разрешена-ли мгновенная перезарядка из инвентаря
bool CWeapon::InventoryFastReloadAllowed(bool bForGL) const
{
    // Подствольники нельзя так перезаряжать
    if (bForGL == true)
        return false;

    // Разрешаем только магазины в инвентаре ГГ
    bool bAllowed = IsMagazine() && C_THIS_WPN->ParentIsActor();

    // Дополнительно проверяем в Lua, что сейчас можно делать быструю перезарядку
    if (bAllowed)
    {
        CLuaWpnAdapter& WpnLua = CLuaWpnAdapter::Get();
        if (WpnLua.m_bIsFastReloadAllowedFnExist)
            bAllowed = WpnLua.m_lfnIsFastReloadAllowed();
    }

    return bAllowed;
}

// Мгновенная перезарядка из инвентаря
void CWeapon::InventoryFastReload(u8 ammoType, bool bForGL)
{
    if (InventoryFastReloadAllowed(bForGL) == false)
        return;

    if (bForGL)
        ReloadGLMagazineWithType(ammoType);
    else
        ReloadMainMagazineWithType(ammoType);
}
