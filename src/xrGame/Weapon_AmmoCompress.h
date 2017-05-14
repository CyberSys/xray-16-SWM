/*********************************************************/
/***** Упаковщик для сохранения и загрузки патронов  *****/ //--#SM+#--
/*********************************************************/

class CCartridge;
class CAmmoCompressUtil
{
public:
    typedef std::pair<u8, u32> ammo_pair; // ammoType, ammoCnt
    DEFINE_VECTOR(ammo_pair, AMMO_VECTOR, AMMO_VECTOR_IT);

    static void CompressMagazine(AMMO_VECTOR& pVOut, CWeapon* pWeapon, bool bForGL = false);
    static void DecompressMagazine(AMMO_VECTOR& pVIn, CWeapon* pWeapon, bool bForGL = false);

    static void PackAmmoInPacket(AMMO_VECTOR& pVAmmoIn, NET_Packet& tNetPacket);
    static void UnpackAmmoFromPacket(AMMO_VECTOR& pVAmmoOut, NET_Packet& tNetPacket);
};