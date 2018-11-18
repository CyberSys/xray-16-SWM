#pragma once

/*********************************/
/***** Худовая модель оружия *****/ //--#SM+#--
/*********************************/

#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "player_hud_motion.h"        //--#SM+#--
#include "player_hud_item_measures.h" //--#SM+#--
#include "firedeps.h"

class player_hud; //--#SM+#--

struct attachable_hud_item
{
    player_hud*       m_parent;
    CHudItem*         m_parent_hud_item;
    shared_str        m_sect_name;
    shared_str        m_def_vis_name; //--#SM+#--
    shared_str        m_cur_vis_name; //--#SM+#--
    IKinematics*      m_model;
    u16               m_attach_place_idx;
    hud_item_measures m_measures;

    // runtime positioning
    Fmatrix m_attach_offset;
    Fmatrix m_item_transform;

    player_hud_motion_container m_hand_motions;

    attachable_hud_item(player_hud* pparent) : m_parent(pparent), m_upd_firedeps_frame(u32(-1)), m_parent_hud_item(NULL) {}
    ~attachable_hud_item();
    void load(const shared_str& sect_name);
    void update(bool bForce);
    void update_hud_additional(Fmatrix& trans);
    void setup_firedeps(firedeps& fd);
    void render();
    void render_item_ui();
    bool render_item_ui_query();
    bool need_renderable();
    void set_bone_visible(const shared_str& bone_name, BOOL bVisibility, BOOL bSilent = FALSE, BOOL bCheckVisibility = FALSE); //--#SM+#--
    void debug_draw_firedeps();

    // hands bind position
    Fvector& hands_attach_pos();
    Fvector& hands_attach_rot();

    // hands runtime offset
    Fvector& hands_offset_pos();
    Fvector& hands_offset_rot();

    // props
    u32  m_upd_firedeps_frame;
    void tune(Ivector values);

    /*--#SM+# Begin--*/
    // anims
    typedef xr_map<attachable_hud_item*, shared_str> ChildMotions;
    struct anim_find_result
    {
        shared_str          anm_alias;
        shared_str          item_motion_name;
        const motion_descr* handsMotionDescr;
        ChildMotions        child_motions_map;
    };
    anim_find_result anim_find(const shared_str& sAnmAliasBase, bool bNoChilds, u8& rnd_idx);

    u32 anim_play_both(anim_find_result anim_to_play, bool bMixIn, bool bNoChilds, float fSpeed = 1.0f, float fStartFromTime = 0.0f);

    u32 anim_play_hands(anim_find_result anim_to_play, bool bMixIn, float fSpeed = 1.0f, float fStartFromTime = 0.0f);

    void anim_play_item(
        shared_str& motion_name, bool bMixIn, bool bNoChilds, float fSpeed = 1.0f, float fStartFromTime = 0.0f, float _fRootStartTime = -1.f);
    void anim_play_item(
        anim_find_result anim_to_play, bool bMixIn, bool bNoChilds, float fSpeed = 1.0f, float fStartFromTime = 0.0f, float _fRootStartTime = -1.f);

    // childs
    xr_vector<attachable_hud_item*> m_child_items;      // Присоединённые потомки
    attachable_hud_item*            m_parent_aitem;     // Ссылка на родительский attachable_item, к которому мы присоединены
    shared_str                      m_socket_bone_name; // У потомков - имя родительской кости, к которой цепляемся при аттаче
    u16 m_socket_bone_id; // У потомков - ID кости родителя, к которой цепляемся при аттаче (высчитывается из m_socket_bone_name)

    attachable_hud_item* FindChildren(const shared_str& sect_name, int* idx = NULL);
    void                 AddChildren(const shared_str& sect_name, bool bRecalcBonesOffsets = true);
    void                 RemoveChildren(const shared_str& sect_name, bool bRecalcBonesOffsets = true);
    void                 UpdateChildrenList(const shared_str& addons_list, bool bShow, bool bUseParser = false);

    void UpdateHudFromChildren(bool bLoadDefaults = true);

    // misc
    void UpdateVisual(shared_str new_visual);

    void calc_child_transform(const Fmatrix& offset, u16 bone_id, Fmatrix& result);

    void ReadBonesOffsetsToHands();

    inline bool IsChildHudItem() { return m_parent_aitem != NULL; }
    /*--#SM+# End--*/
};
