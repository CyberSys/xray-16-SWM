/*****************************************/
/***** Модуль с различными функциями *****/ //--#SM+#--
/*****************************************/

#include "StdAfx.h"
#include "Weapon.h"

// Проверка требуется ли удалить объект (например бесхозный)
int  g_iWeaponRemove = 1;
bool CWeapon::NeedToDestroyObject() const
{
    if (GameID() == eGameIDSingle)
        return false; // В сингле не удаляем
    if (Remote())
        return false;
    if (H_Parent())
        return false;
    if (g_iWeaponRemove == -1)
        return false;
    if (g_iWeaponRemove == 0)
        return true;
    if (TimePassedAfterIndependant() > m_dwWeaponRemoveTime)
        return true;

    return false;
}

// Сколько времени прошло с тех пор как оружие последний раз стало бесхозным
ALife::_TIME_ID CWeapon::TimePassedAfterIndependant() const
{
    if (!H_Parent() && m_dwWeaponIndependencyTime != 0)
        return Level().timeServer() - m_dwWeaponIndependencyTime;
    else
        return 0;
}

// Установить вектор силы полёта оружия после его дропа персонажем (атака бюрера, например)
void CWeapon::SetActivationSpeedOverride(Fvector const& speed)
{
    m_overriden_activation_speed    = speed;
    m_activation_speed_is_overriden = true;
}

// Получить вектор полёта при выкидывании предмета и (при необходимости) сбросить его в дефолт
bool CWeapon::ActivationSpeedOverriden(Fvector& dest, bool clear_override)
{
    if (m_activation_speed_is_overriden)
    {
        if (clear_override)
        {
            m_activation_speed_is_overriden = false;
        }

        dest = m_overriden_activation_speed;
        return true;
    }

    return false;
}

// Проверка для AI - можно-ли этим предметом убивать
bool CWeapon::can_kill() const
{
    if (GetSuitableAmmoTotal(true) || m_ammoTypes.empty())
        return (true);

    return (false);
}
CInventoryItem* CWeapon::can_kill(CInventory* inventory) const
{
    if (GetAmmoElapsed() || m_ammoTypes.empty())
        return (const_cast<CWeapon*>(this));

    TIItemContainer::iterator I = inventory->m_all.begin();
    TIItemContainer::iterator E = inventory->m_all.end();
    for (; I != E; ++I)
    {
        CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return (inventory_item);
    }

    return (0);
}
const CInventoryItem* CWeapon::can_kill(const xr_vector<const CGameObject*>& items) const
{
    if (m_ammoTypes.empty())
        return (this);

    xr_vector<const CGameObject*>::const_iterator I = items.begin();
    xr_vector<const CGameObject*>::const_iterator E = items.end();
    for (; I != E; ++I)
    {
        const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(*I);
        if (!inventory_item)
            continue;

        xr_vector<shared_str>::const_iterator i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
        if (i != m_ammoTypes.end())
            return (inventory_item);
    }

    return (0);
}

// Проверка для AI - готово-ли оружие для использования
bool CWeapon::ready_to_kill() const
{
    return (!IsMisfire() && ((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2) || (GetState() == ePump)) && GetAmmoElapsed());
}

// Возвращает тип поведения АИ с этим оружием
u32 CWeapon::ef_main_weapon_type() const
{
    VERIFY(m_ef_main_weapon_type != u32(-1));
    return (m_ef_main_weapon_type);
}
u32 CWeapon::ef_weapon_type() const
{
    VERIFY(m_ef_weapon_type != u32(-1));
    return (m_ef_weapon_type);
}

// Обновить параметры зрения у AI-владельца
void CWeapon::modify_holder_params(float& range, float& fov) const
{
    if (!IsScopeAttached())
    {
        inherited::modify_holder_params(range, fov);
        return;
    }
    range *= m_addon_holder_range_modifier;
    fov *= m_addon_holder_fov_modifier;
}

// Проверка, является-ли владельцем оружия игрок
BOOL CWeapon::ParentIsActor()
{
    IGameObject* O = H_Parent();
    if (!O)
        return FALSE;

    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    if (!EA)
        return FALSE;

    return EA->cast_actor() != 0;
}

// Может-ли владелец оружия стрелять "супер"-пулей (пробивает броню ?)
BOOL CWeapon::ParentMayHaveAimBullet()
{
    IGameObject*  O  = H_Parent();
    CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
    return EA->cast_actor() != 0;
}

// Возвращает вероятность засчитать успешный хит по ГГ при выстреле по нему из этого оружия
const float& CWeapon::hit_probability() const
{
    VERIFY((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster));
    return (m_hit_probability[g_SingleGameDifficulty]);
}

// Оружие сейчас активно (в руках или стреляет, связано с серверным объектом, net_Relevant)
BOOL CWeapon::IsUpdating()
{
    bool bIsActiveItem = m_pInventory && m_pInventory->ActiveItem() == this;
    return bIsActiveItem || bWorking; // || IsPending() || getVisible();
}

// Проверка можем-ли мы достать детектор в данный момент
bool CWeapon::DetectorCheckCompability(bool bAllowAim) const
{
    u32  state     = GetState();
    bool bAimCheck = bAllowAim ? true : !IsZoomed();
    return bAimCheck && (state != CHUDState::eBore) && (state != CWeapon::eReload) && (state != CWeapon::eReloadFrAB) &&
           (state != CWeapon::eSwitchMag) && (state != CWeapon::eKick) && (state != CWeapon::ePump) && (state != CWeapon::eSwitch);
}

// Проверяет нужно-ли скрыть детектор в данный момент
bool CWeapon::DetectorHideCondition(bool bAllowAim) const
{
    u32  state     = GetState();
    bool bAimCheck = bAllowAim ? false : IsZoomed();
    return (bAimCheck || state == CWeapon::eReload || state == CWeapon::eReloadFrAB || state == CWeapon::eSwitchMag || state == CWeapon::ePump ||
            state == CWeapon::eSwitch);
}
