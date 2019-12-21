#pragma once

/**************************************************/
/***** Параметры расположения худа / эффектов *****/ //--#SM+#--
/**************************************************/

struct hud_item_measures
{
    enum
    {
        e_fire_point    = (1 << 0),
        e_fire_point2   = (1 << 1),
        e_shell_point   = (1 << 2),
        e_16x9_mode_now = (1 << 3)
    };
    Flags8 m_prop_flags;

    Fvector m_item_attach[2];      // pos,rot
    Fvector m_hands_offset[2][5];  // pos,rot/ normal,aim,GL,aim_alt,scope --#SM+#--
    Fvector m_strafe_offset[4][2]; // pos,rot,data1,data2/ normal,aim-GL --#SM+#--

    u16     m_fire_bone;
    Fvector m_fire_point_offset;
    u16     m_fire_bone2;
    Fvector m_fire_point2_offset;
    u16     m_shell_bone;
    Fvector m_shell_point_offset;

    Fvector m_hands_attach[2]; // pos,rot

    bool bReloadAim;      //--#SM+#--
    bool bReloadAimGL;    //--#SM+#--
    bool bReloadInertion; //--#SM+#--
    bool bReloadPitchOfs; //--#SM+#--
    bool bReloadStrafe;   //--#SM+#--
    bool bReloadShooting; //--#SM+#--
    bool bReloadScope;    //--#SM+#--

    void load(const shared_str& sect_name, IKinematics* K);
    void merge_measures_params(hud_item_measures& new_measures); //--#SM+#--

    struct inertion_params
    {
        float m_pitch_offset_r;
        float m_pitch_offset_n;
        float m_pitch_offset_d;
        float m_pitch_low_limit;
        float m_origin_offset;      //<-- outdated
        float m_origin_offset_aim;  //<-- outdated

        float m_tendto_speed;
        float m_tendto_speed_aim;
        float m_tendto_ret_speed;
        float m_tendto_ret_speed_aim;

        float m_min_angle;
        float m_min_angle_aim;

        Fvector4 m_offset_LRUD;
        Fvector4 m_offset_LRUD_aim;
    };
    inertion_params m_inertion_params; //--#SM+#--

    struct shooting_params
    {
        Fvector4 m_shot_max_offset_LRUD;     // Границы сдвига в бок при выстреле от бедра (-x, +x, +y, -y) 
        Fvector4 m_shot_max_offset_LRUD_aim; // Границы сдвига в бок при выстреле в зуме (-x, +x, +y, -y) 
        Fvector2 m_shot_max_rot_UD;          // Границы поворота по вертикали при выстреле от бедра (при смещении вверх \ вниз) 
        Fvector2 m_shot_max_rot_UD_aim;      // Границы поворота по вертикали при выстреле в зуме (при смещении вверх \ вниз) 
        float m_shot_offset_BACKW;           // Расстояние сдвига назад при выстреле от бедра [>= 0.0f]
        float m_shot_offset_BACKW_aim;       // Расстояние сдвига назад при выстреле в зуме [>= 0.0f]
        Fvector2 m_shot_offsets_strafe;      // Фактор изменения наклона (стрейфа) ствола при выстреле (мин.\макс. от бедра) vec2[0.f - 1.f]
        Fvector2 m_shot_offsets_strafe_aim;  // Фактор изменения наклона (стрейфа) ствола при выстреле (мин.\макс. в зуме) vec2[0.f - 1.f]
        Fvector2 m_shot_diff_per_shot;       // Фактор того, насколько большой может быть разница между текущей и новой позицией на каждый выстрел (от бедра \ в зуме) [0.f - 1.f]
        Fvector2 m_shot_power_per_shot;      // Фактор того, насколько сильнее мы приближаемся к границам сдвига и наклона с каждым выстрелом (от бедра \ в зуме) [0.f - 1.f]
        Fvector2 m_ret_time;                 // Максимально возможное время стабилизации ствола в исходное положение после анимации стрельбы (от бедра / в зуме) [секунд][>= 0.0f] - не влияет на стрейф
        Fvector2 m_ret_time_fire;            // Максимально возможное время стабилизации ствола в исходное положение во время анимации стрельбы (от бедра / в зуме) [секунд][>= 0.0f] - не влияет на стрейф
        float m_ret_time_backw_koef;         // Коэфицент времени стабилизации для сдвига назад [>= 0.0f]
    };
    shooting_params m_shooting_params; //--#SM+#--
};
