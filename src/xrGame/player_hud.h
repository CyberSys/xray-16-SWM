#pragma once

/******************************/
/***** Худовая модель рук *****/ //--#SM+#--
/******************************/

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "player_hud_item.h" //--#SM+#--
#include "actor_defs.h"

class CHudItem;
class CMotionDef;

class player_hud
{
public:
    player_hud();
    ~player_hud();

    void load(const shared_str& model_name);
    void load_default() { load("actor_hud_05"); };
    void update(const Fmatrix& trans);

    void render_hud();
    void render_item_ui();
    bool render_item_ui_query();

    const shared_str&    section_name() const { return m_sect_name; }
    attachable_hud_item* create_hud_item(const shared_str& sect);

    attachable_hud_item* attached_item(u16 item_idx) { return m_attached_items[item_idx]; };
    bool                 allow_activation(CHudItem* item);
    void                 attach_item(CHudItem* item);
    void                 detach_item_idx(u16 idx);
    void                 detach_item(CHudItem* item);
    void                 detach_all_items()
    {
        m_attached_items[0] = NULL;
        m_attached_items[1] = NULL;
    };

    void destroy_all_attached_items(); //--#SM+#--

    void tune(Ivector values);
    void calc_transform(u16 attach_slot_idx, const Fmatrix& offset, Fmatrix& result);
    void RecalculateBonesOffsets(); //--#SM+#--

    u32 anim_play(u16 part, const MotionID& sAnmAlias, BOOL bMixIn, float fSpeed = 1.0f, float fStartFromTime = 0.0f); //--#SM+#--

    CMotionDef* motion_def(const MotionID& M);                                                                                             //--#SM+#--
    u32         motion_length_without_childs(const shared_str& sAnmAlias, const shared_str& sHudName, float fSpeed, float fStartFromTime); //--#SM+#--
    u32         motion_length_without_childs(const shared_str& sAnmAlias, attachable_hud_item* pItem, float fSpeed, float fStartFromTime); //--#SM+#--
    u32         motion_length(const MotionID& M, float fSpeed, float fStartFromTime);                                                      //--#SM+#--

    void OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd);

    // Получить m_model --#SM+#--
    IC IKinematicsAnimated* get_model() { return m_model; }
    // Получить m_model_twin --#SM+#--
    IC IKinematicsAnimated* get_model_twin() { return m_model_twin; }

    // Получаем attachable_hud_item из нужного слота --#SM+#--
    IC attachable_hud_item* get_attached_item(int IDX)
    {
        if (IDX < 2)
            return m_attached_items[IDX];

        return NULL;
    }

    // Установить коэфицент интервала для моделей рук (используется в Fade In\Out эффекте при смещениях костей рук через LL_AddTransformToBone) --#SM+#--
    IC void SetAdditionalTransformCurIntervalFactor(float fKI)
    {
        if (m_model != nullptr)
        {
            m_model->LL_SetAdditionalTransformCurIntervalFactor(fKI);
        }

        if (m_model_twin != nullptr)
        {
            m_model_twin->LL_SetAdditionalTransformCurIntervalFactor(fKI);
        }
    }

    bool inertion_allowed();

private:
    void update_inertion(Fmatrix& trans);
    void update_additional(Fmatrix& trans);

    shared_str m_sect_name;

    Fmatrix                         m_attach_offset;
    Fmatrix                         m_transform;
    IKinematicsAnimated*            m_model;
    IKinematicsAnimated*            m_model_twin; //--#SM+#--
    xr_vector<u16>                  m_ancors;
    attachable_hud_item*            m_attached_items[2];
    xr_vector<attachable_hud_item*> m_pool;
};

extern player_hud* g_player_hud;
