/**********************************/
/***** Параметры прицеливания *****/ //--#SM+#--
/**********************************/

#include "stdafx.h"
#include "Weapon_ZoomParams.h"
#include "WeaponBinocularsVision.h"
#include "Torch.h"
#include "xrUICore/Windows/UIWindow.h"
#include "ui/UIXmlInit.h"

ENGINE_API extern float psHUD_FOV;
ENGINE_API extern float psHUD_FOV_def;
float                   g_defScopeZoomFactor = 100.0f;

CUIXml* pWpnScopeXml = NULL;

SZoomParams::SZoomParams()
{
    m_UIScope       = NULL;
    m_pVision       = NULL;
    m_pNight_vision = NULL;
}

SZoomParams::~SZoomParams()
{
    xr_delete(m_UIScope);
    xr_delete(m_pVision);
    xr_delete(m_pNight_vision);
}

void SZoomParams::Initialize(LPCSTR section, LPCSTR prefix, bool bOverrideMode)
{
    string64 _prefix;
    xr_sprintf(_prefix, "%s", prefix != NULL ? prefix : "");
    string128 val_name;

    // Целевой FOV-фактор при зуме (умножается на 0.75)
    strconcat(sizeof(val_name), val_name, "scope_zoom_factor", _prefix);
    m_fZoomFovFactor = READ_IF_EXISTS(pSettings, r_float, section, val_name, (bOverrideMode ? m_fZoomFovFactor : g_defScopeZoomFactor));

    // Целевой FOV при зуме (если > 0, то будет использоваться он вместо scope_zoom_factor)
    strconcat(sizeof(val_name), val_name, "scope_zoom_fov", _prefix);
    m_fZoomFov = READ_IF_EXISTS(pSettings, r_float, section, val_name, (bOverrideMode ? m_fZoomFov : 0.000f));

    // Зум отсутствует
    strconcat(sizeof(val_name), val_name, "scope_no_zoom", _prefix);
    m_bNoZoom = READ_IF_EXISTS(pSettings, r_bool, section, val_name, (bOverrideMode ? m_bNoZoom : false));

    // Целевой HUD FOV при зуме
    strconcat(sizeof(val_name), val_name, "scope_zoom_hud_fov", _prefix);
    m_fZoomHudFov = READ_IF_EXISTS(pSettings, r_float, section, val_name, (bOverrideMode ? m_fZoomHudFov : psHUD_FOV_def));

    // Целевой FOV второго вьюпорта при зуме (линзы)
    strconcat(sizeof(val_name), val_name, "scope_lense_factor", _prefix);
    m_fSecondVPFovFactor = READ_IF_EXISTS(pSettings, r_float, section, val_name, (bOverrideMode ? m_fSecondVPFovFactor : 0.0f));

    // Использовать динамический зум (если указан)
    strconcat(sizeof(val_name), val_name, "scope_dynamic_zoom", _prefix);
    m_bUseDynamicZoom = READ_IF_EXISTS(pSettings, r_bool, section, val_name, (bOverrideMode ? m_bUseDynamicZoom : false));
    if (bOverrideMode == false)
        m_fRTZoomFactor = -1.f;

    // Текстура прицельной сетки
    strconcat(sizeof(val_name), val_name, "scope_texture", _prefix);
    m_sUseScopeTexture = READ_IF_EXISTS(pSettings, r_string, section, val_name, (bOverrideMode ? m_sUseScopeTexture : NULL));

    // Постпроцесс зума
    strconcat(sizeof(val_name), val_name, "scope_nightvision", _prefix);
    m_sUseZoomPostprocess = READ_IF_EXISTS(pSettings, r_string, section, val_name, (bOverrideMode ? m_sUseZoomPostprocess : NULL));

    // Режим бинокля
    strconcat(sizeof(val_name), val_name, "scope_alive_detector", _prefix);
    m_sUseBinocularVision = READ_IF_EXISTS(pSettings, r_string, section, val_name, (bOverrideMode ? m_sUseBinocularVision : NULL));

    // Прочее
    xr_delete(m_UIScope);
    xr_delete(m_pVision);
    xr_delete(m_pNight_vision);
}

// Создать UI-окно текстурного 2D-прицела
void SZoomParams::UpdateUIScope()
{
    // Создаём глобальный UI прицелов
    if (pWpnScopeXml == nullptr)
    {
        pWpnScopeXml = new CUIXml();
        pWpnScopeXml->Load(CONFIG_PATH, UI_PATH, "scopes.xml");
    }

    // Натягиваем текстуру прицельной сетки
    if (!GEnv.isDedicatedServer)
    {
        if (m_sUseScopeTexture != NULL && !m_sUseScopeTexture.equal("none"))
        {
            xr_delete(m_UIScope);
            m_UIScope = new CUIWindow();
            CUIXmlInit::InitWindow(*pWpnScopeXml, m_sUseScopeTexture.c_str(), 0, m_UIScope);
        }
    }
}
