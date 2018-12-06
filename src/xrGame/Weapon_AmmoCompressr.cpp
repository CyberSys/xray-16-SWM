/*********************************************************/
/***** Упаковщик для сохранения и загрузки патронов  *****/ //--#SM+#--
/*********************************************************/

#include "stdafx.h"
#include "Weapon_AmmoCompress.h"

#ifdef XRGAME_EXPORTS
    #include "Weapon_Shared.h"
#endif

// Добавить патронов соответствующего типа в переданный ammo_pair
void CAmmoCompressUtil::AddAmmo(AMMO_VECTOR& pVIn, u16 iAmmoCnt, u8 iAmmoTypeIdx, bool bClearFirst)
{
    if (bClearFirst)
        pVIn.clear();

    ammo_pair data;
    data.first = iAmmoTypeIdx; // Type
    data.second = iAmmoCnt; // Cnt
    pVIn.push_back(data);
}

// Посчитать число всех патронов в ammo_pair, при указании iAmmoTypeIdx - только конкретно этого типа
u32 CAmmoCompressUtil::GetAmmoTotal(AMMO_VECTOR& pVIn, u8 iAmmoTypeIdx)
{
    u32 iAmmoTotal = 0;

    for (u32 i = 0; i < pVIn.size(); i++) 
    {
        ammo_pair* data = &pVIn[i];
        u8 ammoType = data->first;
        u32 ammoCnt = data->second;

        if (iAmmoTypeIdx == u8(-1) || ammoType == iAmmoTypeIdx)
            iAmmoTotal += ammoCnt;
    }

    return iAmmoTotal;
}

#ifdef XRGAME_EXPORTS
// Упаковать магазин в формат ammo_pair
void CAmmoCompressUtil::CompressMagazine(AMMO_VECTOR& pVOut, CWeapon* pWeapon, bool bForGL)
{
    // Получаем нужный магазин
    xr_vector<CCartridge*>* pVMagaz;
    if (bForGL)
        pVMagaz = (pWeapon->m_bGrenadeMode ? &pWeapon->m_magazine : &pWeapon->m_magazine2);
    else
        pVMagaz = (pWeapon->m_bGrenadeMode ? &pWeapon->m_magazine2 : &pWeapon->m_magazine);

    // Пакуем его патроны
    pVOut.clear();

    if (pVMagaz->size() > 0)
    {
        u8        lastAmmoIdx = u8(-1);
        ammo_pair data;
        data.first  = 0;
        data.second = 0;

        for (u32 i = 0; i < pVMagaz->size(); i++)
        {
            CCartridge* pBullet = pVMagaz->at(i);

            //--> Если тип текущего патрона отличается от прошлого, то сохраняем и обнуляем data
            if (pBullet->m_LocalAmmoType != lastAmmoIdx)
            {
                if (data.second > 0)
                    pVOut.push_back(data);

                lastAmmoIdx = pBullet->m_LocalAmmoType;
                data.first  = lastAmmoIdx; // Type
                data.second = 0;           // Cnt
            }

            //--> Увеличиваем счётчик патронов данного типа
            data.second += 1;
        }

        // Сохраняем последнюю порцию патронов отдельно здесь
        if (data.second > 0)
            pVOut.push_back(data);
    }
}

// Распаковать магазин в указанное оружие
void CAmmoCompressUtil::DecompressMagazine(AMMO_VECTOR& pVIn, CWeapon* pWeapon, bool bForGL)
{
    // ВНИМАНИЕ: При распаковке магазина в другое оружие важно чтобы они обладали идиентичными m_ammoTypes, иначе
    // типы патронов будут нарушены -> вылет
    // (Unpack only to a weapon with the same ammo types, located by the same order)

    // Получаем нужный магазин, куда будет произведена распаковка
    xr_vector<CCartridge*>* pVMagaz; // Целевой магазин оружия
    xr_vector<CCartridge>*  pVCartr; // Образцы типов патронов оружия
    int*                    pAmmoElapsed;
    u8*                     pAmmoType;

    if (bForGL)
    {
        pVMagaz      = (pWeapon->m_bGrenadeMode ? &pWeapon->m_magazine : &pWeapon->m_magazine2);
        pVCartr      = (pWeapon->m_bGrenadeMode ? &pWeapon->m_AmmoCartidges : &pWeapon->m_AmmoCartidges2);
        pAmmoElapsed = (pWeapon->m_bGrenadeMode ? &pWeapon->iAmmoElapsed : &pWeapon->iAmmoElapsed2);
        pAmmoType    = (pWeapon->m_bGrenadeMode ? &pWeapon->m_ammoType : &pWeapon->m_ammoType2);
    }
    else
    {
        pVMagaz      = (pWeapon->m_bGrenadeMode ? &pWeapon->m_magazine2 : &pWeapon->m_magazine);
        pVCartr      = (pWeapon->m_bGrenadeMode ? &pWeapon->m_AmmoCartidges2 : &pWeapon->m_AmmoCartidges);
        pAmmoElapsed = (pWeapon->m_bGrenadeMode ? &pWeapon->iAmmoElapsed2 : &pWeapon->iAmmoElapsed);
        pAmmoType    = (pWeapon->m_bGrenadeMode ? &pWeapon->m_ammoType2 : &pWeapon->m_ammoType);
    }

    // Распаковываем
    pVMagaz->clear();                     //--> Очищаем магазин от старых данных
    for (u32 i = 0; i < pVIn.size(); i++) //--> Заполняем новыми
    {
        ammo_pair* data    = &pVIn[i];
        u8        ammoType = data->first;
        u32       ammoCnt  = data->second;

        for (u32 idx = 0; idx < ammoCnt; idx++)
            pVMagaz->push_back(&(pVCartr->at(ammoType)));
    }

    // Пересчитываем iAmmoElapsed и m_ammoType
    *pAmmoElapsed = pVMagaz->size();
    if (*pAmmoElapsed > 0)
        *pAmmoType = pVMagaz->back()->m_LocalAmmoType;
    else
        *pAmmoType = 0;
}
#endif

// Запаковать AMMO_VECTOR в Net Packet
void CAmmoCompressUtil::PackAmmoInPacket(AMMO_VECTOR& pVAmmoIn, NET_Packet& tNetPacket)
{
    u32 size = pVAmmoIn.size();
    tNetPacket.w_u32(size);

    for (u32 i = 0; i < size; i++)
    {
        tNetPacket.w_u8(pVAmmoIn[i].first);
        tNetPacket.w_u32(pVAmmoIn[i].second);
    }
}

// Распаковать AMMO_VECTOR из Net Packet
void CAmmoCompressUtil::UnpackAmmoFromPacket(AMMO_VECTOR& pVAmmoOut, NET_Packet& tNetPacket)
{
    u32 size = 0;
    tNetPacket.r_u32(size);

    pVAmmoOut.clear();

    for (u32 i = 0; i < size; i++)
    {
        ammo_pair m_pair;
        tNetPacket.r_u8(m_pair.first);
        tNetPacket.r_u32(m_pair.second);
        pVAmmoOut.push_back(m_pair);
    }
}
