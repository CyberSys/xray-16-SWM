#pragma once

/******************************************/
/***** Класс универсального оружия *****/ //--#SM+#--
/******************************************/

#include "xrCommon/xr_unordered_set.h"

#include "hud_item_object.h"
#include "ShootingObject.h"
#include "RocketLauncher.h"
#include "ShellLauncher.h"

#include "Weapon_AddonData.h"
#include "Weapon_ZoomParams.h"
#include "Weapon_BipodsParams.h"

#include "firedeps.h"
#include "first_bullet_controller.h"
#include "CameraRecoil.h"

#include "game_cl_single.h"

class CEntity;
class ENGINE_API CMotionDef;
class CParticlesObject;
class CUIWindow;
class CBinocularsVision;
class CNightVisionEffector;
class CWeaponFakeGrenade;
class CSE_ALifeItemWeapon;
class CSE_ALifeItemWeaponAmmo;
class CWeaponMagazined;
class CWeaponMagazinedWGrenade;

#define WEAPON_REMOVE_TIME 60000            // Время по умолчанию, через которое бесхозное оружие будет удалено из игры
#define WEAPON_ININITE_QUEUE -1             // Число, которым в конфиге обозначается режим стрельбы без очереди
#define WEAPON_DEFAULT_QUEUE 1              // Режим стрельбы по умолчанию, когда у оружия это не указано в конфиге
#define WEAPON_AB_BULLET_VIS_UPD_DELAY 150  // Время (в м\сек), в течении которого по особому высчитываем видимость патронов в патронташе
#define WEAPON_ANIM_SHELL_SHOW_TIME 400     // Время (в м\сек), в течении которого показываем анимированную гильзу
#define WEAPON_MAGAZ_3P_HIDE_START_TIME 300 // Время (в м\сек) по умолчанию, когда скрывается магазин (от 3-го лица) после начала перезерадяки
#define WEAPON_MAGAZ_3P_HIDE_END_TIME 1700  // Время (в м\сек) по умолчанию, когда показывается магазин (от 3-го лица) после начала перезерадяки
#define WEAPON_ADDONS_VIS_UPD_INTERVAL 500  // Интервал (в м\сек), через который обновляем визуалы аддонов на оружии
#define WEAPON_ADDONS_KAR_UPD_INTERVAL 150  // Интервал (в м\сек), через который проверяем что штыковая атака попала в цель
#define WEAPON_DEFAULT_LAUNCH_SPEED 40.f    // Скорость по умолчанию для запуска ракеты (если не указана в конфигах)

#define C_THIS_WPN const_cast<CWeapon*>(this)

extern BOOL b_toggle_weapon_aim;
extern CUIXml* pWpnScopeXml;
extern u32 hud_adj_mode;

ENGINE_API extern float psHUD_FOV;
ENGINE_API extern float psHUD_FOV_def;

class CWeapon : public CHudItemObject, //--> Имеет худ
                public CShootingObject, //--> Может стрелять пулями
                public CRocketLauncher, //--> Может стрелять ракетами
                public CShellLauncher //--> Может выкидывать гильзы
{
//======= Описание класса и основные наследуемые методы =======//
    friend class CAmmoCompressUtil; //--> Вспомогательный класс для упаковки \ распаковки патронов в магазине

private:
    typedef CHudItemObject inherited;

protected:
    bool m_bNeed2UpdateIcon; //--> True если необходимо перерисовать иконку оружия в инвентаре (аддоны)
    bool m_bInvShowAmmoCntInMagaz; //--> Если true, то у оружия в инвентаре будет отображено кол-во патронов каждого типа в обойме (UIWpnParams.cpp)
    bool m_bInvShowWeaponStats; //--> Если true, то у оружия в инвентаре будут отображены характеристики
    bool m_bInvShowWeaponAmmo; //--> Если true, то у оружия в инвентаре будут отображены \ подсвечены используемые патроны

    // Время удаления бесхозного оружия
    ALife::_TIME_ID m_dwWeaponRemoveTime;
    ALife::_TIME_ID m_dwWeaponIndependencyTime;

    virtual bool IsNecessaryItem(const shared_str& item_sect);

public:
    CWeapon();
    virtual ~CWeapon();

    virtual CWeapon* cast_weapon() { return this; }
    virtual CWeaponMagazined* cast_weapon_magazined() { return (CWeaponMagazined*)this; }
    virtual CShellLauncher* cast_shell_launcher() { return (CShellLauncher*)this; }

    // Serialization
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual BOOL net_SaveRelevant() { return inherited::net_SaveRelevant(); }
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);

    // General
    virtual void Load(LPCSTR section);
    virtual void PostLoad(LPCSTR section);

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();

    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);
    BOOL IsUpdating(); //--> Does weapon need's update?

    virtual bool Action(u16 cmd, u32 flags);

    virtual void OnH_B_Chield();
    virtual void OnH_A_Chield();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_A_Independent();
    virtual void OnEvent(NET_Packet& P, u16 type); // {inherited::OnEvent(P,type);}

    virtual void Hit(SHit* pHDS);

    virtual void reinit();
    virtual void reload(LPCSTR section);
    virtual void create_physic_shell();
    virtual void activate_physic_shell();
    virtual void setup_physic_shell();

    virtual bool can_kill() const;
    virtual CInventoryItem* can_kill(CInventory* inventory) const;
    virtual const CInventoryItem* can_kill(const xr_vector<const CGameObject*>& items) const;
    virtual bool ready_to_kill() const;
    virtual bool NeedToDestroyObject() const;
    virtual ALife::_TIME_ID TimePassedAfterIndependant() const;

    virtual float Weight() const;
    virtual u32 Cost() const;

    // Инвентарные проверки
    virtual bool InventoryEqualTo(CWeapon* pWpnRef) const;
    IC bool Need2UpdateIcon() const { return m_bNeed2UpdateIcon; }
    IC void SetUpdateIcon(bool bValue) { m_bNeed2UpdateIcon = bValue; }
    IC bool InventoryShowAllAmmoCntInMagazine() const { return m_bInvShowAmmoCntInMagaz; }
    IC bool InventoryShowWeaponStats() const { return m_bInvShowWeaponStats; }
    IC bool InventoryShowWeaponAmmo() const { return m_bInvShowWeaponAmmo; }

//======================= Апгрейды =======================//
private:
    virtual void net_Spawn_install_upgrades(Upgrades_type saved_upgrades);

protected:
    virtual bool install_upgrade_impl(LPCSTR section, bool test);

    bool install_upgrade_ammo_class(LPCSTR section, bool test);
    bool install_upgrade_disp(LPCSTR section, bool test);
    bool install_upgrade_hit(LPCSTR section, bool test);
    bool install_upgrade_addon(LPCSTR section, bool test);
    bool install_upgrade_generic(LPCSTR section, bool test);
    bool install_upgrade_sounds(LPCSTR section, bool test);

public:
    virtual void pre_install_upgrade();

//====================== Состояния оружия ======================//
private:

protected:
    u8 m_sub_state; //--> EWeaponSubStates, под-состояния оружия

    virtual void OnStateSwitch(u32 S, u32 oldState);

    void switch2_Idle();
    void switchFrom_Idle(u32 newS);
    void state_Idle(float dt);

    virtual bool AllowBore();

    void switch2_Showing();
    void switchFrom_Showing(u32 newS);
    void state_Showing(float dt);

    void switch2_Hiding();
    void switchFrom_Hiding(u32 newS);
    void state_Hiding(float dt);

    void switch2_Hidden();
    void switchFrom_Hidden(u32 newS);
    void state_Hidden(float dt);

    void switch2_Bore();
    void switchFrom_Bore(u32 newS);
    void state_Bore(float dt);

    void switch2_Fire();
    void switchFrom_Fire(u32 newS);
    void state_Fire(float dt);

    void switch2_Empty();
    void switchFrom_Misfire(u32 newS);
    void state_Empty(float dt);

    void switch2_Misfire();
    void switchFrom_Empty(u32 newS);
    void state_Misfire(float dt);

    void switch2_Reload(); //--> Перезарядка в 1 этап
    void switch2_StartReload(); //--> Перезарядка в 3 этапа
    void switch2_AddCartgidge();
    void switch2_EndReload();
    void switchFrom_Reload(u32 newS);
    void state_Reload(float dt);

    void switch2_ReloadFrAB();
    void switchFrom_ReloadFrAB(u32 newS);
    void state_ReloadFrAB(float dt);

    void switch2_SwitchMag();
    void switchFrom_SwitchMag(u32 newS);
    void state_SwitchMag(float dt);

    void switch2_Switch();
    void switchFrom_Switch(u32 newS);
    void state_Switch(float dt);

    void switch2_Kick();
    void switchFrom_Kick(u32 newS);
    void state_Kick(float dt);

    void switch2_Pump();
    void switchFrom_Pump(u32 newS);
    void state_Pump(float dt);

    virtual void OnActiveItem();
    virtual void OnHiddenItem();
    virtual void SendHiddenItem(); // same as OnHiddenItem but for client... (sends message to a server)...

    void signal_HideComplete();

public:
    enum EWeaponStates
    {
        eFire = eLastBaseState + 1,
        eFire2,
        eReload,
        eReloadFrAB,
        eSwitchMag,
        eMisfire,
        eMagEmpty,
        eSwitch,
        eKick,
        ePump,
    };
    enum EWeaponSubStates
    {
        // Tri-State Reload
        eSubstateReloadBegin = 0,
        eSubstateReloadInProcess,
        eSubstateReloadEnd,
        // Magazines
        eSubstateMagazSwitch, //--> Смена магазина
        eSubstateMagazDetach, //--> Снятие магазина (старт действия)
        eSubstateMagazDetach_Do, //--> Снятие магазина (сам процесс снятия)
        eSubstateMagazMisfire, //--> Зафиксить осечку (не менять магазин)
        eSubstateMagazFinish, //--> Завершение (магазин уже установлен\снят)
    };

    virtual bool OnBeforeStateSwitch(u32 oldS, u32 newS);
    virtual void UpdateStates(float dt);
    virtual void SwitchState(u32 S);

    void Need2Idle();

    bool Try2Bore(bool bCheckOnlyMode = false);
    void Need2Stop_Bore();

    bool Try2Fire(bool bCheckOnlyMode = false);
    bool Try2Knife(bool bAltAttack);
    void Need2Stop_Fire();

    void Need2Empty();

    void Need2Misfire();

    bool Try2Reload(bool bCheckOnlyMode = false);
    bool Try2ReloadAmmoBelt();
    bool Try2AutoReload();
    bool Try2StopTriStateReload();
    void Need2Reload();
    void Need2Stop_Reload();
    IC bool IsReloading() const { return GetState() == eReload || GetState() == eReloadFrAB || GetState() == eSwitchMag; }
    bool IsTriStateReload() const;
    IC EWeaponSubStates GetReloadState() const { return (EWeaponSubStates)m_sub_state; }

    bool Try2ReloadFrAB(bool bCheckOnlyMode = false);

    bool Try2SwitchMag(bool bCheckOnlyMode = false, bool bFromDetach = false);
    void Need2SwitchMag();
    void Need2Stop_SwitchMag();

    bool Try2Switch(bool bCheckOnlyMode = false);
    void Need2Stop_Switch();

    bool Try2Kick(bool bCheckOnlyMode = false);
    void Need2Stop_Kick();

    bool Try2Pump(bool bCheckOnlyMode = false);
    void Need2Stop_Pump();

//================= Аддоны и аттачменты (общее) =================//
public:
    enum EAddons //--> Индексы-слоты аддонов в массиве m_addons
    {
        eScope = 0,
        eMuzzle = 1,
        eLauncher = 2,
        eMagaz = 3,
        eSpec_1 = 4,
        eSpec_2 = 5,
        eSpec_3 = 6,
        eSpec_4 = 7,
        eAddonsSize = 8,
        eNotExist = 9,
    };
    SAddonData m_addons[EAddons::eAddonsSize]; //--> Массив всех возможных аддонов оружия

private:
    u32 m_dwLastAddonsVisUpdTime; // Время в dwTimeGloabl (м\сек) когда последний раз вызывали плановый апдейт визуалов аддонов

protected:
    void _UpdateAddonsVisibility(SAddonData* m_pAddon);
    void _UpdateHUDAddonVisibility(SAddonData* m_pAddon, bool bForceReset = false);

    void OnAddonInstall(EAddons iSlot, const shared_str& sAddonDataName);
    void OnAddonUnistall(EAddons iSlot, const shared_str& sAddonDataName);

    virtual void OnDetachAddonSpawn(const char* item_section_name, CSE_ALifeDynamicObject* E);

public: 
    xr_unordered_multiset<xr_string> m_IncompatibleAddons; //--> Список-счётчик текущих несовместимых set-секций аддонов

    bool IsGrenadeLauncherAttached() const;
    bool IsScopeAttached() const;
    bool IsSilencerAttached() const;
    bool IsMagazineAttached() const;
    bool IsSpecial_1_Attached() const;
    bool IsSpecial_2_Attached() const;
    bool IsSpecial_3_Attached() const;
    bool IsSpecial_4_Attached() const;

    bool GrenadeLauncherAttachable() const;
    bool ScopeAttachable() const;
    bool SilencerAttachable() const;
    bool MagazineAttachable() const;
    bool Special_1_Attachable() const;
    bool Special_2_Attachable() const;
    bool Special_3_Attachable() const;
    bool Special_4_Attachable() const;

    ALife::EWeaponAddonStatus get_GrenadeLauncherStatus() const { return GetAddonBySlot(eLauncher)->m_attach_status; }
    ALife::EWeaponAddonStatus get_ScopeStatus() const { return GetAddonBySlot(eScope)->m_attach_status; }
    ALife::EWeaponAddonStatus get_SilencerStatus() const { return GetAddonBySlot(eMuzzle)->m_attach_status; }
    ALife::EWeaponAddonStatus get_MagazineStatus() const { return GetAddonBySlot(eMagaz)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_1_Status() const { return GetAddonBySlot(eSpec_1)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_2_Status() const { return GetAddonBySlot(eSpec_2)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_3_Status() const { return GetAddonBySlot(eSpec_3)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_4_Status() const { return GetAddonBySlot(eSpec_4)->m_attach_status; }

    // Для отоброажения иконок апгрейдов в интерфейсе
    int GetScopeX() const { return pSettings->r_s32(GetAddonBySlot(eScope)->GetName(), "scope_x"); }
    int GetScopeY() const { return pSettings->r_s32(GetAddonBySlot(eScope)->GetName(), "scope_y"); }
    int GetSilencerX() const { return pSettings->r_s32(GetAddonBySlot(eMuzzle)->GetName(), "muzzle_x"); }
    int GetSilencerY() const { return pSettings->r_s32(GetAddonBySlot(eMuzzle)->GetName(), "muzzle_y"); }
    int GetGrenadeLauncherX() const { return pSettings->r_s32(GetAddonBySlot(eLauncher)->GetName(), "launcher_x"); }
    int GetGrenadeLauncherY() const { return pSettings->r_s32(GetAddonBySlot(eLauncher)->GetName(), "launcher_y"); }
    int GetMagazineX() const { return pSettings->r_s32(GetAddonBySlot(eMagaz)->GetName(), "magazine_x"); }
    int GetMagazineY() const { return pSettings->r_s32(GetAddonBySlot(eMagaz)->GetName(), "magazine_y"); }
    int GetSpecial_1_X() const { return pSettings->r_s32(GetAddonBySlot(eSpec_1)->GetName(), "special_x"); }
    int GetSpecial_1_Y() const { return pSettings->r_s32(GetAddonBySlot(eSpec_1)->GetName(), "special_y"); }
    int GetSpecial_2_X() const { return pSettings->r_s32(GetAddonBySlot(eSpec_2)->GetName(), "special_x"); }
    int GetSpecial_2_Y() const { return pSettings->r_s32(GetAddonBySlot(eSpec_2)->GetName(), "special_y"); }
    int GetSpecial_3_X() const { return pSettings->r_s32(GetAddonBySlot(eSpec_3)->GetName(), "special_x"); }
    int GetSpecial_3_Y() const { return pSettings->r_s32(GetAddonBySlot(eSpec_3)->GetName(), "special_y"); }
    int GetSpecial_4_X() const { return pSettings->r_s32(GetAddonBySlot(eSpec_4)->GetName(), "special_x"); }
    int GetSpecial_4_Y() const { return pSettings->r_s32(GetAddonBySlot(eSpec_4)->GetName(), "special_y"); }

    // Получить секции предметов установленных аддонов
    const shared_str GetGrenadeLauncherName() const { return GetAddonBySlot(eLauncher)->GetAddonName(); }
    const shared_str GetScopeName() const { return GetAddonBySlot(eScope)->GetAddonName(); }
    const shared_str GetSilencerName() const { return GetAddonBySlot(eMuzzle)->GetAddonName(); }
    const shared_str GetMagazineName() const { return GetAddonBySlot(eMagaz)->GetAddonName(); }
    const shared_str GetSpecial_1_Name() const { return GetAddonBySlot(eSpec_1)->GetAddonName(); }
    const shared_str GetSpecial_2_Name() const { return GetAddonBySlot(eSpec_2)->GetAddonName(); }
    const shared_str GetSpecial_3_Name() const { return GetAddonBySlot(eSpec_3)->GetAddonName(); }
    const shared_str GetSpecial_4_Name() const { return GetAddonBySlot(eSpec_4)->GetAddonName(); }

    // Получить set-секции установленных аддонов
    const shared_str GetGrenadeLauncherSetSect() const
    {
        return IsGrenadeLauncherAttached() ? GetAddonBySlot(eLauncher)->GetName() : nullptr;
    }
    const shared_str GetScopeSetSect() const
    {
        return IsScopeAttached() ? GetAddonBySlot(eScope)->GetName() : nullptr;
    }
    const shared_str GetSilencerSetSect() const
    {
        return IsSilencerAttached() ? GetAddonBySlot(eMuzzle)->GetName() : nullptr;
    }
    const shared_str GetMagazineSetSect() const
    {
        return IsMagazineAttached() ? GetAddonBySlot(eMagaz)->GetName() : nullptr;
    }
    const shared_str GetSpecial_1_SetSect() const
    {
        return IsSpecial_1_Attached() ? GetAddonBySlot(eSpec_1)->GetName() : nullptr;
    }
    const shared_str GetSpecial_2_SetSect() const
    {
        return IsSpecial_2_Attached() ? GetAddonBySlot(eSpec_2)->GetName() : nullptr;
    }
    const shared_str GetSpecial_3_SetSect() const
    {
        return IsSpecial_3_Attached() ? GetAddonBySlot(eSpec_3)->GetName() : nullptr;
    }
    const shared_str GetSpecial_4_SetSect() const
    {
        return IsSpecial_4_Attached() ? GetAddonBySlot(eSpec_4)->GetName() : nullptr;
    }

    EAddons GetAddonSlot(IGameObject* pObj, u8* idx = nullptr) const;
    EAddons GetAddonSlot(LPCSTR section, u8* idx = nullptr, EAddons slotID2Search = eNotExist) const;
    EAddons GetAddonSlotBySetSect(LPCSTR section, u8* idx = nullptr, EAddons slotID2Search = eNotExist) const;

    IC SAddonData* GetAddonBySlot(EAddons slotID) const
    {
        VERIFY(slotID >= 0 && slotID < EAddons::eAddonsSize);
        return (SAddonData*)(&(m_addons[slotID]));
    }

    u8 GetAddonsState() const;
    void SetAddonsState(u8 m_flagsAddOnState);

    bool IsAddonsEqual(CWeapon* pWpn2Cmp) const;

    virtual bool LoadAddons(LPCSTR section, EFuncUpgrMode upgrMode = EFuncUpgrMode::eUpgrNone);
    virtual void InitAddons();

    void UpdateAddons();    
    void UpdateAddonsVisibility(bool bDontUpdateHUD = false);
    void UpdateHUDAddonsVisibility(bool bForceReset = false);

    virtual bool CanAttach(PIItem pIItem);
    virtual bool CanDetach(const char* item_section_name);

    virtual bool Attach(PIItem pIItem, bool b_send_event, bool b_from_actor_menu = false);
    virtual bool AttachMagazine(CWeapon* pMagazineItem, bool b_send_event, bool b_with_anim = false);

    virtual bool Detach(const char* item_section_name, bool b_spawn_item, bool b_from_actor_menu = false);
    virtual bool DetachMagazine(const char* item_section_name, bool b_spawn_item, bool b_with_anim = false);

    SAddonData* InstallAddon(EAddons iSlot, const shared_str& sAddonSetSect, bool bNoUpdate = false);
    SAddonData* InstallAddon(EAddons iSlot, u8 addon_idx, bool bNoUpdate = false);
    SAddonData* UnistallAddon(EAddons iSlot, bool bNoUpdate = false);

//================= Аддоны - Прицел =================//
private:

protected:
    float m_addon_holder_range_modifier; //--> Модификатор дальности зрения для AI
    float m_addon_holder_fov_modifier; //--> Модификатор угла обзора для AI

public:
    virtual void modify_holder_params(float& range, float& fov) const;

//================= Аддоны - Глушитель =================//
private:

protected:
    void ApplySilencerKoeffs();
    void ResetSilencerKoeffs();

public:
    void LoadSilencerKoeffs();

//================= Аддоны - Подствольник =================//
private:

protected:

public:
    bool m_bGrenadeMode;

    virtual void LoadGLParams();

    IC bool bIsGrenadeMode() const { return m_bGrenadeMode; }; // См. m_bGrenadeMode

    bool SwitchGunMode(); //--> Анимированное переключение подствола
    void PerformSwitchGL(); //--> Мгновенное переключение подствола

//================= Аддоны - Рукоятка =================//
private:

protected:
    EAddons m_ForegripSlot; //--> Слот аддона, который заменяет рукоятка

public:
    IC bool IsForegripAttached() const { return m_ForegripSlot != eNotExist; }

//============== Аддоны - Передняя рама (L85) ==============//
private:

protected:
    EAddons m_ForendSlot; //--> Слот аддона, который заменяет рукоятка

public:
    IC bool IsForendAttached() const { return m_ForendSlot != eNotExist; }

//================= Аддоны - Патронташ =================//
private:
    u32 m_dwABHideBulletVisual; //--> Время в dwTimeGlobal (м\сек), после которого мы по особому определяем видимость
                                //    некоторых патронов в патронташе

protected:
    EAddons m_AmmoBeltSlot; //--> Слот аддона, который занимает патронташ

    bool m_bUseAmmoBeltMode; //--> Оружие поддерживает патронташ (исключает подствол)

    shared_str m_sAB_hud; //--> Секции визуалов патронташа для худа (без патронов)
    shared_str m_sAB_vis; //--> Секции визуалов патронташа для мировой (без патронов)

    typedef std::pair<shared_str, shared_str> AmmoBeltData; // hud, vis
    xr_vector<AmmoBeltData> m_AmmoBeltData; //--> Секции визуалов патронов

    ICF void UpdBulletHideTimer() { m_dwABHideBulletVisual = Device.dwTimeGlobal + WEAPON_AB_BULLET_VIS_UPD_DELAY; }

public:
    virtual void LoadAmmoBeltParams();

    IC bool IsAmmoBeltAllowed() const { return m_bUseAmmoBeltMode; }
    IC bool IsAmmoBeltAttached() const { return m_AmmoBeltSlot != eNotExist; }
    IC bool IsAmmoBeltReloadNow() const { return m_bUseAmmoBeltMode && m_bGrenadeMode; }

    void UpdateAmmoBelt();

//========================= Аддоны - Сошки =========================//
private:

protected:
    EAddons m_BipodsSlot; //--> Слот аддона, который занимают сошки

    void UpdateBipods();

    // Каллбэки на установку\снятие сошек
    void OnBipodsAttach(EAddons iSlot, const shared_str& sAddonDataName);
    void OnBipodsDetach(const shared_str& sAddonDataName);

public:
    bipods_data m_bipods; // Параметры сошек оружия

    virtual void LoadBipodsParams();

    virtual bool UpdateCameraFromBipods(IGameObject* pCameraOwner, Fvector noise_dangle);
    virtual void UpdateActorTorchLightPosFromBipods(Fvector* pTorchPos);

    virtual void OnOwnedCameraMoveWhileBipods(CCameraBase* pCam, float fOldYaw, float fOldPitch);

    void Try2DeployBipods();
    void Need2UndeployBipods(bool bInstantly = true);

    IC bool IsBipodsAttached() const { return m_bipods.m_bInstalled; }
    IC bool CWeapon::IsBipodsDeployed() const
    {
        return m_bipods.m_bInstalled &&
            (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON ||
                m_bipods.m_iBipodState == bipods_data::eBS_TranslateInto);
    }

    void DeployBipods(Fvector vDeployPos, Fvector vDeployDir, Fvector vDeployNormal, bool bInstantly = false);
    void UndeployBipods(bool bInstantly = false);

    void BipodsSetCameraLimits(CCameraBase* pCam, bool bLimit);
    void BipodsZoom(u32 flags);

//===================== Аддоны - Магазинное питание =====================//
private:

protected:
    bool m_bUseMagazines; //--> Оружие использует магазинное питание
    bool m_bIsMagazine; //--> Оружие является "магазином"

    u8 m_set_next_magaz_on_reload; //--> Индекс типа магазина, который будет установлен при перезарядке
    u16 m_set_next_magaz_by_id; //--> ID предмета магазина, который будет установлен при перезарядке

    bool m_bMagaz3pIsHidden; //--> Магазин скрыт
    bool m_bMagaz3pHideWhileReload; //--> Нужно скрывать модель магазина (аддон) для 3-го лица при перезарядке

    u16 m_iMagaz3pHideStartTime; // Время (в м\сек) по умолчанию, когда скрывается магазин (от 3-го лица) после начала перезерадяки
    u16 m_iMagaz3pHideEndTime; // Время (в м\сек) по умолчанию, когда показывается магазин (от 3-го лица) после начала перезерадяки

public:
    virtual void LoadMagazinesParams(LPCSTR section);

    void UpdateMagazine3p(bool bForceUpdate = false);

    IC bool IsUseMagazines() const { return m_bUseMagazines; }
    IC bool IsMagazine() const { return m_bIsMagazine; }

    CWeapon* GetBestMagazine(LPCSTR section = nullptr);

    bool SwitchMagazineType();

//=================== Различные методы и свойства оружия ===================//
private:
    Fvector m_overriden_activation_speed; //--> Направление, в которое полетит оружие при выкидывании
    bool m_activation_speed_is_overriden; //--> У предмета переопределено направление выкидывания

protected:
    bool m_bNeed2Pump; //--> True если требуется передёрнуть помпу

    LPCSTR m_strap_bone0;
    LPCSTR m_strap_bone1;
    Fmatrix m_StrapOffset; //--> Смещение оружия на поясе
    bool m_strapped_mode; //--> Оружие находится на поясе
    bool m_can_be_strapped; //--> Оружие может быть повешено на пояс

    Fmatrix m_Offset; //--> Смещение оружия в руках

    EHandDependence eHandDependence; //--> 0-используется без участия рук, 1-одна рука, 2-две руки
    bool m_bIsSingleHanded; //--> Оружие является одноручным [Похоже что рудимент]

    // Для сталкеров, чтоб они знали эффективные границы использования оружия (рудимент и не используется)
    float m_fMinRadius;
    float m_fMaxRadius;

    // Эффективный основной и вспомогательный тип оружия (для AI)
    u32 m_ef_main_weapon_type;
    u32 m_ef_weapon_type;

public:
    bool m_bUsePumpMode; //--> Оружие является помповым
    bool m_bDisableFire; //--> Запретить стрельбу
    bool m_bHideCrosshair; //--> Скрыть перекрестие от бедра

    virtual BOOL ParentIsActor();

    IC bool can_be_strapped() const { return m_can_be_strapped; }; //--> См. m_can_be_strapped

    bool IsSingleHanded() const { return m_bIsSingleHanded; } //--> См. m_bIsSingleHanded
    virtual EHandDependence HandDependence() const { return eHandDependence; } //--> См. eHandDependence

    bool DetectorCheckCompability(bool bAllowAim) const;
    bool DetectorHideCondition(bool bAllowAim) const;

    IC bool Need2Pump() { return m_bNeed2Pump; } // См. m_bNeed2Pump

    // Оружие "на поясе" (для 3-го лица)
    IC LPCSTR strap_bone0() const { return m_strap_bone0; }
    IC LPCSTR strap_bone1() const { return m_strap_bone1; }
    IC void strapped_mode(bool value) { m_strapped_mode = value; }
    IC bool strapped_mode() const { return m_strapped_mode; }

    virtual u32 ef_main_weapon_type() const;
    virtual u32 ef_weapon_type() const;

    virtual void set_ef_main_weapon_type(u32 type) { m_ef_main_weapon_type = type; };
    virtual void set_ef_weapon_type(u32 type) { m_ef_weapon_type = type; };

    virtual bool ActivationSpeedOverriden(Fvector& dest, bool clear_override); //--> См. m_activation_speed_is_overriden
    virtual void SetActivationSpeedOverride(Fvector const& speed);

    virtual void DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const; //--> Для античита
    virtual shared_str const GetAnticheatSectionName() const { return cNameSect(); };

//========== Аммуниция (патроны, обоймы, гранаты, ...) и перезарядка ==========//
private:
    bool m_bNeed2StopTriStateReload; //--> True при попытке остановить перезарядку в 3 этапа
    bool m_bIsReloadFromAB; //--> True при перезарядке из патронташа

protected:
    BOOL m_bAutoSpawnAmmo;  //--> Разрешить доспавн пачек патронов к оружию (CActor::SpawnAmmoForWeapon)
    bool m_bAmmoWasSpawned; //--> True если для оружия были заспавнены патроны через метод SpawnAmmo()

    // Наличие перезарядки в 3-этапа
    bool m_bTriStateReload_main; //--> Для основного ствола
    bool m_bTriStateReload_gl; //--> Для подствола
    bool m_bTriStateReload_ab; //--> Для патронташа
    bool m_bTriStateReload_frab; //--> Для перезарядки из патронташа

    u8 m_set_next_ammoType_on_reload; //--> Тип патронов, который будет выставлен после перезарядки (-1: не менять)
    bool m_bLockType; //--> Заблокировать смену типов патронов

    void ResizeMagazine(int ammo_count, xr_vector<CCartridge*>& Magazine, CCartridge* pCartridge);
    void SetAmmoTypeSafeFor(u8 ammoType, bool bForGL);

    xr_vector<CCartridge> m_AmmoCartidges;
    xr_vector<CCartridge> m_AmmoCartidges2;

    virtual void OnMagazineEmpty();

public:
    xr_vector<CCartridge*> m_magazine; //--> "Магазин" ствола (Основной \ Подствол)
    xr_vector<CCartridge*> m_magazine2; //--> "Магазин" ствола (Подствол \ Основной)

    int iAmmoElapsed; //--> Кол-во патронов в магазине (Основной \ Подствол)
    int iAmmoElapsed2; //--> Кол-во патронов в магазине (Подствол \ Основной)

    int iMagazineSize; //--> Размер магазина (Основной \ Подствол)
    int iMagazineSize2; //--> Размер магазина (Подствол \ Основной)

    CWeaponAmmo* m_pCurrentAmmo; //--> Последняя коробка патронов, которой перезаряжи ствол (не учитывает магазины)
    xr_vector<shared_str> m_ammoTypes; //--> Секции поддерживаемых патронов (Основной \ Подствол)
    xr_vector<shared_str> m_ammoTypes2; //--> Секции поддерживаемых патронов (Подствол \ Основной)
    u8 m_ammoType; //--> Текущий индекс типа патронов (Основной \ Подствол)
    u8 m_ammoType2; //--> Текущий индекс типа патронов (Подствол \ Основной)

public:
    enum
    {
        undefined_ammo_type = u8(-1)
    };

    bool m_bAllowAutoReload; //--> Разрешить авто-перезарядку оружия
    bool m_bAllowUnload; //--> Разрешить разрядку
    bool m_bDisableAnimatedReload; //--> Запретить анимированную перезарядку
    bool m_bUIShowAmmo; //--> Отображать счётчик патронов

    bool m_bHasTracers; //--> Есть-ли трассеры у оружия
    u8 m_u8TracerColorID; //--> Цвет трассеров

    virtual bool LoadMainAmmoParams(LPCSTR section, bool bFromLoad = false, bool bDontUnload = false,
        EFuncUpgrMode upgrMode = EFuncUpgrMode::eUpgrNone);

    virtual bool IsMagazineCanBeUnload(bool bForGL = false);
    IC BOOL AutoSpawnAmmo() const { return m_bAutoSpawnAmmo; }; //--> См. m_bAutoSpawnAmmo
 
    bool unlimited_ammo();

    IC u8 GetAmmoType() const { return (m_bGrenadeMode ? GetGLAmmoType() : GetMainAmmoType()); }
    IC u8 GetMainAmmoType() const { return (m_bGrenadeMode ? m_ammoType2 : m_ammoType); }
    IC u8 GetGLAmmoType() const { return (m_bGrenadeMode ? m_ammoType : m_ammoType2); }

    IC xr_vector<shared_str>* GetMainAmmoTypes() { return (m_bGrenadeMode ? &m_ammoTypes2 : &m_ammoTypes); }
    IC xr_vector<shared_str>* GetGLAmmoTypes() { return (m_bGrenadeMode ? &m_ammoTypes : &m_ammoTypes2); }

    void SetAmmoType(u8 type) { SetAmmoTypeSafeFor(type, m_bGrenadeMode); }
    IC void SetMainAmmoType(u8 type) { SetAmmoTypeSafeFor(type, false); }
    IC void SetGLAmmoType(u8 type) { SetAmmoTypeSafeFor(type, true); }
    bool SwitchAmmoType();

    CartridgesInfoMap CWeapon::GetAmmoInfo(bool bForGL) const;

    int GetAmmoCount(u8 ammo_type) const;
    int GetAmmoCount2(u8 ammo2_type) const;
    int GetAmmoCount_forType(shared_str const& ammo_type) const;
    int GetSuitableAmmoTotal(bool use_item_to_spawn = false) const;

    bool IsAmmoAvailable() const;

    IC int GetAmmoMagSize() const { return iMagazineSize; } //--> Размер магазина (Основной \ Подствол)
    IC int GetMainMagSize() const { return (m_bGrenadeMode ? iMagazineSize2 : iMagazineSize); }
    IC int GetGLMagSize() const { return (m_bGrenadeMode ? iMagazineSize : iMagazineSize2); }

    IC int GetAmmoElapsed() const { return iAmmoElapsed; } //--> Оставшиеся патроны в магазине (Основной \ Подствол)
    IC int GetMainAmmoElapsed() const { return (m_bGrenadeMode ? iAmmoElapsed2 : iAmmoElapsed); }
    IC int GetGLAmmoElapsed() const { return (m_bGrenadeMode ? iAmmoElapsed : iAmmoElapsed2); }

    void SetAmmoElapsed(int ammo_count);
    void SetAmmoElapsedFor(int ammo_count, bool bForGL);

    void TransferAmmo(CWeapon* pTarget, bool bForGL = false, bool bCopyOnly = true);
    void SpawnAmmo(u32 boxCurr = 0xffffffff, LPCSTR ammoSect = NULL, u32 ParentID = 0xffffffff);

    void UnloadMagazine(bool spawn_ammo = true);
    void UnloadMagazineMain(bool spawn_ammo = true);
    void UnloadMagazineGL(bool spawn_ammo = true);

    void ReloadMagazine();
    void ReloadMagazineFrAB();
    void ReloadMagazineWithType(u8 ammoType);
    void ReloadMainMagazineWithType(u8 ammoType);
    void ReloadGLMagazineWithType(u8 ammoType);
    
    bool HaveCartridgeInInventory(u8 cnt);
    u8 AddCartridge(u8 cnt);
    u8 AddCartridgeFrAB(u8 cnt);

    bool SwapBulletFromAB();

    bool InventoryFastReloadAllowed(bool bForGL = false) const;
    void InventoryFastReload(u8 ammoType, bool bForGL = false);

//====================== Основные ТТХ Оружия ======================//
private:
    float m_hit_probability[egdCount]; //--> Вероятность "промазать" при стрельбе по игроку (возможно сейчас не используется)

protected:
    struct SPDM
    {
        float m_fPDM_disp_base;
        float m_fPDM_disp_vel_factor;
        float m_fPDM_disp_accel_factor;
        float m_fPDM_disp_crouch;
        float m_fPDM_disp_crouch_no_acc;
    };
    SPDM m_pdm; //--> Параметры модификации разброса от бедра

    float m_fCurrentCartirdgeDisp; //--> Коэфицент разброса от последнего выпущенного патрона

public:
    float m_fScopeInertionFactor; //--> Чувствительность мыши во время прицеливания

    CameraRecoil cam_recoil; //--> Отдача камеры от бедра (simple mode (walk, run))
    CameraRecoil zoom_cam_recoil; //--> Отдача камеры от бедра (using zoom (ironsight or scope))

    virtual void LoadRecoilParams(LPCSTR section);

    // Чувствительность мыши при активном оружии
    virtual float GetControlInertionFactor() const;

    // Разброс оружия
    float GetBaseDispersion(float cartridge_k);
    float GetFireDispersion(bool with_cartridge, bool for_crosshair = false);
    float GetFireDispersion(float cartridge_k, bool for_crosshair = false);
    float GetFirstBulletDisp() const { return m_first_bullet_controller.get_fire_dispertion(); };

    IC float Get_PDM_Base() const { return m_pdm.m_fPDM_disp_base; };
    IC float Get_PDM_Vel_F() const { return m_pdm.m_fPDM_disp_vel_factor; };
    IC float Get_PDM_Accel_F() const { return m_pdm.m_fPDM_disp_accel_factor; };
    IC float Get_PDM_Crouch() const { return m_pdm.m_fPDM_disp_crouch; };
    IC float Get_PDM_Crouch_NA() const { return m_pdm.m_fPDM_disp_crouch_no_acc; };

    virtual BOOL ParentMayHaveAimBullet();

    const float& hit_probability() const;

//=========================== Износ и осечка у оружия ======================//
private:

protected:
    bool bMisfire; //--> A misfire happens, you'll need to rearm weapon

    // Вероятность осечки при максимальной изношености
    float misfireStartCondition; //--> Изношенность, при которой появляется шанс осечки
    float misfireEndCondition; //--> Изношеность при которой шанс осечки становится константным
    float misfireStartProbability; //--> Шанс осечки при изношености больше чем misfireStartCondition
    float misfireEndProbability; //--> Шанс осечки при изношености больше чем misfireEndCondition
    float conditionDecreasePerQueueShot; //--> Увеличение изношености при выстреле очередью
    float conditionDecreasePerShot; //--> Увеличение изношености при одиночном выстреле

    float fireDispersionConditionFactor; //--> фактор увеличения дисперсии при максимальной изношености (на сколько процентов увеличится дисперсия)

public:
    virtual float GetConditionToShow() const;

    BOOL IsMisfire() const;
    BOOL CheckForMisfire();

    float GetMisfireStartCondition() const { return misfireStartCondition; };
    float GetMisfireEndCondition() const { return misfireEndCondition; };

    float GetConditionDispersionFactor() const;
    float GetConditionMisfireProbability() const;

    IC float GetWeaponDeterioration() const //--> Получить базовый износ оружия за каждый выстрел
    {
        return (m_iShotNum == 1) ? conditionDecreasePerShot : conditionDecreasePerQueueShot;
    };

//======================= Режим оружия "Нож" =======================//
private:

protected:
    CWeaponKnifeHit* m_first_attack; //--> Первая атака ножа
    CWeaponKnifeHit* m_second_attack; //--> Вторая атака ножа

public:
    bool m_bKnifeMode;  //--> Режим ножа (вместо стрельбы пулями использует CWeaponKnifeHit)

//=========================== Удар прикладом и штык-нож ===========================//
private:
    u32 m_dw_last_kick_at_run_upd_time; //--> Время в dwTimeGlobal (м\сек), когда последний раз проверяли штыковую атаку на столкновение

protected:
    EAddons m_BayonetSlot; //--> Слот аддона, который занимает штык нож

    CWeaponKnifeHit* m_kicker_main; //--> Обычная атака прикладом
    CWeaponKnifeHit* m_kicker_alt; //--> Штыковая атка (на бегу)

    bool m_bKickAtRunActivated; //--> Активен режим штыковой атаки на бегу

public:
    virtual void LoadBayonetParams();

    IC bool IsBayonetAttached() { return m_BayonetSlot != eNotExist; } //--> Присутствие штык-ножа
    IC bool IsKickAtRunActive() { return m_bKickAtRunActivated; } //--> См. m_bKickAtRunActivated

    void KickHit(bool bAltMode = false);

//=========================== Прицеливание ===========================//
public:
    enum EZoomTypes //--> Виды прицеливания
    {
        eZoomMain = 0,
        eZoomAlt,
        eZoomGL,
        eZoomTypesCnt
    };

private:
    bool m_bIdleFromZoomOut; //--> true если выходим из прицеливания во время idle \ fire состояния (хак для анимаций)
    bool m_bRememberActorNVisnStatus; //--> Хранит статус активности ночного виденья (от шлема) перед началом прицеливания

protected:
    SZoomParams m_zoom_params[EZoomTypes::eZoomTypesCnt]; // main, alt, gl

    EZoomTypes m_iCurZoomType; //--> Текущий тип прицеливания
    EZoomTypes m_iPrevZoomType; //--> Прошлый тип прицеливания

    bool m_bZoomEnabled; //--> Прицеливание разрешено
    bool m_bAltZoomEnabled; //--> Альтернативное прицеливание разрешено
    bool m_bIsZoomModeNow; //--> Целимся-ли мы сейчас
    bool m_bHideCrosshairInZoom; //--> Скрывать перекрестие во время зума

    float m_fZoomRotationFactor; //--> Степень разворота ствола от бедра к прицеливанию [0.f - 1.f]
    float m_fZoomRotateTime; //--> Скорость прицеливания
    float m_bUseOldZoomFactor; //--> Использовать старый механизм увеличения (FOV прицеливания = m_fCurrentZoomFactor * 0.75f)
    float m_fZoomFovFactorUpgr; //--> Добавляется к текущему FOV зума (для апгрейдов)

    shared_str m_sUseZoomPostprocessUpgr; //--> Пост-эффект прицела (переопределённый апгрейдами)
    shared_str m_sUseBinocularVisionUpgr; //--> Эффект выделения в прицеле живых существ (переопределённый апгрейдами)

public:
    bool m_bDisableFireWhileZooming; //--> Нужно-ли блокировать стрельбу при переходе в\из прицеливание

    virtual void LoadZoomParams(LPCSTR section);
    
    IC bool IsZoomEnabled() const { return m_bZoomEnabled; }
    IC bool IsAltZoomEnabled() const { return m_bAltZoomEnabled; }
    IC bool IsZoomed() const { return m_bIsZoomModeNow; };

    IC SZoomParams& GetZoomParams() const { return C_THIS_WPN->m_zoom_params[m_iCurZoomType]; }
    IC SZoomParams& GetZoomParams(EZoomTypes iType) const { return C_THIS_WPN->m_zoom_params[iType]; }

    IC EZoomTypes GetZoomType() { return m_iCurZoomType; }
    IC EZoomTypes GetPrevZoomType() { return m_iPrevZoomType; }
    void SwitchZoomType(EZoomTypes iType);
    void SwitchZoomType(EZoomTypes iCurType, EZoomTypes iPrevType);

    IC float GetAimZoomFactor() const { return GetZoomParams().m_fZoomFovFactor + m_fZoomFovFactorUpgr; }
    IC float GetSecondVPZoomFactor() const { return GetZoomParams().m_fSecondVPFovFactor; }
    IC float IsSecondVPZoomPresent() const { return GetSecondVPZoomFactor() > 0.000f; }

    IC float GetZRotatingFactor() const { return m_fZoomRotationFactor; }
    IC bool IsRotatingToZoom() const { return (m_fZoomRotationFactor < 1.f); }

    CUIWindow* ZoomTexture() const;
    bool CanUseScopeTexture() const;
    IC bool ZoomHideCrosshair() { return m_bHideCrosshairInZoom || ZoomTexture(); }

    void ZoomDynamicMod(bool bIncrement, bool bForceLimit);
    void ZoomInc(bool bForceLimit = false);
    void ZoomDec(bool bForceLimit = false);

    void OnZoomIn(bool bSilent = false);
    void OnZoomOut(bool bSilent = false);

    void PlaySoundZoomIn();
    void PlaySoundZoomOut();

    IC bool GetRememberActorNVisnStatus() const { return m_bRememberActorNVisnStatus; }; //--> См. m_bRememberActorNVisnStatus
    virtual void EnableActorNVisnAfterZoom();

//====================== Гильзы (2D/3D/Анимированные) ======================//
private:
    u32 m_dwShowAnimatedShellVisual; //--> Время в dwTimeGlobal (м\сек), в течении которого гарантировано отображаем
                                     //    анимированную гильзу

protected:
    shared_str m_sAnimatedShellVisData; //--> Секция визуала анимированной гильзы
    shared_str m_sCurrentAnimatedShellModel; //--> Текущая модель анимированной гильзы (наследуется от патрона)

    ICF void UpdShellShowTimer() { m_dwShowAnimatedShellVisual = Device.dwTimeGlobal + WEAPON_ANIM_SHELL_SHOW_TIME; }

    void UpdateAnimatedShellVisual();

public:
    void LaunchShell2D(Fvector* pVel = nullptr);

//====================== Анимация, Weapon HUD и эффекты ======================//
private:
    int m_overridenAmmoForReloadAnm; //--> Переопределить число патронов для анимации перезарядки (хак)
    bool m_bSwitchAddAnimation; //--> True если нужно проиграть анимацию перехода перезарядки из патронташа в инвентарь

protected:
    enum EZoomAnimStae
    {
        eZANone,
        eZAIn,
        eZAOut,
    };
    EZoomAnimStae m_ZoomAnimState; //--> Указывает какую анимацию проиграть во время прицеливания (PlayAnimZoom)

    bool m_bDisableMovEffAtZoom; //--> Нужно-ли блокировать эффекты ходьбы во время зума
    bool m_bTriStateReload_anim_hack; //--> Нужно-ли использовать подмену некоторых анимаций от 3-го лица (перезарядка в 3 этапа)

    virtual void PlayAnimIdle();
    virtual void PlayAnimIdleOnly();
    virtual void PlayAnimIdleMoving();
    virtual void PlayAnimIdleSprint();

    virtual void PlayAnimBore();
    virtual void PlayAnimShow();
    virtual void PlayAnimHide();

    virtual void PlayAnimZoom();

    virtual void PlayAnimShoot();
    virtual void PlayAnimEmptyClick();
    virtual void PlayAnimKnifeAttack();
    virtual void PlayAnimPump();

    virtual void PlayAnimKick();
    virtual void PlayAnimKickAlt();
    virtual bool PlayAnimKickOut();
    virtual bool PlayAnimKickOutAlt();

    virtual void PlayAnimReload();
    virtual void PlayAnimReloadAB();
    virtual void PlayAnimReloadFrAB();

    virtual bool PlayAnimOpenWeapon();
    virtual bool PlayAnimOpenWeaponAB();
    virtual bool PlayAnimOpenWeaponFrAB();

    virtual void PlayAnimAddOneCartridgeWeapon();
    virtual bool PlayAnimAddOneCartridgeWeaponAB();
    virtual bool PlayAnimAddOneCartridgeWeaponFrAB();

    virtual bool PlayAnimCloseWeapon();
    virtual bool PlayAnimCloseWeaponFromEmpty();
    virtual bool PlayAnimCloseWeaponAB();
    virtual bool PlayAnimCloseWeaponFrAB();
    virtual bool PlayAnimCloseWeaponFrABFromEmpty();

    virtual bool PlayAnimSwitchAddOneCartridge();

    virtual void PlayAnimModeSwitch();

    virtual void OnAnimationEnd(u32 state);
    virtual void OnMotionMark(u32 state, const motion_marks&);

public:
    float m_fLR_MovingFactor; // Фактор бокового наклона худа при ходьбе [-1; +1]

    virtual void OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch);
    virtual void OnFirstAnimationPlayed(const shared_str& sAnmAlias);
    virtual motion_params OnBeforeMotionPlayed(const shared_str& sAnmAlias);

    virtual bool UpdateCameraFromHUD(IGameObject* pCameraOwner, Fvector noise_dangle);
    virtual void UpdateActorTorchLightPosFromHUD(Fvector* pTorchPos);

    virtual bool IsMovementEffectorAllowed() const;

    virtual float GetInertionAimFactor() { return 1.f - GetZRotatingFactor(); }; //--> [От 1.0 - Инерция от бедра, до 0.0 - Инерция при зумме] Какую инерцию использовать
    virtual float GetInertionPowerFactor();

    bool IsWGLAnimRequired() const;

    void ResetIdleAnim();
    IC bool TriStateReloadAnimHack() const { return m_bTriStateReload_anim_hack; }

    // Play HUD Animation
    bool PlaySoundMotion(const shared_str& M, BOOL bMixIn, LPCSTR alias, bool bAssert = false, int anim_idx = -1);
    void PlaySoundMotionNoHUD(LPCSTR sAnmAlias_base, LPCSTR sSndAlias, LPCSTR sAnmAliasDef, LPCSTR sSndAliasDef);

    // Play world (3P) Animation
    bool PlayWorldMotion(const shared_str& M, BOOL bMixIn);
    
//======================== Параметры и эффекты стрельбы ========================//
private:
    firedeps m_current_firedeps; //--> Позиции эффектов стрельбы, дыма и 2D гильз (1 \ 3-ее лицо)

protected:
    first_bullet_controller m_first_bullet_controller; //--> Сверхточная и быстрая первая пуля

    shared_str m_sOverridedRocketSection; //--> Секция гранаты\ракеты, если не NULL то она будет выпущена при выстреле вместо "родной" пули\ракеты

    // Очередь из пуль "без отдачи" (Абакан)
    int m_iBaseDispersionedBulletsCount; //--> После какого патрона, при непрерывной стрельбе, начинается отдача (сделано из-за Абакана)
    float m_fBaseDispersionedBulletsSpeed; //--> Скорость вылета патронов, на которые не влияет отдача (сделано из-за Абакана)
    float m_fInitialBaseBulletSpeed; //--> Начальная скорость пули перед началом "безотдачной" стрельбы

    // Режимы стрельбы
    bool m_bHasDifferentFireModes; //--> Оружие поддерживает разные режимы стрельбы
    xr_vector<s8> m_aFireModes; //--> Лист поддерживаемых режимов стрельбы
 
    int m_iCurFireMode; //--> Текущий режим стрельбы очередью
    int m_iPrefferedFireMode; //--> Предпочитаемый (по умолчанию) режим стрельбы очередью

    int m_iQueueSize; // Текущий максимальный размер очереди, которой можно стрельнуть

    // Информация о кол-ве выпущенных патронов в рамках текущей очереди
    int m_iShotNum; //--> Количество выпущенных патронов в рамках текущей очереди
    bool m_bFireSingleShot; //--> Флаг того, что хотя бы один выстрел мы должны сделать (даже если очень быстро нажали на курок и вызвалось FireEnd)
    bool m_bStopedAfterQueueFired; //--> Флаг того, что мы остановились после того как выпустили ровно столько патронов, сколько было задано в m_iQueueSize

    virtual void UpdateXForm();
    virtual void UpdateHudAdditonal(Fmatrix&);
    virtual void UpdateFireDependencies_internal();
    IC void UpdateFireDependencies()
    {
        if (dwFP_Frame == Device.dwFrame)
            return;
        UpdateFireDependencies_internal();
    };

    void UpdatePosition(const Fmatrix& transform);

    void OnShot(bool bIsRocket = false, bool bIsBaseDispersionedBullet = false);
    void OnRocketLaunch(u16 rocket_id);
    void OnEmptyClick(bool bFromMisfire = false);
    void OnNextFireMode();
    void OnPrevFireMode();

public:
    // Позиция эффекта выстрела (3-ее лицо)
    Fvector vLoadedFirePoint;  //--> Основной ствол
    Fvector vLoadedFirePoint2; //--> Подствол

    virtual void LoadFireParams(LPCSTR section);
    virtual void debug_draw_firedeps();

    bool IsShootingAllowed() const;
    IC int ShotsFired() { return m_iShotNum; } //--> См. m_iShotNum

    virtual void FireStart();
    virtual void FireEnd();

    bool IsGrenadeBullet() const;

    void FireTrace(const Fvector& P, const Fvector& D); //--> Выстрел пулей (вызывает FireBullet)
    bool SpawnAndLaunchRocket(); //--> Выстрел ракетой \ гранатой из основного ствола
    void LaunchGrenade(); //--> Выстрел из подствольника

    virtual void FireBullet(const Fvector& pos, const Fvector& dir, float fire_disp, const CCartridge& cartridge,
        u16 parent_id, u16 weapon_id, bool send_hit);

    IC const Fvector& get_LastFP()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastFP;
    }
    IC const Fvector& get_LastFP2()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastFP2;
    }
    IC const Fvector& get_LastFD()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastFD;
    }
    IC const Fvector& get_LastSP()
    {
        UpdateFireDependencies();
        return m_current_firedeps.vLastSP;
    }

    virtual const Fvector& get_CurrentFirePoint() { return get_LastFP(); }
    virtual const Fvector& get_CurrentFirePoint2() { return get_LastFP2(); }
    virtual const Fmatrix& get_ParticlesXFORM()
    {
        UpdateFireDependencies();
        return m_current_firedeps.m_FireParticlesXForm;
    }

    IC bool HasFireModes() { return m_bHasDifferentFireModes; }; //--> Оружие поддерживает разные режимы стрельбы
    IC s8 GetCurrentFireMode() const //--> Получить текущий режим стрельбы
    {
        return m_bHasDifferentFireModes ? m_aFireModes[m_iCurFireMode] : (s8)WEAPON_DEFAULT_QUEUE;
    }

    void SetQueueSize(int size);
    IC int GetQueueSize() const { return m_iQueueSize; };
    IC bool SingleShotMode() const { return 1 == m_iQueueSize; }

    bool StopedAfterQueueFired() const { return m_bStopedAfterQueueFired; }
    void StopedAfterQueueFired(bool value) { m_bStopedAfterQueueFired = value; }

//====================== Эффект приближения оружия около стен ======================//
private:
    u32 dwUpdateSounds_Frame; //--> Кадр момента вызова UpdateSounds (оптимизиация)
    float m_nearwall_last_hud_fov; //--> HUD FOV от прошлого апдейта

protected:

public:
    float m_nearwall_dist_min; //--> Минимальная дистанция до стены, после которой перестаём увеличивать худ
    float m_nearwall_dist_max; //--> Максимальная дистанция до стены, дальше которой не влияем на худ
    float m_nearwall_target_hud_fov; //--> Целевой HUD FOV при максимальном упирании в стену
    float m_nearwall_speed_mod; //--> Модификатор скорости изменения HUD FOV около стен

//=========================== Звуки ===========================//
private:

protected:
    LPCSTR sReloadSndSectOverride; //--> Секция в конфигах, если не NULL - то все звуки будут считываться из неё
    shared_str m_sSndShotCurrent; //--> Alias звука текущего выстрела (обычный \ глушитель)

    virtual void UpdateSounds();

public:
    virtual void ReloadSound(
        shared_str const& strName, shared_str const& strAlias, bool exclusive = false, int type = sg_SourceType);

    virtual void ReloadAllSounds();
    virtual void ReloadAllSounds(LPCSTR sFromSect);
    virtual void ReloadAllSoundsWithUpgrades();

//================= Освещение, партиклы, рендер =================//
private:

protected:
    float m_crosshair_inertion; //--> Скорость изменения перекрестия в UI (зависит от разбрсоа оружия)
    
    shared_str m_sBulletHUDVisual; //--> Секция визуала текущего патрона в руках (HUD)

    shared_str m_sHolographBone; //--> Название кости голографа на худовой модели, которая будет раскрыта при прицеливании
    float m_fHolographRotationFactor; //--> Фактор поворота ствола от бедра к прицеливанию [0, 1], выше которого отобразим голограф

    // Объект партиклов для стрельбы из подствола
    shared_str m_sFlameParticles2;
    CParticlesObject* m_pFlameParticles2;

    virtual bool IsHudModeNow();

    void UpdateBulletHUDVisual();
    void UpdateWpnExtraVisuals();
    void UpdateGrenadeVisibility();

    // Обработка визуализации выстрела
    void AddShotEffector();
    void RemoveShotEffector();
    void ClearShotEffector();
    void StopShotEffector();

    // Эффекты для подствола
    void StartFlameParticles2();
    void StopFlameParticles2();
    void UpdateFlameParticles2();

    // Партиклы для глушителя
    LPCSTR m_sSilencerFlameParticles;
    LPCSTR m_sSilencerSmokeParticles;

    // Для подсчета в GetSuitableAmmoTotal
    mutable int m_iAmmoCurrentTotal;
    mutable u32 m_BriefInfo_CalcFrame; //--> Кадр на котором просчитали кол-во патронов

public:
    float m_HudFovAddition; //--> Число, добавляемое к дефолтному HUD FOV от бедра (0.0 - 1.0)

    static void ReadMaxBulletBones(IKinematics* pModel);

    float GetFov() const;
    float GetSecondVPFov() const;
    float GetHudFov();

    virtual u8 GetCurrentHudOffsetIdx();
    virtual float GetCrosshairInertion() const { return m_crosshair_inertion; };

    virtual void LoadLights(LPCSTR section, LPCSTR prefix);
    virtual void ForceUpdateFireParticles();
    void CheckFlameParticles(LPCSTR section, LPCSTR prefix);   

    void UpdateSecondVP();
    void StopAllEffects();

    virtual void renderable_Render();
    virtual void render_hud_mode();
    virtual bool need_renderable();

    virtual void render_item_ui();
    virtual bool render_item_ui_query();

    virtual bool use_crosshair() const { return true; } //--> Предмет использует перекрестие
    bool show_crosshair(); //--> Нужно-ли отрисовать перекрестие
    bool show_indicators(); //--> Нужно-ли отрисовать интерфейс

    IC void ForceUpdateAmmo() { m_BriefInfo_CalcFrame = 0; }; //--> Принудительно обновить информацию о патронах в UI

    virtual bool GetBriefInfo(II_BriefInfo& info);
    virtual bool DrawConditionBar() { return !IsMagazine(); }

    virtual void OnChangeVisual();
    virtual void OnAdditionalVisualModelChange(attachable_visual* pChangedVisual);

#ifdef DEBUG
    virtual void OnRender();
#endif
};
