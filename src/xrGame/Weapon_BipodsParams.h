#pragma once

/******************************/
/***** Параметры сошек *****/ //--#SM+#--
/******************************/

struct bipods_data
{
    friend CWeapon;

    enum bipods_state
    {
        eBS_SwitchedOFF = 0, // Выключены
        eBS_TranslateInto, // Установка
        eBS_SwitchedON, // Включены
        eBS_TranslateOutro // Выключение
    };
    bipods_state m_iBipodState;

    bool m_bInstalled;

    shared_str m_sBipod_hud;
    shared_str m_sBipod_vis;

    bool m_bAnimated; // Флаг наличия двигающихся костей у сошек
    float m_i_range; // Макс. расстояние установки сошек от центра камеры
    float m_i_angle_f; // Мин. необходимая величина наклона поверхности (1 - идеально ровная, 0 - стена, -1 - потолок)
    float m_cam_dist; // Макс. дистанция от точки установки до камеры
    float m_cam_y_offset; // Смещение координаты Y у камеры от точки установки
    float m_hud_z_offset; // Смещение худа по Z-координате
    float m_pitch_vis_factor; // Фактор влияния наклона по X на степень раздвига ножек (больше => меньше)
    float m_inertia_power; // Модификатор силы инерции худа
    float m_pos_displ; // Сдвиг точки установки m_vBipodInitPos в направлении m_vBipodInitDir
    float m_deploy_time; // Время на установку сошек (секунды)
    float m_undeploy_time; // Время на снятие сошек (секунды)
    float m_fZoomFOV; // FOV при зуме в сошках (без прицела)
    float m_fHUD_FOV; // HUD FOV при разложенных сошках
    float m_fDispersionMod; // Модификатор разброса пуль при разложенных сошках
    float m_fRecoilMod; // Модификатор отдачи при разложенных сошках
    float m_max_dist; // Максимальное расстояние, на которое игрок может отойти от установленных сошек
    Fvector2 m_yaw_limit; // Лимиты вращения сошек по Y
    Fvector2 m_pitch_limit; // Лимиты вращения сошек по X
    Fvector m_torch_offset; // Координаты смещения света фонарика от центра худа
    Fvector m_deploy_pos; // Смещение главной кости при установке сошек
    Fvector m_deploy_rot; // Поворот главной кости при установке сошек
    Fvector m_legs_rot; // Базовый поворот ножек при установке сошек

    Fvector m_vParentInitPos;
    Fvector m_vBipodInitPos;
    Fvector m_vBipodInitDir;
    Fvector m_vBipodInitNormal;
    Fvector2 m_vPrevYP;
    bool m_bUseZoomFov;
    float m_fCurScopeZoomFov;
    float m_translation_factor; // 0.f - 1.f

    bipods_data()
        : m_bInstalled(false), m_bUseZoomFov(false), m_iBipodState(eBS_SwitchedOFF), m_translation_factor(0.0f),
          m_fCurScopeZoomFov(0.0f)
    {
        m_vPrevYP.set(0.f, 0.f);
    }

private:
    bool m_bFirstCamUpdate;

    // Параметры камеры до входа в режим сошек
    Fvector m_vOldCamPos;
    Fvector2 m_vOldCamYP;
    Fvector2 m_fOldYawLimit;
    Fvector2 m_fOldPitchLimit;
    bool m_bOldClampYaw;
    bool m_bOldClampPitch;
};
