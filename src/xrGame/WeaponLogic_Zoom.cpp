/**************************************/
/***** Логика зума и прицеливания *****/ //--#SM+#--
/**************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"
#include "WeaponBinocularsVision.h"
#include "Torch.h"

// Получить необходимые параметры для динамического зума
void GetZoomData(const float scope_factor, float& delta, float& min_zoom_factor, bool bOldMode = true)
{
    float def_fov            = (bOldMode ? float(g_fov) : 100.0f);
    float min_zoom_k         = 0.3f;
    float zoom_step_count    = 3.0f;
    float delta_factor_total = def_fov - scope_factor;
    VERIFY(delta_factor_total > 0);
    min_zoom_factor = def_fov - delta_factor_total * min_zoom_k;
    delta           = (delta_factor_total * (1 - min_zoom_k)) / zoom_step_count;
}

// Увеличение динамического зума
void CWeapon::ZoomInc(bool bForceLimit) { ZoomDynamicMod(true, bForceLimit); }

// Уменьшение динамического зума
void CWeapon::ZoomDec(bool bForceLimit) { ZoomDynamicMod(false, bForceLimit); }

// Меняем динамический зум в обе стороны
void CWeapon::ZoomDynamicMod(bool bIncrement, bool bForceLimit)
{
    if (!GetZoomParams().m_bUseDynamicZoom)
        return;

    float delta, min_zoom_factor, max_zoom_factor;
    max_zoom_factor = (IsSecondVPZoomPresent() ? (GetSecondVPZoomFactor() * 100.f) : GetAimZoomFactor());
    GetZoomData(max_zoom_factor, delta, min_zoom_factor, (m_bUseOldZoomFactor && !IsSecondVPZoomPresent()));

    if (bForceLimit)
    {
        GetZoomParams().m_fRTZoomFactor = (bIncrement ? max_zoom_factor : min_zoom_factor);
    }
    else
    {
        R_ASSERT(GetZoomParams().m_fRTZoomFactor >= 0.0f);

        float f = (bIncrement ? GetZoomParams().m_fRTZoomFactor - delta : GetZoomParams().m_fRTZoomFactor + delta);
        clamp(f, max_zoom_factor, min_zoom_factor);

        GetZoomParams().m_fRTZoomFactor = f;
    }
}

// Вход в зум
void CWeapon::OnZoomIn(bool bSilent)
{
    if (IsZoomed())
        return;

    m_bIsZoomModeNow = true;

    // Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject* object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponZoomIn)(object->lua_game_object(), this->lua_game_object());
#endif
    //-Alundaio

    if (bSilent == false)
    {
        if (m_bDisableFireWhileZooming)
            Need2Stop_Fire();

        PlaySoundZoomIn();

        m_bIdleFromZoomOut = false;

        if (m_bNeed2Pump == false && (GetState() == eIdle || GetState() == eFire))
        {
            m_ZoomAnimState = eZAIn;
            PlayAnimZoom();
        }
    }

    // Инициализируем динамический зум
    if (GetZoomParams().m_bUseDynamicZoom && GetZoomParams().m_fRTZoomFactor < 0.0f)
        ZoomDec(true); //--> По умолчанию динамический зум на минимуме

    // Отключаем инерцию (Заменено GetInertionFactor())
    // EnableHudInertion	(FALSE);

    //if(GetZoomParams().m_bZoomDofEnabled && !IsScopeAttached()) ! Проверять на текстуру 2D-прицела
    //	GamePersistent().SetEffectorDOF	(GetZoomParams().m_ZoomDof);

    if (GetHUDmode())
        GamePersistent().SetPickableEffectorDOF(true);

    // Использовать рамки от бинокля?
    if (GetZoomParams().m_sUseBinocularVision.size() && IsScopeAttached() && NULL == GetZoomParams().m_pVision)
        GetZoomParams().m_pVision = new CBinocularsVision(GetZoomParams().m_sUseBinocularVision);

    // Использовать постэффект для зума
    if (GetZoomParams().m_sUseZoomPostprocess.size() && IsScopeAttached())
    {
        CActor* pA = smart_cast<CActor*>(H_Parent());
        if (pA)
        {
            if (NULL == GetZoomParams().m_pNight_vision)
            {
                GetZoomParams().m_pNight_vision = new CNightVisionEffector(GetZoomParams().m_sUseZoomPostprocess /*"device_torch"*/);
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
void CWeapon::OnZoomOut(bool bSilent)
{
    if (!IsZoomed())
        return;

    if (bSilent == false)
    {
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
    }

    m_bIsZoomModeNow = false;

    // Alundaio
#ifdef EXTENDED_WEAPON_CALLBACKS
    CGameObject* object = smart_cast<CGameObject*>(H_Parent());
    if (object)
        object->callback(GameObject::eOnWeaponZoomOut)(object->lua_game_object(), this->lua_game_object());
#endif
    //-Alundaio

    ResetSubStateTime();

    // Включаем инерцию
    // EnableHudInertion	(TRUE);

    // Zoom DoF
    // 	GamePersistent().RestoreEffectorDOF	();

    if (GetHUDmode())
        GamePersistent().SetPickableEffectorDOF(false);

    // Отключить постэффект зума
    xr_delete(GetZoomParams().m_pVision);
    if (GetZoomParams().m_pNight_vision)
    {
        GetZoomParams().m_pNight_vision->Stop(100000.0f, false);
        xr_delete(GetZoomParams().m_pNight_vision);
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
        return GetZoomParams().m_UIScope;
    else
        return NULL;
}

// Можно-ли использовать текстурный прицел при зуме?
bool CWeapon::UseScopeTexture()
{
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

// Переключение режимов прицеливания (Обычный\Альтернативный)
void CWeapon::SwitchZoomType(EZoomTypes iType)
{
    bool bZoomed = IsZoomed();

    // <!> Тип зума ещё старый
    if (bZoomed)
        OnZoomOut(true);

    // Меняем тип зума
    m_iPrevZoomType = m_iCurZoomType;
    m_iCurZoomType  = iType;

    // <!> Тип зума уже новый
    if (iType != eZoomMain || IsScopeAttached()) //--> Только ради обратной совместимости с конфигами оригиниальной игры
        GetZoomParams().UpdateUIScope();

    if (bZoomed)
        OnZoomIn(true);
}

void CWeapon::SwitchZoomType(EZoomTypes iCurType, EZoomTypes iPrevType)
{
    m_iPrevZoomType = iPrevType;
    SwitchZoomType(iCurType);
    m_iPrevZoomType = iPrevType;
}
