//////////////////////////////////////////////////////////////////////
// CustomShell.h:	ракета, которой стрел¤ет ShellLauncher
//					(умеет лететь, светитьс¤ и отыгрывать партиклы)
//////////////////////////////////////////////////////////////////////

// Поменять описание в шапке !  //--#SM_TODO+#--

#pragma once

#include "physic_item.h"
#include "xrPhysics/PHUpdateObject.h"

class CShellLauncher;

class CCustomShell : public CPhysicItem, public CPHUpdateObject
{
private:
    typedef CPhysicItem inherited;
    friend CShellLauncher;

public:
    virtual void renderable_Render();

    //////////////////////////////////////////////////////////////////////////
    //	Generic
    //////////////////////////////////////////////////////////////////////////

    CCustomShell(void);
    virtual ~CCustomShell(void);

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void net_Destroy();
    virtual BOOL AlwaysTheCrow() { return TRUE; }

    virtual void reinit();
    virtual void reload(LPCSTR section);

    virtual void OnH_A_Independent();
    virtual void OnH_B_Independent(bool just_before_destroy);
    virtual void OnH_B_Chield();
    virtual void OnH_A_Chield();
    virtual void UpdateCL();

    virtual BOOL UsedAI_Locations() { return (FALSE); }
    virtual bool Useful() const { return (TRUE); } /// ???

    //создание физической оболочки
    virtual void activate_physic_shell();
    virtual void create_physic_shell();

    virtual void setup_physic_shell();

    virtual void PhDataUpdate(float step);
    virtual void PhTune(float step);

    virtual bool IsCollideWithBullets() { return false; }     // Level_bullet_manager_firetrace.cpp
    virtual bool IsCollideWithActorCamera() { return false; } // xrPhysics\ActorCameraCollision.cpp

    //////////////////////////////////////////////////////////////////////////
    //	Shell Properties
    //////////////////////////////////////////////////////////////////////////

    u32  m_dwShellLaunchTime;
    u32  m_dwFOVStableTime;
    u32  m_dwFOVTranslateTime;
    bool bHUD_mode;

    bool bDynCollideDisabled;
    u32  m_dwDynCollideDisabledTime;

    u32 m_launch_point_idx;

public:
    virtual void SetLaunchParams(const Fmatrix& xform, const Fvector& vel, const Fvector& angular_vel);

    virtual void OnRenderHUD(IGameObject* pCurViewEntity); //--#SM+#--

protected:
    //указатель на владельца ShellLauncher - который стрел¤ет ракету
    CGameObject* m_pOwner;

    //параметры которые задаютс¤ ShellLauncher-ом перед пуском
    Fmatrix m_LaunchXForm;
    Fvector m_vLaunchVelocity;
    Fvector m_vLaunchAngularVelocity;

    //////////////////////////////////////////////////////////////////////////
    //	Lights
    //////////////////////////////////////////////////////////////////////////
protected:
    //флаг, что подсветка может быть включена
    bool m_bLightsEnabled;
    //флаг, что подсветка будет остановлена
    //вместе с двигателем
    bool m_bStopLightsWithEngine;
    //подсветка во врем¤ полета и работы двигател¤
    ref_light m_pTrailLight;
    Fcolor    m_TrailLightColor;
    float     m_fTrailLightRange;

    // ћ¤гкое затухание света, как у взрывов
protected:
    virtual void StartLights();
    virtual void StopLights();
    virtual void UpdateLights();

    //////////////////////////////////////////////////////////////////////////
    //	Particles
    //////////////////////////////////////////////////////////////////////////
protected:
    //им¤ партиклов полета
    shared_str        m_sFlyParticles;
    CParticlesObject* m_pFlyParticles;

    Fvector m_vPrevVel;

protected:
    virtual void StartFlyParticles();
    virtual void StopFlyParticles();

    virtual void UpdateParticles();
};