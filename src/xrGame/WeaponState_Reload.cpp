/***********************************/
/***** Состояние "Перезарядка" *****/ //--#SM+#--
/***********************************/

#include "StdAfx.h"
#include "Weapon.h"

// Логика перезарядки для НПС в CObjectActionReload()

// Пробуем начать перезарядку на клиенте
bool CWeapon::Try2Reload(bool bCheckOnlyMode)
{
    // Разрешена-ли перезарядка
    if (m_bDisableAnimatedReload)
        return false;

    // Если оружие использует магазинное питание, то обрабатываем перезарядку там
    if (m_bUseMagazines && !m_bGrenadeMode)
    {
        if (bCheckOnlyMode == true)
            return false;

        // Возможно все магазины текущего типа уже пустые, или магаз у нас не установлен -> попробуем сменить тип
        SAddonData* pAddonMagaz = GetAddonBySlot(eMagaz);

        if (pAddonMagaz->bActive == false)
        {
            return SwitchMagazineType();
        }

        if (iAmmoElapsed == 0)
        {
            CWeapon* pNewMagaz = GetBestMagazine(pAddonMagaz->GetAddonName().c_str());
            if (pNewMagaz == NULL || pNewMagaz->iAmmoElapsed == 0)
            {
                return SwitchMagazineType();
            }
        }

        return Try2SwitchMag(false);
    }

    if (!bCheckOnlyMode && GetState() == eReload)
        return false;
    if (IsPending() == true)
        return false;

    int AmmoElapsed = iAmmoElapsed;
    if (m_bIsReloadFromAB == false && m_set_next_ammoType_on_reload != undefined_ammo_type)
        AmmoElapsed = 0; //--> Требуется сменить тип патронов

    if (AmmoElapsed >= iMagazineSize)
        return false;

    if (m_bIsReloadFromAB == false)
    {
        //*** Перезарядка из инвентаря***//
        if (m_pInventory)
        {
            if (!bCheckOnlyMode)
            {
                // Колбэк в скрипты
                if (IsGameTypeSingle() && ParentIsActor())
                {
                    int AC = GetSuitableAmmoTotal();
                    Actor()->callback(GameObject::eWeaponNoAmmoAvailable)(lua_game_object(), AC);
                }
            }

            m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str()));
            if (m_pCurrentAmmo || unlimited_ammo())
            {
                //--> Если в инвентаре есть патроны текущего типа, то начинаем перезарядку
                if (!bCheckOnlyMode)
                    SwitchState(eReload);
                return true;
            }
            else
                for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
                {
                    //--> Иначе пробуем перезарядиться другим типом
                    m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
                    if (m_pCurrentAmmo)
                    {
                        if (!bCheckOnlyMode)
                        {
                            m_ammoType = i;
                            SwitchState(eReload);
                        }
                        return true;
                    }
                }
        }
    }
    else
    {
        //*** Перезарядка из патронташа ***//
        if (Try2ReloadFrAB(true))
        {
            if (!bCheckOnlyMode)
                SwitchState(eReload);
            return true;
        }
    }

    if (!bCheckOnlyMode)
    {
        m_bIsReloadFromAB = false;
        Need2Idle();
    }

    return false;
}

// Пробуем начать зарядку патронташа на клиенте
bool CWeapon::Try2ReloadAmmoBelt()
{
    if (m_bUseMagazines)
        return false;
    if (IsMisfire())
        return false;
    if (!IsAmmoBeltAttached())
        return false;
    if (m_bIsReloadFromAB)
        return false;
    if (m_bGrenadeMode)
        return false;

    PerformSwitchGL();

    if (!Try2Reload())
    {
        PerformSwitchGL();
        return false;
    }

    return true;
}

// Автоперезарядка при опустошении магазина
bool CWeapon::Try2AutoReload()
{
    // НПС перезаряжаются из отдельной логики
    if (!ParentIsActor())
        return false;

    // Проверяем условия авто-перезарядки
    if (m_bAllowAutoReload && GetAmmoElapsed() < GetAmmoMagSize())
        return Try2Reload();
    
    // Иначе не перезаряжаемся
    return false;
}

// Пробуем остановить перезарядку в три стадии
bool CWeapon::Try2StopTriStateReload()
{
    if (IsTriStateReload() && GetState() == eReload && (m_sub_state == eSubstateReloadInProcess || m_sub_state == eSubstateReloadBegin))
    {
        m_bNeed2StopTriStateReload = true;
        return true;
    }

    return false;
}

// Принудительная перезарядка
void CWeapon::Need2Reload()
{
    if (m_bUseMagazines && !m_bGrenadeMode)
        return Need2SwitchMag();

    SwitchState(eReload);
}

// Нужно остановить перезарядку на клиенте
void CWeapon::Need2Stop_Reload()
{
    if (m_bUseMagazines && !m_bGrenadeMode)
        return Need2Stop_SwitchMag();

    if (GetState() != eReload)
        return;
    Need2Idle();
}

// Переключение стэйта на "Перезарядка"
void CWeapon::switch2_Reload()
{
    if (m_bUseMagazines && !m_bGrenadeMode)
        return Need2Idle();

    if (m_sub_state == eSubstateReloadBegin) // Если мы в процессе перезарядки в три стадии, то игнорируем проверку
        if (!Try2Reload(true))               //--> Повторная проверка для МП, где вызов стэйтов идёт в обход Try-функций
        {
            Need2Idle();
            return;
        }

    VERIFY(GetState() == eReload);

    m_bSwitchAddAnimation      = false;
    m_bNeed2StopTriStateReload = false;

    OnZoomOut();

    if (IsTriStateReload() == true)
    { // Перезарядка в три стадии
        if (m_magazine.size() >= (u32)iMagazineSize)
            m_sub_state = eSubstateReloadEnd; // Магазин полный, заканчиваем перезарядку
        else
        { // Магазин не полный, ищем откуда можем зарядить
            bool bHaveCartridge = HaveCartridgeInInventory(1);
            bool bIsAmmoAviable = (m_bIsReloadFromAB ? GetGLAmmoElapsed() > 0 : bHaveCartridge);
            if (!bIsAmmoAviable)
                if (m_bIsReloadFromAB && bHaveCartridge)
                {
                    m_bIsReloadFromAB     = false; // Попробуем зарядить из инвентаря
                    m_bSwitchAddAnimation = true;
                    m_sub_state           = eSubstateReloadInProcess;
                }
                else
                    m_sub_state = eSubstateReloadEnd;
        }

        switch (m_sub_state)
        {
        case eSubstateReloadBegin: //--> Начало перезарядки
            switch2_StartReload();
            break;
        case eSubstateReloadInProcess: //--> Процесс перезарядки
            switch2_AddCartgidge();
            break;
        case eSubstateReloadEnd: //--> Конец перезарядки
            switch2_EndReload();
            break;
        };
    }
    else
    { // Обычная перезарядка
        m_overridenAmmoForReloadAnm = GetMainAmmoElapsed();

        PlayAnimReload();
    }

    SetPending(TRUE);
}

// Переключение на другой стэйт из стэйта "Перезарядка"
void CWeapon::switchFrom_Reload(u32 newS)
{
    if (newS != eReload)
    {
        m_set_next_ammoType_on_reload = undefined_ammo_type;
        m_sub_state                   = eSubstateReloadBegin;
        m_overridenAmmoForReloadAnm   = -1;
        m_bIsReloadFromAB             = false;
        m_bNeed2StopTriStateReload    = false;
        m_bSwitchAddAnimation         = false;

        // Если заряжали патронташ, то меняем магазины местами.
        if (IsAmmoBeltReloadNow())
            PerformSwitchGL();
    }
}

// Обновление оружия в состоянии "Перезарядка"
void CWeapon::state_Reload(float dt) { m_dwShowAnimatedShellVisual = 0; }

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Перезарядка в три стадии (Начало)
void CWeapon::switch2_StartReload()
{
    if (!m_bIsReloadFromAB)
        if (!HaveCartridgeInInventory(1))
            return;

    m_overridenAmmoForReloadAnm = GetMainAmmoElapsed();

    if (PlayAnimOpenWeapon() == false)
    {
        m_sub_state = eSubstateReloadInProcess;
        Need2Reload();
    }

    SetPending(TRUE);
}

// Перезарядка в три стадии (Добавление патрона)
void CWeapon::switch2_AddCartgidge()
{
    if (!m_bIsReloadFromAB)
        if (!HaveCartridgeInInventory(1))
            return;

    PlayAnimAddOneCartridgeWeapon();
    SetPending(TRUE);
}

// Перезарядка в три стадии (Конец перезарядки)
void CWeapon::switch2_EndReload()
{
    SetPending(FALSE);

    if (PlayAnimCloseWeapon() == false)
        Need2Stop_Reload();
}

// Проходит-ли перезарядка в три стадии
bool CWeapon::IsTriStateReload() const
{
    if (m_bUseMagazines && !m_bGrenadeMode)
        return false; //--> При использовании магазинов запрещаем перезарядку в три стадии

    if (C_THIS_WPN->IsAmmoBeltReloadNow())
        return m_bTriStateReload_ab;

    if (m_bIsReloadFromAB)
        return m_bTriStateReload_frab;

    if (m_bGrenadeMode)
        return m_bTriStateReload_gl;

    return m_bTriStateReload_main;
}
