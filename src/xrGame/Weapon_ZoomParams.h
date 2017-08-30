#pragma once

/**********************************/
/***** Параметры прицеливания *****/ //--#SM+#--
/**********************************/

class CUIWindow;
class CBinocularsVision;
class CNightVisionEffector;

struct SZoomParams
{
public:
    SZoomParams();
    ~SZoomParams();

    void Initialize(LPCSTR section, LPCSTR prefix = NULL, bool bOverrideMode = false);
    void UpdateUIScope();

    bool  m_bZoomDofEnabled;    // DOF-эффект во время зума
    float m_fRTZoomFactor;      // Последний сохранённый FOV для зума с регулируемой кратностью
    float m_fZoomFovFactor;     // Модификатор FOV при зуме (g_fov * (m_fZoomFovFactor / 100.f))
    float m_fSecondVPFovFactor; // Модификатор для FOV во втором вьюпорте при зуме
    float m_fZoomHudFov;        // Целевой HUD FOV при зуме

    Fvector               m_ZoomDof;             // Параметры DOF-эффекта при прицеливании
    Fvector4              m_ReloadDof;           // Параметры DOF-эффекта при перезарядке
    bool                  m_bUseDynamicZoom;     // Прицел с регулируемой кратностью
    shared_str            m_sUseScopeTexture;    // 2D-текстура прицельной сетки
    shared_str            m_sUseZoomPostprocess; // Пост-эффект прицела
    shared_str            m_sUseBinocularVision; // Эффект выделения в прицеле живых существ
    CUIWindow*            m_UIScope;
    CBinocularsVision*    m_pVision;
    CNightVisionEffector* m_pNight_vision;
};