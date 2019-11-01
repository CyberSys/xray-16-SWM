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
        Fvector4 m_shot_max_offset_LRUD;
        Fvector4 m_shot_max_offset_LRUD_aim;
        Fvector2 m_shot_offset_BACKW;
        float m_ret_speed;
        float m_ret_speed_aim;
        float m_min_LRUD_power;
    };
    shooting_params m_shooting_params; //--#SM+#--
};
