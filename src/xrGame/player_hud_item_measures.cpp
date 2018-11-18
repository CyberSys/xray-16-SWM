/**************************************************/
/***** Параметры расположения худа / эффектов *****/ //--#SM+#--
/**************************************************/

#include "stdafx.h"
#include "player_hud.h"
#include "player_hud_item_measures.h"
#include "HudItem.h"
#include "ui_base.h"
#include "player_hud_inertion.h"

void hud_item_measures::load(const shared_str& sect_name, IKinematics* K)
{
    bool     is_16x9 = UI().is_widescreen();
    string64 _prefix;
    xr_sprintf(_prefix, "%s", is_16x9 ? "_16x9" : "");
    string128 val_name;

    strconcat(sizeof(val_name), val_name, "hands_position", _prefix);
    m_hands_attach[0] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(sizeof(val_name), val_name, "hands_orientation", _prefix);
    m_hands_attach[1] = pSettings->r_fvector3(sect_name, val_name);

    m_item_attach[0] = pSettings->r_fvector3(sect_name, "item_position");
    m_item_attach[1] = pSettings->r_fvector3(sect_name, "item_orientation");

    shared_str bone_name;
    m_prop_flags.set(e_fire_point, pSettings->line_exist(sect_name, "fire_bone"));
    if (m_prop_flags.test(e_fire_point))
    {
        bone_name           = pSettings->r_string(sect_name, "fire_bone");
        m_fire_bone         = K->LL_BoneID(bone_name);
        m_fire_point_offset = pSettings->r_fvector3(sect_name, "fire_point");
    }
    else
        m_fire_point_offset.set(0, 0, 0);

    m_prop_flags.set(e_fire_point2, pSettings->line_exist(sect_name, "fire_bone2"));
    if (m_prop_flags.test(e_fire_point2))
    {
        bone_name            = pSettings->r_string(sect_name, "fire_bone2");
        m_fire_bone2         = K->LL_BoneID(bone_name);
        m_fire_point2_offset = pSettings->r_fvector3(sect_name, "fire_point2");
    }
    else
        m_fire_point2_offset.set(0, 0, 0);

    m_prop_flags.set(e_shell_point, pSettings->line_exist(sect_name, "shell_bone"));
    if (m_prop_flags.test(e_shell_point))
    {
        bone_name            = pSettings->r_string(sect_name, "shell_bone");
        m_shell_bone         = K->LL_BoneID(bone_name);
        m_shell_point_offset = pSettings->r_fvector3(sect_name, "shell_point");
    }
    else
        m_shell_point_offset.set(0, 0, 0);

    m_hands_offset[0][0].set(0, 0, 0);
    m_hands_offset[1][0].set(0, 0, 0);

    strconcat(sizeof(val_name), val_name, "aim_hud_offset_pos", _prefix);
    m_hands_offset[0][1] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(sizeof(val_name), val_name, "aim_hud_offset_rot", _prefix);
    m_hands_offset[1][1] = pSettings->r_fvector3(sect_name, val_name);

    strconcat(sizeof(val_name), val_name, "gl_hud_offset_pos", _prefix);
    m_hands_offset[0][2] = pSettings->r_fvector3(sect_name, val_name);
    strconcat(sizeof(val_name), val_name, "gl_hud_offset_rot", _prefix);
    m_hands_offset[1][2] = pSettings->r_fvector3(sect_name, val_name);

    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point") == pSettings->line_exist(sect_name, "fire_bone"), sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point2") == pSettings->line_exist(sect_name, "fire_bone2"), sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "shell_point") == pSettings->line_exist(sect_name, "shell_bone"), sect_name.c_str());

    m_prop_flags.set(e_16x9_mode_now, is_16x9);

    ////////////////////////////////////////////
    //--#SM+# Begin--
    const Fvector vZero = {0.f, 0.f, 0.f};

    // Альтернативное прицеливание
    strconcat(sizeof(val_name), val_name, "aim_hud_offset_alt_pos", _prefix);
    m_hands_offset[0][3] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vZero);
    strconcat(sizeof(val_name), val_name, "aim_hud_offset_alt_rot", _prefix);
    m_hands_offset[1][3] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vZero);

    // Настройки стрейфа (боковая ходьба)
    Fvector vDefStrafeValue;
    vDefStrafeValue.set(vZero);

    //--> Смещение в стрейфе
    strconcat(sizeof(val_name), val_name, "strafe_hud_offset_pos", _prefix);
    m_strafe_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);
    strconcat(sizeof(val_name), val_name, "strafe_hud_offset_rot", _prefix);
    m_strafe_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);

    //--> Поворот в стрейфе
    strconcat(sizeof(val_name), val_name, "strafe_aim_hud_offset_pos", _prefix);
    m_strafe_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);
    strconcat(sizeof(val_name), val_name, "strafe_aim_hud_offset_rot", _prefix);
    m_strafe_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);

    //--> Параметры стрейфа
    float fFullStrafeTime     = READ_IF_EXISTS(pSettings, r_float, sect_name, "strafe_transition_time", 0.01f);
    float fFullStrafeTime_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "strafe_aim_transition_time", 0.01f);
    bool  bStrafeEnabled      = READ_IF_EXISTS(pSettings, r_bool, sect_name, "strafe_enabled", false);
    bool  bStrafeEnabled_aim  = READ_IF_EXISTS(pSettings, r_bool, sect_name, "strafe_aim_enabled", false);

    m_strafe_offset[2][0].set(bStrafeEnabled, fFullStrafeTime, NULL);         // normal
    m_strafe_offset[2][1].set(bStrafeEnabled_aim, fFullStrafeTime_aim, NULL); // aim-GL

    // Нужно-ли использовать координаты худа из потомка, когда он присоединён?
    bReloadAim      = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_aim_position", false);
    bReloadAimGL    = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_aim_gl_position", false);
    bReloadInertion = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_inertion_params", false);
    bReloadPitchOfs = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_pitch_offsets", false);

    // Загрузка параметров смещения / инерции
    m_inertion_params.m_pitch_offset_r  = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_right", PITCH_OFFSET_R);
    m_inertion_params.m_pitch_offset_n  = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up", PITCH_OFFSET_N);
    m_inertion_params.m_pitch_offset_d  = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_forward", PITCH_OFFSET_D);
    m_inertion_params.m_pitch_low_limit = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up_low_limit", PITCH_LOW_LIMIT);

    m_inertion_params.m_origin_offset     = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_offset", ORIGIN_OFFSET);
    m_inertion_params.m_origin_offset_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_aim_offset", ORIGIN_OFFSET_AIM);
    m_inertion_params.m_tendto_speed      = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_speed", TENDTO_SPEED);
    m_inertion_params.m_tendto_speed_aim  = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_aim_speed", TENDTO_SPEED_AIM);

    // Msg("Measures loaded from %s [%d][%d][%d][%d]", sect_name.c_str(), bReloadAim, bReloadAimGL, bReloadInertion, bReloadPitchOfs);
    //--#SM+# End--
}

// Частично переопределяем худовые параметры другими (обычно из потомка) --#SM+#--
void hud_item_measures::merge_measures_params(hud_item_measures& new_measures)
{
    // Координаты прицеливания основного ствола
    if (new_measures.bReloadAim)
    {
        // aim_hud_offset_pos
        m_hands_offset[0][1].set(new_measures.m_hands_offset[0][1]);
        // aim_hud_offset_rot
        m_hands_offset[1][1].set(new_measures.m_hands_offset[1][1]);

        // aim_hud_offset_alt_pos
        m_hands_offset[0][3].set(new_measures.m_hands_offset[0][3]);
        // aim_hud_offset_alt_rot
        m_hands_offset[1][3].set(new_measures.m_hands_offset[1][3]);
    }

    // Координаты прицеливания подствольника
    if (new_measures.bReloadAimGL)
    {
        // gl_hud_offset_pos
        m_hands_offset[0][2].set(new_measures.m_hands_offset[0][2]);
        // gl_hud_offset_rot
        m_hands_offset[1][2].set(new_measures.m_hands_offset[1][2]);
    }

    // Инерция
    if (new_measures.bReloadInertion)
    {
        m_inertion_params.m_origin_offset     = new_measures.m_inertion_params.m_origin_offset;
        m_inertion_params.m_origin_offset_aim = new_measures.m_inertion_params.m_origin_offset_aim;
        m_inertion_params.m_tendto_speed      = new_measures.m_inertion_params.m_tendto_speed;
        m_inertion_params.m_tendto_speed_aim  = new_measures.m_inertion_params.m_tendto_speed_aim;
    }

    // Инерция - смещение ствола при вертикальных поторотах камеры
    if (new_measures.bReloadPitchOfs)
    {
        m_inertion_params.m_pitch_offset_r  = new_measures.m_inertion_params.m_pitch_offset_r;
        m_inertion_params.m_pitch_offset_n  = new_measures.m_inertion_params.m_pitch_offset_n;
        m_inertion_params.m_pitch_offset_d  = new_measures.m_inertion_params.m_pitch_offset_d;
        m_inertion_params.m_pitch_low_limit = new_measures.m_inertion_params.m_pitch_low_limit;
    }

    // Msg("Measures reloaded [%d][%d]", new_measures.bReloadAim, new_measures.bReloadAimGL);
}

Fvector& attachable_hud_item::hands_attach_pos() { return m_measures.m_hands_attach[0]; }

Fvector& attachable_hud_item::hands_attach_rot() { return m_measures.m_hands_attach[1]; }

Fvector& attachable_hud_item::hands_offset_pos()
{
    u8 idx = m_parent_hud_item->GetCurrentHudOffsetIdx();
    return m_measures.m_hands_offset[0][idx];
}

Fvector& attachable_hud_item::hands_offset_rot()
{
    u8 idx = m_parent_hud_item->GetCurrentHudOffsetIdx();
    return m_measures.m_hands_offset[1][idx];
}

const Fvector& player_hud::attach_rot() const
{
    if (m_attached_items[0])
        return m_attached_items[0]->hands_attach_rot();
    else if (m_attached_items[1])
        return m_attached_items[1]->hands_attach_rot();
    else
        return Fvector().set(0, 0, 0);
}

const Fvector& player_hud::attach_pos() const
{
    if (m_attached_items[0])
        return m_attached_items[0]->hands_attach_pos();
    else if (m_attached_items[1])
        return m_attached_items[1]->hands_attach_pos();
    else
        return Fvector().set(0, 0, 0);
}

void attachable_hud_item::setup_firedeps(firedeps& fd)
{
    update(false);
    // fire point&direction
    if (m_measures.m_prop_flags.test(hud_item_measures::e_fire_point))
    {
        Fmatrix& fire_mat = m_model->LL_GetTransform(m_measures.m_fire_bone);
        fire_mat.transform_tiny(fd.vLastFP, m_measures.m_fire_point_offset);
        m_item_transform.transform_tiny(fd.vLastFP);

        fd.vLastFD.set(0.f, 0.f, 1.f);
        m_item_transform.transform_dir(fd.vLastFD);
        VERIFY(_valid(fd.vLastFD));
        VERIFY(_valid(fd.vLastFD));

        fd.m_FireParticlesXForm.identity();
        fd.m_FireParticlesXForm.k.set(fd.vLastFD);
        Fvector::generate_orthonormal_basis_normalized(fd.m_FireParticlesXForm.k, fd.m_FireParticlesXForm.j, fd.m_FireParticlesXForm.i);
        VERIFY(_valid(fd.m_FireParticlesXForm));
    }

    if (m_measures.m_prop_flags.test(hud_item_measures::e_fire_point2))
    {
        Fmatrix& fire_mat = m_model->LL_GetTransform(m_measures.m_fire_bone2);
        fire_mat.transform_tiny(fd.vLastFP2, m_measures.m_fire_point2_offset);
        m_item_transform.transform_tiny(fd.vLastFP2);
        VERIFY(_valid(fd.vLastFP2));
        VERIFY(_valid(fd.vLastFP2));
    }

    if (m_measures.m_prop_flags.test(hud_item_measures::e_shell_point))
    {
        Fmatrix& fire_mat = m_model->LL_GetTransform(m_measures.m_shell_bone);
        fire_mat.transform_tiny(fd.vLastSP, m_measures.m_shell_point_offset);
        m_item_transform.transform_tiny(fd.vLastSP);
        VERIFY(_valid(fd.vLastSP));
        VERIFY(_valid(fd.vLastSP));
    }
}

void player_hud::update_additional(Fmatrix& trans)
{
    if (m_attached_items[0])
        m_attached_items[0]->update_hud_additional(trans);

    if (m_attached_items[1])
        m_attached_items[1]->update_hud_additional(trans);
}

void attachable_hud_item::update_hud_additional(Fmatrix& trans)
{
    if (m_parent_hud_item)
    {
        m_parent_hud_item->UpdateHudAdditonal(trans);
    }
}

// Обновляем худовую информацию родителя с учётом присоединённых потомков --#SM+#--
void attachable_hud_item::UpdateHudFromChildren(bool bLoadDefaults)
{
    // Грузим дефолты
    if (bLoadDefaults)
        m_measures.load(m_sect_name, m_model);

    // Переопределяем их сведениями из потомков
    xr_vector<attachable_hud_item*>::iterator it = m_child_items.begin();
    while (it != m_child_items.end())
    {
        // На случаи если у потомка есть свои потомки
        (*it)->UpdateHudFromChildren(bLoadDefaults);

        // Обновляем худовые параметры данными потомка
        this->m_measures.merge_measures_params((*it)->m_measures);

        // next
        ++it;
    }
}
