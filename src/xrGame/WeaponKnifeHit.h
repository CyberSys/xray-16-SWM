#pragma once

/*********************************************/
/***** Удар ножом (урон по сфере)  ********/ //--#SM+#--
/*********************************************/

#include "xrEngine/xr_collide_form.h"
#include "Weapon.h"

class CWeaponKnifeHit
{
public:
    CWeaponKnifeHit(const shared_str& section, CWeapon* _owner);
    ~CWeaponKnifeHit();

    void         ReLoad(const shared_str& section);                   // ПереЗагрузить параметры из конфига
    virtual bool KnifeStrike(const Fvector& pos, const Fvector& dir); // Произвести атаку в заданном направлении

#ifdef DEBUG
    virtual void OnRender();
#endif

    CWeapon* m_owner; // Оружие, которому принадлежит этот хиттер

    // Различные параметры атаки
    ALife::EHitType m_eHitType;
    Fvector4        fvHitPower;
    float           fCurrentHit;
    float           fHitImpulse;
    float           fFireDist;

    float    m_HitDistance;
    Fvector3 m_HitSpashDir;
    float    m_HitSplashRadius;

    u32 m_SplashHitsCount;
    u32 m_SplashPerVictimsHCount;

    float m_NextHitDivideFactor;

    float fWallmarkSize;
    u16   knife_material_idx;

    Fvector hit_basis_vector;

private:
    typedef buffer_vector<Fvector> shot_targets_t;

#ifdef DEBUG
    struct dbg_draw_data
    {
        typedef xr_vector<std::pair<Fvector, float>>   spheres_t;
        typedef xr_vector<Fobb>                        obbes_t;
        typedef xr_vector<std::pair<Fvector, Fvector>> lines_t;
        typedef xr_vector<Fvector>                     targets_t;

        spheres_t m_spheres;
        lines_t   m_pick_vectors;
        targets_t m_targets_vectors;
        obbes_t   m_target_boxes;
    };
    dbg_draw_data m_dbg_data;
#endif

    void         MakeShot(Fvector const& pos, Fvector const& dir, float const k_hit = 1.0f);
    void         GetVictimPos(CEntityAlive* victim, Fvector& pos_dest);
    u32          SelectHitsToShot(shot_targets_t& dst_dirs, Fvector const& f_pos);
    bool         SelectBestHitVictim(Fvector const& f_pos, Fmatrix& parent_xform_dest, Fvector& fendpos_dest, Fsphere& query_sphere);
    IGameObject* TryPick(Fvector const& start_pos, Fvector const& dir, float const dist);

    static BOOL RayQueryCallback(collide::rq_result& result, LPVOID this_ptr);

    collide::rq_results m_ray_query_results;
    u16                 m_except_id;
    IGameObject*        m_last_picked_obj;

    typedef xr_vector<ISpatial*>         spartial_base_t;
    typedef buffer_vector<CEntityAlive*> victims_list_t;

    struct victim_bone_data
    {
        CCF_Skeleton::SElement const* m_bone_element;
        u16                           m_victim_id;
        u16                           m_shots_count;
    }; //struct	victim_bone_data

    typedef AssociativeVector<u16, u16>                       victims_hits_count_t;
    typedef buffer_vector<std::pair<victim_bone_data, float>> victims_shapes_list_t;

    spartial_base_t      m_spartial_query_res;
    victims_hits_count_t m_victims_hits_count;

    class victim_filter
    {
    public:
        victim_filter(u16 except_id, Fvector const& pos, float query_distance);
        victim_filter(victim_filter const& copy);
        bool operator()(spartial_base_t::value_type const& left) const;

    private:
        victim_filter& operator=(victim_filter const& copy){};

        u16     m_except_id;
        Fvector m_start_pos;
        float   m_query_distance;
    }; //class victim_filter

    class best_victim_selector
    {
    public:
        best_victim_selector(u16 except_id, Fvector const& pos, float query_distance, spartial_base_t::value_type& dest_result);

        best_victim_selector(best_victim_selector const& copy);
        void operator()(spartial_base_t::value_type const& left);

    private:
        best_victim_selector& operator=(best_victim_selector const& copy){};

        Fvector                      m_start_pos;
        float                        m_min_dist;
        float                        m_query_distance;
        u16                          m_except_id;
        spartial_base_t::value_type& m_dest_result;
    }; //struct best_victim_selector

    static bool shapes_compare_predicate(victims_shapes_list_t::value_type const& left, victims_shapes_list_t::value_type const& right)
    {
        return left.second < right.second;
    }

    static void create_victims_list(spartial_base_t spartial_result, victims_list_t& victims_dest);
    static u32  get_entity_bones_count(CEntityAlive const* entity);
    void        fill_shapes_list(CEntityAlive const* entity, Fvector const& camera_endpos, victims_shapes_list_t& dest_shapes);
    void        make_hit_sort_vectors(Fvector& basis_hit_specific, float& max_dist);
    void        fill_shots_list(victims_shapes_list_t& victims_shapres, Fsphere const& query, shot_targets_t& dest_shots);
};
