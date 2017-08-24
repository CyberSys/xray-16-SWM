#include "stdafx.h"
#include "player_hud.h"
#include "HudItem.h"
#include "ui_base.h"
#include "actor.h"
#include "physic_item.h"
#include "static_cast_checked.hpp"
#include "actoreffector.h"
#include "xrEngine/IGame_Persistent.h"
#include "inventory_item.h" //--#SM+#--
#include "weapon.h" //--#SM+#--

player_hud* g_player_hud = NULL;
Fvector _ancor_pos;
Fvector _wpn_root_pos;

// clang-format off
static const float PITCH_OFFSET_R       = 0.017f;   // Насколько сильно ствол смещается вбок (влево) при вертикальных поворотах камеры	--#SM+#--
static const float PITCH_OFFSET_N       = 0.012f;   // Насколько сильно ствол поднимается\опускается при вертикальных поворотах камеры	--#SM+#--
static const float PITCH_OFFSET_D       = 0.02f;    // Насколько сильно ствол приближается\отдаляется при вертикальных поворотах камеры --#SM+#--
static const float PITCH_LOW_LIMIT      = -PI;      // Минимальное значение pitch при использовании совместно с PITCH_OFFSET_N			--#SM+#--
static const float ORIGIN_OFFSET        = -0.05f;   // Фактор влияния инерции на положение ствола (чем меньше, тем маштабней инерция)	--#SM+#--
static const float ORIGIN_OFFSET_AIM    = -0.03f;   // (Для прицеливания) --#SM+#--
static const float TENDTO_SPEED         = 5.f;      // Скорость нормализации положения ствола --#SM+#--
static const float TENDTO_SPEED_AIM     = 8.f;      // (Для прицеливания) --#SM+#--
// clang-format on

float CalcMotionSpeed(const shared_str& anim_name) // Рудимент, отныне скорость анимации считается в CHudItem --#SM+#--
{
    /*
	if(!IsGameTypeSingle() && (anim_name=="anm_show" || anim_name=="anm_hide") )
		return 2.0f;
	else
		return 1.0f;
	*/

    R_ASSERT2(false, "Outdated function");

    return 1.0f;
}

player_hud_motion* player_hud_motion_container::find_motion(const shared_str& name)
{
    xr_vector<player_hud_motion>::iterator it = m_anims.begin();
    xr_vector<player_hud_motion>::iterator it_e = m_anims.end();
    for (; it != it_e; ++it)
    {
        const shared_str& s = (true) ? (*it).m_alias_name : (*it).m_base_name;
        if (s == name)
            return &(*it);
    }
    return NULL;
}

void player_hud_motion_container::load(IKinematicsAnimated* model, const shared_str& sect) //--#SM+#--
{
    CInifile::Sect& _sect = pSettings->r_section(sect);
    CInifile::SectCIt _b = _sect.Data.begin();
    CInifile::SectCIt _e = _sect.Data.end();
    player_hud_motion* pm = NULL;

    string512 buff;
    MotionID motion_ID;

    for (; _b != _e; ++_b)
    {
        if (strstr(_b->first.c_str(), "anm_") == _b->first.c_str())
        {
            const shared_str& anm = _b->second;
            m_anims.resize(m_anims.size() + 1);
            pm = &m_anims.back();
            //base and alias name
            pm->m_alias_name = _b->first;

            if (_GetItemCount(anm.c_str()) == 1)
            {
                pm->m_base_name = anm;
                pm->m_additional_name = anm;
                pm->m_socket_name = "_";
            }
            else
            {
                if (_GetItemCount(anm.c_str()) == 3)
                {
                    string512 str_item;
                    _GetItem(anm.c_str(), 0, str_item);
                    pm->m_base_name = str_item;

                    _GetItem(anm.c_str(), 1, str_item);
                    pm->m_additional_name = str_item;

                    _GetItem(anm.c_str(), 2, str_item);
                    pm->m_socket_name = str_item;
                }
                else
                {
                    R_ASSERT2(_GetItemCount(anm.c_str()) == 2, anm.c_str());
                    string512 str_item;
                    _GetItem(anm.c_str(), 0, str_item);
                    pm->m_base_name = str_item;

                    _GetItem(anm.c_str(), 1, str_item);
                    pm->m_additional_name = str_item;

                    pm->m_socket_name = "_";
                }
            }

            //and load all motions for it
            if (!pm->m_base_name.equal("_") && !pm->m_base_name.equal("X"))
            {
                for (u32 i = 0; i <= 8; ++i)
                {
                    if (i == 0)
                        xr_strcpy(buff, pm->m_base_name.c_str());
                    else
                        xr_sprintf(buff, "%s%d", pm->m_base_name.c_str(), i);

                    motion_ID = model->ID_Cycle_Safe(buff);
                    if (motion_ID.valid())
                    {
                        pm->m_animations.resize(pm->m_animations.size() + 1);
                        pm->m_animations.back().mid = motion_ID;
                        pm->m_animations.back().name = buff;
#ifdef DEBUG
//						Msg(" alias=[%s] base=[%s] name=[%s]",pm->m_alias_name.c_str(), pm->m_base_name.c_str(), buff);
#endif // #ifdef DEBUG
                    }
                }
                R_ASSERT2(pm->m_animations.size(), make_string("motion not found [%s]", pm->m_base_name.c_str()).c_str());
            }
        }
    }
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

void attachable_hud_item::set_bone_visible(const shared_str& bone_names, BOOL bVisibility, BOOL bSilent, BOOL bCheckVisibility) //--#SM+#--
{
    string128 _bone_name;
    int count = _GetItemCount(bone_names.c_str());
    for (int it = 0; it < count; ++it)
    {
        _GetItem(bone_names.c_str(), it, _bone_name);
        const shared_str& bone_name = _bone_name;

        ///////////////////////////////////////////////////////////////
        u16 bone_id;
        bone_id = m_model->LL_BoneID(bone_name);
        if (bone_id == BI_NONE)
        {
            if (bSilent)
                return;
            R_ASSERT2(
                0, make_string("model [%s] has no bone [%s]", pSettings->r_string(m_sect_name, "item_visual"), bone_name.c_str()).c_str());
        }

        if (bCheckVisibility == FALSE || m_model->LL_GetBoneVisible(bone_id) != bVisibility)
        {
            m_model->LL_SetBoneVisible(bone_id, bVisibility, TRUE);
        }

        /* --#SM+#-- SM_TODO
		Если к кости привязана геометрия с разными текстурами (surface), то при раскрытии кости
		иногда лишь часть геометрии (с одной текстурой) становится видимой, а другая нет.
		Стабильно это можно увидеть на М4A3 (из SWM 3.0), если одеть прицел и снять, то ручка для
		переноски будет раз через раз невидимой. Притом что LL_GetBoneVisible будет возвращать
		true. 
		Как "действенное" решение проблемы была убрана проверка, что кость уже сокрыта + само скрытие худовых
		костей стоит на апдейте. 
		Этот "баг" (?) тянется с оригинал. Другого решения пока не знаю.

		UPD: Возможно причина кроется в том, что в движке одна модель разбивается на несколько составных, по одной на каждый материал (FHierarchyVisual)
		*/
        ///////////////////////////////////////////////////////////////
    }
}

void attachable_hud_item::update(bool bForce) //--#SM+#--
{
    if (!bForce && m_upd_firedeps_frame == Device.dwFrame)
        return;
    bool is_16x9 = UI().is_widescreen();
    bool bUpdateHudData = false;

    if (!!m_measures.m_prop_flags.test(hud_item_measures::e_16x9_mode_now) != is_16x9)
    {
        bUpdateHudData = true;
        m_measures.load(m_sect_name, m_model);
    }

    Fvector ypr = m_measures.m_item_attach[1];
    ypr.mul(PI / 180.f);
    m_attach_offset.setHPB(ypr.x, ypr.y, ypr.z);
    m_attach_offset.translate_over(m_measures.m_item_attach[0]);

    if (!bIsSocketAddon())
        m_parent->calc_transform(m_attach_place_idx, m_attach_offset, m_item_transform);
    else
        m_parent_aitem->calc_socket_transform(m_attach_offset, m_socket_bone_id, m_item_transform);
    m_upd_firedeps_frame = Device.dwFrame;

    IKinematicsAnimated* ka = m_model->dcast_PKinematicsAnimated();
    if (ka)
    {
        ka->UpdateTracks();
        ka->dcast_PKinematics()->CalculateBones_Invalidate();
        ka->dcast_PKinematics()->CalculateBones(TRUE);
    }

    // Обновляем сокет-аддоны
    for (u32 i = 0; i < m_socket_items.size(); i++)
    {
        if (m_socket_items[i])
        {
            if (m_socket_items[i]->m_parent_hud_item != this->m_parent_hud_item)
                m_socket_items[i]->m_parent_hud_item = this->m_parent_hud_item;

            m_socket_items[i]->update(bForce);
        }
    }

    if (bUpdateHudData)
        UpdateHudFromChildren();
}

// Расчитываем позицию нашего худа относительно другого --#SM+#--
void attachable_hud_item::calc_socket_transform(const Fmatrix& offset, u16 bone_id, Fmatrix& result)
{
    Fmatrix ancor_m = m_model->LL_GetTransform(bone_id);
    result.mul(m_item_transform, ancor_m);
    result.mulB_43(offset);
}

void attachable_hud_item::update_hud_additional(Fmatrix& trans)
{
    if (m_parent_hud_item)
    {
        m_parent_hud_item->UpdateHudAdditonal(trans);
    }
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

bool attachable_hud_item::need_renderable() { return m_parent_hud_item->need_renderable(); }

void attachable_hud_item::render() //--#SM+#--
{
    GlobalEnv.Render->set_Transform(&m_item_transform);
    GlobalEnv.Render->add_Visual(m_model->dcast_RenderVisual());

    if (!bIsSocketAddon())
    {
        debug_draw_firedeps();
        m_parent_hud_item->render_hud_mode();
    }

    // Рендерим сокет-аддоны
    for (u32 i = 0; i < m_socket_items.size(); i++)
    {
        if (m_socket_items[i])
        {
            m_socket_items[i]->render();
        }
    }
}

bool attachable_hud_item::render_item_ui_query() //--#SM+#--
{
    bool bRes = m_parent_hud_item->render_item_3d_ui_query();

    // Обновляем UI в сокет-аддонах
    for (u32 i = 0; i < m_socket_items.size(); i++)
    {
        if (m_socket_items[i])
        {
            m_socket_items[i]->render_item_ui_query();
        }
    }

    return bRes;
}

void attachable_hud_item::render_item_ui() //--#SM+#--
{
    m_parent_hud_item->render_item_3d_ui();

    // Обновляем UI в сокет-аддонах
    for (u32 i = 0; i < m_socket_items.size(); i++)
    {
        if (m_socket_items[i])
        {
            m_socket_items[i]->render_item_ui();
        }
    }
}

void hud_item_measures::load(const shared_str& sect_name, IKinematics* K)
{
    bool is_16x9 = UI().is_widescreen();
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
        bone_name = pSettings->r_string(sect_name, "fire_bone");
        m_fire_bone = K->LL_BoneID(bone_name);
        m_fire_point_offset = pSettings->r_fvector3(sect_name, "fire_point");
    }
    else
        m_fire_point_offset.set(0, 0, 0);

    m_prop_flags.set(e_fire_point2, pSettings->line_exist(sect_name, "fire_bone2"));
    if (m_prop_flags.test(e_fire_point2))
    {
        bone_name = pSettings->r_string(sect_name, "fire_bone2");
        m_fire_bone2 = K->LL_BoneID(bone_name);
        m_fire_point2_offset = pSettings->r_fvector3(sect_name, "fire_point2");
    }
    else
        m_fire_point2_offset.set(0, 0, 0);

    m_prop_flags.set(e_shell_point, pSettings->line_exist(sect_name, "shell_bone"));
    if (m_prop_flags.test(e_shell_point))
    {
        bone_name = pSettings->r_string(sect_name, "shell_bone");
        m_shell_bone = K->LL_BoneID(bone_name);
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

    ////////////////////////////////////////////
    //--#SM+# Begin--
    Fvector vDefStrafeValue;
    vDefStrafeValue.set(0.f, 0.f, 0.f);

    // Смещение в стрейфе
    strconcat(sizeof(val_name), val_name, "strafe_hud_offset_pos", _prefix);
    m_strafe_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);
    strconcat(sizeof(val_name), val_name, "strafe_hud_offset_rot", _prefix);
    m_strafe_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);

    // Поворот в стрейфе
    strconcat(sizeof(val_name), val_name, "strafe_aim_hud_offset_pos", _prefix);
    m_strafe_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);
    strconcat(sizeof(val_name), val_name, "strafe_aim_hud_offset_rot", _prefix);
    m_strafe_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, val_name, vDefStrafeValue);

    // Параметры стрейфа
    float fFullStrafeTime = READ_IF_EXISTS(pSettings, r_float, sect_name, "strafe_transition_time", 0.01f);
    float fFullStrafeTime_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "strafe_aim_transition_time", 0.01f);
    bool bStrafeEnabled = READ_IF_EXISTS(pSettings, r_bool, sect_name, "strafe_enabled", false);
    bool bStrafeEnabled_aim = READ_IF_EXISTS(pSettings, r_bool, sect_name, "strafe_aim_enabled", false);

    m_strafe_offset[2][0].set(bStrafeEnabled, fFullStrafeTime, NULL); // normal
    m_strafe_offset[2][1].set(bStrafeEnabled_aim, fFullStrafeTime_aim, NULL); // aim-GL
    //--#SM+# End--
    ////////////////////////////////////////////

    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point") == pSettings->line_exist(sect_name, "fire_bone"), sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "fire_point2") == pSettings->line_exist(sect_name, "fire_bone2"), sect_name.c_str());
    R_ASSERT2(pSettings->line_exist(sect_name, "shell_point") == pSettings->line_exist(sect_name, "shell_bone"), sect_name.c_str());

    //Нужно ли использовать координаты из сокет-аддона, когда он присоединён? --#SM+#--
    bReloadAim = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_aim_position", false);
    bReloadAimGL = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_aim_gl_position", false);
    bReloadInertion = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_inertion_params", false);
    bReloadPitchOfs = READ_IF_EXISTS(pSettings, r_bool, sect_name, "use_new_pitch_offsets", false);

    m_prop_flags.set(e_16x9_mode_now, is_16x9);

    //Загрузка параметров инерции --#SM+# Begin--
    m_inertion_params.m_pitch_offset_r = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_right", PITCH_OFFSET_R);
    m_inertion_params.m_pitch_offset_n = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up", PITCH_OFFSET_N);
    m_inertion_params.m_pitch_offset_d = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_forward", PITCH_OFFSET_D);
    m_inertion_params.m_pitch_low_limit = READ_IF_EXISTS(pSettings, r_float, sect_name, "pitch_offset_up_low_limit", PITCH_LOW_LIMIT);

    m_inertion_params.m_origin_offset = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_offset", ORIGIN_OFFSET);
    m_inertion_params.m_origin_offset_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_origin_aim_offset", ORIGIN_OFFSET_AIM);
    m_inertion_params.m_tendto_speed = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_speed", TENDTO_SPEED);
    m_inertion_params.m_tendto_speed_aim = READ_IF_EXISTS(pSettings, r_float, sect_name, "inertion_tendto_aim_speed", TENDTO_SPEED_AIM);
    //--#SM+# End--

    //Msg("Measures loaded from %s [%d][%d][%d][%d]", sect_name.c_str(), bReloadAim, bReloadAimGL, bReloadInertion, bReloadPitchOfs);
}

// Пробуем взять координаты прицеливания из сокет-аддонов --#SM+#--
void hud_item_measures::reload_aim_from(hud_item_measures& new_measures)
{
    // SM_TODO
    if (new_measures.bReloadAim)
    {
        // aim_hud_offset_pos
        m_hands_offset[0][1].set(new_measures.m_hands_offset[0][1]);
        // aim_hud_offset_rot
        m_hands_offset[1][1].set(new_measures.m_hands_offset[1][1]);
    }

    // SM_TODO
    if (new_measures.bReloadAimGL)
    {
        // gl_hud_offset_pos
        m_hands_offset[0][2].set(new_measures.m_hands_offset[0][2]);
        // gl_hud_offset_rot
        m_hands_offset[1][2].set(new_measures.m_hands_offset[1][2]);
    }

    // Перезагружаем параметры инерции
    if (new_measures.bReloadInertion)
    {
        m_inertion_params.m_origin_offset = new_measures.m_inertion_params.m_origin_offset;
        m_inertion_params.m_origin_offset_aim = new_measures.m_inertion_params.m_origin_offset_aim;
        m_inertion_params.m_tendto_speed = new_measures.m_inertion_params.m_tendto_speed;
        m_inertion_params.m_tendto_speed_aim = new_measures.m_inertion_params.m_tendto_speed_aim;
    }

    // Перезагружаем параметры смещения ствола при разном наклоне камеры
    if (new_measures.bReloadPitchOfs)
    {
        m_inertion_params.m_pitch_offset_r = new_measures.m_inertion_params.m_pitch_offset_r;
        m_inertion_params.m_pitch_offset_n = new_measures.m_inertion_params.m_pitch_offset_n;
        m_inertion_params.m_pitch_offset_d = new_measures.m_inertion_params.m_pitch_offset_d;
        m_inertion_params.m_pitch_low_limit = new_measures.m_inertion_params.m_pitch_low_limit;
    }

    //Msg("Measures reloaded [%d][%d]", new_measures.bReloadAim, new_measures.bReloadAimGL);
}

attachable_hud_item::~attachable_hud_item() //--#SM+#--
{
    IRenderVisual* v = m_model->dcast_RenderVisual();
    GlobalEnv.Render->model_Delete(v);
    m_model = NULL;
    delete_data(m_socket_items);
    //SM_TODO Тудушка старая
    // TODO: но мы не сообщаем родителю о том что удалились, ссылка на нас всё ещё хранится у него <!>
}

void attachable_hud_item::load(const shared_str& sect_name) //--#SM+#--
{
    m_sect_name = sect_name;

    // Visual
    m_model = NULL;
    m_def_vis_name = pSettings->r_string(m_sect_name, "item_visual");
    m_cur_vis_name = NULL;
    UpdateVisual(m_def_vis_name);

    // Measures
    m_attach_place_idx = pSettings->r_u16(sect_name, "attach_place_idx");
    m_measures.load(sect_name, m_model);

    // Socket data
    m_socket_bone_name = READ_IF_EXISTS(pSettings, r_string, sect_name, "socket_bone_name", NULL);
    m_socket_bone_id = NULL;
    m_parent_aitem = NULL;
}

u32 attachable_hud_item::anim_play(const shared_str& anm_name_b,
    BOOL bMixIn,
    const CMotionDef*& md,
    u8& rnd_idx,
    player_hud_motion** ret_anm) //--#SM+#--
{
    float speed = m_parent->GetSpeedModOverridden();
    float fStartTime = m_parent->GetStartTimeOverridden();

    R_ASSERT(strstr(anm_name_b.c_str(), "anm_") == anm_name_b.c_str());
    string256 anim_name_r;
    bool is_16x9 = UI().is_widescreen() && !bIsSocketAddon();
    xr_sprintf(anim_name_r, "%s%s", anm_name_b.c_str(), ((m_attach_place_idx == 1) && is_16x9) ? "_16x9" : "");

    if (bIsSocketAddon())
    {
        // Для сокет-аддонов проверяем, есть ли у них отдельная анимация под худовую секцию предмета, на который они одеты
        string512 anim_name_spec;
        xr_sprintf(anim_name_spec, "%s+%s", anim_name_r, m_parent_aitem->m_sect_name.c_str());

        if (pSettings->line_exist(this->m_sect_name, anim_name_spec))
            xr_sprintf(anim_name_r, "%s", anim_name_spec);
    }

    player_hud_motion* anm = m_hand_motions.find_motion(anim_name_r);

    player_hud_motion* new_anm = NULL;
    LPCSTR new_additional_name = NULL;
    LPCSTR new_socket_name = NULL;

    // Запускаем анимацию в сокет-аддонах, возвращаем новые анимации (если есть) для худа, на который они одеты
    for (u32 i = 0; i < m_socket_items.size(); i++)
    {
        if (m_socket_items[i])
        {
            player_hud_motion* tmp_anm = new player_hud_motion();

            m_socket_items[i]->anim_play(anim_name_r, bMixIn, md, rnd_idx, &tmp_anm);

            if (tmp_anm != NULL)
            {
                // Сохраняем имена анимаций из сокет-аддонов, которые заменят анимации предмета, на который они одеты
                if (tmp_anm->m_socket_name != "_") //-> Не изменять ничего
                    new_socket_name = tmp_anm->m_socket_name.c_str();
                else if (tmp_anm->m_socket_name != "X") //-> Удалить текущую замену (если была)
                    new_socket_name = NULL;

                if (tmp_anm->m_additional_name != "_")
                    new_additional_name = tmp_anm->m_additional_name.c_str();
                else if (tmp_anm->m_additional_name != "X")
                    new_additional_name = NULL;

                if (tmp_anm->m_base_name != "_")
                    new_anm = tmp_anm;
                else if (tmp_anm->m_base_name != "X")
                    new_anm = NULL;
            }

            tmp_anm = NULL;
        }
    }

    // Смотрим, нужно ли перезаписать анимации анимациями сокет-аддонов
    if (new_anm)
    {
        if (anm)
        {
            new_anm->m_additional_name = anm->m_additional_name;
            new_anm->m_socket_name = anm->m_additional_name;
        }

        anm = new_anm;
    }

    if (bIsSocketAddon() && !anm)
    {
        if (ret_anm != NULL)
        {
            xr_delete(*ret_anm);
        }

        // Если у сокет-аддона не нашли требуемой анимации, то пытаемся вызвать anm_idle, потом return
        if (anm_name_b != "anm_idle")
            return this->anim_play("anm_idle", bMixIn, md, rnd_idx);
        else
            return NULL;
    }

    R_ASSERT2(anm, make_string("model [%s] has no motion alias defined [%s]", m_sect_name.c_str(), anim_name_r).c_str());

    if (!bIsSocketAddon())
        R_ASSERT2(anm->m_animations.size(),
            make_string(
                "model [%s] has no motion defined in motion_alias [%s]", pSettings->r_string(m_sect_name, "item_visual"), anim_name_r)
                .c_str());

    if (new_additional_name)
        anm->m_additional_name = new_additional_name;

    if (new_socket_name)
        anm->m_socket_name = new_socket_name;

    if (ret_anm != NULL)
    {
        xr_delete(*ret_anm);
        *ret_anm = anm;
    }

    rnd_idx = 0;

    if (anm->m_animations.size() > 0)
        (u8) Random.randI(anm->m_animations.size());

    const motion_descr& M = anm->m_animations[rnd_idx];

    u32 ret = NULL;

    if (!bIsSocketAddon()) // Анимацию рук из сокет-аддонов не вызываем
        ret = g_player_hud->anim_play(m_attach_place_idx, M.mid, bMixIn, md, speed);

    if (m_model->dcast_PKinematicsAnimated())
    {
        IKinematicsAnimated* ka = m_model->dcast_PKinematicsAnimated();

        shared_str item_anm_name;

        if (!bIsSocketAddon())
        {
            if (anm->m_base_name != anm->m_additional_name)
                item_anm_name = anm->m_additional_name;
            else
                item_anm_name = M.name;
        }
        else
        {
            item_anm_name = anm->m_socket_name;
        }

        MotionID M2 = ka->ID_Cycle_Safe(item_anm_name);
        if (!M2.valid())
            M2 = ka->ID_Cycle_Safe("idle");
        else if (bDebug)
            Msg("playing item animation [%s]", item_anm_name.c_str());

        R_ASSERT3(M2.valid(), "model has no motion [idle] ", pSettings->r_string(m_sect_name, "item_visual"));

        u16 root_id = m_model->LL_GetBoneRoot();
        CBoneInstance& root_binst = m_model->LL_GetBoneInstance(root_id);
        root_binst.set_callback_overwrite(TRUE);
        root_binst.mTransform.identity();

        u16 pc = ka->partitions().count();
        for (u16 pid = 0; pid < pc; ++pid)
        {
            CBlend* B = ka->PlayCycle(pid, M2, bMixIn);
            R_ASSERT(B);
            B->speed *= speed;

            // Устанавливаем время начала анимации
            if ((fStartTime >= 0.0f) || (fStartTime < -1.f))
            { // Выставляем время в точных значениях
                clamp(fStartTime, 0.0f, B->timeTotal);
                B->timeCurrent = abs(fStartTime);
            }
            else
            { // Выставляем время в процентных значениях
                B->timeCurrent = abs(fStartTime) * B->timeTotal;
            }
        }

        m_model->CalculateBones_Invalidate();
    }

    R_ASSERT2(m_parent_hud_item, "parent hud item is NULL");
    CPhysicItem& parent_object = m_parent_hud_item->object();
    //R_ASSERT2		(parent_object, "object has no parent actor");
    //CObject*		parent_object = static_cast_checked<CObject*>(&m_parent_hud_item->object());

    if (IsGameTypeSingle() && parent_object.H_Parent() == Level().CurrentControlEntity() && !bIsSocketAddon())
    {
        CActor* current_actor = static_cast_checked<CActor*>(Level().CurrentControlEntity());
        VERIFY(current_actor);
        CEffectorCam* ec = current_actor->Cameras().GetCamEffector(eCEWeaponAction);

        if (NULL == ec)
        {
            string_path ce_path;
            string_path anm_name;
            strconcat(sizeof(anm_name), anm_name, "camera_effects\\weapon\\", M.name.c_str(), ".anm");
            if (FS.exist(ce_path, "$game_anims$", anm_name))
            {
                CAnimatorCamEffector* e = new CAnimatorCamEffector();
                e->SetType(eCEWeaponAction);
                e->SetHudAffect(false);
                e->SetCyclic(false);
                e->Start(anm_name);
                current_actor->Cameras().AddCamEffector(e);
            }
        }
    }

    return ret;
}

// Функция поиска установленного сокет-аддона --#SM+#--
attachable_hud_item* attachable_hud_item::FindChildren(const shared_str& sect_name, int* idx)
{
    if (idx != NULL)
        *idx = 0;

    for (u32 i = 0; i < m_socket_items.size(); i++)
    {
        attachable_hud_item* item = m_socket_items[i];
        if (item != NULL && item->m_sect_name.equal(sect_name))
        {
            if (idx != NULL)
                *idx = i;
            return item;
        }
    }
    return NULL;
}

// Функция установки сокет-аддона --#SM+#--
void attachable_hud_item::AddChildren(const shared_str& sect_name)
{
    // Проверка что такого сокет-аддона ещё нету в списке
    if (FindChildren(sect_name) != NULL)
        return;

    // Добавляем новый
    attachable_hud_item* res = new attachable_hud_item(this->m_parent);
    res->load(sect_name);
    res->m_hand_motions.load(m_parent->get_model(), sect_name);

    res->m_parent_aitem = this;
    res->m_parent = this->m_parent;
    res->m_parent_hud_item = this->m_parent_hud_item;

    if (res->m_socket_bone_name != NULL)
    {
        res->m_socket_bone_id = m_model->LL_BoneID(res->m_socket_bone_name);
        if (res->m_socket_bone_id == BI_NONE)
            res->m_socket_bone_id = NULL;
    }

    // Если он анимирован, то инициализируем
    if (res->m_model->dcast_PKinematicsAnimated())
    {
        IKinematicsAnimated* ka = res->m_model->dcast_PKinematicsAnimated();

        MotionID M2 = ka->ID_Cycle_Safe("idle");

        R_ASSERT3(M2.valid(), "model has no motion [idle] ", pSettings->r_string(res->m_sect_name, "item_visual"));

        u16 root_id = res->m_model->LL_GetBoneRoot();
        CBoneInstance& root_binst = res->m_model->LL_GetBoneInstance(root_id);
        root_binst.set_callback_overwrite(TRUE);
        root_binst.mTransform.identity();

        u16 pc = ka->partitions().count();
        for (u16 pid = 0; pid < pc; ++pid)
        {
            CBlend* B = ka->PlayCycle(pid, M2, false);
            R_ASSERT(B);
        }

        res->m_model->CalculateBones_Invalidate();
    }

    // Добавляем в список аттачей
    m_socket_items.push_back(res);

    // Пересчитываем смещения костей
    if (m_parent->m_bOffsetsRecalcEnabled)
        m_parent->RecalculateBonesOffsets();
    else
        m_parent->m_bNeed2RecalcOffsets = true;

    // Переопределяем параметры худа
    UpdateHudFromChildren();
}

// Функция снятия сокет-аддона по его секции --#SM+#--
void attachable_hud_item::RemoveChildren(const shared_str& sect_name)
{
    int idx = 0;
    attachable_hud_item* item = FindChildren(sect_name, &idx);

    if (item != NULL)
    {
        xr_delete(item);
        m_socket_items.erase(m_socket_items.begin() + idx);
        UpdateHudFromChildren();

        // Пересчитываем смещения костей
        if (m_parent->m_bOffsetsRecalcEnabled)
            m_parent->RecalculateBonesOffsets();
        else
            m_parent->m_bNeed2RecalcOffsets = true;
    }
}

// Обновляет присоединёные сокет-аддоны --#SM+#--
void attachable_hud_item::UpdateChildrenList(const shared_str& addons_list, bool bShow, bool bUseParser)
{
    m_parent->m_bOffsetsRecalcEnabled = false; //--> Чтобы не пересчитывать смещения после каждого аттача
    m_parent->m_bNeed2RecalcOffsets = false;

    if (bUseParser) // Это список секций
    {
        if (addons_list.size())
        {
            string128 _hudName;
            LPCSTR _addons_list = addons_list.c_str();

            int count = _GetItemCount(_addons_list);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(_addons_list, it, _hudName);
                if (bShow)
                    this->AddChildren(_hudName);
                else
                    this->RemoveChildren(_hudName);
            }
        }
    }
    else // Это одна секция
    {
        if (bShow)
            this->AddChildren(addons_list);
        else
            this->RemoveChildren(addons_list);
    }

    if (m_parent->m_bNeed2RecalcOffsets)
        m_parent->RecalculateBonesOffsets();

    m_parent->m_bOffsetsRecalcEnabled = true;
}

// Обновляем худовую информацию родителя инфой из сокет-аддонов --#SM+#--
void attachable_hud_item::UpdateHudFromChildren(bool bLoadDefaults)
{
    // Грузим дефолты
    if (bLoadDefaults)
        m_measures.load(m_sect_name, m_model);

    // Обновляем их сведениями из аддонов
    xr_vector<attachable_hud_item*>::iterator it = m_socket_items.begin();
    while (it != m_socket_items.end())
    {
        // На случаи если к сокет-аддону тоже что-то присоединено
        (*it)->UpdateHudFromChildren(bLoadDefaults);

        // Загружаем новые координаты прицеливания для худа (если есть)
        this->m_measures.reload_aim_from((*it)->m_measures);

        // next
        ++it;
    }
}

// Загружаем и обновляем визуал сокет-аддона --#SM+#--
void attachable_hud_item::UpdateVisual(shared_str new_visual)
{
    if (new_visual == NULL)
        new_visual = m_def_vis_name;

    if (m_cur_vis_name.equal(new_visual) == false)
    {
        // Удаляем старую
        if (m_model != NULL)
        {
            IRenderVisual* v = m_model->dcast_RenderVisual();
            GlobalEnv.Render->model_Delete(v);
        }

        // Устанавливаем новую
        m_cur_vis_name = new_visual;
        m_model = smart_cast<IKinematics*>(GlobalEnv.Render->model_Create(m_cur_vis_name.c_str()));

        // Считываем число костей с привязкой к числу патронов
        CWeapon::ReadMaxBulletBones(m_model);
    }
}

// Считываем смещения для костей из худовых секций сокет-аддонов, и регистрируем эту информацию в текущей модели рук --#SM+#--
void attachable_hud_item::ReadBonesOffsetsToHands()
{
    xr_vector<attachable_hud_item*>::iterator it = m_socket_items.begin();
    while (it != m_socket_items.end())
    {
        // На случаи если к сокет-аддону тоже что-то присоединено
        (*it)->ReadBonesOffsetsToHands();

        // Ищем смещения в худовой секции
        u32 lines_count = pSettings->line_count((*it)->m_sect_name);
        for (u32 i = 0; i < lines_count; ++i)
        {
            LPCSTR line_name = NULL;
            LPCSTR line_value = NULL;
            pSettings->r_line((*it)->m_sect_name, i, &line_name, &line_value);

            if (line_name && xr_strlen(line_name))
            {
                if (NULL != strstr(line_name, "hand_bone_offset"))
                {
                    string128 str_bone;
                    _GetItem(line_name, 1, str_bone, '|'); //--> Считываем имя кости
                    string128 str_pos;
                    _GetItem(line_value, 0, str_pos, '|'); //--> Считываем позицию
                    string128 str_rot;
                    _GetItem(line_value, 1, str_rot, '|'); //--> Считываем поворот

                    // Передаём данные в модель
                    KinematicsABT::additional_bone_transform offsets;
                    Fvector vPos, vRot;

                    offsets.m_bone_id = m_parent->get_model()->dcast_PKinematics()->LL_BoneID(str_bone);
                    sscanf(str_pos, "%f,%f,%f", &vPos.x, &vPos.y, &vPos.z);
                    sscanf(str_rot, "%f,%f,%f", &vRot.x, &vRot.y, &vRot.z);
                    vRot.mul(PI / 180.f); //--> Преобразуем углы в радианы

                    offsets.setRotLocal(vRot);
                    offsets.setPosOffset(vPos);

                    m_parent->get_model()->LL_AddTransformToBone(offsets);
                }
            }
        }

        // next
        ++it;
    }
}

player_hud::player_hud()
{
    m_model = NULL;
    m_attached_items[0] = NULL;
    m_attached_items[1] = NULL;
    m_fStartTime = DEFAULT_ANIM_START_TIME_OVERRIDEN; //--#SM+#--
    m_fSpeedMod = 1.f; //--#SM+#--
    m_bOffsetsRecalcEnabled = true; //--#SM+#--
    m_bNeed2RecalcOffsets = false; //--#SM+#--
    m_transform.identity();
}

player_hud::~player_hud()
{
    IRenderVisual* v = m_model->dcast_RenderVisual();
    GlobalEnv.Render->model_Delete(v);
    m_model = NULL;

    xr_vector<attachable_hud_item*>::iterator it = m_pool.begin();
    xr_vector<attachable_hud_item*>::iterator it_e = m_pool.end();
    for (; it != it_e; ++it)
    {
        attachable_hud_item* a = *it;
        xr_delete(a);
    }
    m_pool.clear();
}

void player_hud::load(const shared_str& player_hud_sect)
{
    if (player_hud_sect == m_sect_name)
        return;
    bool b_reload = (m_model != NULL);
    if (m_model)
    {
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GlobalEnv.Render->model_Delete(v);
    }

    m_sect_name = player_hud_sect;
    const shared_str& model_name = pSettings->r_string(player_hud_sect, "visual");
    m_model = smart_cast<IKinematicsAnimated*>(GlobalEnv.Render->model_Create(model_name.c_str()));

    CInifile::Sect& _sect = pSettings->r_section(player_hud_sect);
    CInifile::SectCIt _b = _sect.Data.begin();
    CInifile::SectCIt _e = _sect.Data.end();
    for (; _b != _e; ++_b)
    {
        if (strstr(_b->first.c_str(), "ancor_") == _b->first.c_str())
        {
            const shared_str& _bone = _b->second;
            m_ancors.push_back(m_model->dcast_PKinematics()->LL_BoneID(_bone));
        }
    }

    //	Msg("hands visual changed to[%s] [%s] [%s]", model_name.c_str(), b_reload?"R":"", m_attached_items[0]?"Y":"");

    if (!b_reload)
    {
        m_model->PlayCycle("hand_idle_doun");
    }
    else
    {
        if (m_attached_items[1])
            m_attached_items[1]->m_parent_hud_item->on_a_hud_attach();

        if (m_attached_items[0])
            m_attached_items[0]->m_parent_hud_item->on_a_hud_attach();
    }

    m_bOffsetsRecalcEnabled = true; //--#SM+#--
    RecalculateBonesOffsets(); //--#SM+#--

    m_model->dcast_PKinematics()->CalculateBones_Invalidate();
    m_model->dcast_PKinematics()->CalculateBones(TRUE);
}

bool player_hud::render_item_ui_query()
{
    bool res = false;
    if (m_attached_items[0])
        res |= m_attached_items[0]->render_item_ui_query();

    if (m_attached_items[1])
        res |= m_attached_items[1]->render_item_ui_query();

    return res;
}

void player_hud::render_item_ui()
{
    if (m_attached_items[0])
        m_attached_items[0]->render_item_ui();

    if (m_attached_items[1])
        m_attached_items[1]->render_item_ui();
}

void player_hud::render_hud() //--#SM+#--
{
    if (!m_attached_items[0] && !m_attached_items[1])
        return;

    bool b_r0 = (m_attached_items[0] && m_attached_items[0]->need_renderable());
    bool b_r1 = (m_attached_items[1] && m_attached_items[1]->need_renderable());

    if (!b_r0 && !b_r1)
        return;
    if (g_actor)
        m_model->dcast_RenderVisual()->getVisData().obj_data = g_actor->Visual()->getVisData().obj_data;

    GlobalEnv.Render->set_Transform(&m_transform);
    GlobalEnv.Render->add_Visual(m_model->dcast_RenderVisual()); //Шейдерную инфу от модели ГГ переносим на модель рук

    if (m_attached_items[0])
    {
        m_attached_items[0]->m_model->dcast_RenderVisual()->getVisData().obj_data = m_attached_items[0]
                                                                                        ->m_parent_hud_item->item()
                                                                                        .cast_game_object()
                                                                                        ->Visual()
                                                                                        ->getVisData()
                                                                                        .obj_data; //Аналогично для оружия\детектора
        m_attached_items[0]->render();
    }

    if (m_attached_items[1])
    {
        m_attached_items[1]->m_model->dcast_RenderVisual()->getVisData().obj_data = m_attached_items[1]
                                                                                        ->m_parent_hud_item->item()
                                                                                        .cast_game_object()
                                                                                        ->Visual()
                                                                                        ->getVisData()
                                                                                        .obj_data; //Аналогично для оружия\детектора
        m_attached_items[1]->render();
    }
}

#include "xrCore/Animation/Motion.hpp"

u32 player_hud::motion_length(const shared_str& anim_name, const shared_str& hud_name, const CMotionDef*& md)
{
    float speed = 1.f; //--#SM+#-- Не изменять здесь скорость <!>
    attachable_hud_item* pi = create_hud_item(hud_name);
    player_hud_motion* pm = pi->m_hand_motions.find_motion(anim_name);
    if (!pm)
        return 100; // ms TEMPORARY
    R_ASSERT2(pm, make_string("hudItem model [%s] has no motion with alias [%s]", hud_name.c_str(), anim_name.c_str()).c_str());
    return motion_length(pm->m_animations[0].mid, md, speed);
}

u32 player_hud::motion_length(const MotionID& M, const CMotionDef*& md, float speed) //--#SM+#--
{
    md = m_model->LL_GetMotionDef(M);
    VERIFY(md);

    // Вызов этой функции потенциально обозначает, что мы уже определились какую анимацию играть.
    // Поэтому здесь и производим сброс переопределённого времени начала и скорости анимации.
    float fStartTime = GetStartTimeOverridden();
    SetAnimStartTime(DEFAULT_ANIM_START_TIME_OVERRIDEN);

    speed = GetSpeedModOverridden(); // Может быть отрицательной <!>
    SetAnimSpeedMod(1.f);

    if (speed < 0.0f)
        R_ASSERT2(fStartTime != DEFAULT_ANIM_START_TIME_OVERRIDEN,
            make_string("When use negative speed [%f], SetAnimStartTime(<time>) is required [%f]", speed, fStartTime).c_str());

    if (md->flags & esmStopAtEnd)
    {
        CMotion* motion = m_model->LL_GetRootMotion(M);
        float motion_length = motion->GetLength();
        if ((fStartTime >= 0.0f) || (fStartTime < -1.f))
        { // Выставляем время в точных значениях
            clamp(fStartTime, 0.0f, motion_length);
        }
        else
        { // Выставляем время в процентных значениях
            fStartTime = (-fStartTime) * motion_length;
        }

        u32 anim_time = 0;
        if (speed >= 0.0f)
            anim_time = iFloor(0.5f + 1000.f * (motion_length - fStartTime) / (md->Dequantize(md->speed) * speed));
        else
            anim_time = iFloor(0.5f + 1000.f * (fStartTime) / (md->Dequantize(md->speed) * abs(speed)));

        return anim_time;
    }
    return 0;
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

void player_hud::update(const Fmatrix& cam_trans)
{
    Fmatrix trans = cam_trans;
    update_inertion(trans);
    update_additional(trans);

    Fvector ypr = attach_rot();
    ypr.mul(PI / 180.f);
    m_attach_offset.setHPB(ypr.x, ypr.y, ypr.z);
    m_attach_offset.translate_over(attach_pos());
    m_transform.mul(trans, m_attach_offset);
    // insert inertion here

    m_model->UpdateTracks();
    m_model->dcast_PKinematics()->CalculateBones_Invalidate();
    m_model->dcast_PKinematics()->CalculateBones(TRUE);

    if (m_attached_items[0])
        m_attached_items[0]->update(true);

    if (m_attached_items[1])
        m_attached_items[1]->update(true);
}

void player_hud::SetAnimStartTime(float fStartTime) //--#SM+#--
{
    if (fStartTime < DEFAULT_ANIM_START_TIME_OVERRIDEN)
        fStartTime = DEFAULT_ANIM_START_TIME_OVERRIDEN;

    m_fStartTime = fStartTime;
}

void player_hud::SetAnimSpeedMod(float fSpeedMod) //--#SM+#--
{
    if (fSpeedMod == 0.0f)
        fSpeedMod = 1.f;
    m_fSpeedMod = fSpeedMod;
}

u32 player_hud::anim_play(u16 part, const MotionID& M, BOOL bMixIn, const CMotionDef*& md, float speed)
{
    u16 part_id = u16(-1);
    if (attached_item(0) && attached_item(1))
        part_id = m_model->partitions().part_id((part == 0) ? "right_hand" : "left_hand");

    u16 pc = m_model->partitions().count();
    for (u16 pid = 0; pid < pc; ++pid)
    {
        if (pid == 0 || pid == part_id || part_id == u16(-1))
        {
            CBlend* B = m_model->PlayCycle(pid, M, bMixIn);
            R_ASSERT(B);
            B->speed *= speed;

            // Устанавливаем время начала анимации --#SM+#--
            float fStartTime = GetStartTimeOverridden();
            if ((fStartTime >= 0.0f) || (fStartTime < -1.f))
            { // Выставляем время в точных значениях
                clamp(fStartTime, 0.0f, B->timeTotal);
                B->timeCurrent = abs(fStartTime);
            }
            else
            { // Выставляем время в процентных значениях
                B->timeCurrent = abs(fStartTime) * B->timeTotal;
            }
        }
    }
    m_model->dcast_PKinematics()->CalculateBones_Invalidate();

    return motion_length(M, md, speed);
}

void player_hud::update_additional(Fmatrix& trans)
{
    if (m_attached_items[0])
        m_attached_items[0]->update_hud_additional(trans);

    if (m_attached_items[1])
        m_attached_items[1]->update_hud_additional(trans);
}

void player_hud::update_inertion(Fmatrix& trans) //--#SM+#--
{
    if (inertion_allowed())
    {
        attachable_hud_item* pMainHud = m_attached_items[0];

        Fmatrix xform;
        Fvector& origin = trans.c;
        xform = trans;

        static Fvector st_last_dir = {0, 0, 0};

        // load params
        hud_item_measures::inertion_params inertion_data;
        if (pMainHud != NULL)
        { // Загружаем параметры инерции из основного худа
            inertion_data.m_pitch_offset_r = pMainHud->m_measures.m_inertion_params.m_pitch_offset_r;
            inertion_data.m_pitch_offset_n = pMainHud->m_measures.m_inertion_params.m_pitch_offset_n;
            inertion_data.m_pitch_offset_d = pMainHud->m_measures.m_inertion_params.m_pitch_offset_d;
            inertion_data.m_pitch_low_limit = pMainHud->m_measures.m_inertion_params.m_pitch_low_limit;
            inertion_data.m_origin_offset = pMainHud->m_measures.m_inertion_params.m_origin_offset;
            inertion_data.m_origin_offset_aim = pMainHud->m_measures.m_inertion_params.m_origin_offset_aim;
            inertion_data.m_tendto_speed = pMainHud->m_measures.m_inertion_params.m_tendto_speed;
            inertion_data.m_tendto_speed_aim = pMainHud->m_measures.m_inertion_params.m_tendto_speed_aim;
        }
        else
        { // Загружаем дефолтные параметры инерции
            inertion_data.m_pitch_offset_r = PITCH_OFFSET_R;
            inertion_data.m_pitch_offset_n = PITCH_OFFSET_N;
            inertion_data.m_pitch_offset_d = PITCH_OFFSET_D;
            inertion_data.m_pitch_low_limit = PITCH_LOW_LIMIT;
            inertion_data.m_origin_offset = ORIGIN_OFFSET;
            inertion_data.m_origin_offset_aim = ORIGIN_OFFSET_AIM;
            inertion_data.m_tendto_speed = TENDTO_SPEED;
            inertion_data.m_tendto_speed_aim = TENDTO_SPEED_AIM;
        }

        // calc difference
        Fvector diff_dir;
        diff_dir.sub(xform.k, st_last_dir);

        // clamp by PI_DIV_2
        Fvector last;
        last.normalize_safe(st_last_dir);
        float dot = last.dotproduct(xform.k);
        if (dot < EPS)
        {
            Fvector v0;
            v0.crossproduct(st_last_dir, xform.k);
            st_last_dir.crossproduct(xform.k, v0);
            diff_dir.sub(xform.k, st_last_dir);
        }

        // tend to forward
        float _tendto_speed, _origin_offset;
        if (pMainHud != NULL && pMainHud->m_parent_hud_item->GetCurrentHudOffsetIdx() > 0)
        { // Худ в режиме "Прицеливание"
            float factor = pMainHud->m_parent_hud_item->GetInertionFactor();
            _tendto_speed = inertion_data.m_tendto_speed_aim - (inertion_data.m_tendto_speed_aim - inertion_data.m_tendto_speed) * factor;
            _origin_offset =
                inertion_data.m_origin_offset_aim - (inertion_data.m_origin_offset_aim - inertion_data.m_origin_offset) * factor;
        }
        else
        { // Худ в режиме "От бедра"
            _tendto_speed = inertion_data.m_tendto_speed;
            _origin_offset = inertion_data.m_origin_offset;
        }

        // Фактор силы инерции
        if (pMainHud != NULL)
        {
            float power_factor = pMainHud->m_parent_hud_item->GetInertionPowerFactor();
            _tendto_speed *= power_factor;
            _origin_offset *= power_factor;
        }

        st_last_dir.mad(diff_dir, _tendto_speed * Device.fTimeDelta);
        origin.mad(diff_dir, _origin_offset);

        // pitch compensation
        float pitch = angle_normalize_signed(xform.k.getP());

        if (pMainHud != NULL)
            pitch *= pMainHud->m_parent_hud_item->GetInertionFactor();

        // Отдаление\приближение
        origin.mad(xform.k, -pitch * inertion_data.m_pitch_offset_d);

        // Сдвиг в противоположную часть экрана
        origin.mad(xform.i, -pitch * inertion_data.m_pitch_offset_r);

        // Подьём\опускание
        clamp(pitch, inertion_data.m_pitch_low_limit, PI);
        origin.mad(xform.j, -pitch * inertion_data.m_pitch_offset_n);
    }
}

attachable_hud_item* player_hud::create_hud_item(const shared_str& sect)
{
    xr_vector<attachable_hud_item*>::iterator it = m_pool.begin();
    xr_vector<attachable_hud_item*>::iterator it_e = m_pool.end();
    for (; it != it_e; ++it)
    {
        attachable_hud_item* itm = *it;
        if (itm->m_sect_name == sect)
            return itm;
    }
    attachable_hud_item* res = new attachable_hud_item(this);
    res->load(sect);
    res->m_hand_motions.load(m_model, sect);
    m_pool.push_back(res);

    return res;
}

bool player_hud::allow_activation(CHudItem* item)
{
    if (m_attached_items[1])
        return m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);
    else
        return true;
}

void player_hud::attach_item(CHudItem* item)
{
    attachable_hud_item* pi = create_hud_item(item->HudSection());
    int item_idx = pi->m_attach_place_idx;

    if (m_attached_items[item_idx] != pi)
    {
        if (m_attached_items[item_idx])
            m_attached_items[item_idx]->m_parent_hud_item->on_b_hud_detach();

        m_attached_items[item_idx] = pi;
        pi->m_parent_hud_item = item;

        if (item_idx == 0 && m_attached_items[1])
            m_attached_items[1]->m_parent_hud_item->CheckCompatibility(item);

        item->on_a_hud_attach();
    }
    pi->m_parent_hud_item = item;
}

void player_hud::detach_item_idx(u16 idx)
{
    if (NULL == attached_item(idx))
        return;

    m_attached_items[idx]->m_parent_hud_item->on_b_hud_detach();

    m_attached_items[idx]->m_parent_hud_item = NULL;
    m_attached_items[idx] = NULL;

    if (idx == 1 && attached_item(0))
    {
        u16 part_idR = m_model->partitions().part_id("right_hand");
        u32 bc = m_model->LL_PartBlendsCount(part_idR);
        for (u32 bidx = 0; bidx < bc; ++bidx)
        {
            CBlend* BR = m_model->LL_PartBlend(part_idR, bidx);
            if (!BR)
                continue;

            MotionID M = BR->motionID;

            u16 pc = m_model->partitions().count();
            for (u16 pid = 0; pid < pc; ++pid)
            {
                if (pid != part_idR)
                {
                    CBlend* B = m_model->PlayCycle(pid, M, TRUE); //this can destroy BR calling UpdateTracks !
                    if (BR->blend_state() != CBlend::eFREE_SLOT)
                    {
                        u16 bop = B->bone_or_part;
                        *B = *BR;
                        B->bone_or_part = bop;
                    }
                }
            }
        }
    }
    else if (idx == 0 && attached_item(1))
    {
        OnMovementChanged(mcAnyMove);
    }
}

void player_hud::detach_item(CHudItem* item)
{
    if (NULL == item->HudItemData())
        return;
    u16 item_idx = item->HudItemData()->m_attach_place_idx;

    if (m_attached_items[item_idx] == item->HudItemData())
    {
        detach_item_idx(item_idx);
    }
}

void player_hud::calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result)
{
    Fmatrix ancor_m = m_model->dcast_PKinematics()->LL_GetTransform(m_ancors[attach_slot_idx]);
    result.mul(m_transform, ancor_m);
    result.mulB_43(offset);
}

bool player_hud::inertion_allowed()
{
    attachable_hud_item* hi = m_attached_items[0];
    if (hi)
    {
        bool res = (hi->m_parent_hud_item->HudInertionEnabled() && hi->m_parent_hud_item->HudInertionAllowed());
        return res;
    }
    return true;
}

void player_hud::OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd)
{
    if (cmd == 0)
    {
        if (m_attached_items[0])
        {
            if (m_attached_items[0]->m_parent_hud_item->GetState() == CHUDState::eIdle)
                m_attached_items[0]->m_parent_hud_item->PlayAnimIdle();
        }
        if (m_attached_items[1])
        {
            if (m_attached_items[1]->m_parent_hud_item->GetState() == CHUDState::eIdle)
                m_attached_items[1]->m_parent_hud_item->PlayAnimIdle();
        }
    }
    else
    {
        if (m_attached_items[0])
            m_attached_items[0]->m_parent_hud_item->OnMovementChanged(cmd);

        if (m_attached_items[1])
            m_attached_items[1]->m_parent_hud_item->OnMovementChanged(cmd);
    }
}

// Пересчитать смещения костей с учётом всех текущих сокет-аддонов
void player_hud::RecalculateBonesOffsets() //--#SM+#--
{
    m_bNeed2RecalcOffsets = false;

    // Очищаем все текущие смещения
    m_model->LL_ClearAdditionalTransform(BI_NONE);

    // Считываем новые
    if (m_attached_items[0])
        m_attached_items[0]->ReadBonesOffsetsToHands();
    if (m_attached_items[1])
        m_attached_items[1]->ReadBonesOffsetsToHands();
}
