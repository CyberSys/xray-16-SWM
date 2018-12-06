/*********************************************************/
/***** Упаковщик для сохранения и загрузки патронов  *****/ //--#SM+#--
/*********************************************************/

#pragma once

class NET_Packet;
class CCartridge;
class CWeapon;

class CAmmoCompressUtil
{
public:
    typedef std::pair<u8, u32> ammo_pair; // ammoType, ammoCnt
    DEFINE_VECTOR(ammo_pair, AMMO_VECTOR, AMMO_VECTOR_IT);

    static void AddAmmo(AMMO_VECTOR& pVIn, u16 iAmmoCnt, u8 iAmmoTypeIdx, bool bClearFirst = false);
    static u32 GetAmmoTotal(AMMO_VECTOR& pVIn, u8 iAmmoTypeIdx = u8(-1));

    static void PackAmmoInPacket(AMMO_VECTOR& pVAmmoIn, NET_Packet& tNetPacket);
    static void UnpackAmmoFromPacket(AMMO_VECTOR& pVAmmoOut, NET_Packet& tNetPacket);

#ifdef XRGAME_EXPORTS // to prevent this from being inside xrSE_Factory.dll
    static void CompressMagazine(AMMO_VECTOR& pVOut, CWeapon* pWeapon, bool bForGL = false);
    static void DecompressMagazine(AMMO_VECTOR& pVIn, CWeapon* pWeapon, bool bForGL = false);
#endif
};
