#pragma once

//--#SM+#--

#include "Include/xrRender/KinematicsAnimated.h"
#include "xrPhysics/PhysicsShell.h"
#include "PHShellCreator.h"
#include "ai_sounds.h"
#include "hud_item_object.h"
#include "Actor_Flags.h"
#include "ShootingObject.h"
#include "RocketLauncher.h"
#include "ShellLauncher.h"
#include "WeaponKnifeHit.h"
#include "WeaponAmmo.h"
#include "CameraRecoil.h"
#include "firedeps.h"
#include "first_bullet_controller.h"
#include "Common/object_broker.h"
#include "game_cl_single.h"
#include "Weapon_AmmoCompress.h"
#include "Weapon_AddonData.h"
#include "Weapon_ZoomParams.h"

class CEntity;
class ENGINE_API CMotionDef;
class CSE_ALifeItemWeapon;
class CSE_ALifeItemWeaponAmmo;
class CWeaponMagazined;
class CWeaponMagazinedWGrenade;
class CParticlesObject;
class CUIWindow;
class CBinocularsVision;
class CNightVisionEffector;

class CWeaponFakeGrenade;
//class ammo_pair;

#define WEAPON_REMOVE_TIME 60000
#define ROTATION_TIME 0.25f
#define WEAPON_ININITE_QUEUE -1
#define WEAPON_DEFAULT_QUEUE 1

// clang-format off
// Прочитать данные сперва из секции аддона внутри оружия, потом самого аддона, и если там тоже нету - из секции оружия.
#define READ_ADDON_DATA(DEF_method,DEF_Name,DEF_fnc_1,DEF_fnc_2,DEF_def_val) \
			READ_IF_EXISTS(pSettings, DEF_method, DEF_fnc_1,	DEF_Name,	 \
			READ_IF_EXISTS(pSettings, DEF_method, DEF_fnc_2,	DEF_Name,	 \
			READ_IF_EXISTS(pSettings, DEF_method, cNameSect(),	DEF_Name,	 \
		DEF_def_val)));
// clang-format on

class CWeapon : public CHudItemObject, public CShootingObject, public CRocketLauncher, public CShellLauncher
{
    friend class CAmmoCompressUtil;

private:
    typedef CHudItemObject inherited;

public:
#define C_THIS const_cast<CWeapon*>(this) // Грязный хак .__.

    virtual CShellLauncher* cast_shell_launcher() { return (CShellLauncher*)this; } //--#SM+#--

    void LoadAddons(LPCSTR section); // Виртуалку?

    void StopAllEffects();

    virtual void LoadLights(LPCSTR section, LPCSTR prefix);

    void CheckFlameParticles(LPCSTR section, LPCSTR prefix);

    u32      m_dwHideBulletVisual;
    ICF void UpdBulletHideTimer() { m_dwHideBulletVisual = Device.dwTimeGlobal + 150; }

    u32      m_dwShowShellVisual;
    ICF void UpdShellShowTimer() { m_dwShowShellVisual = Device.dwTimeGlobal + 400; }

    /********************************/
    //bool bSynchronizeAddons;

    // UpdateGrenadeVisibility(!!iAmmoElapsed || S == eReload)
    // TODO: При обновлении стэйта <!>

    typedef std::pair<shared_str, shared_str> AmmoBeltData; // hud, vis
    xr_vector<AmmoBeltData>                   m_AmmoBeltData;

    enum EAddons
    {
        eScope    = 0,
        eMuzzle   = 1,
        eLauncher = 2,
        eMagaz    = 3,
        eSpec_1   = 4,
        eSpec_2   = 5,
        eSpec_3   = 6,
        eSpec_4   = 7,
        eSIZE     = 8,
        eNotExist = 9,
    };

    SAddonData m_addons[eSIZE];

    EAddons GetAddonSlot(IGameObject* pObj, u8* idx = NULL);
    EAddons GetAddonSlot(LPCSTR section, u8* idx = NULL);

    SAddonData* GetAddonBySlot(EAddons slotID)
    {
        R_ASSERT(slotID >= 0 && slotID < eSIZE);
        SAddonData* pTest = &(m_addons[slotID]);
        return (SAddonData*)(pTest);
    }
    /********************************/

    bool IsAddonsEqual(CWeapon* pWpn2Cmp);

    // TODO: новые функции сделай виртуалками, если нужно
    virtual void LoadRecoilParams(LPCSTR section);
    virtual void LoadZoomParams(LPCSTR section);

    void UpdateSecondVP(); //

    /////////////////////////////////////////////////////
    void switchFrom_Fire(u32 newS);
    bool Try2Fire(bool bCheckOnlyMode = false); // Клиент
    void Need2Stop_Fire();                      // Клиент

    virtual bool OnBeforeStateSwitch(u32 oldS, u32 newS);

    void         Need2Idle();
    void         switchFrom_Idle(u32 newS);
    virtual void state_Idle(float dt); // TODO: Разберись зачем тебе виртуальные

    bool         Try2Reload(bool bCheckOnlyMode = false);
    bool         Try2ReloadAmmoBelt();
    void         Need2Stop_Reload();
    void         switchFrom_Reload(u32 newS);
    virtual void state_Reload(float dt);

    bool         Try2ReloadFrAB(bool bCheckOnlyMode = false);
    void         switch2_ReloadFrAB();
    void         switchFrom_ReloadFrAB(u32 newS);
    virtual void state_ReloadFrAB(float dt);

    bool         Try2SwitchMag(bool bCheckOnlyMode = false, bool bFromDetach = false);
    void         Need2Stop_SwitchMag();
    void         switchFrom_SwitchMag(u32 newS);
    virtual void switch2_SwitchMag();
    virtual void state_SwitchMag(float dt);
    virtual void Need2SwitchMag();

    IC bool IsReloading() { return GetState() == eReload || GetState() == eReloadFrAB || GetState() == eSwitchMag; }

    bool         Try2Switch(bool bCheckOnlyMode = false);
    void         Need2Stop_Switch();
    void         switchFrom_Switch(u32 newS);
    virtual void switch2_Switch();
    virtual void state_Switch(float dt);

    bool         Try2Kick(bool bCheckOnlyMode = false);
    void         Need2Stop_Kick();
    void         switchFrom_Kick(u32 newS);
    virtual void switch2_Kick();
    virtual void state_Kick(float dt);

    bool         Try2Pump(bool bCheckOnlyMode = false);
    void         Need2Stop_Pump();
    void         switchFrom_Pump(u32 newS);
    virtual void switch2_Pump();
    virtual void state_Pump(float dt);

    bool         Try2Bore(bool bCheckOnlyMode = false);
    void         Need2Stop_Bore();
    void         switchFrom_Bore(u32 newS); // TODO: В виртуалки наверно
    virtual void switch2_Bore();
    virtual void state_Bore(float dt);

    void         switchFrom_Hiding(u32 newS);
    virtual void switch2_Hiding();
    virtual void state_Hiding(float dt);

    void         switchFrom_Showing(u32 newS);
    virtual void switch2_Showing();
    virtual void state_Showing(float dt);

    void         switchFrom_Hidden(u32 newS);
    virtual void switch2_Hidden();
    virtual void state_Hidden(float dt);

    void Need2Empty();
    void Need2Misfire();
    //	virtual void	switch2_MagEmpty	();
    virtual void switch2_Misfire();

    virtual void switch2_Idle();
    virtual void switch2_Fire();
    virtual void switch2_Empty();
    virtual void switch2_Reload();

    void Need2Reload();

    virtual bool Try2AutoReload();

    virtual bool Try2StopTriStateReload();

    virtual void state_Fire(float dt);
    virtual void state_Empty(float dt);
    virtual void state_Misfire(float dt);

    void switchFrom_Misfire(u32 newS);
    void switchFrom_Empty(u32 newS);

    // TODO: Виртуалки???
    void UpdateStates(float dt); // TODO: В худ??? Для артефактов и еды
    /////////////////////////////////////////////////////

    virtual bool IsMovementEffectorAllowed(); //--#SM+#--
    bool         m_bDisableMovEffAtZoom;

    CWeapon(); //--#SM+#--
    virtual ~CWeapon();

    virtual void DropShell(Fvector* pVel = NULL);

    float m_fLR_MovingFactor; // !!!!

    bool m_bKnifeMode;
    bool m_bAllowAutoReload;
    bool m_bUsePumpMode;

    CWeaponKnifeHit* m_first_attack;  // Первая атака ножа --#SM+#--
    CWeaponKnifeHit* m_second_attack; // Вторая атака ножа --#SM+#--

    struct bipods_data
    {
        friend CWeapon;

        enum bipods_state
        {
            eBS_SwitchedOFF = 0, // Выключены
            eBS_TranslateInto,   // Установка
            eBS_SwitchedON,      // Включены
            eBS_TranslateOutro   // Выключение
        };
        bipods_state m_iBipodState;

        bool       m_bInstalled;
        EAddons    m_BipodsSlot;
        shared_str m_sBipod_hud;
        shared_str m_sBipod_vis;

        bool     m_bAnimated;        // Флаг наличия двигающихся костей у сошек
        float    m_i_range;          // Макс. расстояние установки сошек от центра камеры
        float    m_i_angle_f;        // Мин. необходимая величина наклона поверхности (1 - идеально ровная, 0 - стена, -1 - потолок)
        float    m_cam_dist;         // Макс. дистанция от точки установки до камеры
        float    m_cam_y_offset;     // Смещение координаты Y у камеры от точки установки
        float    m_hud_z_offset;     // Смещение худа по Z-координате
        float    m_pitch_vis_factor; // Фактор влияния наклона по X на степень раздвига ножек (больше => меньше)
        float    m_inertia_power;    // Модификатор силы инерции худа
        float    m_pos_displ;        // Сдвиг точки установки m_vBipodInitPos в направлении m_vBipodInitDir
        float    m_deploy_time;      // Время на установку сошек (секунды)
        float    m_undeploy_time;    // Время на снятие сошек (секунды)
        float    m_fZoomFOV;         // FOV при зуме в сошках (без прицела)
        float    m_fHUD_FOV;         // HUD FOV при разложенных сошках
        float    m_fDispersionMod;   // Модификатор разброса пуль при разложенных сошках
        float    m_fRecoilMod;       // Модификатор отдачи при разложенных сошках
        float    m_max_dist;         // Максимальное расстояние, на которое игрок может отойти от установленных сошек
        Fvector2 m_yaw_limit;        // Лимиты вращения сошек по Y
        Fvector2 m_pitch_limit;      // Лимиты вращения сошек по X
        Fvector  m_torch_offset;     // Координаты смещения света фонарика от центра худа
        Fvector  m_deploy_pos;       // Смещение главной кости при установке сошек
        Fvector  m_deploy_rot;       // Поворот главной кости при установке сошек
        Fvector  m_legs_rot;         // Базовый поворот ножек при установке сошек

        Fvector  m_vParentInitPos;
        Fvector  m_vBipodInitPos;
        Fvector  m_vBipodInitDir;
        Fvector  m_vBipodInitNormal;
        Fvector2 m_vPrevYP;
        bool     m_bUseZoomFov;
        float    m_fCurScopeZoomFov;
        float    m_translation_factor; // 0.f - 1.f

        bipods_data()
            : m_bInstalled(false), m_bUseZoomFov(false), m_iBipodState(eBS_SwitchedOFF), m_translation_factor(0.0f), m_fCurScopeZoomFov(0.0f)
        {
            m_vPrevYP.set(0.f, 0.f);
        }

    private:
        bool m_bFirstCamUpdate;

        // Параметры камеры до входа в режим сошек
        Fvector  m_vOldCamPos;
        Fvector2 m_vOldCamYP;
        Fvector2 m_fOldYawLimit;
        Fvector2 m_fOldPitchLimit;
        bool     m_bOldClampYaw;
        bool     m_bOldClampPitch;

        // Каллбэки на установку\снятие аддона
        void OnBipodsAttach(EAddons iSlot, const shared_str& sAddonDataName);
        void OnBipodsDetach(const shared_str& sAddonDataName);
    };
    bipods_data m_bipods;

    virtual void OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch);

    virtual bool UpdateCameraFromHUD(IGameObject* pCameraOwner, Fvector noise_dangle);
    virtual void UpdateActorTorchLightPosFromHUD(Fvector* pTorchPos);

    virtual bool UpdateCameraFromBipods(IGameObject* pCameraOwner, Fvector noise_dangle);
    virtual void UpdateActorTorchLightPosFromBipods(Fvector* pTorchPos);

    virtual void Try2DeployBipods();
    virtual void Need2UndeployBipods(bool bInstantly = true);
    virtual bool IsBipodsDeployed();
    IC bool      IsBipodsAttached() { return m_bipods.m_bInstalled; }

    virtual void DeployBipods(Fvector vDeployPos, Fvector vDeployDir, Fvector vDeployNormal, bool bInstantly = false);
    virtual void UndeployBipods(bool bInstantly = false);
    virtual void UpdateBipodsParams();
    virtual void UpdateBipods();
    virtual void OnOwnedCameraMoveWhileBipods(CCameraBase* pCam, float fOldYaw, float fOldPitch);
    virtual void BipodsSetCameraLimits(CCameraBase* pCam, bool bLimit);
    virtual void BipodsZoom(u32 flags);

    // Magazine 3P
    bool         m_bMagaz3pHideWhileReload;
    bool         m_bMagaz3pIsHidden;
    u16          m_iMagaz3pHideStartTime;
    u16          m_iMagaz3pHideEndTime;
    virtual void UpdateMagazine3p(bool bForceUpdate = false);

    // Generic
    virtual void Load(LPCSTR section);
    virtual void PostLoad(LPCSTR section); //--#SM+#--

    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual void net_Export(NET_Packet& P);
    virtual void net_Import(NET_Packet& P);

    virtual CWeapon*          cast_weapon() { return this; }
    virtual CWeaponMagazined* cast_weapon_magazined() { return (CWeaponMagazined*)this; }
    //	virtual CWeaponMagazinedWGrenade*	cast_weapon_magazined_wgl	()					{return (CWeaponMagazinedWGrenade*)this;}

    //serialization
    virtual void save(NET_Packet& output_packet);
    virtual void load(IReader& input_packet);
    virtual BOOL net_SaveRelevant() { return inherited::net_SaveRelevant(); }

    virtual void UpdateCL();
    virtual void shedule_Update(u32 dt);

    virtual void renderable_Render();
    virtual void render_hud_mode();
    virtual bool need_renderable();

    virtual void render_item_ui();
    virtual bool render_item_ui_query();

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

    virtual void SwitchState(u32 S);

    virtual void OnActiveItem();
    virtual void OnHiddenItem();
    virtual void SendHiddenItem(); //same as OnHiddenItem but for client... (sends message to a server)...

    virtual bool OnBeforeMotionPlayed(const shared_str& sMotionName);

    bool PlaySoundMotion(const shared_str& M, BOOL bMixIn, LPCSTR alias, bool bAssert = false, int anim_idx = -1);
    bool IsWGLAnimRequired();

    bool PlayWorldMotion(const shared_str& M, BOOL bMixIn);

    virtual void ReloadAllSounds(); // --#SM+#-- Перезагрузить все звуки оружия, с учётом текущих аддонов
    void         ReloadSound(       // --#SM+#-- Перезагрузить конкретный звук, с учётом текущих аддонов
                shared_str const& strName,
                shared_str const& strAlias,
                bool              exclusive = false,
                int               type      = sg_SourceType);

public:
    virtual bool                  can_kill() const;
    virtual CInventoryItem*       can_kill(CInventory* inventory) const;
    virtual const CInventoryItem* can_kill(const xr_vector<const CGameObject*>& items) const;
    virtual bool                  ready_to_kill() const;
    virtual bool                  NeedToDestroyObject() const;
    virtual ALife::_TIME_ID       TimePassedAfterIndependant() const;

protected:
    //время удаления оружия
    ALife::_TIME_ID m_dwWeaponRemoveTime;
    ALife::_TIME_ID m_dwWeaponIndependencyTime;

    virtual bool IsHudModeNow();

public:
    void         signal_HideComplete();
    virtual bool Action(u16 cmd, u32 flags);

#ifdef DEBUG
    virtual void OnRender();
#endif

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
        eKick, //--#SM+#--
        ePump,
    };
    enum EWeaponSubStates
    {
        // Tri-State Reload
        eSubstateReloadBegin = 0,
        eSubstateReloadInProcess,
        eSubstateReloadEnd,
        // Magazines
        eSubstateMagazSwitch,    //--> Смена магазина
        eSubstateMagazDetach,    //--> Снятие магазина (старт действия)
        eSubstateMagazDetach_Do, //--> Снятие магазина (сам процесс снятия)
        eSubstateMagazMisfire,   //--> Зафиксить осечку (не менять магазин)
        eSubstateMagazFinish,    //--> Завершение (магазин уже установлен\снят)
    };
    enum
    {
        undefined_ammo_type = u8(-1)
    };

    //IC BOOL					IsValid				()	const		{	return iAmmoElapsed;	}
    // Does weapon need's update?
    BOOL IsUpdating();

    //bool					Try2FireGL			();

    virtual void OnRocketLaunch(u16 rocket_id);

    BOOL IsMisfire() const;
    BOOL CheckForMisfire();

    BOOL             AutoSpawnAmmo() const { return m_bAutoSpawnAmmo; };
    bool             IsTriStateReload() const;
    EWeaponSubStates GetReloadState() const { return (EWeaponSubStates)m_sub_state; }

    IC bool TriStateReloadAnimHack() const { return m_bTriStateReload_anim_hack; }

protected:
    bool m_bTriStateReload_main;
    bool m_bTriStateReload_gl;
    bool m_bTriStateReload_ab;
    bool m_bTriStateReload_frab;

    bool m_bTriStateReload_anim_hack;

    u8 m_sub_state;
    // a misfire happens, you'll need to rearm weapon
    bool bMisfire;

    BOOL         m_bAutoSpawnAmmo;
    virtual bool AllowBore();

public:
    bool IsGrenadeLauncherAttached() const;
    bool IsScopeAttached() const;
    bool IsSilencerAttached() const;
    bool IsMagazineAttached() const;   //--#SM+#--
    bool IsSpecial_1_Attached() const; //--#SM+#--
    bool IsSpecial_2_Attached() const; //--#SM+#--
    bool IsSpecial_3_Attached() const; //--#SM+#--
    bool IsSpecial_4_Attached() const; //--#SM+#--

    bool GrenadeLauncherAttachable() const;
    bool ScopeAttachable() const;
    bool SilencerAttachable() const;
    bool MagazineAttachable() const;   //--#SM+#--
    bool Special_1_Attachable() const; //--#SM+#--
    bool Special_2_Attachable() const; //--#SM+#--
    bool Special_3_Attachable() const; //--#SM+#--
    bool Special_4_Attachable() const; //--#SM+#--

    ALife::EWeaponAddonStatus get_GrenadeLauncherStatus() const { return C_THIS->GetAddonBySlot(eLauncher)->m_attach_status; }
    ALife::EWeaponAddonStatus get_ScopeStatus() const { return C_THIS->GetAddonBySlot(eScope)->m_attach_status; }
    ALife::EWeaponAddonStatus get_SilencerStatus() const { return C_THIS->GetAddonBySlot(eMuzzle)->m_attach_status; }
    ALife::EWeaponAddonStatus get_MagazineStatus() const { return C_THIS->GetAddonBySlot(eMagaz)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_1_Status() const { return C_THIS->GetAddonBySlot(eSpec_1)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_2_Status() const { return C_THIS->GetAddonBySlot(eSpec_2)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_3_Status() const { return C_THIS->GetAddonBySlot(eSpec_3)->m_attach_status; }
    ALife::EWeaponAddonStatus get_Special_4_Status() const { return C_THIS->GetAddonBySlot(eSpec_4)->m_attach_status; }

    bool m_bUIShowAmmo;
    bool m_bDisableFire;

    bool         m_bAllowUnload;
    virtual bool IsMagazineCanBeUnload(bool bForGL = false);

    virtual void PlaySoundZoomIn();
    virtual void PlaySoundZoomOut();

    virtual bool UseScopeTexture();

    void UpdateAddons(); //--#SM+#--
    void UpdateAddonsAnim();

    //обновление видимости для косточек аддонов
    void UpdateAddonsVisibility();
    void _UpdateAddonsVisibility(SAddonData* m_pAddon); //--#SM+#--
    void UpdateHUDAddonsVisibility();
    void _UpdateHUDAddonVisibility(SAddonData* m_pAddon); //--#SM+#--
    u32  m_dwAddons_last_upd_time;                        //--#SM+#-- Когда последний раз вызывали апдейт аддонов()

    //инициализация свойств присоединенных аддонов
    virtual void InitAddons();

    //для отоброажения иконок апгрейдов в интерфейсе
    //TODO: const int
    int GetScopeX() { return pSettings->r_s32(GetAddonBySlot(eScope)->GetName(), "scope_x"); }
    int GetScopeY() { return pSettings->r_s32(GetAddonBySlot(eScope)->GetName(), "scope_y"); }
    int GetSilencerX() { return pSettings->r_s32(GetAddonBySlot(eMuzzle)->GetName(), "muzzle_x"); }            //--#SM+#--
    int GetSilencerY() { return pSettings->r_s32(GetAddonBySlot(eMuzzle)->GetName(), "muzzle_y"); }            //--#SM+#--
    int GetGrenadeLauncherX() { return pSettings->r_s32(GetAddonBySlot(eLauncher)->GetName(), "launcher_x"); } //--#SM+#--
    int GetGrenadeLauncherY() { return pSettings->r_s32(GetAddonBySlot(eLauncher)->GetName(), "launcher_y"); } //--#SM+#--
    int GetMagazineX() { return pSettings->r_s32(GetAddonBySlot(eMagaz)->GetName(), "magazine_x"); }           //--#SM+#--
    int GetMagazineY() { return pSettings->r_s32(GetAddonBySlot(eMagaz)->GetName(), "magazine_y"); }           //--#SM+#--
    int GetSpecial_1_X() { return pSettings->r_s32(GetAddonBySlot(eSpec_1)->GetName(), "special_x"); }         //--#SM+#--
    int GetSpecial_1_Y() { return pSettings->r_s32(GetAddonBySlot(eSpec_1)->GetName(), "special_y"); }         //--#SM+#--
    int GetSpecial_2_X() { return pSettings->r_s32(GetAddonBySlot(eSpec_2)->GetName(), "special_x"); }         //--#SM+#--
    int GetSpecial_2_Y() { return pSettings->r_s32(GetAddonBySlot(eSpec_2)->GetName(), "special_y"); }         //--#SM+#--
    int GetSpecial_3_X() { return pSettings->r_s32(GetAddonBySlot(eSpec_3)->GetName(), "special_x"); }         //--#SM+#--
    int GetSpecial_3_Y() { return pSettings->r_s32(GetAddonBySlot(eSpec_3)->GetName(), "special_y"); }         //--#SM+#--
    int GetSpecial_4_X() { return pSettings->r_s32(GetAddonBySlot(eSpec_4)->GetName(), "special_x"); }         //--#SM+#--
    int GetSpecial_4_Y() { return pSettings->r_s32(GetAddonBySlot(eSpec_4)->GetName(), "special_y"); }         //--#SM+#--

    // TODO: переименовать подствол
    const shared_str GetGrenadeLauncherName() const { return C_THIS->GetAddonBySlot(eLauncher)->GetAddonName(); } //--#SM+#--
    const shared_str GetScopeName() const { return C_THIS->GetAddonBySlot(eScope)->GetAddonName(); }
    const shared_str GetSilencerName() const { return C_THIS->GetAddonBySlot(eMuzzle)->GetAddonName(); }   //--#SM+#--
    const shared_str GetMagazineName() const { return C_THIS->GetAddonBySlot(eMagaz)->GetAddonName(); }    //--#SM+#--
    const shared_str GetSpecial_1_Name() const { return C_THIS->GetAddonBySlot(eSpec_1)->GetAddonName(); } //--#SM+#--
    const shared_str GetSpecial_2_Name() const { return C_THIS->GetAddonBySlot(eSpec_2)->GetAddonName(); } //--#SM+#--
    const shared_str GetSpecial_3_Name() const { return C_THIS->GetAddonBySlot(eSpec_3)->GetAddonName(); } //--#SM+#--
    const shared_str GetSpecial_4_Name() const { return C_THIS->GetAddonBySlot(eSpec_4)->GetAddonName(); } //--#SM+#--

    const shared_str GetGrenadeLauncherSetSect() const
    {
        if (IsGrenadeLauncherAttached())
            return C_THIS->GetAddonBySlot(eLauncher)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetScopeSetSect() const
    {
        if (IsScopeAttached())
            return C_THIS->GetAddonBySlot(eScope)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetSilencerSetSect() const
    {
        if (IsSilencerAttached())
            return C_THIS->GetAddonBySlot(eMuzzle)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetMagazineSetSect() const
    {
        if (IsMagazineAttached())
            return C_THIS->GetAddonBySlot(eMagaz)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetSpecial_1_SetSect() const
    {
        if (IsSpecial_1_Attached())
            return C_THIS->GetAddonBySlot(eSpec_1)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetSpecial_2_SetSect() const
    {
        if (IsSpecial_2_Attached())
            return C_THIS->GetAddonBySlot(eSpec_2)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetSpecial_3_SetSect() const
    {
        if (IsSpecial_3_Attached())
            return C_THIS->GetAddonBySlot(eSpec_3)->GetName();
        else
            return NULL;
    } //--#SM+#--
    const shared_str GetSpecial_4_SetSect() const
    {
        if (IsSpecial_4_Attached())
            return C_THIS->GetAddonBySlot(eSpec_4)->GetName();
        else
            return NULL;
    } //--#SM+#--

    IC void ForceUpdateAmmo() { m_BriefInfo_CalcFrame = 0; }

    // TODO:
    u8   GetAddonsState() const;
    void SetAddonsState(u8 m_flagsAddOnState);

    // Не производит удаление или возврат предмета <!>
    SAddonData* InstallAddon(EAddons iSlot, u8 addon_idx, bool bNoUpdate = false);
    SAddonData* UnistallAddon(EAddons iSlot, bool bNoUpdate = false);

    void OnAddonInstall(EAddons iSlot, const shared_str& sAddonDataName);
    void OnAddonUnistall(EAddons iSlot, const shared_str& sAddonDataName);

protected:
    /*
	//состояние подключенных аддонов
	u8 m_flagsAddOnState;

	//возможность подключения различных аддонов
	ALife::EWeaponAddonStatus	m_eScopeStatus;
	ALife::EWeaponAddonStatus	m_eSilencerStatus;
	ALife::EWeaponAddonStatus	m_eGrenadeLauncherStatus;
	ALife::EWeaponAddonStatus	m_eMagazineStatus;
	ALife::EWeaponAddonStatus	m_eSpecial_1_Status;
	ALife::EWeaponAddonStatus	m_eSpecial_2_Status;
	ALife::EWeaponAddonStatus	m_eSpecial_3_Status;
	ALife::EWeaponAddonStatus	m_eSpecial_4_Status;
	*/

    //названия секций подключаемых аддонов //--#SM+#--
    /*
	shared_str		m_sScopeName;
	shared_str		m_sSilencerName;
	shared_str		m_sGrenadeLauncherName;
	*/

    //смещение иконов апгрейдов в инвентаре //--#SM+#--
    /*
	int	m_iScopeX, m_iScopeY;
	int	m_iSilencerX, m_iSilencerY;
	int	m_iGrenadeLauncherX, m_iGrenadeLauncherY;
	*/

public:
    enum EZoomTypes
    {
        eZoomMain = 0,
        eZoomAlt,
        eZoomGL,
        eZoomTypesCnt
    };

protected:
    SZoomParams m_zoom_params[EZoomTypes::eZoomTypesCnt]; // main, alt, gl

    EZoomTypes m_iCurZoomType;         // Текущий тип прицеливания
    EZoomTypes m_iPrevZoomType;        // Прошлый тип прицеливания
    bool       m_bZoomEnabled;         // Прицеливание разрешено
    bool       m_bAltZoomEnabled;      // Альтернативное прицеливание разрешено
    bool       m_bIsZoomModeNow;       // Целимся-ли мы сейчас
    bool       m_bHideCrosshairInZoom; // Скрывать перекрестие во время зума
    float      m_fZoomRotationFactor;  // Степень разворота ствола от бедра к прицеливанию [0.f - 1.f]
    float      m_fZoomRotateTime;      // Скорость прицеливания
    float      m_bUseOldZoomFactor;    // Использовать старый механизм увеличения (FOV прицеливания = m_fCurrentZoomFactor * 0.75f)

private:
    bool m_bIdleFromZoomOut;

public:
    IC bool IsZoomEnabled() const { return m_bZoomEnabled; }
    IC bool IsAltZoomEnabled() const { return m_bAltZoomEnabled; }
    IC bool IsZoomed() const { return m_bIsZoomModeNow; };

    IC SZoomParams& GetZoomParams() const { return C_THIS->m_zoom_params[m_iCurZoomType]; }
    IC SZoomParams& GetZoomParams(EZoomTypes iType) const { return C_THIS->m_zoom_params[iType]; }

    IC EZoomTypes GetZoomType() { return m_iCurZoomType; }
    IC EZoomTypes GetPrevZoomType() { return m_iPrevZoomType; }
    virtual void  SwitchZoomType(EZoomTypes iType);
    virtual void  SwitchZoomType(EZoomTypes iCurType, EZoomTypes iPrevType);

    IC float GetAimZoomFactor() const { return GetZoomParams().m_fZoomFovFactor; }
    IC float GetSecondVPZoomFactor() const { return GetZoomParams().m_fSecondVPFovFactor; }
    IC float IsSecondVPZoomPresent() const { return GetSecondVPZoomFactor() > 0.000f; }

    IC float GetZRotatingFactor() const { return m_fZoomRotationFactor; }
    IC bool  IsRotatingToZoom() const { return (m_fZoomRotationFactor < 1.f); }

    virtual void ZoomDynamicMod(bool bIncrement, bool bForceLimit);
    virtual void ZoomInc(bool bForceLimit = false);
    virtual void ZoomDec(bool bForceLimit = false);
    virtual void OnZoomIn(bool bSilent = false);
    virtual void OnZoomOut(bool bSilent = false);

    virtual float GetFov();
    virtual float GetSecondVPFov();
    virtual float GetHudFov();

    CUIWindow* ZoomTexture();
    bool       ZoomHideCrosshair() { return m_bHideCrosshairInZoom || ZoomTexture(); }

    virtual u8 GetCurrentHudOffsetIdx();

    float m_HudFovAddition;

    float m_nearwall_dist_max;
    float m_nearwall_dist_min;
    float m_nearwall_last_hud_fov;
    float m_nearwall_target_hud_fov;
    float m_nearwall_speed_mod;

    virtual float Weight() const;
    virtual u32   Cost() const;

public:
    virtual EHandDependence HandDependence() const { return eHandDependence; }
    bool                    IsSingleHanded() const { return m_bIsSingleHanded; }

public:
    IC LPCSTR strap_bone0() const { return m_strap_bone0; }
    IC LPCSTR strap_bone1() const { return m_strap_bone1; }
    IC void   strapped_mode(bool value) { m_strapped_mode = value; }
    IC bool   strapped_mode() const { return m_strapped_mode; }

protected:
    LPCSTR  m_strap_bone0;
    LPCSTR  m_strap_bone1;
    Fmatrix m_StrapOffset;
    bool    m_strapped_mode;
    bool    m_can_be_strapped;

    Fmatrix m_Offset;
    // 0-используется без участия рук, 1-одна рука, 2-две руки
    EHandDependence eHandDependence;
    bool            m_bIsSingleHanded;

public:
    //загружаемые параметры
    Fvector vLoadedFirePoint;
    Fvector vLoadedFirePoint2;

private:
    firedeps m_current_firedeps;

protected:
    virtual void UpdateFireDependencies_internal();
    virtual void UpdatePosition(const Fmatrix& transform); //.
    virtual void UpdateXForm();
    virtual void UpdateHudAdditonal(Fmatrix&);
    IC void      UpdateFireDependencies()
    {
        if (dwFP_Frame == Device.dwFrame)
            return;
        UpdateFireDependencies_internal();
    };

    virtual void LoadFireParams(LPCSTR section);

public:
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
    virtual void ForceUpdateFireParticles();
    virtual void debug_draw_firedeps();

protected:
    //	virtual bool			MovingAnimAllowedNow	();
    virtual void OnStateSwitch(u32 S, u32 oldState);
    virtual void OnAnimationEnd(u32 state);
    virtual void OnMotionMark(u32 state, const motion_marks&); //--#SM+#--

    //трассирование полета пули
    virtual void  FireTrace(const Fvector& P, const Fvector& D);
    virtual float GetWeaponDeterioration();

    virtual void FireStart();
    virtual void FireEnd();

    ///virtual void			Reload				();
    //	void			StopShooting		();

    // удар прикладом   //--#SM+#--
    CWeaponKnifeHit* m_kicker_main;
    CWeaponKnifeHit* m_kicker_alt;
    bool             m_bKickAtRun;
    u32              m_dw_last_kick_at_run;

    //	virtual void			KickStart			();
    //	virtual void			KickEnd				();
    //	virtual void			switch2_Kick		();

    virtual void KickHit(bool bAltMode = false);
    //	virtual void			KickAtRunUpdate		();

    virtual void PlayAnimKick();
    virtual void PlayAnimKickAlt();
    virtual bool PlayAnimKickOut();
    virtual bool PlayAnimKickOutAlt();

    bool         m_bNeed2Pump;
    virtual void PlayAnimPump();

    bool         Try2Knife(bool bAltAttack);
    virtual void PlayAnimKnifeAttack();

    virtual bool PlayAnimOpenWeapon();
    virtual bool PlayAnimOpenWeaponFrAB();
    virtual void PlayAnimAddOneCartridgeWeapon();
    virtual bool PlayAnimCloseWeapon();
    virtual bool PlayAnimCloseWeaponFrAB();
    virtual bool PlayAnimCloseWeaponFromEmpty();
    virtual bool PlayAnimCloseWeaponFrABFromEmpty();
    virtual bool PlayAnimSwitchAddOneCartridge();

    virtual bool PlayAnimOpenWeaponAB();
    virtual bool PlayAnimAddOneCartridgeWeaponAB();
    virtual bool PlayAnimAddOneCartridgeWeaponFrAB();
    virtual bool PlayAnimCloseWeaponAB();

    int m_overridenAmmoForReloadAnm;

    void switch2_StartReload();
    void switch2_AddCartgidge();
    void switch2_EndReload();

    bool       HaveCartridgeInInventory(u8 cnt);
    virtual u8 AddCartridge(u8 cnt);
    virtual u8 AddCartridgeFrAB(u8 cnt);

    // обработка визуализации выстрела
    virtual void OnShot(bool bIsRocket = false);
    virtual void AddShotEffector();
    virtual void RemoveShotEffector();
    virtual void ClearShotEffector();
    virtual void StopShotEffector();

public:
    IC bool Need2Pump() { return m_bNeed2Pump; }

    IC bool IsKickAtRunActive() { return m_bKickAtRun; }

    float         GetBaseDispersion(float cartridge_k);
    float         GetFireDispersion(bool with_cartridge, bool for_crosshair = false);
    virtual float GetFireDispersion(float cartridge_k, bool for_crosshair = false);

    //параметы оружия в зависимоти от его состояния исправности
    float         GetConditionDispersionFactor() const;
    float         GetConditionMisfireProbability() const;
    virtual float GetConditionToShow() const;

    virtual float GetInertionFactor() { return 1.f - GetZRotatingFactor(); }; //--#SM+#--
    virtual float GetInertionPowerFactor();                                   //--#SM+#--

public:
    CameraRecoil cam_recoil;      // simple mode (walk, run)
    CameraRecoil zoom_cam_recoil; // using zoom =(ironsight or scope)

protected:
    //фактор увеличения дисперсии при максимальной изношености
    //(на сколько процентов увеличится дисперсия)
    float fireDispersionConditionFactor;
    //вероятность осечки при максимальной изношености

    // modified by Peacemaker [17.10.08]
    //	float					misfireProbability;
    //	float					misfireConditionK;
    float misfireStartCondition;         //изношенность, при которой появляется шанс осечки
    float misfireEndCondition;           //изношеность при которой шанс осечки становится константным
    float misfireStartProbability;       //шанс осечки при изношености больше чем misfireStartCondition
    float misfireEndProbability;         //шанс осечки при изношености больше чем misfireEndCondition
    float conditionDecreasePerQueueShot; //увеличение изношености при выстреле очередью
    float conditionDecreasePerShot;      //увеличение изношености при одиночном выстреле

public:
    float GetMisfireStartCondition() const { return misfireStartCondition; };
    float GetMisfireEndCondition() const { return misfireEndCondition; };

protected:
    struct SPDM
    {
        float m_fPDM_disp_base;
        float m_fPDM_disp_vel_factor;
        float m_fPDM_disp_accel_factor;
        float m_fPDM_disp_crouch;
        float m_fPDM_disp_crouch_no_acc;
    };
    SPDM m_pdm;

    float                   m_crosshair_inertion;
    first_bullet_controller m_first_bullet_controller;

protected:
    //для отдачи оружия
    Fvector m_vRecoilDeltaAngle;

    //для сталкеров, чтоб они знали эффективные границы использования
    //оружия
    float m_fMinRadius;
    float m_fMaxRadius;

protected:
    //для второго ствола
    void StartFlameParticles2();
    void StopFlameParticles2();
    void UpdateFlameParticles2();

protected:
    shared_str m_sFlameParticles2;
    //объект партиклов для стрельбы из 2-го ствола
    CParticlesObject* m_pFlameParticles2;

public:
    // Alundaio
    void SetAmmoType(u8 type) { SetAmmoTypeSafeFor(type, m_bGrenadeMode); }
    u8 GetAmmoType() { return (m_bGrenadeMode ? GetGLAmmoType() : GetMainAmmoType()); }
    //-Alundaio

    int GetAmmoCount_forType(shared_str const& ammo_type) const;
    int GetAmmoCount(u8 ammo_type) const;

    IC int GetAmmoMagSize() const { return iMagazineSize; }
    IC int GetAmmoElapsed() const { return /*int(m_magazine.size())*/ iAmmoElapsed; }

    IC int GetMainAmmoElapsed() const { return (m_bGrenadeMode ? iAmmoElapsed2 : iAmmoElapsed); }
    IC u8  GetMainAmmoType() const { return (m_bGrenadeMode ? m_ammoType2 : m_ammoType); }
    IC int GetMainMagSize() const { return (m_bGrenadeMode ? iMagazineSize2 : iMagazineSize); }
    IC xr_vector<shared_str>* GetMainAmmoTypes() { return (m_bGrenadeMode ? &m_ammoTypes2 : &m_ammoTypes); }

    IC int GetGLAmmoElapsed() const { return (m_bGrenadeMode ? iAmmoElapsed : iAmmoElapsed2); }
    IC u8  GetGLAmmoType() const { return (m_bGrenadeMode ? m_ammoType : m_ammoType2); }
    IC int GetGLMagSize() const { return (m_bGrenadeMode ? iMagazineSize : iMagazineSize2); }
    IC xr_vector<shared_str>* GetGLAmmoTypes() { return (m_bGrenadeMode ? &m_ammoTypes : &m_ammoTypes2); }

    int GetSuitableAmmoTotal(bool use_item_to_spawn = false) const;

    void SetAmmoElapsed(int ammo_count);
    void SetAmmoElapsedFor(int ammo_count, bool bForGL);
    void TransferAmmo(CWeapon* pTarget, bool bForGL = false, bool bCopyOnly = true);

    virtual void OnMagazineEmpty();
    void         SpawnAmmo(u32 boxCurr = 0xffffffff, LPCSTR ammoSect = NULL, u32 ParentID = 0xffffffff);
    bool         SwitchAmmoType();

    virtual float Get_PDM_Base() const { return m_pdm.m_fPDM_disp_base; };
    virtual float Get_PDM_Vel_F() const { return m_pdm.m_fPDM_disp_vel_factor; };
    virtual float Get_PDM_Accel_F() const { return m_pdm.m_fPDM_disp_accel_factor; };
    virtual float Get_PDM_Crouch() const { return m_pdm.m_fPDM_disp_crouch; };
    virtual float Get_PDM_Crouch_NA() const { return m_pdm.m_fPDM_disp_crouch_no_acc; };
    virtual float GetCrosshairInertion() const { return m_crosshair_inertion; };
    float         GetFirstBulletDisp() const { return m_first_bullet_controller.get_fire_dispertion(); };

protected:
    int iAmmoElapsed;  // ammo in magazine, currently
    int iMagazineSize; // size (in bullets) of magazine

    void ResizeMagazine(int ammo_count, xr_vector<CCartridge*>& Magazine, CCartridge* pCartridge);

    void SetAmmoTypeSafeFor(u8 ammoType, bool bForGL);

    xr_vector<CCartridge> m_AmmoCartidges;
    xr_vector<CCartridge> m_AmmoCartidges2;

    //для подсчета в GetSuitableAmmoTotal
    mutable int m_iAmmoCurrentTotal;
    mutable u32 m_BriefInfo_CalcFrame; //кадр на котором просчитали кол-во патронов
    bool        m_bAmmoWasSpawned;

    virtual bool IsNecessaryItem(const shared_str& item_sect);

public:
    xr_vector<shared_str> m_ammoTypes;
    /*
	struct SScopes
	{
		shared_str			m_sScopeName;
		int					m_iScopeX;
		int					m_iScopeY;
	};
	DEFINE_VECTOR(SScopes*, ADDONS_VECTOR, ADDONS_VECTOR_IT);
	ADDONS_VECTOR			m_scopes;

	u8						cur_scope;
*/

    /*
	//DEFINE_VECTOR(shared_str, ADDONS_VECTOR, ADDONS_VECTOR_IT);	//--#SM+#--
	ADDONS_VECTOR			m_scopes;
	u8						m_cur_scope;


	ADDONS_VECTOR			m_under_barrels;
	u8						m_cur_launcher;
	ADDONS_VECTOR			m_muzzles;
	u8						m_cur_muzzle;
	ADDONS_VECTOR			m_magazines;
	u8						m_cur_magazine;
	ADDONS_VECTOR			m_specials_1;
	u8						m_cur_special_1;
	ADDONS_VECTOR			m_specials_2;
	u8						m_cur_special_2;
	ADDONS_VECTOR			m_specials_3;
	u8						m_cur_special_3;
	ADDONS_VECTOR			m_specials_4;
	u8						m_cur_special_4;

	*/

    virtual void OnChangeVisual();
    virtual void OnAdditionalVisualModelChange(attachable_visual* pChangedVisual);

    static void ReadMaxBulletBones(IKinematics* pModel);

    //TODO: Баг с несохранением m_cur_scope

    CWeaponAmmo* m_pCurrentAmmo;
    u8           m_ammoType;
    //-	shared_str				m_ammoName; <== deleted
    bool m_bHasTracers;
    u8   m_u8TracerColorID;
    u8   m_set_next_ammoType_on_reload;

    // Multitype ammo support
    xr_vector<CCartridge*> m_magazine;
    //CCartridge				m_DefaultCartridge;
    float m_fCurrentCartirdgeDisp;

    bool    unlimited_ammo();
    IC bool can_be_strapped() const { return m_can_be_strapped; };

protected:
    u32 m_ef_main_weapon_type;
    u32 m_ef_weapon_type;

public:
    virtual u32 ef_main_weapon_type() const;
    virtual u32 ef_weapon_type() const;

    // Alundaio
    virtual void set_ef_main_weapon_type(u32 type) { m_ef_main_weapon_type = type; };
    virtual void set_ef_weapon_type(u32 type) { m_ef_weapon_type = type; };
    //-Alundaio

protected:
    // This is because when scope is attached we can't ask scope for these params
    // therefore we should hold them by ourself :-((
    float m_addon_holder_range_modifier;
    float m_addon_holder_fov_modifier;

public:
    virtual void modify_holder_params(float& range, float& fov) const;
    virtual bool use_crosshair() const { return true; }
    bool         show_crosshair();
    bool         show_indicators();
    virtual BOOL ParentMayHaveAimBullet();
    virtual BOOL ParentIsActor();

private:
    virtual bool install_upgrade_ammo_class(LPCSTR section, bool test);
    bool         install_upgrade_disp(LPCSTR section, bool test);
    bool         install_upgrade_hit(LPCSTR section, bool test);
    bool         install_upgrade_addon(LPCSTR section, bool test);

protected:
    virtual bool install_upgrade_impl(LPCSTR section, bool test);

private:
    float m_hit_probability[egdCount];

public:
    const float& hit_probability() const;

private:
    Fvector      m_overriden_activation_speed;
    bool         m_activation_speed_is_overriden;
    virtual bool ActivationSpeedOverriden(Fvector& dest, bool clear_override);

    bool m_bRememberActorNVisnStatus;

public:
    virtual void SetActivationSpeedOverride(Fvector const& speed);
    bool         GetRememberActorNVisnStatus() { return m_bRememberActorNVisnStatus; };
    virtual void EnableActorNVisnAfterZoom();

    virtual void             DumpActiveParams(shared_str const& section_name, CInifile& dst_ini) const;
    virtual shared_str const GetAnticheatSectionName() const { return cNameSect(); };

    // CWeaponMagazined *********************************************
public:
protected:
    //звук текущего выстрела
    shared_str m_sSndShotCurrent;

    //дополнительная информация о глушителе
    LPCSTR m_sSilencerFlameParticles;
    LPCSTR m_sSilencerSmokeParticles;

    /*
	ESoundTypes		m_eSoundShow;
	ESoundTypes		m_eSoundHide;
	ESoundTypes		m_eSoundClose;	//--#SM+#--
	ESoundTypes		m_eSoundShot;
	ESoundTypes		m_eSoundEmptyClick;
	ESoundTypes		m_eSoundReload;
	ESoundTypes		m_eSoundOpen;
	ESoundTypes		m_eSoundAddCartridge;
	*/

    bool m_sounds_enabled;
    // General
    //кадр момента пересчета UpdateSounds
    u32 dwUpdateSounds_Frame;

protected:
    /*
	virtual void	switch2_Idle	();
	virtual void	switch2_Fire	();
	virtual void	switch2_Empty	();
	virtual void	switch2_Reload	();
	virtual void	switch2_Hiding	();
	virtual void	switch2_Hidden	();
	virtual void	switch2_Showing	();
	*/

    virtual void OnEmptyClick(bool bFromMisfire = false);

    virtual void UpdateSounds();

    //	bool			TryReload		();

protected:
    virtual void ReloadMagazine();
    virtual void ReloadMagazineFrAB();
    virtual void ReloadMagazineWithType(u8 ammoType);
    virtual void ReloadMainMagazineWithType(u8 ammoType);
    virtual void ReloadGLMagazineWithType(u8 ammoType);

    void ApplySilencerKoeffs();
    void ResetSilencerKoeffs();

    //	virtual void	state_Fire		(float dt);
    //	virtual void	state_MagEmpty	(float dt);
    //	virtual void	state_Misfire	(float dt);
public:
    void LoadSilencerKoeffs();

    virtual bool Attach(PIItem pIItem, bool b_send_event);
    virtual bool Detach(const char* item_section_name, bool b_spawn_item);
    //		bool	DetachScope		(const char* item_section_name, bool b_spawn_item);	//--#SM+#--

    virtual bool CanAttach(PIItem pIItem);
    virtual bool CanDetach(const char* item_section_name);

    virtual void OnDetachAddonSpawn(const char* item_section_name, CSE_ALifeDynamicObject* E);

    bool         IsAmmoAvailable();
    virtual void UnloadMagazine(bool spawn_ammo = true);
    virtual void UnloadMagazineMain(bool spawn_ammo = true);
    virtual void UnloadMagazineGL(bool spawn_ammo = true);

    virtual bool GetBriefInfo(II_BriefInfo& info);
    virtual bool DrawConditionBar() { return !IsMagazine(); }

public:
    virtual bool SwitchMode();
    virtual bool SingleShotMode() { return 1 == m_iQueueSize; }
    virtual void SetQueueSize(int size);
    IC int       GetQueueSize() const { return m_iQueueSize; };
    virtual bool StopedAfterQueueFired() { return m_bStopedAfterQueueFired; }
    virtual void StopedAfterQueueFired(bool value) { m_bStopedAfterQueueFired = value; }

protected:
    //максимальный размер очереди, которой можно стрельнуть
    int m_iQueueSize;
    //количество реально выстреляных патронов
    int m_iShotNum;
    //после какого патрона, при непрерывной стрельбе, начинается отдача (сделано из-за Абакана)
    int m_iBaseDispersionedBulletsCount;
    //скорость вылета патронов, на которые не влияет отдача (сделано из-за Абакана)
    float m_fBaseDispersionedBulletsSpeed;
    //скорость вылета остальных патронов
    float   m_fOldBulletSpeed;
    Fvector m_vStartPos, m_vStartDir;
    //флаг того, что мы остановились после того как выстреляли
    //ровно столько патронов, сколько было задано в m_iQueueSize
    bool m_bStopedAfterQueueFired;
    //флаг того, что хотя бы один выстрел мы должны сделать
    //(даже если очень быстро нажали на курок и вызвалось FireEnd)
    bool m_bFireSingleShot;
    //режимы стрельбы
    bool          m_bHasDifferentFireModes;
    xr_vector<s8> m_aFireModes;
    int           m_iCurFireMode;
    int           m_iPrefferedFireMode;

    //переменная блокирует использование
    //только разных типов патронов
    bool m_bLockType;

public:
    void       OnNextFireMode();
    void       OnPrevFireMode();
    bool       HasFireModes() { return m_bHasDifferentFireModes; };
    virtual s8 GetCurrentFireMode()
    {
        if (m_bHasDifferentFireModes)
            return m_aFireModes[m_iCurFireMode];
        return (s8)WEAPON_DEFAULT_QUEUE;
    }

protected:
protected:
    virtual bool AllowFireWhileWorking() { return false; }

    //виртуальные функции для проигрывания анимации HUD
    virtual void PlayAnimShow();
    virtual void PlayAnimHide();
    virtual void PlayAnimReload();
    virtual void PlayAnimReloadAB();
    virtual void PlayAnimReloadFrAB();
    virtual void PlayAnimIdle();
    virtual void PlayAnimIdleOnly();
    virtual void PlayAnimShoot();
    //	virtual void	PlayReloadSound		 ();
    //	virtual void	PlayAnimAim			 ();						//--#SM+#--
    virtual void PlayAnimBore();       //--#SM+#--
    virtual void PlayAnimIdleMoving(); //--#SM+#--
    virtual void PlayAnimIdleSprint(); //--#SM+#--
    virtual void PlayAnimEmptyClick();

    enum EZoomAnimStae
    {
        eZANone,
        eZAIn,
        eZAOut,
    };
    EZoomAnimStae m_ZoomAnimState;
    bool          m_bDisableFireWhileZooming;
    virtual void  PlayAnimZoom(); //--#SM+#--

    virtual bool IsShootingAllowed();

public: //--#SM+#--
    virtual int ShotsFired() { return m_iShotNum; }

    virtual void FireBullet(
        const Fvector& pos, const Fvector& dir, float fire_disp, const CCartridge& cartridge, u16 parent_id, u16 weapon_id, bool send_hit);

    // CWeaponMagazinedWGL ***************************************
public:
    //	bool	m_bNeed2LaunchGrenade;
    void LaunchGrenade();

    //переключение в режим подствольника
    void PerformSwitchGL();

    //виртуальные функции для проигрывания анимации HUD

    virtual void PlayAnimModeSwitch();

private:
    virtual void net_Spawn_install_upgrades(Upgrades_type saved_upgrades);

    int GetAmmoCount2(u8 ammo2_type) const;

public:
    //дополнительные параметры патронов
    //для подствольника
    //-	CWeaponAmmo*			m_pAmmo2;
    xr_vector<shared_str> m_ammoTypes2;
    u8                    m_ammoType2;

    int                    iMagazineSize2;
    xr_vector<CCartridge*> m_magazine2;

    virtual bool SwapBulletFromAB();

    bool m_bGrenadeMode; // SM_TODO: В приват

    bool m_bUseMagazines;
    bool m_bIsMagazine;

    virtual bool InventoryFastReloadAllowed(bool bForGL = false);
    virtual void InventoryFastReload(u8 ammoType, bool bForGL = false);

    IC bool IsMagazine() { return m_bIsMagazine; }

    u8  m_set_next_magaz_on_reload;
    u16 m_set_next_magaz_by_id;

    bool     SwitchMagazineType();
    CWeapon* GetBestMagazine(LPCSTR section = NULL);

    virtual void LoadMainAmmoParams(LPCSTR section, bool bFromLoad = false, bool bDontUnload = false);
    virtual void LoadMagazinesParams(LPCSTR section);

    bool       m_bUseAmmoBeltMode; // Пусть при одевании аддона устанавливается у клиента
    EAddons    m_AmmoBeltSlot;
    shared_str m_sAB_hud;
    shared_str m_sAB_vis;

    IC bool IsAmmoBeltAttached() { return m_AmmoBeltSlot != eNotExist; }
    IC bool IsAmmoBeltReloadNow() { return m_bUseAmmoBeltMode && m_bGrenadeMode; }

    EAddons m_ForegripSlot;
    IC bool IsForegripAttached() { return m_ForegripSlot != eNotExist; }

    EAddons m_ForendSlot;
    IC bool IsForendAttached() { return m_ForendSlot != eNotExist; }

    void UpdateAmmoBelt();

    shared_str m_sBulletVisual;
    void       UpdateBulletVisual();

    void UpdateWpnVisuals();

    shared_str m_sShellVisual;
    shared_str m_sCurrentShellModel;
    void       UpdateShellVisual(); // Временная ф-ия (?)

    //	CCartridge				m_DefaultCartridge2;
    int iAmmoElapsed2;

    shared_str m_sOverridedRocketSection;

    bool IsGrenadeBullet();
    bool SpawnAndLaunchRocket();

    virtual void UpdateGrenadeVisibility();

    virtual bool bIsGrenadeMode() { return m_bGrenadeMode; }; //--#SM+#--

    void UpdateAmmoBeltParams(); //--#SM+#--
    void UpdateGLParams();       //--#SM+#--

    EAddons m_BayonetSlot;
    IC bool IsBayonetAttached() { return m_BayonetSlot != eNotExist; }
    void    UpdateBayonetParams(); //--#SM+#--

    bool m_bNeed2StopTriStateReload;
    bool m_bIsReloadFromAB;
    bool m_bSwitchAddAnimation;

    bool DetectorCheckCompability(bool bAllowAim);
    bool DetectorHideCondition(bool bAllowAim);

    float         m_fScopeInertionFactor;
    virtual float GetControlInertionFactor() const;

    shared_str m_sHolographBone;
    float      m_fHolographRotationFactor;
};
