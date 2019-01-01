#include "stdafx.h"
#include "Weapon_Shared.h"

/**************************************/
/***** Код для работы с патронами *****/ //--#SM+#--
/**************************************/

// Можно-ли разряжать магазин оружия
bool CWeapon::IsMagazineCanBeUnload(bool bForGL)
{
    if (bForGL)
        return true;

    return m_bAllowUnload;
}

// Являются-ли текущие патроны гранатой или ракетой
bool CWeapon::IsGrenadeBullet() const { return pSettings->line_exist(m_ammoTypes[m_ammoType].c_str(), "fake_grenade_name"); }

// Есть-ли в инвентаре любые подходящие патроны.
bool CWeapon::IsAmmoAvailable() const
{
    if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str())))
        return (true);
    else
        for (u32 i = 0; i < m_ammoTypes.size(); ++i)
            if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str())))
                return (true);
    return (false);
}

// Подсчитать кол-во доступных патронов всех типов для данного оружия (основной ствол)
int CWeapon::GetSuitableAmmoTotal(bool use_item_to_spawn) const
{
    int ae_count = iAmmoElapsed;
    if (!m_pInventory)
    {
        return ae_count;
    }

    // Чтоб не делать лишних пересчетов
    if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
    {
        return ae_count + m_iAmmoCurrentTotal;
    }
    m_BriefInfo_CalcFrame = Device.dwFrame;

    m_iAmmoCurrentTotal = 0;
    for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
    {
        m_iAmmoCurrentTotal += GetAmmoCount_forType(m_ammoTypes[i]);

        if (!use_item_to_spawn)
        {
            continue;
        }
        if (!inventory_owner().item_to_spawn())
        {
            continue;
        }
        m_iAmmoCurrentTotal += inventory_owner().ammo_in_box_to_spawn();
    }
    return ae_count + m_iAmmoCurrentTotal;
}

// Получить доступное число патронов указанного типа (первый магазин)
int CWeapon::GetAmmoCount(u8 ammo_type) const
{
    VERIFY(m_pInventory);
    R_ASSERT(ammo_type < m_ammoTypes.size());

    return GetAmmoCount_forType(m_ammoTypes[ammo_type]);
}

// Получить доступное число патронов указанного типа (второй магазин)
int CWeapon::GetAmmoCount2(u8 ammo2_type) const
{
    VERIFY(m_pInventory);
    R_ASSERT(ammo2_type < m_ammoTypes2.size());

    return GetAmmoCount_forType(m_ammoTypes2[ammo2_type]);
}

// Получить доступное число патронов указанного типа (тип патрона через строку)
int CWeapon::GetAmmoCount_forType(shared_str const& ammo_type) const
{
    int res = 0;

    TIItemContainer::iterator itb = m_pInventory->m_belt.begin();
    TIItemContainer::iterator ite = m_pInventory->m_belt.end();
    for (; itb != ite; ++itb)
    {
        CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(*itb);
        if (pAmmo && (pAmmo->cNameSect() == ammo_type))
        {
            res += pAmmo->m_boxCurr;
        }
    }

    itb = m_pInventory->m_ruck.begin();
    ite = m_pInventory->m_ruck.end();
    for (; itb != ite; ++itb)
    {
        CWeaponAmmo* pAmmo = smart_cast<CWeaponAmmo*>(*itb);
        if (pAmmo && (pAmmo->cNameSect() == ammo_type))
        {
            res += pAmmo->m_boxCurr;
        }
    }
    return res;
}

// Включены-ли для оружия неограниченные патроны
bool CWeapon::unlimited_ammo()
{
    if (IsGameTypeSingle())
    {
        if (m_pInventory)
        {
            return inventory_owner().unlimited_ammo() && m_AmmoCartidges[m_ammoType].m_flags.test(CCartridge::cfCanBeUnlimited);
        }
        else
            return false;
    }

    return ((GameID() == eGameIDDeathmatch) && m_AmmoCartidges[m_ammoType].m_flags.test(CCartridge::cfCanBeUnlimited));
};

// Попытаться сменить текущий тип патронов (анимированное переключение)
bool CWeapon::SwitchAmmoType()
{
    if (OnClient())
        return false;

    if (IsTriStateReload() && GetState() != eReload)
        return false; //--> При перезарядке в три стадии запрещаем смену типа вне перезарядки

    if (m_bIsReloadFromAB)
        return false;

    if (m_bUseMagazines)
        return SwitchMagazineType();

    u8 ammoType = ((m_set_next_ammoType_on_reload == undefined_ammo_type) ? m_ammoType : m_set_next_ammoType_on_reload);

    u8   l_newType = ammoType;
    bool b1, b2;
    do
    {
        l_newType = u8((u32(l_newType + 1)) % m_ammoTypes.size());
        b1        = (l_newType != ammoType);
        b2        = unlimited_ammo() ? false : (!m_pInventory->GetAny(m_ammoTypes[l_newType].c_str()));
    } while (b1 && b2);

    if (l_newType != ammoType)
    {
        m_set_next_ammoType_on_reload = l_newType;
        m_BriefInfo_CalcFrame         = 0;

        if (OnServer())
        {
            if (GetState() != eReload)
                if (IsTriStateReload() == false && !Try2Reload())
                    m_set_next_ammoType_on_reload = undefined_ammo_type;
        }
    }
    return true;
}

// Установить текущее число патронов для текущего ствола (основной\подствол)
void CWeapon::SetAmmoElapsed(int ammo_count)
{
    iAmmoElapsed = ammo_count;
    ResizeMagazine(ammo_count, m_magazine, &m_AmmoCartidges[m_ammoType]);
}

// Установить текущее число патронов для основного ствола или подствола
void CWeapon::SetAmmoElapsedFor(int ammo_count, bool bForGL)
{
    if (!bForGL)
    {
        // Устанавливаем патроны для основного магазина
        if (!m_bGrenadeMode)
        {
            R_ASSERT(m_AmmoCartidges.size() > 0);
            SetAmmoElapsed(ammo_count);
        }
        else
        {
            R_ASSERT(m_AmmoCartidges2.size() > 0);
            iAmmoElapsed2 = ammo_count;
            ResizeMagazine(iAmmoElapsed2, m_magazine2, &m_AmmoCartidges2[m_ammoType2]);
        }
    }
    else
    {
        // Устанавливаем патроны для подствольника
        if (IsGrenadeLauncherAttached() || m_bUseAmmoBeltMode)
        {
            if (!m_bGrenadeMode)
            {
                R_ASSERT(m_AmmoCartidges2.size() > 0);
                iAmmoElapsed2 = ammo_count;
                ResizeMagazine(iAmmoElapsed2, m_magazine2, &m_AmmoCartidges2[m_ammoType2]);
            }
            else
            {
                R_ASSERT(m_AmmoCartidges.size() > 0);
                SetAmmoElapsed(ammo_count);
            }
        }
    }
}

// Установить текущий тип патронов для основного ствола или подствола
void CWeapon::SetAmmoTypeSafeFor(u8 ammoType, bool bForGL)
{
    if (!bForGL)
    {
        // Устанавливаем патроны для основного магазина
        if (!m_bGrenadeMode)
        {
            m_ammoType = 0;
            if (ammoType < m_ammoTypes.size())
                m_ammoType = ammoType;
        }
        else
        {
            m_ammoType2 = 0;
            if (ammoType < m_ammoTypes2.size())
                m_ammoType2 = ammoType;
        }
    }
    else
    {
        // Устанавливаем патроны для подствольника
        if (!m_bGrenadeMode)
        {
            m_ammoType2 = 0;
            if (ammoType < m_ammoTypes2.size())
                m_ammoType2 = ammoType;
        }
        else
        {
            m_ammoType = 0;
            if (ammoType < m_ammoTypes.size())
                m_ammoType = ammoType;
        }
    }
}

// Изменить кол-во патронов в обойме (не пересчитывает счётчики)
void CWeapon::ResizeMagazine(int ammo_count, xr_vector<CCartridge*>& Magazine, CCartridge* pCartridge)
{
    u32 uAmmo = u32(ammo_count);

    if (uAmmo != Magazine.size())
    {
        if (uAmmo > Magazine.size())
        {
            while (uAmmo > Magazine.size())
                Magazine.push_back(pCartridge);
        }
        else
        {
            while (uAmmo < Magazine.size())
                Magazine.pop_back();
        };
    };
}

// Зарядить текущий магазин из инвентаря
void CWeapon::ReloadMagazine()
{
    m_BriefInfo_CalcFrame = 0;

    // Устранить осечку при перезарядке
    if (IsMisfire())
        bMisfire = false;

    if (!m_bLockType)
    {
        m_pCurrentAmmo = NULL;
    }

    if (!m_pInventory)
        return;

    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
    {
        m_ammoType                    = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = undefined_ammo_type;
    }

    if (!unlimited_ammo())
    {
        if (m_ammoTypes.size() <= m_ammoType)
            return;

        LPCSTR tmp_sect_name = m_ammoTypes[m_ammoType].c_str();

        if (!tmp_sect_name)
            return;

        // Попытаться найти в инвентаре патроны текущего типа
        m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(tmp_sect_name));

        if (!m_pCurrentAmmo && !m_bLockType)
        {
            for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
            {
                // Проверить патроны всех подходящих типов
                m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[i].c_str()));
                if (m_pCurrentAmmo)
                {
                    m_ammoType = i;
                    break;
                }
            }
        }
    }

    // Нет патронов для перезарядки
    if (!m_pCurrentAmmo && !unlimited_ammo())
        return;

    // Разрядить магазин, если загружаем патронами другого типа
    if (!m_bLockType && !m_magazine.empty() && (!m_pCurrentAmmo || xr_strcmp(m_pCurrentAmmo->cNameSect(), m_magazine.back()->m_ammoSect)))
        UnloadMagazine();

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    while (iAmmoElapsed < iMagazineSize)
    {
        if (!unlimited_ammo())
        {
            if (!m_pCurrentAmmo->Get(m_AmmoCartidges[m_ammoType], true))
                break;
        }
        ++iAmmoElapsed;
        m_magazine.push_back(&m_AmmoCartidges[m_ammoType]);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    // Выкинуть коробку патронов, если она пустая
    if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr && OnServer())
        m_pCurrentAmmo->SetDropManual(TRUE);

    if (iMagazineSize > iAmmoElapsed)
    {
        m_bLockType = true;
        ReloadMagazine();
        m_bLockType = false;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

// Зарядить текущий магазин из патронташа
void CWeapon::ReloadMagazineFrAB()
{
    R_ASSERT(m_bGrenadeMode == false);

    while (iAmmoElapsed < iMagazineSize)
    {
        if (!SwapBulletFromAB())
            return;
    }
}

// Зарядить текущий магазин из инвентаря патронами заданного типа
void CWeapon::ReloadMagazineWithType(u8 ammoType)
{
    m_set_next_ammoType_on_reload = ammoType;
    ReloadMagazine();
}

// Зарядить основной магазин из инвентаря патронами заданного типа
void CWeapon::ReloadMainMagazineWithType(u8 ammoType)
{
    bool bGMode = m_bGrenadeMode;
    if (bGMode)
        PerformSwitchGL();

    R_ASSERT(m_bGrenadeMode == false);
    ReloadMagazineWithType(ammoType);

    if (bGMode)
        PerformSwitchGL();
}

// Зарядить вторичный магазин из инвентаря патронами заданного типа
void CWeapon::ReloadGLMagazineWithType(u8 ammoType)
{
    bool bGMode = m_bGrenadeMode;
    if (!bGMode)
        PerformSwitchGL();

    R_ASSERT(m_bGrenadeMode == true);
    ReloadMagazineWithType(ammoType);

    if (!bGMode)
        PerformSwitchGL();
}

// Разрядить текущий магазин в инвентарь
void CWeapon::UnloadMagazine(bool spawn_ammo)
{
    xr_map<LPCSTR, u16> l_ammo; // section, cnt

    while (!m_magazine.empty())
    {
        CCartridge*                   l_cartridge = m_magazine.back();
        xr_map<LPCSTR, u16>::iterator l_it;
        for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
        {
            if (!xr_strcmp(*l_cartridge->m_ammoSect, l_it->first))
            {
                ++(l_it->second);
                break;
            }
        }

        if (l_it == l_ammo.end())
            l_ammo[*l_cartridge->m_ammoSect] = 1;
        m_magazine.pop_back();
        --iAmmoElapsed;
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    ResetIdleAnim();

    if (!spawn_ammo)
        return;

    xr_map<LPCSTR, u16>::iterator l_it;
    for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
    {
        if (m_pInventory)
        {
            CWeaponAmmo* l_pA = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(l_it->first));
            if (l_pA)
            {
                u16 l_free      = l_pA->m_boxSize - l_pA->m_boxCurr;
                l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
                l_it->second    = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
            }
        }
        if (l_it->second && !unlimited_ammo())
            SpawnAmmo(l_it->second, l_it->first);
    }
}

// Разрядить основной магазин в инвентарь
void CWeapon::UnloadMagazineMain(bool spawn_ammo)
{
    bool bGMode = m_bGrenadeMode;
    if (bGMode)
        PerformSwitchGL();

    R_ASSERT(m_bGrenadeMode == false);
    UnloadMagazine(spawn_ammo);

    if (bGMode)
        PerformSwitchGL();
}

// Разрядить вторичный магазин в инвентарь
void CWeapon::UnloadMagazineGL(bool spawn_ammo)
{
    bool bGMode = m_bGrenadeMode;
    if (!bGMode)
        PerformSwitchGL();

    R_ASSERT(m_bGrenadeMode == true);
    UnloadMagazine(spawn_ammo);

    if (!bGMode)
        PerformSwitchGL();
}

// Проверяем наличие в инвентаре нужного числа патронов для данного оружия
bool CWeapon::HaveCartridgeInInventory(u8 cnt)
{
    if (unlimited_ammo())
        return true;
    if (!m_pInventory)
        return false;

    u32 ac = GetAmmoCount(m_ammoType);
    if (ac < cnt)
    {
        for (u8 i = 0; i < u8(m_ammoTypes.size()); ++i)
        {
            if (m_ammoType == i)
                continue;
            ac += GetAmmoCount(i);
            if (ac >= cnt)
            {
                m_ammoType = i;
                break;
            }
        }
    }
    return ac >= cnt;
}

// Добавить в магазин нужное число патронов текущего типа
u8 CWeapon::AddCartridge(u8 cnt)
{
    // Устранить осечку при перезарядке
    if (IsMisfire())
        bMisfire = false;

    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
    {
        m_ammoType                    = m_set_next_ammoType_on_reload;
        m_set_next_ammoType_on_reload = undefined_ammo_type;
    }

    if (!HaveCartridgeInInventory(1))
        return 0;

    m_pCurrentAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(m_ammoTypes[m_ammoType].c_str()));
    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    while (cnt)
    {
        if (!unlimited_ammo())
        {
            if (!m_pCurrentAmmo->Get(m_AmmoCartidges[m_ammoType], true))
                break;
        }
        --cnt;
        ++iAmmoElapsed;
        m_magazine.push_back(&m_AmmoCartidges[m_ammoType]);
    }

    VERIFY((u32)iAmmoElapsed == m_magazine.size());

    //выкинуть коробку патронов, если она пустая
    if (m_pCurrentAmmo && !m_pCurrentAmmo->m_boxCurr && OnServer())
        m_pCurrentAmmo->SetDropManual(TRUE);

    return cnt;
}

// Добавить в магазин нужное число патронов из патронташа
u8 CWeapon::AddCartridgeFrAB(u8 cnt)
{
    R_ASSERT(m_bGrenadeMode == false);

    while (cnt)
    {
        if (!SwapBulletFromAB())
            break;
        cnt--;
    }

    return cnt;
}

// Попытаться вставить 1 патрон из патронташа в основной магазин
bool CWeapon::SwapBulletFromAB()
{
    R_ASSERT(m_bGrenadeMode == false);

    if (iAmmoElapsed >= iMagazineSize)
        return false;
    if (iAmmoElapsed2 == 0)
        return false;

    CCartridge* pBullet = m_magazine2.back();
    m_ammoType          = pBullet->m_LocalAmmoType;

    m_magazine.push_back(pBullet);
    iAmmoElapsed++;
    m_magazine2.pop_back();
    iAmmoElapsed2--;

    return true;
}

// Скопировать патроны из данного оружия в целевое
void CWeapon::TransferAmmo(CWeapon* pTarget, bool bForGL, bool bCopyOnly)
{
    xr_vector<CCartridge*>*pVMagaz, *pVMagazT;
    xr_vector<CCartridge>* pVCartr, *pVCartrT;
    xr_vector<shared_str>* pVAmmoTypes, *pVAmmoTypesT;
    int *                  pMagazineSize, *pMagazineSizeT;
    int *                  pAmmoElapsed, *pAmmoElapsedT;
    u8 *                   pAmmoType, *pAmmoTypeT;

    // Получаем все необоходимые объекты
    if (bForGL)
    {
        pVMagaz        = (m_bGrenadeMode ? &m_magazine : &m_magazine2);
        pVMagazT       = (pTarget->m_bGrenadeMode ? &pTarget->m_magazine : &pTarget->m_magazine2);
        pVCartr        = (m_bGrenadeMode ? &m_AmmoCartidges : &m_AmmoCartidges2);
        pVCartrT       = (pTarget->m_bGrenadeMode ? &pTarget->m_AmmoCartidges : &pTarget->m_AmmoCartidges2);
        pVAmmoTypes    = (m_bGrenadeMode ? &m_ammoTypes : &m_ammoTypes2);
        pVAmmoTypesT   = (pTarget->m_bGrenadeMode ? &pTarget->m_ammoTypes : &pTarget->m_ammoTypes2);
        pMagazineSize  = (m_bGrenadeMode ? &iMagazineSize : &iMagazineSize2);
        pMagazineSizeT = (pTarget->m_bGrenadeMode ? &pTarget->iMagazineSize : &pTarget->iMagazineSize2);
        pAmmoElapsed   = (m_bGrenadeMode ? &iAmmoElapsed : &iAmmoElapsed2);
        pAmmoElapsedT  = (pTarget->m_bGrenadeMode ? &pTarget->iAmmoElapsed : &pTarget->iAmmoElapsed2);
        pAmmoType      = (m_bGrenadeMode ? &m_ammoType : &m_ammoType2);
        pAmmoTypeT     = (pTarget->m_bGrenadeMode ? &pTarget->m_ammoType : &pTarget->m_ammoType2);
    }
    else
    {
        pVMagaz        = (m_bGrenadeMode ? &m_magazine2 : &m_magazine);
        pVMagazT       = (pTarget->m_bGrenadeMode ? &pTarget->m_magazine2 : &pTarget->m_magazine);
        pVCartr        = (m_bGrenadeMode ? &m_AmmoCartidges2 : &m_AmmoCartidges);
        pVCartrT       = (pTarget->m_bGrenadeMode ? &pTarget->m_AmmoCartidges2 : &pTarget->m_AmmoCartidges);
        pVAmmoTypes    = (m_bGrenadeMode ? &m_ammoTypes2 : &m_ammoTypes);
        pVAmmoTypesT   = (pTarget->m_bGrenadeMode ? &pTarget->m_ammoTypes2 : &pTarget->m_ammoTypes);
        pMagazineSize  = (m_bGrenadeMode ? &iMagazineSize2 : &iMagazineSize);
        pMagazineSizeT = (pTarget->m_bGrenadeMode ? &pTarget->iMagazineSize2 : &pTarget->iMagazineSize);
        pAmmoElapsed   = (m_bGrenadeMode ? &iAmmoElapsed2 : &iAmmoElapsed);
        pAmmoElapsedT  = (pTarget->m_bGrenadeMode ? &pTarget->iAmmoElapsed2 : &pTarget->iAmmoElapsed);
        pAmmoType      = (m_bGrenadeMode ? &m_ammoType2 : &m_ammoType);
        pAmmoTypeT     = (pTarget->m_bGrenadeMode ? &pTarget->m_ammoType2 : &pTarget->m_ammoType);
    }

    // Копируем параметры магазина в целевой ствол
    *pVCartrT       = *pVCartr;
    *pVAmmoTypesT   = *pVAmmoTypes;
    *pMagazineSizeT = *pMagazineSize;
    *pAmmoTypeT     = *pAmmoType;
    *pAmmoElapsedT  = *pAmmoElapsed;

    pVMagazT->clear();
    for (u32 i = 0; i < pVMagaz->size(); i++)
    {
        CCartridge* pBullet = pVMagaz->at(i);
        pVMagazT->push_back(&(pVCartrT->at(pBullet->m_LocalAmmoType)));
    }

    // При необходимости очищаем патроны у оригинала
    if (bCopyOnly == false)
    {
        pVMagaz->clear();
        *pAmmoType    = 0;
        *pAmmoElapsed = 0;
    }
}

// Заспавнить патроны в инвентарь
void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID)
{
    if (!m_ammoTypes.size())
        return;
    if (OnClient())
        return;
    m_bAmmoWasSpawned = true;

    int l_type = 0;
    l_type %= m_ammoTypes.size();

    if (!ammoSect)
        ammoSect = m_ammoTypes[l_type].c_str();

    ++l_type;
    l_type %= m_ammoTypes.size();

    CSE_Abstract* D = F_entity_Create(ammoSect);

    {
        CSE_ALifeItemAmmo* l_pA = smart_cast<CSE_ALifeItemAmmo*>(D);
        R_ASSERT(l_pA);
        l_pA->m_boxSize = (u16)pSettings->r_s32(ammoSect, "box_size");
        D->s_name       = ammoSect;
        D->set_name_replace("");
        //.		D->s_gameid					= u8(GameID());
        D->s_RP = 0xff;
        D->ID   = 0xffff;
        if (ParentID == 0xffffffff)
            D->ID_Parent = (u16)H_Parent()->ID();
        else
            D->ID_Parent = (u16)ParentID;

        D->ID_Phantom = 0xffff;
        D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
        D->RespawnTime  = 0;
        l_pA->m_tNodeID = GEnv.isDedicatedServer ? u32(-1) : ai_location().level_vertex_id();

        if (boxCurr == 0xffffffff)
            boxCurr = l_pA->m_boxSize;

        while (boxCurr)
        {
            l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
            NET_Packet P;
            D->Spawn_Write(P, TRUE);
            Level().Send(P, net_flags(TRUE));

            if (boxCurr > l_pA->m_boxSize)
                boxCurr -= l_pA->m_boxSize;
            else
                boxCurr = 0;
        }
    }
    F_entity_Destroy(D);
}

// Попытаться заспавнить ракету от текущего патрона, после спавна будет произведён её выстрел
bool CWeapon::SpawnAndLaunchRocket()
{
    if (iAmmoElapsed > 0)
    {
        if (IsGrenadeBullet() || m_sOverridedRocketSection != NULL)
        {
            shared_str fake_grenade_name;
            if (m_sOverridedRocketSection != NULL)
                fake_grenade_name = m_sOverridedRocketSection;
            else
                fake_grenade_name = pSettings->r_string(m_ammoTypes[m_ammoType].c_str(), "fake_grenade_name");

            CRocketLauncher::SpawnRocket(*fake_grenade_name, this);

            VERIFY(m_magazine.size());
            m_magazine.pop_back();
            --iAmmoElapsed;
            VERIFY((u32)iAmmoElapsed == m_magazine.size());

            return true;
        }
    }

    return false;
}
