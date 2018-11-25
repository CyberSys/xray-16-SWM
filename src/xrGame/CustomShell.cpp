/**********************************/
/***** Присоединяемые визуалы *****/ //--#SM+#--
/**********************************/

// SM_TODO ^^^

#include "stdafx.h"
#include "CustomShell.h"
#include "ParticlesObject.h"
#include "../xrphysics/PhysicsShell.h"
#include "../xrphysics/extendedgeom.h"
#include "../xrphysics/calculatetriangle.h"
#include "../xrphysics/tri-colliderknoopc/dctriangle.h"

#include "level.h"
#include "xrMessages.h"
#include "../xrEngine/gamemtllib.h"
//#include "tri-colliderknoopc/dTriList.h"
#include "../Include/xrRender/RenderVisual.h"
//#include "CalculateTriangle.h"
#include "actor.h"
#ifdef DEBUG
#include "PHDebug.h"
#include "game_base_space.h"
#endif

#include "Include/xrRender/Kinematics.h"
#include "CharacterPhysicsSupport.h"
#include "HUDManager.h"

// 		alife_object->m_flags.set	(CSE_ALifeObject::flCanSave,FALSE); из Missle.cpp

// Время жизни <!> в секцию

CCustomShell::CCustomShell()
{
    m_bStopLightsWithEngine = true;
    m_bLightsEnabled        = false;

    m_vPrevVel.set(0, 0, 0);

    m_pTrailLight = NULL;
    m_LaunchXForm.identity();
    m_vLaunchVelocity.set(0, 0, 0);
    m_vLaunchAngularVelocity.set(0, 0, 0);

    m_dwShellLaunchTime  = 0;
    m_dwFOVTranslateTime = 0;
    m_dwFOVStableTime    = 0;
    bHUD_mode            = false;

    bDynCollideDisabled        = false;
    m_dwDynCollideDisabledTime = 150; // Заменить на время, когда включается физическая оболочка

    m_launch_point_idx = 0;
}

CCustomShell::~CCustomShell() { m_pTrailLight.destroy(); }

void CCustomShell::renderable_Render()
{
    if (bHUD_mode)
        return;
    inherited::renderable_Render();
}

void CCustomShell::OnRenderHUD(IGameObject* pCurViewEntity)
{
    if (!bHUD_mode)
        return;

    extern ENGINE_API float psHUD_FOV;

    float fFactor = (m_dwFOVTranslateTime < 1 || (Device.dwTimeGlobal <= (m_dwShellLaunchTime + m_dwFOVStableTime)) ?
                         1.f :
                         (1.f - ((float)(Device.dwTimeGlobal - (m_dwShellLaunchTime + m_dwFOVStableTime)) / (float)m_dwFOVTranslateTime)));

    // 1 - только заспавнился, <= 0 - закончился

    float fHUD_FOV_delta = Device.fFOV - (psHUD_FOV * Device.fFOV);
    float fHUD_FOV       = Device.fFOV - (fHUD_FOV_delta * fFactor);

    //	Msg("[%d]: fFactor = %f, fHUD_FOV = %f, Device.dwTimeGlobal = %d, m_dwShellLaunchTime = %d", ID(), fFactor, fHUD_FOV, Device.dwTimeGlobal, m_dwShellLaunchTime);

    Visual()->getVisData().obj_data->m_hud_custom_fov = fHUD_FOV;

    GEnv.Render->set_Transform(&XFORM());
    GEnv.Render->add_Visual(Visual());

    if (fFactor <= 0.0f)
    {
        bHUD_mode                                         = false;
        Visual()->getVisData().obj_data->m_hud_custom_fov = -1.f;
    }
}

void CCustomShell::reinit() // NetSpawn, после reload() тамже
{
    inherited::reinit();

    m_pTrailLight.destroy();
    m_pTrailLight = GEnv.Render->light_create();
    m_pTrailLight->set_shadow(true);

    m_pFlyParticles = NULL;

    m_pOwner = NULL;

    m_vPrevVel.set(0, 0, 0);
}

BOOL CCustomShell::net_Spawn(CSE_Abstract* DC)
{
    BOOL result = inherited::net_Spawn(DC);
    m_LaunchXForm.set(XFORM());

    HUD().RenderHUD_Add(this);

    shared_str sLP_idx = DC->name_replace();
    R_ASSERT(sLP_idx != NULL && sLP_idx != "");

    m_launch_point_idx = atoi(sLP_idx.c_str());

    return result;
}

void CCustomShell::net_Destroy()
{
    //	Msg("---------net_Destroy [%d] frame[%d]",ID(), Device.dwFrame);

    HUD().RenderHUD_Remove(this);

    CPHUpdateObject::Deactivate();
    inherited::net_Destroy();
}

void CCustomShell::SetLaunchParams(const Fmatrix& xform, const Fvector& vel, const Fvector& angular_vel)
{
    VERIFY2(_valid(xform), "SetLaunchParams. Invalid xform argument!");
    m_LaunchXForm     = xform;
    m_vLaunchVelocity = vel;
    //	if(m_pOwner->ID()==Actor()->ID())
    //	{
    //		Msg("set p start v:	%f,%f,%f	\n",m_vLaunchVelocity.x,m_vLaunchVelocity.y,m_vLaunchVelocity.z);
    //	}
    m_vLaunchAngularVelocity = angular_vel;
}

#include "weapon.h" // УДАЛИТЬ

void CCustomShell::activate_physic_shell() // OnH_B_Independent
{
    R_ASSERT(m_pOwner != NULL); // Вообще это нафиг не надо
    R_ASSERT(m_launch_point_idx > 0);

    CWeapon*        pWpn      = m_pOwner->cast_weapon();
    CShellLauncher* pLauncher = pWpn->cast_shell_launcher();

    bHUD_mode           = pWpn->GetHUDmode();
    m_dwShellLaunchTime = Device.dwTimeGlobal;

    // Если за время спавна гильзы у нашего родителя благодаря чёрной магии успеи смениться параметры запуска
    if (pLauncher->m_launch_points.size() < m_launch_point_idx)
    {
        Msg("! m_launch_point_idx changed");
        return; // TODO: Удаляй гильзу как бракованную
    }

    CShellLauncher::_launch_point& point =
        (bHUD_mode ? pLauncher->m_launch_points[m_launch_point_idx - 1].point_hud : pLauncher->m_launch_points[m_launch_point_idx - 1].point_world);

    // Msg("ActivatePS for [%d]", ID());

    Fvector p1, d1, p, d;

    // Если в дирекцию или vCurSpawnVel (?) просунуть одни нули то будут жёсткие лаги.

    /*
	p1.set								(point.vCurSpawnPos);
	d1.set								(point.vCurSpawnDir);
	p = p1;
	d = d1;

	Fmatrix								launch_matrix;
	launch_matrix.identity				();
	launch_matrix.k.set					(d);
	Fvector::generate_orthonormal_basis(launch_matrix.k,
										launch_matrix.j, launch_matrix.i);
	launch_matrix.c.set					(p);
	*/

    Fmatrix launch_matrix;
    launch_matrix.identity();
    launch_matrix = point.GetLaunchMatrix(); //--> Тоже в const заверни

    Fvector l_vel = point.GetLaunchVel();

    Fvector a_vel;
    float   fi, teta, r;
    fi        = ::Random.randF(0.f, 2.f * M_PI);
    teta      = ::Random.randF(0.f, M_PI);
    r         = ::Random.randF(2.f * M_PI, 3.f * M_PI);
    float rxy = r * _sin(teta);
    a_vel.set(rxy * _cos(fi), rxy * _sin(fi), r * _cos(teta));
    a_vel.mul(15.0f);

    // Оружие кто то держит
    IGameObject* pParentRoot = pWpn->H_Root();

    CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(pParentRoot);
    if (entity_alive && entity_alive->character_physics_support())
    {
        Fvector parent_vel;
        entity_alive->character_physics_support()->movement()->GetCharacterVelocity(parent_vel);
        l_vel.add(parent_vel);
    }
    else
    {
        if (pParentRoot != this && pParentRoot->cast_physics_shell_holder() != NULL)
        {
            Fvector vParentLVel;
            pParentRoot->cast_physics_shell_holder()->PHGetLinearVell(vParentLVel);
            l_vel.add(vParentLVel);
        }
    }

    //	l_vel.normalize						();
    //	l_vel.mul							(1.0f);	// launch speed (?) Вообще не нужно, задавай в векторе

    //	CRocketLauncher::LaunchRocket		(launch_matrix, d, zero_vel);

    ////////////////////////////////////////

    R_ASSERT(H_Parent());
    R_ASSERT(!m_pPhysicsShell);
    create_physic_shell();

    R_ASSERT(m_pPhysicsShell);
    if (m_pPhysicsShell->isActive())
        return;
    VERIFY2(_valid(launch_matrix), "CCustomRocket::activate_physic_shell. Invalid m_LaunchXForm!");

    //	if(m_pOwner->ID()==Actor()->ID())
    //	{
    //		Msg("start v:	%f,%f,%f	\n",m_vLaunchVelocity.x,m_vLaunchVelocity.y,m_vLaunchVelocity.z);
    //	}
    m_pPhysicsShell->Activate(launch_matrix, l_vel, a_vel);
    m_pPhysicsShell->Update();

    XFORM().set(m_pPhysicsShell->mXFORM);
    Position().set(m_pPhysicsShell->mXFORM.c);
    m_pPhysicsShell->set_PhysicsRefObject(this);
    //	m_pPhysicsShell->set_ObjectContactCallback(ObjectContactCallback);
    m_pPhysicsShell->set_ContactCallback(NULL);
    m_pPhysicsShell->SetAirResistance(0.f, 0.f);
    m_pPhysicsShell->set_DynamicScales(1.f, 1.f);
    m_pPhysicsShell->SetAllGeomTraced();

    m_pPhysicsShell->DisableCharacterCollision();

    //m_pPhysicsShell->SetIgnoreDynamic(); // Временно для исключения попаданий от пуль
    bDynCollideDisabled = true;
}

void CCustomShell::create_physic_shell() // Из setup_physic_shell или activate_physic_shell
{
    inherited::create_physic_shell();
}

void CCustomShell::setup_physic_shell() // NetSpawn
{
    R_ASSERT(!m_pPhysicsShell);
    create_physic_shell();
    m_pPhysicsShell->Activate(XFORM(), 0, XFORM()); //,true
    IKinematics* kinematics = smart_cast<IKinematics*>(Visual());
    R_ASSERT(kinematics);
    kinematics->CalculateBones_Invalidate();
    kinematics->CalculateBones(TRUE);
}

//////////////////////////////////////////////////////////////////////////
// Shell specific functions
//////////////////////////////////////////////////////////////////////////

void CCustomShell::Load(LPCSTR section)
{
    inherited::Load(section);

    reload(section);
}

void CCustomShell::reload(LPCSTR section)
{
    inherited::reload(section);

    m_bLightsEnabled = !!pSettings->r_bool(section, "lights_enabled");
    if (m_bLightsEnabled)
    {
        sscanf(pSettings->r_string(section, "trail_light_color"), "%f,%f,%f", &m_TrailLightColor.r, &m_TrailLightColor.g, &m_TrailLightColor.b);
        m_fTrailLightRange = pSettings->r_float(section, "trail_light_range");
    }

    if (pSettings->line_exist(section, "fly_particles"))
        m_sFlyParticles = pSettings->r_string(section, "fly_particles");
}

void CCustomShell::OnH_B_Chield()
{
    inherited::OnH_B_Chield();
    //	Msg("! CCustomShell::OnH_B_Chield called, id[%d] frame[%d]",ID(),Device.dwFrame);
}
void CCustomShell::OnH_A_Chield()
{
    inherited::OnH_A_Chield();
    // Msg("! CCustomShell::OnH_A_Chield called, id[%d] pid[%d] frame[%d]",ID(),H_Parent()->ID(),Device.dwFrame);

    // THROW
    m_pOwner = H_Parent()->cast_game_object();

    CShellLauncher* pLauncher = m_pOwner->cast_shell_launcher();
    R_ASSERT(pLauncher);

    CShellLauncher::launch_points& points = pLauncher->m_launch_points[m_launch_point_idx - 1];

    m_dwFOVTranslateTime = points.dwFOVTranslateTime;
    m_dwFOVStableTime    = points.dwFOVStableTime;

    H_SetParent(NULL);
    // 	missile->set_destroy_time(m_dwDestroyTimeMax);
}

void CCustomShell::OnH_B_Independent(bool just_before_destroy)
{
    inherited::OnH_B_Independent(just_before_destroy);
    //-------------------------------------------
    //	m_pOwner = H_Parent() ? smart_cast<CGameObject*>(H_Parent()->H_Root()) : NULL; // Функцию для получения владельца оружия, если нужна
    //-------------------------------------------

    // Msg("! CCustomShell::OnH_B_Independent called, id[%d] pid[%d] frame[%d]", ID(), (m_pOwner != NULL ? m_pOwner->ID() : 0), Device.dwFrame);
}

void CCustomShell::OnH_A_Independent()
{
    inherited::OnH_A_Independent();

    if (!g_pGameLevel->bReady)
        return;
    setVisible(true);
    // Msg("! CCustomShell::OnH_A_Independent called, id[%d] frame[%d]",ID(),Device.dwFrame);

    // !!!
    //StartFlyParticles();
    //StartLights();

    /////////////////////
    CPhysicsShellHolder& obj = *this;
    VERIFY(obj.Visual());
    IKinematics* K = obj.Visual()->dcast_PKinematics();
    VERIFY(K);
    if (!obj.PPhysicsShell())
    {
        Msg("! ERROR: PhysicsShell is NULL, object [%s][%d]", obj.cName().c_str(), obj.ID());
        return;
    }
    if (!obj.PPhysicsShell()->isFullActive())
    {
        K->CalculateBones_Invalidate();
        K->CalculateBones(TRUE);
    }
    obj.PPhysicsShell()->GetGlobalTransformDynamic(&obj.XFORM());
    K->CalculateBones_Invalidate();
    K->CalculateBones(TRUE);
    obj.spatial_move();
}

#include "xrPhysics\\PHCollideValidator.h"

void CCustomShell::UpdateCL()
{
    inherited::UpdateCL();

    UpdateLights();
    UpdateParticles();

    if (bDynCollideDisabled && m_pPhysicsShell != NULL && (Device.dwTimeGlobal > (m_dwShellLaunchTime + m_dwDynCollideDisabledTime)))
    {
        //_flags<CLClassBits>& bits = const_cast<_flags<CLClassBits>&>(m_pPhysicsShell->collide_class_bits());
        //bits.set((u32)(1 << 3), FALSE);

        // m_pPhysicsShell->DisableCharacterCollision();	// Нам не нужна коллизия с НПС

        //Msg("~TEST!!!!!!!!!!!!!!!");

        bDynCollideDisabled = false;
    }
}

//////////////////////////////////////////////////////////////////////////
//	Lights
//////////////////////////////////////////////////////////////////////////
void CCustomShell::StartLights()
{
    if (!m_bLightsEnabled)
        return;

    //включить световую подсветку от двигателя
    m_pTrailLight->set_color(m_TrailLightColor.r, m_TrailLightColor.g, m_TrailLightColor.b);

    m_pTrailLight->set_range(m_fTrailLightRange);
    m_pTrailLight->set_position(Position());
    m_pTrailLight->set_active(true);
}

void CCustomShell::StopLights()
{
    if (!m_bLightsEnabled)
        return;
    m_pTrailLight->set_active(false);
}

void CCustomShell::UpdateLights()
{
    if (!m_bLightsEnabled || !m_pTrailLight->get_active())
        return;
    m_pTrailLight->set_position(Position());
}

void CCustomShell::PhDataUpdate(float step) {}
void CCustomShell::PhTune(float step) {}

//////////////////////////////////////////////////////////////////////////
//	Particles
//////////////////////////////////////////////////////////////////////////

void CCustomShell::UpdateParticles()
{
    if (!m_pFlyParticles)
        return;

    Fvector vel;
    PHGetLinearVell(vel);

    vel.add(m_vPrevVel, vel);
    vel.mul(0.5f);
    m_vPrevVel.set(vel);

    Fmatrix particles_xform;
    particles_xform.identity();
    particles_xform.k.set(XFORM().k);
    particles_xform.k.mul(-1.f);
    Fvector dir = particles_xform.k;
    Fvector::generate_orthonormal_basis(particles_xform.k, particles_xform.j, particles_xform.i);
    particles_xform.c.set(XFORM().c);
    dir.normalize_safe(); //1m offset fake -(
    particles_xform.c.add(dir);

    if (m_pFlyParticles)
        m_pFlyParticles->UpdateParent(particles_xform, vel);
}

void CCustomShell::StartFlyParticles()
{
    VERIFY(m_pFlyParticles == NULL);

    if (!m_sFlyParticles)
        return;
    m_pFlyParticles = CParticlesObject::Create(*m_sFlyParticles, FALSE);

    UpdateParticles();
    m_pFlyParticles->Play(false);

    VERIFY(m_pFlyParticles);
    VERIFY3(m_pFlyParticles->IsLooped(), "must be a looped particle system for shell fly: %s", *m_sFlyParticles);
}
void CCustomShell::StopFlyParticles()
{
    if (m_pFlyParticles == NULL)
        return;
    m_pFlyParticles->Stop();
    m_pFlyParticles->SetAutoRemove(true);
    m_pFlyParticles = NULL;
}

/*
void CCustomShell::StopFlying				()
{
	StopFlyParticles();
	StopLights();
}
*/