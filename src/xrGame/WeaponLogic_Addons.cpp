#include "StdAfx.h"
#include "Weapon.h"
#include "WeaponKnifeHit.h"
#include "player_hud.h"

/*************************/
/***** Аддоны оружия *****/ //--#SM+#--
/*************************/

extern float g_defScopeZoomFactor;

// Проверяет одинаковы-ли аддоны двух CWeapon
bool CWeapon::IsAddonsEqual(CWeapon* pWpn2Cmp) const
{
    // Сперва проверяем типы аддонов
    for (int i = 0; i < EAddons::eAddonsSize; i++)
    {
        if (m_addons[i].bActive != pWpn2Cmp->m_addons[i].bActive)
            return false;
    }

    // Потом, если понадобилось, сравниваем их секции
    for (int i = 0; i < EAddons::eAddonsSize; i++)
    {
        if (m_addons[i].bActive == true && pWpn2Cmp->m_addons[i].bActive == true)
            if (m_addons[i].GetName() != pWpn2Cmp->m_addons[i].GetName())
                return false;
    }

    return true;
}

// Пакует присоединённые аддоны в число-флаг (не учитывает их индексы)
u16 CWeapon::GetAddonsState() const
{
    u16 m_flagsAddOnState = 0;

    if (IsGrenadeLauncherAttached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

    if (IsScopeAttached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;

    if (IsSilencerAttached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;

    if (IsMagazineAttached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonMagazine;

    if (IsSpecial_1_Attached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSpecial_1;

    if (IsSpecial_2_Attached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSpecial_2;

    if (IsSpecial_3_Attached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSpecial_3;

    if (IsSpecial_4_Attached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSpecial_4;

    if (IsSpecial_5_Attached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSpecial_5;

    if (IsSpecial_6_Attached())
        m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSpecial_6;

    return m_flagsAddOnState;
}

// Распаковывает число-флаг и присоединяет соответствующие аддоны к оружию (по 0 индексу)
void CWeapon::SetAddonsState(u16 m_flagsAddOnState)
{
    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)
        InstallAddon(eLauncher, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonScope)
        InstallAddon(eScope, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSilencer)
        InstallAddon(eSilencer, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonMagazine)
        InstallAddon(eMagaz, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSpecial_1)
        InstallAddon(eSpec_1, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSpecial_2)
        InstallAddon(eSpec_2, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSpecial_3)
        InstallAddon(eSpec_3, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSpecial_4)
        InstallAddon(eSpec_4, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSpecial_5)
        InstallAddon(eSpec_5, first_addon_idx, true);

    if (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonSpecial_6)
        InstallAddon(eSpec_6, first_addon_idx, true);

    UpdateAddons();
    ResetIdleAnim();
}

// Установить аддон по его set-секции в оружии (не требует предмета)
// Не производит удаление или возврат предмета (для этого есть Attach \ Detach) <!>
SAddonData* CWeapon::InstallAddon(EAddons iSlot, const shared_str& sAddonSetSect, bool bNoUpdate)
{
    u8 addon_idx = empty_addon_idx;
    GetAddonSlotBySetSect(sAddonSetSect.c_str(), &addon_idx, iSlot);

    return InstallAddon(iSlot, addon_idx, bNoUpdate);
}

// Установить аддон (не требует предмета)
// Не производит удаление или возврат предмета (для этого есть Attach \ Detach) <!>
SAddonData* CWeapon::InstallAddon(EAddons iSlot, u8 addon_idx, bool bNoUpdate)
{
    SAddonData* pAddon = GetAddonBySlot(iSlot);

    if (pAddon->m_attach_status == ALife::eAddonPermanent)
        addon_idx = 0;

    if (pAddon->addon_idx == addon_idx)
        return pAddon;

    if (addon_idx == empty_addon_idx)
    {
        return UnistallAddon(iSlot, bNoUpdate);
    }

    if (pAddon->AddonsCount() > 0 && addon_idx < pAddon->AddonsCount())
    {
        pAddon->addon_idx   = addon_idx;
        pAddon->bActive     = true;

        // Обновляем список несовместимых аддонов
        LPCSTR sDisallowStr = READ_IF_EXISTS(pSettings, r_string, pAddon->GetName(), WEAPON_ADDON_DIS_L, nullptr);
        if (sDisallowStr != nullptr)
        {
            string128 _sect_name;
            int count = _GetItemCount(sDisallowStr);
            for (int i = 0; i < count; ++i)
            {
                LPCSTR _itm = _GetItem(sDisallowStr, i, _sect_name);
                m_IncompatibleAddons.insert(_itm);
            }
        }

        OnAddonInstall(iSlot, pAddon->GetName());

        if (!bNoUpdate)
        {
            UpdateAddons();
            ResetIdleAnim();
        }
    }

    return pAddon;
}

// Отсоединить аддон (не возвращает предмет в инвентарь)
// Не производит удаление или возврат предмета (для этого есть Attach \ Detach) <!>
SAddonData* CWeapon::UnistallAddon(EAddons iSlot, bool bNoUpdate)
{
    SAddonData* pAddon = GetAddonBySlot(iSlot);

    if (pAddon->bActive == false && pAddon->addon_idx == empty_addon_idx)
        return pAddon;

    if (pAddon->m_attach_status == ALife::eAddonPermanent)
        return pAddon;

    // Обновляем список несовместимых аддонов
    LPCSTR sDisallowStr = READ_IF_EXISTS(pSettings, r_string, pAddon->GetName(), WEAPON_ADDON_DIS_L, nullptr);
    if (sDisallowStr != nullptr)
    {
        string128 _sect_name;
        int count = _GetItemCount(sDisallowStr);
        for (int i = 0; i < count; ++i)
        {
            LPCSTR _itm = _GetItem(sDisallowStr, i, _sect_name);

            const auto it = m_IncompatibleAddons.find(_itm);
            if (it != m_IncompatibleAddons.end())
                m_IncompatibleAddons.erase(it); //--> Уменьшаем счётчик на единицу
        }
    }

    OnAddonUnistall(iSlot, pAddon->GetName());

    pAddon->addon_idx   = empty_addon_idx;
    pAddon->bActive     = false;

    if (!bNoUpdate)
    {
        UpdateAddons();
        ResetIdleAnim();
    }

    return pAddon;
}

// Колбэк на установку аддона
void CWeapon::OnAddonInstall(EAddons iSlot, const shared_str& sAddonSetSect)
{
    const shared_str& sAddonName = GetAddonBySlot(iSlot)->GetAddonName();

    // Изменяем характеристики оружия коэфицентами аддона
    GetAddonBySlot(iSlot)->ApplyWeaponKoefs();

    // Установка патронташа
    bool bIsAmmoBelt = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_ammo_belt", false);
    if (bIsAmmoBelt)
    {
        R_ASSERT4(IsAmmoBeltAttached() == false,
            "Ammo Belt already installed <!>",
            sAddonSetSect.c_str(),
            GetAddonBySlot(m_AmmoBeltSlot)->GetName().c_str());
        m_AmmoBeltSlot = iSlot;
    }

    // Установка рукоятки
    bool bIsForegrip = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_foregrip", false);
    if (bIsForegrip)
    {
        R_ASSERT4(IsForegripAttached() == false,
            "Foregrip already installed <!>",
            sAddonSetSect.c_str(),
            GetAddonBySlot(m_ForegripSlot)->GetName().c_str());
        m_ForegripSlot = iSlot;
    }

    // Установка пламегасителя
    bool bIsFlashHider = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_flashider", false);
    if (bIsFlashHider)
    {
        R_ASSERT4(IsFlashHiderAttached() == false,
            "FlashHider already installed <!>",
            sAddonSetSect.c_str(),
            GetAddonBySlot(m_FlashHiderSlot)->GetName().c_str());
        m_FlashHiderSlot = iSlot;
    }

    // Установка цевья
    bool bIsForend = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_forend", false);
    if (bIsForend)
    {
        R_ASSERT4(
            IsForendAttached() == false, "Forend already installed <!>", sAddonSetSect.c_str(), GetAddonBySlot(m_ForendSlot)->GetName().c_str());
        m_ForendSlot = iSlot;
    }

    // Установка сошек
    bool bIsBipods = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_bipods", false);
    if (bIsBipods)
    {
        R_ASSERT4(IsBipodsAttached() == false,
            "Bipods already installed <!>",
            sAddonSetSect.c_str(),
            GetAddonBySlot(m_BipodsSlot)->GetName().c_str());
        OnBipodsAttach(iSlot, sAddonSetSect);
    }

    // Установка штык-ножа
    bool bIsBayonet = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_bayonet", false);
    if (bIsBayonet)
    {
        R_ASSERT4(
            IsBayonetAttached() == false, "Bayonet already installed <!>", sAddonSetSect.c_str(), GetAddonBySlot(m_BayonetSlot)->GetName().c_str());
        Need2Stop_Kick();
        m_BayonetSlot = iSlot;
    }

    R_ASSERT4((IsGrenadeLauncherAttached() == false) || m_ForegripSlot == eNotExist,
        "Grenade Launcher and Foregrip can't be combined <!>",
        GetAddonBySlot(m_ForegripSlot)->GetName().c_str(),
        GetAddonBySlot(eLauncher)->GetName().c_str());
    R_ASSERT4((IsGrenadeLauncherAttached() == false) || m_bUseAmmoBeltMode == false,
        "Grenade Launcher and Ammo Belt can't be combined <!>",
        GetAddonBySlot(m_AmmoBeltSlot)->GetName().c_str(),
        GetAddonBySlot(eLauncher)->GetName().c_str());

    if (iSlot == eLauncher || bIsAmmoBelt || bIsForegrip)
        Need2Stop_Reload();

    // Установка магазина
    if (iSlot == eMagaz)
    {
        SAddonData* pAddonAB = GetAddonBySlot(eMagaz);
        LoadMainAmmoParams(pAddonAB->GetAddonName().c_str(), false, m_bUseMagazines);
    }
}

// Колбэк на снятие аддона
void CWeapon::OnAddonUnistall(EAddons iSlot, const shared_str& sAddonSetSect)
{
    // Сбрасываем коэфиценты характеристик оружия
    GetAddonBySlot(iSlot)->ResetWeaponKoefs();

    // Снимаем все аддоны, зависящие от нашего
    for (int iSlotNext = 0; iSlotNext < EAddons::eAddonsSize; iSlotNext++)
    {
        if (iSlotNext != iSlot)
        {
            SAddonData* pAddonNext = GetAddonBySlot((EAddons)iSlotNext);
            if (pAddonNext->bActive)
            {
                shared_str sRequiredAddonSetSect =
                    READ_IF_EXISTS(pSettings, r_string, pAddonNext->GetName(), WEAPON_ADDON_REQ_L, nullptr);
                if (sRequiredAddonSetSect != nullptr && sRequiredAddonSetSect.equal(sAddonSetSect))
                {
                    Detach(pAddonNext->GetAddonName().c_str(), true, false);
                }
            }
        }
    }

    // Снятие патронташа
    bool bIsAmmoBelt = (IsAmmoBeltAttached() && iSlot == m_AmmoBeltSlot);
    if (bIsAmmoBelt)
        m_AmmoBeltSlot = eNotExist;

    // Снятие рукоятки
    bool bIsForegrip = (IsForegripAttached() && iSlot == m_ForegripSlot);
    if (bIsForegrip)
        m_ForegripSlot = eNotExist;

    // Снятие пламегасителя
    bool bIsFlashHider = (IsFlashHiderAttached() && iSlot == m_FlashHiderSlot);
    if (bIsFlashHider)
        m_FlashHiderSlot = eNotExist;

    // Снятие цевья
    bool bIsForend = (IsForendAttached() && iSlot == m_ForendSlot);
    if (bIsForend)
        m_ForendSlot = eNotExist;

    if (iSlot == eLauncher || bIsAmmoBelt)
    {
        Need2Stop_Reload();

        // Разряжаем подствольник \ патронташ
        if (!m_bGrenadeMode)
            PerformSwitchGL(); // Включаем подствольник \ патронташ

        R_ASSERT(m_bGrenadeMode == true);

        UnloadMagazine();  // Разряжаем
        PerformSwitchGL(); // Выключаем
    }

    // Снятие магазина
    if (iSlot == eMagaz)
        LoadMainAmmoParams(cNameSect_str(), false, m_bUseMagazines);

    // Снятие сошек
    bool bIsBipods = (IsBipodsAttached() && iSlot == m_BipodsSlot);
    if (bIsBipods)
        OnBipodsDetach(sAddonSetSect);

    // Снятие штык-ножа
    bool bIsBayonet = (IsBayonetAttached() && iSlot == m_BayonetSlot);
    if (bIsBayonet)
    {
        Need2Stop_Kick();
        m_BayonetSlot = eNotExist;
    }
}

// Колбэк на спавн аддона после снятия
void CWeapon::OnDetachAddonSpawn(const char* item_section_name, CSE_ALifeDynamicObject* E)
{
    if (m_bUseMagazines == false)
        return;

    CSE_ALifeItemWeapon* const EW = smart_cast<CSE_ALifeItemWeapon*>(E); // Addon item
    if (EW == NULL)
        return;

    if (GetAddonSlot(item_section_name) != eMagaz)
        return;

    // Копируем патроны из текущего оружия в его отсоединённый магазин
    CAmmoCompressUtil::AMMO_VECTOR pVAmmoMain;
    CAmmoCompressUtil::CompressMagazine(pVAmmoMain, this, false);
    EW->m_pAmmoMain = pVAmmoMain;
}

// Обновить состояние оружия в соответствии с текущей конфигурацией аддонов
void CWeapon::UpdateAddons()
{
    InitAddons();
    UpdateAddonsVisibility();
    UpdateGrenadeVisibility(); //--> Ракета РПГ
}

// Принудительно попытаться обновить текущую Idle-анимацию (при установке аддонов)
void CWeapon::ResetIdleAnim()
{
    if (IsPending() == FALSE)
        PlayAnimIdle();
}

/* Загрузить параметры аддонов оружия из конфига
   section - секция откуда грузить данные (секция оружия или апгрейда);
   *upgrMode - режим работы при загрузке данных из секции апгрейда (отсутствует, тест, установка);
*/
bool CWeapon::LoadAddons(LPCSTR section, EFuncUpgrMode upgrMode)
{
    //******** Обратная совместимость с оригинальной системой аддонов ********//

    ALife::EWeaponAddonStatus scope_status = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_s32, section, "scope_status", 0);
    if ((scope_status == ALife::eAddonPermanent) && !pSettings->line_exist(section, "scopes_sect"))
    {
        // Прицел (Для новой системы скрытия\показа костей на худе, сокет-аддонов)
        string256 m_sScopeName_str;
        xr_strcpy(m_sScopeName_str, section);
        xr_strcat(m_sScopeName_str, "_scp_gen");

        pSettings->w_string(section, "scopes_sect", m_sScopeName_str);
        pSettings->w_string(m_sScopeName_str, "scope_name", section);
        pSettings->w_s32(m_sScopeName_str, "scope_x", 0);
        pSettings->w_s32(m_sScopeName_str, "scope_y", 0);
    }

    ALife::EWeaponAddonStatus sil_status = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_s32, section, "silencer_status", 0);
    if ((sil_status == ALife::eAddonAttachable || sil_status == ALife::eAddonPermanent) && !pSettings->line_exist(section, "silencers_sect"))
    {
        // Глушитель
        LPCSTR m_sSilencerName = pSettings->r_string(section, "silencer_name");
        int    m_iSilencerX    = pSettings->r_s32(section, "silencer_x");
        int    m_iSilencerY    = pSettings->r_s32(section, "silencer_y");

        string256 m_sSilencerName_str;
        xr_strcpy(m_sSilencerName_str, section);
        xr_strcat(m_sSilencerName_str, "_sil_gen");

        pSettings->w_string(section, "silencers_sect", m_sSilencerName_str);
        pSettings->w_string(m_sSilencerName_str, "silencer_name", m_sSilencerName);
        pSettings->w_s32(m_sSilencerName_str, "silencer_x", m_iSilencerX);
        pSettings->w_s32(m_sSilencerName_str, "silencer_y", m_iSilencerY);
    }

    ALife::EWeaponAddonStatus grl_status = (ALife::EWeaponAddonStatus)READ_IF_EXISTS(pSettings, r_s32, section, "grenade_launcher_status", 0);
    if ((grl_status == ALife::eAddonAttachable || grl_status == ALife::eAddonPermanent) && !pSettings->line_exist(section, "launchers_sect"))
    {
        // Подствольник
        LPCSTR m_sGrenadeLauncherName = pSettings->r_string(section, "grenade_launcher_name");
        int    m_iGrenadeLauncherX    = pSettings->r_s32(section, "grenade_launcher_x");
        int    m_iGrenadeLauncherY    = pSettings->r_s32(section, "grenade_launcher_y");

        string256 m_sGrenadeLauncherName_str;
        xr_strcpy(m_sGrenadeLauncherName_str, section);
        xr_strcat(m_sGrenadeLauncherName_str, "_gl_gen");

        pSettings->w_string(section, "launchers_sect", m_sGrenadeLauncherName_str);
        pSettings->w_string(m_sGrenadeLauncherName_str, "launcher_name", m_sGrenadeLauncherName);
        pSettings->w_s32(m_sGrenadeLauncherName_str, "launcher_x", m_iGrenadeLauncherX);
        pSettings->w_s32(m_sGrenadeLauncherName_str, "launcher_y", m_iGrenadeLauncherY);

        if (!pSettings->line_exist(section, "scope_zoom_factor_gl"))
            pSettings->w_float(
                section, "scope_zoom_factor_gl", READ_IF_EXISTS(pSettings, r_float, section, "scope_zoom_factor", g_defScopeZoomFactor));
    }

    //******** Инициализируем массив аддонов оружия ********//
    bool bAddonsInitialized = (upgrMode == eUpgrNone ? true : false);

    /////////////////////
    // clang-format off
#define DEF_InitAddonSlot(DEF_slot, DEF_sects, DEF_name, DEF_status, DEF_bone)                      \
    {                                                                                               \
        if (upgrMode == eUpgrNone || pSettings->line_exist(section, DEF_status)                     \
                                  || pSettings->line_exist(section, DEF_sects))                     \
        {                                                                                           \
            if (upgrMode != eUpgrTest)                                                              \
                m_addons[DEF_slot].Initialize(section, DEF_sects, DEF_name, DEF_status, DEF_bone);  \
            bAddonsInitialized = true;                                                              \
        }                                                                                           \
    }                                                                                           

    DEF_InitAddonSlot(eScope,    "scopes_sect",     "scope_name",    "scope_status",            "wpn_scope");
    DEF_InitAddonSlot(eSilencer, "silencers_sect",  "silencer_name", "silencer_status",         "wpn_silencer");
    DEF_InitAddonSlot(eLauncher, "launchers_sect",  "launcher_name", "grenade_launcher_status", "wpn_launcher");
    DEF_InitAddonSlot(eMagaz,    "magazines_sect",  "magazine_name", "magazine_status",         "wpn_a_magazine");
    DEF_InitAddonSlot(eSpec_1,   "specials_1_sect", "special_name",  "special_1_status",        "wpn_a_special_1");
    DEF_InitAddonSlot(eSpec_2,   "specials_2_sect", "special_name",  "special_2_status",        "wpn_a_special_2");
    DEF_InitAddonSlot(eSpec_3,   "specials_3_sect", "special_name",  "special_3_status",        "wpn_a_special_3");
    DEF_InitAddonSlot(eSpec_4,   "specials_4_sect", "special_name",  "special_4_status",        "wpn_a_special_4");
    DEF_InitAddonSlot(eSpec_5,   "specials_5_sect", "special_name",  "special_5_status",        "wpn_a_special_5");
    DEF_InitAddonSlot(eSpec_6,   "specials_6_sect", "special_name",  "special_6_status",        "wpn_a_special_6");
    // clang-format on

    return bAddonsInitialized;
}

//****** Инициализируем текущие аддоны на оружии ******//

// Обновить параметры оружия в соответствии одетым аддонам
void CWeapon::InitAddons()
{
    //******** Инициализируем параметры прицела ********//
    if (IsScopeAttached())
    {
        //**** Прицел одет ****//

        // Параметры НПС
        m_addon_holder_range_modifier = READ_ADDON_DATA(r_float, "holder_range_modifier", GetScopeSetSect(), GetScopeName(), m_holder_range_modifier);
        m_addon_holder_fov_modifier   = READ_ADDON_DATA(r_float, "holder_fov_modifier", GetScopeSetSect(), GetScopeName(), m_holder_fov_modifier);

        // Параметры зума
        LPCSTR section = cNameSect_str();

        m_bZoomEnabled = true; //--> Всегда разрешаем зум с прицелом

        //--> Разрешён-ли альтернативный зум
        m_bAltZoomEnabled = READ_ADDON_DATA(r_bool, "zoom_alt_enabled", GetScopeSetSect(), GetScopeName(), false);

        //--> Считываем основу из главной секции
        GetZoomParams(eZoomMain).Initialize(section, NULL, false);
        if (IsAltZoomEnabled())
            GetZoomParams(eZoomAlt).Initialize(section, "_alt", false);

        //--> Переписываем данными из секции аддона
        GetZoomParams(eZoomMain).Initialize(GetScopeName().c_str(), NULL, true);
        if (IsAltZoomEnabled())
            GetZoomParams(eZoomAlt).Initialize(GetScopeName().c_str(), "_alt", true);

        //--> Переписываем данными из секции аддона в оружии
        GetZoomParams(eZoomMain).Initialize(GetScopeSetSect().c_str(), NULL, true);
        if (IsAltZoomEnabled())
            GetZoomParams(eZoomAlt).Initialize(GetScopeSetSect().c_str(), "_alt", true);

        // Коллиматорная метка, должна быть скрыта без зума
        m_sHolographBone           = READ_ADDON_DATA(r_string, "holograph_bone", GetScopeSetSect(), GetScopeName(), NULL);
        m_fHolographRotationFactor = READ_ADDON_DATA(r_float, "holograph_rotation_factor", GetScopeSetSect(), GetScopeName(), 1.0f);
        m_vHolographOffset         = READ_ADDON_DATA(r_fvector3, "holograph_offset", GetScopeSetSect(), GetScopeName(), Fvector3().set(0.0f, 0.0f, 0.0f));
    }
    else
    {
        //**** Прицел не одет - грузим дефолт ****//
        LoadZoomParams(cNameSect_str());
    }

    // Обновляем тип прицеливания при смене любых аддонов
    SwitchZoomType(IsAltZoomEnabled() ? GetZoomType() : eZoomMain);

    //******** Инициализируем параметры глушителя ********//
    LoadSilencerKoeffs(); //--> Загружаем баллистические параметры глушителя

    if (IsSilencerAttached())
    {
        // Меняем звук выстрела
        m_sSndShotCurrent = "sndSilencerShot";

        // Меняем эффекты от выстрела
        LoadFlameParticles(*cNameSect(), "silencer_");          //--> Из конфига оружия
        LoadFlameParticles(*GetSilencerSetSect(), "silencer_"); //--> Из сэт-секции глушителя
        CheckFlameParticles(*cNameSect(), "silencer_");

        // Меняем свет от выстрела у шутера
        LoadLights(*cNameSect(), "silencer_");

        // Применяем коэфиценты глушителя
        ApplySilencerKoeffs();
    }
    else
    {
        // Меняем звук выстрела
        m_sSndShotCurrent = "sndShot";

        // Меняем эффекты от выстрела
        LoadFlameParticles(*cNameSect(), ""); //--> Из конфига оружия
        CheckFlameParticles(*cNameSect(), "");

        // Меняем свет от выстрела у шутера
        LoadLights(*cNameSect(), "");

        // Сбрасываем коэфиценты глушителя
        ResetSilencerKoeffs();
    }

    //******** Магазин от третьего лица во время перезарядки ********//
    if (IsMagazineAttached())
    {
        m_bMagaz3pHideWhileReload = READ_ADDON_DATA(r_bool, "hide_magaz3p_while_reload", GetMagazineSetSect(), GetMagazineName(), false);
        m_iMagaz3pHideStartTime   = READ_ADDON_DATA(r_u16, "hide_magaz3p_start_time", GetMagazineSetSect(), GetMagazineName(), WEAPON_MAGAZ_3P_HIDE_START_TIME);
        m_iMagaz3pHideEndTime     = READ_ADDON_DATA(r_u16, "hide_magaz3p_end_time", GetMagazineSetSect(), GetMagazineName(), WEAPON_MAGAZ_3P_HIDE_END_TIME);

        R_ASSERT3(m_iMagaz3pHideStartTime < m_iMagaz3pHideEndTime,
            "hide_magaz3p_start_time can't be > hide_magaz3p_end_time",
            GetMagazineSetSect().c_str());
    }
    else
    {
        m_bMagaz3pHideWhileReload = false;
        m_iMagaz3pHideStartTime   = 0;
        m_iMagaz3pHideEndTime     = 0;
    }
    UpdateMagazine3p(true);

    //******** Инициализируем параметры удара прикладом ********//
    LoadBayonetParams();

    //******** Инициализируем параметры патронташа ********//
    LoadAmmoBeltParams();

    //******** Инициализируем параметры подствольника ********//
    LoadGLParams();

    //******** Инициализируем параметры сошек ********//
    LoadBipodsParams();

    //******** Перезагружаем все звуки оружия ********//
    ReloadAllSoundsWithUpgrades();
}

//****** Получить слот аддона из предмета ******//

// По предмету аддона (idx - вернёт туда индекс аддона в массиве)
CWeapon::EAddons CWeapon::GetAddonSlot(IGameObject* pObj, u8* idx) const { return GetAddonSlot(pObj->cNameSect().c_str(), idx); }

// По СЕКЦИИ ПРЕДМЕТА аддона (idx - вернёт туда индекс аддона в массиве)
CWeapon::EAddons CWeapon::GetAddonSlot(LPCSTR section, u8* idx, EAddons slotID2Search) const
{
#define DEF_GetAddonSlot(DEF_slot)                                                    \
    {                                                                                 \
        SAddonData*      pAddon = GetAddonBySlot(DEF_slot);                           \
        ADDONS_VECTOR_IT it     = pAddon->m_addons_list.begin();                      \
        for (; it != pAddon->m_addons_list.end(); it++)                               \
        {                                                                             \
            if (pSettings->r_string((*it), pAddon->m_addon_alias.c_str()) == section) \
            {                                                                         \
                if (idx != NULL)                                                      \
                    *idx = u8(it - pAddon->m_addons_list.begin());                    \
                return DEF_slot;                                                      \
            }                                                                         \
        }                                                                             \
    }


    if (slotID2Search == eNotExist)
    {
        DEF_GetAddonSlot(eScope);
        DEF_GetAddonSlot(eSilencer);
        DEF_GetAddonSlot(eLauncher);
        DEF_GetAddonSlot(eMagaz);
        DEF_GetAddonSlot(eSpec_1);
        DEF_GetAddonSlot(eSpec_2);
        DEF_GetAddonSlot(eSpec_3);
        DEF_GetAddonSlot(eSpec_4);
        DEF_GetAddonSlot(eSpec_5);
        DEF_GetAddonSlot(eSpec_6);
    }
    else
        DEF_GetAddonSlot(slotID2Search);

#undef DEF_GetAddonSlot

    return eNotExist;
}

// По SET-СЕКЦИИ АДДОНА в оружии (idx - вернёт туда индекс аддона в массиве)
CWeapon::EAddons CWeapon::GetAddonSlotBySetSect(LPCSTR section, u8* idx, EAddons slotID2Search) const
{
#define DEF_GetAddonSlot(DEF_slot)                                                    \
    {                                                                                 \
        SAddonData* pAddon = GetAddonBySlot(DEF_slot);                                \
        ADDONS_VECTOR_IT it = pAddon->m_addons_list.begin();                          \
        for (; it != pAddon->m_addons_list.end(); it++)                               \
        {                                                                             \
            if ((*it).equal(section))                                                 \
            {                                                                         \
                if (idx != NULL)                                                      \
                    *idx = u8(it - pAddon->m_addons_list.begin());                    \
                return DEF_slot;                                                      \
            }                                                                         \
        }                                                                             \
    }

    if (slotID2Search == eNotExist)
    {
        DEF_GetAddonSlot(eScope);
        DEF_GetAddonSlot(eSilencer);
        DEF_GetAddonSlot(eLauncher);
        DEF_GetAddonSlot(eMagaz);
        DEF_GetAddonSlot(eSpec_1);
        DEF_GetAddonSlot(eSpec_2);
        DEF_GetAddonSlot(eSpec_3);
        DEF_GetAddonSlot(eSpec_4);
        DEF_GetAddonSlot(eSpec_5);
        DEF_GetAddonSlot(eSpec_6);
    }
    else
        DEF_GetAddonSlot(slotID2Search);

#undef DEF_GetAddonSlot

    return eNotExist;
}

//****** Проверки на присоединённость аддонов ******//

bool CWeapon::IsGrenadeLauncherAttached() const { return m_addons[eLauncher].bActive == true; }
bool CWeapon::IsScopeAttached() const { return m_addons[eScope].bActive == true; }
bool CWeapon::IsSilencerAttached() const { return m_addons[eSilencer].bActive == true; }
bool CWeapon::IsMagazineAttached() const { return m_addons[eMagaz].bActive == true; }
bool CWeapon::IsSpecial_1_Attached() const { return m_addons[eSpec_1].bActive == true; }
bool CWeapon::IsSpecial_2_Attached() const { return m_addons[eSpec_2].bActive == true; }
bool CWeapon::IsSpecial_3_Attached() const { return m_addons[eSpec_3].bActive == true; }
bool CWeapon::IsSpecial_4_Attached() const { return m_addons[eSpec_4].bActive == true; }
bool CWeapon::IsSpecial_5_Attached() const { return m_addons[eSpec_5].bActive == true; }
bool CWeapon::IsSpecial_6_Attached() const { return m_addons[eSpec_6].bActive == true; }

//****** Проверки на возможность присоединения аддонов ******//

bool CWeapon::GrenadeLauncherAttachable() const { return (ALife::eAddonAttachable == get_GrenadeLauncherStatus()); }
bool CWeapon::ScopeAttachable() const { return (ALife::eAddonAttachable == get_ScopeStatus()); }
bool CWeapon::SilencerAttachable() const { return (ALife::eAddonAttachable == get_SilencerStatus()); }
bool CWeapon::MagazineAttachable() const { return (ALife::eAddonAttachable == get_MagazineStatus()); }
bool CWeapon::Special_1_Attachable() const { return (ALife::eAddonAttachable == get_Special_1_Status()); }
bool CWeapon::Special_2_Attachable() const { return (ALife::eAddonAttachable == get_Special_2_Status()); }
bool CWeapon::Special_3_Attachable() const { return (ALife::eAddonAttachable == get_Special_3_Status()); }
bool CWeapon::Special_4_Attachable() const { return (ALife::eAddonAttachable == get_Special_4_Status()); }
bool CWeapon::Special_5_Attachable() const { return (ALife::eAddonAttachable == get_Special_5_Status()); }
bool CWeapon::Special_6_Attachable() const { return (ALife::eAddonAttachable == get_Special_6_Status()); }

// Обновляем визуальное состояние всех аддонов данного типа на худе
void CWeapon::_UpdateHUDAddonVisibility(SAddonData* m_pAddon, bool bForceReset)
{
    // Если аддон данного типа присоединён, то его кости\аттачи имеют приоритет над другими
    xr_vector<shared_str> hide_bones_attached; // Кости,  которые будут скрыты
    xr_vector<shared_str> show_bones_attached; // Кости,  которые будут показаны
    xr_vector<shared_str> hud_vis_attached;    // Аттачи, которые будут одеты

    bool bIsAddonActive = (bForceReset ? false: m_pAddon->bActive);

    u8 start_idx = 0;
    if (bIsAddonActive)
        start_idx = m_pAddon->addon_idx;

    HudItemData()->set_bone_visible(m_pAddon->m_addon_bone, bIsAddonActive, TRUE); //-- Для совместимости с оригиналом

    // Перебираем все аддоны данного типа и производим операции над костями и аттачами модели
    for (u32 i = start_idx; i < m_pAddon->m_addons_list.size();)
    {
        // Если условие true, значит элемент с этим индексом мы уже обработали первым
        if (start_idx == i && start_idx > 0 && bIsAddonActive == false)
        {
            i++;
            continue;
        }

        const shared_str& addon_set_sect  = m_pAddon->m_addons_list[i];
        const shared_str& addon_name_sect = pSettings->r_string(addon_set_sect.c_str(), m_pAddon->m_addon_alias.c_str());

        xr_vector<shared_str> hide_bones_vec; // Кости для скрытия у текущего аддона цикла
        xr_vector<shared_str> show_bones_vec; // Кости для показа у текущего аддона цикла
        xr_vector<shared_str> hud_vis_vec;    // Аттачи у текущего аддона цикла

        /////////////////////
        // clang-format off
		LPCSTR hide_bones = READ_IF_EXISTS(pSettings, r_string, addon_set_sect,		"hide_bones_hud",									
							READ_IF_EXISTS(pSettings, r_string, addon_name_sect,	"hide_bones_hud", NULL)								
		);																															
		LPCSTR show_bones = READ_IF_EXISTS(pSettings, r_string, addon_set_sect,		"show_bones_hud",									
							READ_IF_EXISTS(pSettings, r_string, addon_name_sect,	"show_bones_hud", NULL)								
		);																															
		LPCSTR hud_vis	  = m_pAddon->GetVisuals("visuals_hud", false, i).c_str();
        // clang-format on

        // Парсим строки
        if (hide_bones)
        {
            string128 _itm_name;
            int       count = _GetItemCount(hide_bones);
            for (int it = 0; it < count; ++it)
            {
                LPCSTR _itm = _GetItem(hide_bones, it, _itm_name);
                hide_bones_vec.push_back(_itm);

                if (bIsAddonActive) // Для присоединённого аддона добавляем ещё и в отдельный вектор
                    hide_bones_attached.push_back(_itm);
            }
        }
        if (show_bones)
        {
            string128 _itm_name;
            int       count = _GetItemCount(show_bones);
            for (int it = 0; it < count; ++it)
            {
                LPCSTR _itm = _GetItem(show_bones, it, _itm_name);
                show_bones_vec.push_back(_itm);

                if (bIsAddonActive)
                    show_bones_attached.push_back(_itm);
            }
        }
        if (hud_vis)
        {
            string128 _itm_name;
            int       count = _GetItemCount(hud_vis);
            for (int it = 0; it < count; ++it)
            {
                LPCSTR _itm = _GetItem(hud_vis, it, _itm_name);
                hud_vis_vec.push_back(_itm);

                if (bIsAddonActive)
                    hud_vis_attached.push_back(_itm);
            }
        }

        // Скрываем\раскрываем нужные кости и аттачи на худе
        for (u32 j = 0; j < hide_bones_vec.size(); j++)
            if (bIsAddonActive || std::find(hide_bones_attached.begin(), hide_bones_attached.end(), hide_bones_vec[j]) == hide_bones_attached.end())
                HudItemData()->set_bone_visible(hide_bones_vec[j], !bIsAddonActive, TRUE);

        for (u32 j = 0; j < show_bones_vec.size(); j++)
            if (bIsAddonActive || std::find(show_bones_attached.begin(), show_bones_attached.end(), show_bones_vec[j]) == show_bones_attached.end())
                HudItemData()->set_bone_visible(show_bones_vec[j], bIsAddonActive, TRUE);
        for (u32 j = 0; j < hud_vis_vec.size(); j++)
            if (bIsAddonActive || std::find(hud_vis_attached.begin(), hud_vis_attached.end(), hud_vis_vec[j]) == hud_vis_attached.end())
                HudItemData()->UpdateChildrenList(hud_vis_vec[j], bIsAddonActive);
        /////////////////////

        // Решаем какой элемент обработать дальше
        if (bIsAddonActive)
        {
            if (start_idx > 0)
                i = 0;
            else
                i = 1;

            bIsAddonActive = false;
        }
        else
        {
            i++;
        }
    }

    show_bones_attached.clear();
    hide_bones_attached.clear();
    hud_vis_attached.clear();
}

void CWeapon::UpdateHUDAddonsVisibility(bool bForceReset)
{
    if (!ParentIsActor() || HudItemData() == nullptr)
        return;

    _UpdateHUDAddonVisibility(GetAddonBySlot(eScope), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSilencer), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eLauncher), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eMagaz), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSpec_1), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSpec_2), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSpec_3), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSpec_4), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSpec_5), bForceReset);
    _UpdateHUDAddonVisibility(GetAddonBySlot(eSpec_6), bForceReset);
}

// Обновляем отображение косточки гранаты на худе
void CWeapon::UpdateGrenadeVisibility()
{
    bool vis_hud, vis_weap;
    int  ammoElapsed = m_bGrenadeMode ? iAmmoElapsed2 : iAmmoElapsed;

    vis_hud  = (!!ammoElapsed || GetState() == eReload);
    vis_weap = !!ammoElapsed;

    if (GetHUDmode())
    {
        HudItemData()->set_bone_visible("grenade", vis_hud, TRUE);
    }

    IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual());
    VERIFY(pWeaponVisual);

    u16 BONE_ID = pWeaponVisual->LL_BoneID("grenade");
    if (BONE_ID != BI_NONE)
        pWeaponVisual->LL_SetBoneVisible(BONE_ID, vis_weap, TRUE);
}

// Обновляем визуальное состояние всех аддонов данного типа на мировой модели
void CWeapon::_UpdateAddonsVisibility(SAddonData* m_pAddon)
{
    IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual());
    R_ASSERT(pWeaponVisual);

    bool bIsAddonActive = m_pAddon->bActive;

    // Если аддон данного типа присоединён, то его кости\аттачи имеют приоритет над другими
    xr_vector<shared_str> hide_bones_attached; // Кости,  которые точно будут скрыты
    xr_vector<shared_str> show_bones_attached; // Кости,  которые точно будут показаны
    xr_vector<shared_str> ext_vis_attached;    // Аттачи, которые точно будут одеты

    u8 start_idx = 0;
    if (bIsAddonActive)
        start_idx = m_pAddon->addon_idx;

    // Для совместимости с оригиналом
    u16 def_bone_id = pWeaponVisual->LL_BoneID(m_pAddon->m_addon_bone);
    if (def_bone_id != BI_NONE)
        if ((bool)pWeaponVisual->LL_GetBoneVisible(def_bone_id) != bIsAddonActive)
            pWeaponVisual->LL_SetBoneVisible(def_bone_id, bIsAddonActive, TRUE);

    // Перебираем все аддоны данного типа и производим операции над костями и аттачами модели
    for (u32 i = start_idx; i < m_pAddon->m_addons_list.size();)
    {
        // Если условие true, значит элемент с этим индексом мы уже обработали первым
        if (start_idx == i && start_idx > 0 && bIsAddonActive == false)
        {
            i++;
            continue;
        }

        const shared_str& addon_set_sect  = m_pAddon->m_addons_list[i];
        const shared_str& addon_name_sect = pSettings->r_string(addon_set_sect.c_str(), m_pAddon->m_addon_alias.c_str());

        xr_vector<shared_str> hide_bones_vec; // Кости для скрытия у текущего аддона цикла
        xr_vector<shared_str> show_bones_vec; // Кости для показа у текущего аддона цикла
        xr_vector<shared_str> ext_vis_vec;    // Аттачи у текущего аддона цикла

        /////////////////////
        // clang-format off
		LPCSTR hide_bones = READ_IF_EXISTS(pSettings, r_string, addon_set_sect,		"hide_bones",									
							READ_IF_EXISTS(pSettings, r_string, addon_name_sect,	"hide_bones",	 NULL)								
		);																															
		LPCSTR show_bones = READ_IF_EXISTS(pSettings, r_string, addon_set_sect,		"show_bones",									
							READ_IF_EXISTS(pSettings, r_string, addon_name_sect,	"show_bones",	 NULL)								
		);																															
        LPCSTR world_vis  = m_pAddon->GetVisuals("visuals_world", false, i).c_str();
        // clang-format on

        // Парсим строки
        if (hide_bones)
        {
            string128 _itm_name;
            int       count = _GetItemCount(hide_bones);
            for (int it = 0; it < count; ++it)
            {
                LPCSTR _itm = _GetItem(hide_bones, it, _itm_name);
                hide_bones_vec.push_back(_itm);

                if (bIsAddonActive) // Для присоединённого аддона добавляем ещё и в отдельный вектор
                    hide_bones_attached.push_back(_itm);
            }
        }
        if (show_bones)
        {
            string128 _itm_name;
            int       count = _GetItemCount(show_bones);
            for (int it = 0; it < count; ++it)
            {
                LPCSTR _itm = _GetItem(show_bones, it, _itm_name);
                show_bones_vec.push_back(_itm);

                if (bIsAddonActive)
                    show_bones_attached.push_back(_itm);
            }
        }
        if (world_vis)
        {
            string128 _itm_name;
            int       count = _GetItemCount(world_vis);
            for (int it = 0; it < count; ++it)
            {
                LPCSTR _itm = _GetItem(world_vis, it, _itm_name);
                ext_vis_vec.push_back(_itm);

                if (bIsAddonActive)
                    ext_vis_attached.push_back(_itm);
            }
        }

        // Скрываем\раскрываем нужные кости и аттачи на худе
        for (u32 j = 0; j < hide_bones_vec.size(); j++)
            if (bIsAddonActive || std::find(hide_bones_attached.begin(), hide_bones_attached.end(), hide_bones_vec[j]) == hide_bones_attached.end())
            {
                u16 bone_id = pWeaponVisual->LL_BoneID(hide_bones_vec[j]);
                if (bone_id != BI_NONE)
                    if ((bool)pWeaponVisual->LL_GetBoneVisible(bone_id) == bIsAddonActive)
                        pWeaponVisual->LL_SetBoneVisible(bone_id, !bIsAddonActive, TRUE);
            }

        for (u32 j = 0; j < show_bones_vec.size(); j++)
            if (bIsAddonActive || std::find(show_bones_attached.begin(), show_bones_attached.end(), show_bones_vec[j]) == show_bones_attached.end())
            {
                u16 bone_id = pWeaponVisual->LL_BoneID(show_bones_vec[j]);
                if (bone_id != BI_NONE)
                    if ((bool)pWeaponVisual->LL_GetBoneVisible(bone_id) != bIsAddonActive)
                        pWeaponVisual->LL_SetBoneVisible(bone_id, bIsAddonActive, TRUE);
            }

        for (u32 j = 0; j < ext_vis_vec.size(); j++)
            if (bIsAddonActive || std::find(ext_vis_attached.begin(), ext_vis_attached.end(), ext_vis_vec[j]) == ext_vis_attached.end())
            {
                if (!m_pAddon->bHideVis3p && bIsAddonActive)
                    this->AttachAdditionalVisual(ext_vis_vec[j]);
                else
                    this->DetachAdditionalVisual(ext_vis_vec[j]);
            }

        /////////////////////

        // Решаем какой элемент обработать дальше
        if (bIsAddonActive)
        {
            if (start_idx > 0)
                i = 0;
            else
                i = 1;

            bIsAddonActive = false;
        }
        else
        {
            i++;
        }
    }

    show_bones_attached.clear();
    hide_bones_attached.clear();
    ext_vis_attached.clear();
}

void CWeapon::UpdateAddonsVisibility(bool bDontUpdateHUD)
{
    m_dwLastAddonsVisUpdTime = Device.dwTimeGlobal;

    _UpdateAddonsVisibility(GetAddonBySlot(eScope));
    _UpdateAddonsVisibility(GetAddonBySlot(eSilencer));
    _UpdateAddonsVisibility(GetAddonBySlot(eLauncher));
    _UpdateAddonsVisibility(GetAddonBySlot(eMagaz));
    _UpdateAddonsVisibility(GetAddonBySlot(eSpec_1));
    _UpdateAddonsVisibility(GetAddonBySlot(eSpec_2));
    _UpdateAddonsVisibility(GetAddonBySlot(eSpec_3));
    _UpdateAddonsVisibility(GetAddonBySlot(eSpec_4));
    _UpdateAddonsVisibility(GetAddonBySlot(eSpec_5));
    _UpdateAddonsVisibility(GetAddonBySlot(eSpec_6));

    // Обновляем кости на мировой модели
    IKinematics* pWeaponVisual = smart_cast<IKinematics*>(Visual());
    R_ASSERT(pWeaponVisual);
    pWeaponVisual->CalculateBones_Invalidate();
    pWeaponVisual->CalculateBones(TRUE);

    // Пробуем обновить худ
    if (!bDontUpdateHUD)
        UpdateHUDAddonsVisibility();
}

//****** Проверки на возможность присоединения аддонов ******//

// Можем-ли мы присоединить данный предмет к нашему оружию.
bool CWeapon::CanAttach(PIItem pIItem)
{
    u8      addonIdx = 0;
    EAddons iSlot    = GetAddonSlot(pIItem->cast_game_object(), &addonIdx);
    if (iSlot != eNotExist)
    {
        SAddonData*         pAddon          = GetAddonBySlot(iSlot);
        const shared_str&   sAddonSetSect   = pAddon->GetNameByIdx(addonIdx);
        const shared_str&   sAddonName      = pIItem->m_section_id;
        bool                bIsSlotFree     = pAddon->bActive == false;
        bool                bAttachable     = pAddon->m_attach_status == ALife::EWeaponAddonStatus::eAddonAttachable;

        // Проверяем на возможность замены аддона
        if (bAttachable == false)
            return false;

        // Проверяем на совместимость с другими аддонами
        //--> Быстрая проверка - сработает если у нас уже установлен аддон, который блокирует этот
        if (m_IncompatibleAddons.count(sAddonSetSect.c_str()) > 0)
            return false;

        //--> Медленная проверка - на случаи если это мы блокируем аддон, который уже был установлен (нет в m_IncompatibleAddons)
        {
            LPCSTR sDisallowStr = READ_IF_EXISTS(pSettings, r_string, sAddonSetSect, WEAPON_ADDON_DIS_L, nullptr);
            if (sDisallowStr != nullptr)
            {
                string128 _sect_name;
                int count = _GetItemCount(sDisallowStr);
                for (int i = 0; i < count; ++i)
                {
                    shared_str sDisallowedAddonSSect = _GetItem(sDisallowStr, i, _sect_name);
                    for (int iSlotNext = 0; iSlotNext < EAddons::eAddonsSize; iSlotNext++)
                    {
                        if (iSlotNext != iSlot)
                        {
                            SAddonData* pAddonNext = GetAddonBySlot((EAddons)iSlotNext);
                            if (pAddonNext->bActive && pAddonNext->GetName().equal(sDisallowedAddonSSect))
                            {
                                return false;
                            }
                        }
                    }
                }
            }
        }

        // Проверяем на наличие соседних аддонов, требуемых для установки этого
        shared_str sRequiredAddonSetSect = READ_IF_EXISTS(pSettings, r_string, sAddonSetSect, WEAPON_ADDON_REQ_L, nullptr);
        if (sRequiredAddonSetSect != nullptr)
        {
            bool bRequiredAddonExist = false;

            for (int iSlotNext = 0; iSlotNext < EAddons::eAddonsSize; iSlotNext++)
            {
                if (iSlotNext != iSlot)
                {
                    SAddonData* pAddonNext = GetAddonBySlot((EAddons)iSlotNext);
                    if (pAddonNext->bActive && pAddonNext->GetName().equal(sRequiredAddonSetSect))
                    {
                        bRequiredAddonExist = true;
                        break;
                    }
                }
            }

            if (bRequiredAddonExist == false)
            {
                return false;
            }
        }

        // Магазин
        if (m_bUseMagazines == true && iSlot == eMagaz)
        {
            CWeapon* pMagaz = pIItem->cast_weapon();
            if ((GetState() != eSwitchMag) && (pMagaz != NULL && pMagaz->IsMagazine()) && Try2SwitchMag(true, true))
            {
                return pMagaz->HaveMinRequiredAmmoInMag();
            }

            return false;
        }

        // Глушитель
        if (iSlot == eSilencer)
        {
            return IsFlashHiderAttached() == false && IsBayonetAttached() == false;
        }

        // Подствольник
        if (iSlot == eLauncher)
        {
            return m_bUseAmmoBeltMode == false && IsForegripAttached() == false && IsBipodsAttached() == false;
        }

        // Патронташ
        bool bIsAmmoBelt = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_ammo_belt", false);
        if (bIsAmmoBelt)
        {
            return m_bUseAmmoBeltMode == true && IsAmmoBeltAttached() == false;
        }

        // Подствольная рукоятка
        bool bIsForegrip = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_foregrip", false);
        if (bIsForegrip)
        {
            return IsGrenadeLauncherAttached() == false;
        }

        // Пламегаситель
        bool bIsFlashHider = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_flashider", false);
        if (bIsFlashHider)
        {
            return IsSilencerAttached() == false;
        }

        // Сошки
        bool bIsBipods = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_bipods", false);
        if (bIsBipods)
        {
            return IsBipodsDeployed() == false && IsBipodsAttached() == false && IsGrenadeLauncherAttached() == false;
        }

        // Штык-нож
        bool bIsBayonet = READ_IF_EXISTS(pSettings, r_bool, sAddonName, "is_bayonet", false);
        if (bIsBayonet)
        {
            return IsSilencerAttached() == false;
        }

        // Всё остальное
        return true;
    }

    return inherited::CanAttach(pIItem);
}

// Присоединить аддон
bool CWeapon::Attach(PIItem pIItem, bool b_send_event, bool b_from_actor_menu)
{
    if (CanAttach(pIItem) == false)
        return false; //--> Дублирующая проверка

    SetUpdateIcon(true); //--> Форсируем обновление иконки

    bool result = false;

    // Проверяем есть-ли у предмета слот в этом оружии
    u8      addon_idx = 0;
    EAddons iSlot     = GetAddonSlot(pIItem->cast_game_object(), &addon_idx);
    if (iSlot != eNotExist)
    {
        // Если это магазинное питание, то соединяем аддон по другому
        if (m_bUseMagazines == true && iSlot == eMagaz)
            return AttachMagazine(pIItem->cast_weapon(), true, b_from_actor_menu);

        // При установке из инвентаря - снимаем текущий аддон в этом слоте
        if (b_from_actor_menu)
        {
            if (GetAddonBySlot(iSlot)->bActive == true)
                Detach(GetAddonBySlot(iSlot)->GetAddonName().c_str(), true, false);
        }

        // "Устанавливаем" всё остальное здесь
        InstallAddon(iSlot, addon_idx, true);
        result = true;
    }

    // Если аддон подошёл, то ...
    if (result)
    {
        if (b_send_event && OnServer())
        {
            pIItem->object().DestroyObject(); //--> Уничтожить подсоединенную вещь из инвентаря
        };

        UpdateAddons(); //--> Обновить состояние оружия
        ResetIdleAnim();

        return true;
    }
    else
        return inherited::Attach(pIItem, b_send_event);
}

// Корректно присоединить аддон "Магазин"
// (в том числе для оружия с магазинным питанием)
bool CWeapon::AttachMagazine(CWeapon* pMagazineItem, bool b_send_event, bool b_with_anim)
{
    u8 addon_idx = eNotExist;
    EAddons iSlot = GetAddonSlot(pMagazineItem, &addon_idx);

    // Проверяем что предмет - магазин
    if (iSlot != eMagaz)
        return false;

    // Если не используется магазинное питание - ставим по общей схеме
    if (m_bUseMagazines == false)
        return Attach((PIItem)pMagazineItem, b_send_event, false);

    // Проверяем на возможность проигрывания анимации
    bool bCanPlayAnim = b_with_anim && !IsHidden();

    // Если уже стоит магазин - пробуем его снять
    if (!bCanPlayAnim && GetAddonBySlot(eMagaz)->bActive == true)
        Detach(GetAddonBySlot(eMagaz)->GetAddonName().c_str(), MagazineAttachable(), false);

    //>>> Код установки магазина для оружия с магазинным питанием находится здесь <<<//
    if (bCanPlayAnim && ParentIsActor())
    { //--> Вызываем анимацию, которая по окончанию установит магазин
        if (GetState() == eSwitchMag)
            return false;

        m_set_next_magaz_on_reload = addon_idx;
        m_set_next_magaz_by_id = pMagazineItem->object_id();

        bool result = Try2SwitchMag();
        if (result == false)
        {
            m_set_next_magaz_on_reload = empty_addon_idx;
            m_set_next_magaz_by_id = u16(-1);
        }

        return result;
    }
    else
    { //--> Устанавливаем магазин без анимации
        InstallAddon(iSlot, addon_idx);
        pMagazineItem->TransferAmmo(this);

        if (b_send_event && OnServer())
            pMagazineItem->DestroyObject(); //--> Уничтожить подсоединённую вещь из инвентаря
    }

    return true;
}


// Можем-ли мы отсоединить аддон
bool CWeapon::CanDetach(const char* item_section_name)
{
    EAddons iSlot = GetAddonSlot(item_section_name);
    if (iSlot != eNotExist)
    {
        if (m_bUseMagazines && iSlot == eMagaz)
        {
            return (GetState() != CWeapon::eSwitchMag) && Try2SwitchMag(true, true);
        }

        return true;
    }

    return inherited::CanDetach(item_section_name);
}

// Отсоединить аддон
bool CWeapon::Detach(const char* item_section_name, bool b_spawn_item, bool b_from_actor_menu)
{
    if (CanDetach(item_section_name) == false)
        return false; //--> Дублирующая проверка

    SetUpdateIcon(true); //--> Форсируем обновление иконки

    bool detached = false;

    // Получаем слот аддона по его секции и отсоединяем
    u8      addon_idx = 0;
    EAddons iSlot     = GetAddonSlot(item_section_name);
    if (iSlot != eNotExist)
    {
        // Если это магазинное питание, то отсоединяем аддон по другому
        if (m_bUseMagazines == true && iSlot == eMagaz)
            return DetachMagazine(item_section_name, b_spawn_item, b_from_actor_menu);

        // Снимаем аддон
        SAddonData* pAddon = UnistallAddon(iSlot, true);

        detached = true;
    }

    if (detached == true)
    {
        UpdateAddons();
        ResetIdleAnim();
        return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
    }

    return inherited::Detach(item_section_name, b_spawn_item);
}

// Корректно (с сохранением патронов и разрядкой) отсоединить аддон "Магазин"
// (в том числе для оружия с магазинным питанием)
bool CWeapon::DetachMagazine(const char* item_section_name, bool b_spawn_item, bool b_with_anim)
{
    // Проверяем что слот магазина пуст
    if (GetAddonBySlot(eMagaz)->bActive == false)
        return false;

    // Если не используется магазинное питание - снимаем магазин по общей схеме
    if (m_bUseMagazines == false)
        return Detach(item_section_name, b_spawn_item);

    // Проверяем на возможность проигрывания анимации
    bool bCanPlayAnim = b_with_anim && !IsHidden();

    //>>> Код снятия магазина для оружия с магазинным питанием находится здесь <<<//
    if (bCanPlayAnim && ParentIsActor())
    { //--> Вызываем анимацию, которая по окончанию снимет магазин (не учитывает флаг b_spawn_item)
        if (GetState() == eSwitchMag)
            return false;

        m_set_next_magaz_on_reload = empty_addon_idx;
        m_sub_state = eSubstateMagazDetach;

        if (Try2SwitchMag() == false)
        {
            m_sub_state = eSubstateReloadBegin;
            return false;
        }
    }
    else 
    { //--> Снимаем магазин без анимации
        // Спавним магазин со всеми патронами
        if (b_spawn_item)
            CInventoryItemObject::Detach(item_section_name, true);

        // Разряжаем сам ствол
        UnloadMagazineMain(false);

        // Снимаем аддон
        UnistallAddon(eMagaz);
    }

    return true;
}

//******** Различные функции для глушителя ********//

// Загрузить коэфиценты текущего глушителя
void CWeapon::LoadSilencerKoeffs()
{
    if (IsSilencerAttached())
    {
        LPCSTR sect                     = GetSilencerName().c_str();
        m_silencer_koef.hit_power       = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_power_k", 1.0f);
        m_silencer_koef.hit_impulse     = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_hit_impulse_k", 1.0f);
        m_silencer_koef.bullet_speed    = READ_IF_EXISTS(pSettings, r_float, sect, "bullet_speed_k", 1.0f);
        m_silencer_koef.fire_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "fire_dispersion_base_k", 1.0f);
        m_silencer_koef.cam_dispersion  = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_k", 1.0f);
        m_silencer_koef.cam_disper_inc  = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_inc_k", 1.0f);
        m_silencer_koef.shooting_shake  = READ_IF_EXISTS(pSettings, r_float, sect, "shooting_shake_k", 1.0f);
    }

    clamp(m_silencer_koef.hit_power, 0.0f, 1000.0f);
    clamp(m_silencer_koef.hit_impulse, 0.0f, 1000.0f);
    clamp(m_silencer_koef.bullet_speed, 0.0f, 1000.0f);
    clamp(m_silencer_koef.fire_dispersion, 0.0f, 1000.0f);
    clamp(m_silencer_koef.cam_dispersion, 0.0f, 1000.0f);
    clamp(m_silencer_koef.cam_disper_inc, 0.0f, 1000.0f);
    clamp(m_silencer_koef.shooting_shake, 0.0f, 1000.0f);
}

void CWeapon::ApplySilencerKoeffs() { cur_silencer_koef = m_silencer_koef; }

void CWeapon::ResetSilencerKoeffs() { cur_silencer_koef.Reset(); }

//******** Различные функции для подствола ********//

void CWeapon::PerformSwitchGL()
{
    m_bGrenadeMode = !m_bGrenadeMode;

    if (m_bGrenadeMode)
        SwitchZoomType(eZoomGL);
    else
        SwitchZoomType(GetPrevZoomType());

    m_ammoTypes.swap(m_ammoTypes2);
    m_AmmoCartidges.swap(m_AmmoCartidges2);
    m_magazine.swap(m_magazine2);

    std::swap(iMagazineSize, iMagazineSize2);
    std::swap(iAmmoElapsed, iAmmoElapsed2);
    std::swap(m_ammoType, m_ammoType2);

    iAmmoElapsed                  = (int)m_magazine.size();
    m_set_next_ammoType_on_reload = undefined_ammo_type;
    m_BriefInfo_CalcFrame         = 0;
}

// Обновляем данные текущего патронташа
void CWeapon::LoadAmmoBeltParams()
{
    if (!m_bUseAmmoBeltMode)
        return;

    R_ASSERT(m_bGrenadeMode == false);
    m_AmmoBeltData.clear();
    iMagazineSize2 = 0;

    if (IsAmmoBeltAttached())
    {
        SAddonData* pAddonAB = GetAddonBySlot(m_AmmoBeltSlot);

        m_sAB_hud = pAddonAB->GetVisuals("visuals_hud", true);
        m_sAB_vis = pAddonAB->GetVisuals("visuals_world", true);

        iMagazineSize2 = READ_ADDON_DATA(r_s32, "ammo_belt_mag_size", pAddonAB->GetName(), pAddonAB->GetAddonName(), iMagazineSize);

        LPCSTR sAmmoBeltData = pSettings->r_string(pAddonAB->GetName(), "ammo_belt_data");
        int    iAmmoIdx      = 1;
        while (true)
        {
            string128 sLine;
            xr_sprintf(sLine, "%s_%d", "bullet", iAmmoIdx);

            if (pSettings->line_exist(sAmmoBeltData, sLine))
            {
                shared_str hud = NULL;
                shared_str vis = NULL;

                LPCSTR sData = pSettings->r_string(sAmmoBeltData, sLine);
                int    count = _GetItemCount(sData);

                string256 _buffer;
                hud = _GetItem(sData, 0, _buffer);
                if (count > 1)
                    vis = _GetItem(sData, 1, _buffer);

                m_AmmoBeltData.push_back(AmmoBeltData(hud, vis));
            }
            else
                break;

            iAmmoIdx++;
        }

        //--> Вид анимации зарядки в\из патронташа - одинарная или в три стадии
        m_bTriStateReload_ab   = READ_ADDON_DATA(r_bool, "tri_state_reload_ab", pAddonAB->GetName(), pAddonAB->GetAddonName(), false);
        m_bTriStateReload_frab = READ_ADDON_DATA(r_bool, "tri_state_reload_frab", pAddonAB->GetName(), pAddonAB->GetAddonName(), false);
    }
    else
    {
        m_bTriStateReload_ab   = false;
        m_bTriStateReload_frab = false;
    }

    // В режиме патронташа загружаем в магазин от подствола иные данные
    m_ammoTypes2.clear();
    m_AmmoCartidges2.clear();

    m_ammoTypes2     = m_ammoTypes;
    m_AmmoCartidges2 = m_AmmoCartidges;
}

// Обновляем данные текущего подствольного гранатомёта
void CWeapon::LoadGLParams()
{
    // В режиме патронташа не обрабатываем
    if (m_bUseAmmoBeltMode)
        return;

    if (IsGrenadeLauncherAttached())
    {
        // Подствольник доступен
        CRocketLauncher::m_fLaunchSpeed = READ_ADDON_DATA(r_float, "grenade_vel", GetGrenadeLauncherSetSect(), GetGrenadeLauncherName(), 1.f);
        LPCSTR S                        = READ_ADDON_DATA(r_string,
            "grenade_class",
            GetGrenadeLauncherSetSect(),
            GetGrenadeLauncherName(),
            pSettings->r_string(this->cNameSect(), "grenade_class"));

        // load ammo classes SECOND (grenade_class)

        iMagazineSize2 = 1; //-> Максимальное число гранат в подстволе

        xr_vector<shared_str>* ammoTypes     = NULL;
        xr_vector<CCartridge>* AmmoCartidges = NULL;

        if (m_bGrenadeMode)
        {
            ammoTypes     = &m_ammoTypes;
            AmmoCartidges = &m_AmmoCartidges;
        }
        else
        {
            ammoTypes     = &m_ammoTypes2;
            AmmoCartidges = &m_AmmoCartidges2;
        }

        ammoTypes->clear();
        AmmoCartidges->clear();
        if (S && S[0])
        {
            string128 _ammoItem;
            int       count = _GetItemCount(S);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(S, it, _ammoItem);
                ammoTypes->push_back(_ammoItem);

                CCartridge cartridge;
                cartridge.Load(_ammoItem, it);
                AmmoCartidges->push_back(cartridge);
            }
        }

        SetAmmoTypeSafeFor(m_bGrenadeMode ? m_ammoType : m_ammoType2, true);

        // Подствольный зум
        LPCSTR section = cNameSect_str();

        //--> Считываем основну из главной секции
        GetZoomParams(eZoomGL).Initialize(section, "_gl", false);

        //--> Переписываем данными из секции аддона
        GetZoomParams(eZoomGL).Initialize(GetGrenadeLauncherName().c_str(), "_gl", true);

        //--> Переписываем данными из секции аддона в оружии
        GetZoomParams(eZoomGL).Initialize(GetGrenadeLauncherSetSect().c_str(), "_gl", true);

        //--> Вид анимации перезарядки - одинарная или в три стадии
        m_bTriStateReload_gl = READ_ADDON_DATA(r_bool, "tri_state_reload_gl", GetGrenadeLauncherSetSect(), GetGrenadeLauncherName(), false);
    }
    else
    {
        // Подствольник отсутствует
        SetAmmoElapsedFor(0, true);
        SetAmmoTypeSafeFor(0, true);

        m_ammoTypes2.clear();
        m_AmmoCartidges2.clear();

        iMagazineSize2 = 0;

        m_bTriStateReload_gl = false;
    }
}

//******** Прочие вспомогательные функции ********//

// Обновляем данные для удара прикладом/штык-ножа
void CWeapon::LoadBayonetParams()
{
    const shared_str sBayoneteSetSect = IsBayonetAttached() ? GetAddonBySlot(m_BayonetSlot)->GetName() : cNameSect();
    const shared_str sBayoneteName    = IsBayonetAttached() ? GetAddonBySlot(m_BayonetSlot)->GetAddonName() : cNameSect();

    shared_str sMainAttack = READ_ADDON_DATA(r_string, "kick_attack_main", sBayoneteSetSect, sBayoneteName, NULL);
    if ((sMainAttack != NULL) && !sMainAttack.equal("none"))
    {
        if (m_kicker_main == NULL)
            m_kicker_main = new CWeaponKnifeHit(sMainAttack, this);
        else
            m_kicker_main->ReLoad(sMainAttack);
    }
    else
        xr_delete(m_kicker_main);

    shared_str sAltAttack = READ_ADDON_DATA(r_string, "kick_attack_alt", sBayoneteSetSect, sBayoneteName, NULL);
    if ((sAltAttack != NULL) && !sAltAttack.equal("none"))
    {
        if (m_kicker_alt == NULL)
            m_kicker_alt = new CWeaponKnifeHit(sAltAttack, this);
        else
            m_kicker_alt->ReLoad(sAltAttack);
    }
    else
        xr_delete(m_kicker_alt);
}
