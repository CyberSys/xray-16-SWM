/******************************/
/***** Худовая модель рук *****/ //--#SM+#--
/******************************/

#include "stdafx.h"
#include "player_hud.h"
#include "HudItem.h"
#include "Actor.h"
#include "inventory_item.h" //--#SM+#--
#include "xrCore/Animation/Motion.hpp"

player_hud* g_player_hud = NULL;
Fvector     _ancor_pos;
Fvector     _wpn_root_pos;

player_hud::player_hud()
{
    m_model             = NULL;
    m_attached_items[0] = NULL;
    m_attached_items[1] = NULL;
    m_transform.identity();
}

player_hud::~player_hud()
{
    IRenderVisual* v = m_model->dcast_RenderVisual();
    GlobalEnv.Render->model_Delete(v);
    m_model = NULL;

    xr_vector<attachable_hud_item*>::iterator it   = m_pool.begin();
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
        m_model = NULL;
    }

    m_sect_name                  = player_hud_sect;
    const shared_str& model_name = pSettings->r_string(player_hud_sect, "visual");
    m_model                      = smart_cast<IKinematicsAnimated*>(GlobalEnv.Render->model_Create(model_name.c_str()));

    CInifile::Sect&   _sect = pSettings->r_section(player_hud_sect);
    CInifile::SectCIt _b    = _sect.Data.begin();
    CInifile::SectCIt _e    = _sect.Data.end();
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

    RecalculateBonesOffsets(); //--#SM+#--

    m_model->dcast_PKinematics()->CalculateBones_Invalidate();
    m_model->dcast_PKinematics()->CalculateBones(TRUE);
}

attachable_hud_item* player_hud::create_hud_item(const shared_str& sect)
{
    xr_vector<attachable_hud_item*>::iterator it   = m_pool.begin();
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
    GlobalEnv.Render->add_Visual(m_model->dcast_RenderVisual());

    // Информацию для шейдеров от модели ГГ переносим на худовую модель рук
    if (m_attached_items[0])
    {
        m_attached_items[0]->m_model->dcast_RenderVisual()->getVisData().obj_data =
            m_attached_items[0]->m_parent_hud_item->item().cast_game_object()->Visual()->getVisData().obj_data;
        m_attached_items[0]->render();
    }

    if (m_attached_items[1])
    {
        m_attached_items[1]->m_model->dcast_RenderVisual()->getVisData().obj_data =
            m_attached_items[1]->m_parent_hud_item->item().cast_game_object()->Visual()->getVisData().obj_data;
        m_attached_items[1]->render();
    }
}

CMotionDef* player_hud::motion_def(const MotionID& M) //--#SM+#--
{
    CMotionDef* md = m_model->LL_GetMotionDef(M);
    VERIFY(md);
    return md;
}

u32 player_hud::motion_length_without_childs(const shared_str& sAnmAlias, const shared_str& sHudName, float fSpeed, float fStartFromTime) //--#SM+#--
{
    // У нас нет доступа к предмету "в руках" - поэтому создаём "новый" по его секции
    attachable_hud_item* pi = create_hud_item(sHudName);

    return motion_length_without_childs(sAnmAlias, pi, fSpeed, fStartFromTime);
}

u32 player_hud::motion_length_without_childs(const shared_str& sAnmAlias, attachable_hud_item* pItem, float fSpeed, float fStartFromTime) //--#SM+#--
{
    u8                                    rnd_idx;
    attachable_hud_item::anim_find_result motion_data = pItem->anim_find(sAnmAlias, true, rnd_idx);

    return motion_length(motion_data.handsMotionDescr->mid, fSpeed, fStartFromTime);
}

u32 player_hud::motion_length(const MotionID& M, float fSpeed, float fStartFromTime) //--#SM+#--
{
    CMotionDef* md = motion_def(M);
    if (md->flags & esmStopAtEnd)
    {
        CMotion* motion        = m_model->LL_GetRootMotion(M);
        float    motion_length = motion->GetLength();
        fStartFromTime         = CalculateMotionStartSeconds(fStartFromTime, motion_length);

        if (fSpeed >= 0.0f)
            return iFloor(0.5f + 1000.f * (motion_length - fStartFromTime) / (md->Dequantize(md->speed) * fSpeed));
        else
            return iFloor(0.5f + 1000.f * (fStartFromTime) / (md->Dequantize(md->speed) * abs(fSpeed)));
    }
    return 0;
}

u32 player_hud::anim_play(u16 part, const MotionID& sAnmAlias, BOOL bMixIn, float fSpeed, float fStartFromTime) //--#SM+#--
{
    u16 part_id = u16(-1);
    if (attached_item(0) && attached_item(1))
        part_id = m_model->partitions().part_id((part == 0) ? "right_hand" : "left_hand");

    u16 pc = m_model->partitions().count();
    for (u16 pid = 0; pid < pc; ++pid)
    {
        if (pid == 0 || pid == part_id || part_id == u16(-1))
        {
            CBlend* B = m_model->PlayCycle(pid, sAnmAlias, bMixIn);
            ModifyBlendParams(B, fSpeed, fStartFromTime); //--#SM+#--
        }
    }
    m_model->dcast_PKinematics()->CalculateBones_Invalidate();

    return motion_length(sAnmAlias, fSpeed, fStartFromTime); //--#SM+#--
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
    attachable_hud_item* pi       = create_hud_item(item->HudSection());
    int                  item_idx = pi->m_attach_place_idx;

    if (m_attached_items[item_idx] != pi)
    {
        if (m_attached_items[item_idx])
            m_attached_items[item_idx]->m_parent_hud_item->on_b_hud_detach();

        m_attached_items[item_idx] = pi;
        pi->m_parent_hud_item      = item;

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
    m_attached_items[idx]                    = NULL;

    if (idx == 1 && attached_item(0))
    {
        u16 part_idR = m_model->partitions().part_id("right_hand");
        u32 bc       = m_model->LL_PartBlendsCount(part_idR);
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
                        u16 bop         = B->bone_or_part;
                        *B              = *BR;
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

void player_hud::calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result)
{
    Fmatrix ancor_m = m_model->dcast_PKinematics()->LL_GetTransform(m_ancors[attach_slot_idx]);
    result.mul(m_transform, ancor_m);
    result.mulB_43(offset);
}

// Пересчитать кастомные смещения для костей худовой модели --#SM+#--
void player_hud::RecalculateBonesOffsets() //
{
    // Очищаем все текущие смещения
    m_model->LL_ClearAdditionalTransform(BI_NONE);

    // Считываем новые
    if (m_attached_items[0])
        m_attached_items[0]->ReadBonesOffsetsToHands();
    if (m_attached_items[1])
        m_attached_items[1]->ReadBonesOffsetsToHands();
}
