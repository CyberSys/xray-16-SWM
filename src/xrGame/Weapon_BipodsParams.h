#pragma once

/******************************/
/***** Параметры сошек *****/ //--#SM+#--
/******************************/

class CPHCollideHelper;

struct bipods_data
{
    friend CWeapon;

    enum bipods_state
    {
        eBS_SwitchedOFF = 0, // Выключены
        eBS_TranslateInto, // Анимация установки
        eBS_SwitchedON, // Включены
        eBS_TranslateOutro // Анимация снятия
    };
    bipods_state m_iBipodState; //--> Текущее состояние сошек

    bool m_bInstalled; //--> Присутствуют-ли сошки на оружии

    Fvector m_vBipodInitPos; //--> Актуальная позиция установки сошек
    Fvector m_vBipodInitDir; //--> Актуальная дирекция установки сошек
    Fvector m_vBipodInitNormal; //--> Актуальная нормаль установки сошек

    bool m_bZoomMode; //--> Включён-ли режим прицеливания

    float m_fTranslationFactor; //--> Степень стадии установки сошек (0.f сложены - 1.f установлены)

    bipods_data()
        : m_bInstalled(false), m_bZoomMode(false), m_iBipodState(eBS_SwitchedOFF), m_fTranslationFactor(0.0f),
          bDbgDraw(false)
    {
        m_vPrevYP.set(0.f, 0.f);
        sInstCamAnm = nullptr;

        m_pPHWpnBody = nullptr;
        m_pPHBipodsLegL = nullptr;
        m_pPHBipodsLegR = nullptr;
    }

public:
    shared_str sBipodsHudSect; // Худовая секция визуала сошек
    shared_str sBipodsVisSect; // Мировая секция визуала сошек

    bool bDbgDraw; // Отрисовывать отладочную информацию (только R1/R2 Mixed)
    bool bAnimatedLegs; // Флаг наличия двигающихся костей у сошек (ножек сошек)
    bool bEnableWpnTilt; // Должно-ли оружие принимать наклон поверхности при установке
    bool bInvertBodyYaw; // Инвертировать Y-поворот основания при повороте камеры
    bool bInvertBodyPitch; // Инвертировать X-поворот основания при повороте камеры
    bool bInvertLegsPitch; // Инвертировать X-поворот ножек при повороте камеры
    bool bDeployWhenBayonetInst; // Разложить сошки если установлен штык-нож
    float fInstRangeMax; // Макс. расстояние установки сошек от центра камеры
    float fInstAngleMax; // Максимальный наклон поверхности в градусах (0 - ровная, 90 - стена, 180 - потолок)
    float fCamDistMax; // Макс. дистанция от точки установки до камеры
    float fCamYOffset; // Смещение координаты Y у камеры от точки установки
    float fHudZOffset; // Смещение худа по Z-координате
    float fPitch2LegsTiltFactor; // Фактор влияния наклона по X на степень раздвига ножек (больше => меньше)
    float fInertiaMod; // Модификатор силы инерции худа
    float fPosZOffset; // Сдвиг точки установки m_vBipodInitPos в направлении m_vBipodInitDir
    float fDeployTime; // Время на установку сошек (секунды)
    float fUndeployTime; // Время на снятие сошек (секунды)
    float fZoomFOV; // FOV при зуме в сошках (без прицела)
    float fHudFOVFactor; // HUD FOV при разложенных сошках
    float fDispersionMod; // Модификатор разброса пуль при разложенных сошках
    float fRecoilMod; // Модификатор отдачи при разложенных сошках
    Fvector2 vCamYawLimits; // Лимиты вращения сошек по Y
    Fvector2 vCamPitchLimits; // Лимиты вращения сошек по X
    Fvector vTorchOffset; // Координаты смещения света фонарика от центра худа
    Fvector vBoneMDeployPosOffs; // Смещение главной кости при установке сошек
    Fvector vBoneMDeployRotOffs; // Поворот главной кости при установке сошек
    Fvector vBoneLDeployRotOffs; // Базовый поворот ножек при установке сошек
    shared_str sInstCamAnm; // Путь до анимации камеры (.anm) во время установки
    float fCamAnmSpeed; // Фактор скорости анимации камеры

private:
    bool m_bFirstCamUpdate; //--> True когда делаем первый апдейт камеры после установки сошек
    bool m_bCamLimitsWasSaved; //--> True если мы уже запомнили параметры камеры игрока, False если сбросили в дефолт
    bool m_bBipodsIntroEffPlayed; //--> True если уже был проигран эффект установки сошек (snd + cam anm)

    Fvector2 m_vPrevYP; //--> Yaw\Pitch камеры с предыдущего кадра
    Fvector m_vParentInitPos; //--> Позиция владельца оружия в момент установки сошек

    // Параметры камеры до входа в режим сошек
    Fvector m_vOldCamPos;
    Fvector2 m_vOldCamYP;
    Fvector2 m_fOldYawLimit;
    Fvector2 m_fOldPitchLimit;
    bool m_bOldClampYaw;
    bool m_bOldClampPitch;

    // Параметры поворота модели сошек с прошлого кадра
    Fvector m_vPrevLegsXYZ;

    // Физическая модель сошек (для просчёта столкновений и проверки наличия места)
    CPHCollideHelper* m_pPHWpnBody;
    CPHCollideHelper* m_pPHBipodsLegL;
    CPHCollideHelper* m_pPHBipodsLegR;

    // Для отладки
    enum EDBGCollideType
    {
        ctNoCollide = 0,
        ctBox,
        ctCamera,
        ctCount
    };

    Fmatrix dbg_mPHCamBody; //--> XFORM камеры игрока
    Fvector dbg_vPHCamBodyHalfsize; //--> Размер камеры игрока (в половину)
    EDBGCollideType dbg_CollideType; //--> По какой причине не прошли проверку коллизии
};
