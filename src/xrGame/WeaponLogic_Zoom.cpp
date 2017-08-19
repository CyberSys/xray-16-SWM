/**************************************/
/***** Логика зума и прицеливания *****/ //--#SM+#--
/**************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Получить необходимые параметры для динамического зума
void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor)
{
    float def_fov            = float(g_fov);
    float min_zoom_k         = 0.3f;
    float zoom_step_count    = 3.0f;
    float delta_factor_total = def_fov - scope_factor;
    VERIFY(delta_factor_total > 0);
    min_zoom_factor = def_fov - delta_factor_total * min_zoom_k;
    delta           = (delta_factor_total * (1 - min_zoom_k)) / zoom_step_count;
}

// Увеличение динамического зума
void CWeapon::ZoomInc()
{
    if (!IsScopeAttached())
        return;
    if (!m_zoom_params.m_bUseDynamicZoom)
        return;
    float delta, min_zoom_factor;
    GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

    float f = GetZoomFactor() - delta;
    clamp(f, m_zoom_params.m_fScopeZoomFactor, min_zoom_factor);
    SetZoomFactor(f);
}

// Уменьшение динамического зума
void CWeapon::ZoomDec()
{
    if (!IsScopeAttached())
        return;
    if (!m_zoom_params.m_bUseDynamicZoom)
        return;
    float delta, min_zoom_factor;
    GetZoomData(m_zoom_params.m_fScopeZoomFactor, delta, min_zoom_factor);

    float f = GetZoomFactor() + delta;
    clamp(f, m_zoom_params.m_fScopeZoomFactor, min_zoom_factor);
    SetZoomFactor(f);
}

// Вход в зум
void CWeapon::OnZoomIn()
{
    if (IsZoomed())
        return;

    if (m_bDisableFireWhileZooming)
        Need2Stop_Fire();

    PlaySoundZoomIn();

    m_zoom_params.m_bIsZoomModeNow = true;
    m_bIdleFromZoomOut             = false;

    if (m_bNeed2Pump == false && (GetState() == eIdle || GetState() == eFire))
    {
        m_ZoomAnimState = eZAIn;
        PlayAnimZoom();
    }

    // Динамический или простой зум
    if (m_zoom_params.m_bUseDynamicZoom)
        SetZoomFactor(m_fRTZoomFactor);
    else
        m_zoom_params.m_fCurrentZoomFactor = CurrentZoomFactor();

    // Отключаем инерцию (Заменено GetInertionFactor())
    // EnableHudInertion	(FALSE);

    // Zoom DoF SM_TODO: Реализовать в будущем аля ЧН
    //if(m_zoom_params.m_bZoomDofEnabled && !IsScopeAttached())
    //	GamePersistent().SetEffectorDOF	(m_zoom_params.m_ZoomDof);

    if (GetHUDmode())
        GamePersistent().SetPickableEffectorDOF(true);

    // Использовать рамки от бинокля?
    if (m_zoom_params.m_sUseBinocularVision.size() && IsScopeAttached() && NULL == m_zoom_params.m_pVision)
        m_zoom_params.m_pVision = new CBinocularsVision(m_zoom_params.m_sUseBinocularVision);

    // Использовать постэффект для зума
    if (m_zoom_params.m_sUseZoomPostprocess.size() && IsScopeAttached())
    {
        CActor* pA = smart_cast<CActor*>(H_Parent());
        if (pA)
        {
            if (NULL == m_zoom_params.m_pNight_vision)
            {
                m_zoom_params.m_pNight_vision = new CNightVisionEffector(m_zoom_params.m_sUseZoomPostprocess /*"device_torch"*/);
            }
        }
    }

    // Эффект камеры при зуме
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (pActor)
    {
        CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>(pActor->Cameras().GetCamEffector(eCEZoom));
        if (!S)
        {
            S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(new CEffectorZoomInertion());
            S->Init(this);
        };
        S->SetRndSeed(pActor->GetZoomRndSeed());
        R_ASSERT(S);
    }
}

// Выход из зума
void CWeapon::OnZoomOut()
{
    if (!IsZoomed())
        return;

    if (m_bDisableFireWhileZooming)
        Need2Stop_Fire();

    PlaySoundZoomOut();

    if (GetState() == eIdle || GetState() == eFire)
    {
        m_bIdleFromZoomOut = true;

        if (m_bNeed2Pump == false)
        {
            m_ZoomAnimState = eZAOut;
            PlayAnimZoom();
        }
    }

    m_zoom_params.m_bIsZoomModeNow     = false;
    m_zoom_params.m_fCurrentZoomFactor = g_fov;
    m_fRTZoomFactor                    = GetZoomFactor(); //store current

    ResetSubStateTime();

    // Включаем инерцию
    // EnableHudInertion	(TRUE);

    // Zoom DoF
    // 	GamePersistent().RestoreEffectorDOF	();

    if (GetHUDmode())
        GamePersistent().SetPickableEffectorDOF(false);

    // Отключить постэффект зума
    xr_delete(m_zoom_params.m_pVision);
    if (m_zoom_params.m_pNight_vision)
    {
        m_zoom_params.m_pNight_vision->Stop(100000.0f, false);
        xr_delete(m_zoom_params.m_pNight_vision);
    }

    // Убрать эффект камеры
    CActor* pActor = smart_cast<CActor*>(H_Parent());

    if (pActor)
        pActor->Cameras().RemoveCamEffector(eCEZoom);
}

// Включить эффект ночного видения
void CWeapon::EnableActorNVisnAfterZoom()
{
    CActor* pA = smart_cast<CActor*>(H_Parent());
    if (IsGameTypeSingle() && !pA)
        pA = g_actor;

    if (pA)
    {
        CTorch* pTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
        if (pTorch)
        {
            pTorch->SwitchNightVision(true, false);
            pTorch->GetNightVision()->PlaySounds(CNightVisionEffector::eIdleSound);
        }
    }
}

// Получить текущию UI-текстуру прицела для зума
CUIWindow* CWeapon::ZoomTexture()
{
    if (UseScopeTexture())
        return m_UIScope;
    else
        return NULL;
}

// Можно-ли использовать текстурный прицел при зуме?
bool CWeapon::UseScopeTexture()
{
    if (IsGrenadeLauncherAttached() && m_bGrenadeMode)
        return false;

    if (IsBipodsDeployed() && !m_bipods.m_bUseZoomFov)
        return false;

    return true;
};

// Чувствительность мышкии с оружием в руках
float CWeapon::GetControlInertionFactor() const
{
    float fInertionFactor = inherited::GetControlInertionFactor();
    if (IsScopeAttached() && IsZoomed())
        return m_fScopeInertionFactor;

    return fInertionFactor;
}
