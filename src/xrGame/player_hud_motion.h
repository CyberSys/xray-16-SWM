#pragma once

/****************************/
/***** Худовые анимации *****/ //--#SM+#--
/****************************/

#include "Include/xrRender/KinematicsAnimated.h"

struct motion_descr
{
    MotionID   mid;
    shared_str name;
};

struct player_hud_motion
{
    shared_str              m_alias_name;
    shared_str              m_base_name;       // Анимация рук
    shared_str              m_additional_name; // Анимация предмета
    shared_str              m_child_name;      // Анимация атачей на предмете (потомков) --#SM+#--
    xr_vector<motion_descr> m_animations;
};

struct player_hud_motion_container
{
    xr_vector<player_hud_motion> m_anims;
    player_hud_motion*           find_motion(const shared_str& name);
    void                         load(IKinematicsAnimated* model, const shared_str& sect);
};

float ModifyBlendParams(CBlend* pMotionBlend, float fSpeed, float fStartFromTime); //--#SM+#--
float CalculateMotionStartSeconds(float fStartFromTime, float fMotionLength);      //--#SM+#--