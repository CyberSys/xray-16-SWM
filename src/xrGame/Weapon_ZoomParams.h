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

    // Мир
    float m_fZoomFovFactor;      // Модификатор FOV при зуме (g_fov * 0.75f)) - совместимость с оригиналом
    float m_fZoomFov;            // Целевой FOV при зуме (заменяет m_fZoomFovFactor)
    float m_fScopeBipodsZoomFov; // Целевой FOV при зуме в сошках с одетым прицелом
    bool  m_bNoZoom;             // Прицел с регулируемой кратностью
    float m_fZoomHudFov;         // Целевой HUD FOV при зуме

    // Линза
    float m_fZoomFovSVP;        // Целевой FOV при зуме (заменяет m_fZoomFovFactor)
    bool  m_bNoZoomSVP;         // Прицел с регулируемой кратностью
    float m_fZoomHudFovSVP;     // Целевой HUD FOV при зуме
    float m_fLenseTubeKoef;     // Коэфицент размера текстуры трубы прицела
    float m_fLenseDistortKoef;  // Коэфицент искажения картинки в линзе

    bool m_bUseDynamicZoom;     // Прицел с регулируемой кратностью
    float m_fRTZoomFactor;      // Последний сохранённый FOV для зума с регулируемой кратностью

    float m_fScopeInertionFactor; //--> Чувствительность мыши во время прицеливания

    bool                  m_bZoomDofEnabled;     // DOF-эффект во время зума
    Fvector               m_ZoomDof;             // Параметры DOF-эффекта при прицеливании
    Fvector4              m_ReloadDof;           // Параметры DOF-эффекта при перезарядке
    shared_str            m_sUseScopeTexture;    // 2D-текстура прицельной сетки
    shared_str            m_sUseZoomPostprocess; // Пост-эффект прицела
    shared_str            m_sUseBinocularVision; // Эффект выделения в прицеле живых существ
    CUIWindow*            m_UIScope;
    CBinocularsVision*    m_pVision;
    CNightVisionEffector* m_pNight_vision;
};
