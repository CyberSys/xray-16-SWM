/*********************************************/
/***** Удар ножом (урон по сфере)  ********/ //--#SM+#--
/*********************************************/

#include "stdafx.h"

#include "xrEngine/gamemtllib.h"
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "../Include/xrRender/Kinematics.h"
#include "level.h"
#include "level_bullet_manager.h"
#include "Actor.h"
#include "Entity.h"
#include "ActorEffector.h"
#include "WeaponKnife.h"

#ifdef DEBUG
#include "debug_renderer.h"
extern BOOL g_bDrawBulletHit;
#endif //#ifdef DEBUG

#define DEF_KNIFE_MATERIAL_NAME "objects\\knife"

// Конструктор
CWeaponKnifeHit::CWeaponKnifeHit(const shared_str& section, CWeapon* _owner)
{
    m_owner = _owner; // Оружие, которому принадлежит этот хитер
    ReLoad(section);  // Загрузка параметров из конфига
}

// Деструктор
CWeaponKnifeHit::~CWeaponKnifeHit() {}

// ПереЗагрузка параметров из конфига
void CWeaponKnifeHit::ReLoad(const shared_str& section)
{
    LPCSTR sMatIDX     = READ_IF_EXISTS(pSettings, r_string, section, "wm_material", DEF_KNIFE_MATERIAL_NAME);
    knife_material_idx = GMLib.GetMaterialIdx(sMatIDX); // Материал ножа

    fWallmarkSize = pSettings->r_float(section, "wm_size"); // Размер волмарка

    // Параметры радиуса атаки.
    m_HitSpashDir     = pSettings->r_fvector3(section, "splash_direction");
    m_HitDistance     = pSettings->r_float(section, "spash_dist");
    m_HitSplashRadius = pSettings->r_float(section, "spash_radius");

    m_SplashHitsCount        = pSettings->r_u32(section, "splash_hits_count");
    m_SplashPerVictimsHCount = pSettings->r_u32(section, "splash_pervictim_hcount");

    m_NextHitDivideFactor = pSettings->r_float(section, "splash_hit_divide_factor");

#ifdef DEBUG
    m_dbg_data.m_pick_vectors.reserve(m_SplashHitsCount);
#endif

    // Параметры направления сплэша
    hit_basis_vector = pSettings->r_fvector3(section, "hit_basis_vector");

    // Параметры хита\импульса атаки.
    string32   buffer;
    shared_str s_sHitPower = pSettings->r_string_wb(section, "hit_power");

    fvHitPower[egdMaster] = (float)atof(_GetItem(*s_sHitPower, 0, buffer)); //первый параметр - это хит для уровня игры мастер
    fvHitPower[egdNovice] = fvHitPower[egdStalker] = fvHitPower[egdVeteran] =
        fvHitPower[egdMaster]; //изначально параметры для других уровней сложности такие же

    int num_game_diff_param = _GetItemCount(*s_sHitPower); // узнаём колличество параметров для хитов
    if (num_game_diff_param > 1)                           // если задан второй параметр хита
    {
        fvHitPower[egdVeteran] = (float)atof(_GetItem(*s_sHitPower, 1, buffer)); //то вычитываем его для уровня ветерана
    }
    if (num_game_diff_param > 2) // если задан третий параметр хита
    {
        fvHitPower[egdStalker] = (float)atof(_GetItem(*s_sHitPower, 2, buffer)); //то вычитываем его для уровня сталкера
    }
    if (num_game_diff_param > 3) // если задан четвёртый параметр хита
    {
        fvHitPower[egdNovice] = (float)atof(_GetItem(*s_sHitPower, 3, buffer)); //то вычитываем его для уровня новичка
    }

    m_eHitType  = ALife::g_tfString2HitType(pSettings->r_string(section, "hit_type"));
    fHitImpulse = pSettings->r_float(section, "hit_impulse");
    fFireDist   = m_HitDistance + m_HitSplashRadius;
}

// Пробуем нанести удар в заданном направлении, возвращает true если потенциально попали во что-то (объект или стену)
bool CWeaponKnifeHit::KnifeStrike(const Fvector& pos, const Fvector& dir)
{
    // Определяем силу удара
    if (m_owner->ParentIsActor()) // Если из оружия стреляет игрок
    {
        if (GameID() == eGameIDSingle)
        {
            fCurrentHit = fvHitPower[g_SingleGameDifficulty];
        }
        else
        {
            fCurrentHit = fvHitPower[egdMaster];
        }
    }
    else
    {
        fCurrentHit = fvHitPower[egdMaster];
    }

    // Если в перекрестии кто то есть - то мы уже нашли цель
    IGameObject* real_victim = TryPick(pos, dir, m_HitDistance);
    if (real_victim)
    {
        MakeShot(pos, dir);
        return true;
    }

    // Смотрим попадает-ли кто-нибудь в сферу атаки
    shot_targets_t dest_hits(_alloca(sizeof(Fvector) * m_SplashHitsCount), m_SplashHitsCount);
    if (SelectHitsToShot(dest_hits, pos))
    {
#ifdef DEBUG
        m_dbg_data.m_targets_vectors.clear();
        std::copy(dest_hits.begin(), dest_hits.end(), std::back_insert_iterator<dbg_draw_data::targets_t>(m_dbg_data.m_targets_vectors));
#endif
        float tmp_k_hit = 1.0f;
        for (shot_targets_t::const_iterator i = dest_hits.begin(), ie = dest_hits.end(); i != ie; ++i)
        {
            Fvector shot_dir;
            shot_dir.set(*i).sub(pos).normalize();
            MakeShot(pos, shot_dir, tmp_k_hit);
            tmp_k_hit *= m_NextHitDivideFactor;
        }

        return dest_hits.size() > 0;
    }

    // Просто стреляем вперёд
    MakeShot(pos, dir);

    // И проверяем попадём ли мы (скорее всего) в статику или нет
    collide::rq_result l_rq;
    return (bool)Level().ObjectSpace.RayPick(pos, dir, fFireDist, collide::rqtStatic, l_rq, NULL);
}

// SM_TODO: Причесать код, удалить лишнее
/////////////////////////////////////////////////////

void CWeaponKnifeHit::MakeShot(Fvector const& pos, Fvector const& dir, float const k_hit)
{
    CCartridge cartridge;
    cartridge.param_s.buckShot = 1;
    cartridge.param_s.impair   = 1.0f;
    cartridge.param_s.kDisp    = 1.0f;
    cartridge.param_s.kHit     = k_hit;
    //.	cartridge.param_s.kCritical		= 1.0f;
    cartridge.param_s.kImpulse = 1.0f;
    cartridge.param_s.kAP      = EPS_L;
    cartridge.m_flags.set(CCartridge::cfTracer, FALSE);
    cartridge.m_flags.set(CCartridge::cfRicochet, FALSE);
    cartridge.param_s.fWallmarkSize = fWallmarkSize;
    cartridge.bullet_material_idx   = knife_material_idx;

    /*
	CWeaponKnife*	pKnife			= smart_cast<CWeaponKnife*>(m_owner);
	if (pKnife)
	{
		// Этот код был в оригинальном ноже, вроде не критичен и возможно старый костыль
		while(pKnife->m_magazine.size() < 2)	pKnife->m_magazine.push_back(cartridge);
		pKnife->iAmmoElapsed					= pKnife->m_magazine.size();
	}
	*/

    // clang-format off
	bool SendHit						= m_owner->SendHitAllowed(m_owner->H_Parent());	// Для MP
	Level().BulletManager().AddBullet(	pos, 
										dir, 
										2000.f, 
										fCurrentHit, 
										fHitImpulse, 
										m_owner->H_Parent()->ID(), 
										m_owner->ID(), 
										m_eHitType, 
										fFireDist, 
										cartridge, 
										1.f, 
										SendHit);
    // clang-format on
}

static bool intersect(Fsphere const& bone, Fsphere const& query) { return bone.P.distance_to_sqr(query.P) < _sqr(bone.R + query.R); }

static bool intersect(Fobb bone, Fsphere const& query)
{
    Fmatrix transform;
    bone.m_halfsize.add(Fvector().set(query.R, query.R, query.R));
    bone.xform_full(transform);
    transform.invert();

    Fvector new_position;
    transform.transform_tiny(new_position, query.P);

    return (new_position.x >= -1.f) && (new_position.y >= -1.f) && (new_position.z >= -1.f) && (new_position.x <= 1.f) && (new_position.y <= 1.f) &&
           (new_position.z <= 1.f);
}

static bool intersect(Fcylinder const& bone, Fsphere const& query)
{
    Fvector const bone2query     = Fvector().sub(query.P, bone.m_center);
    float const   axe_projection = bone2query.dotproduct(bone.m_direction);
    float const   half_height    = bone.m_height / 2.f;
    if (_abs(axe_projection) > half_height + query.R)
        return false;

    VERIFY(bone2query.square_magnitude() >= _sqr(axe_projection));
    float const axe_projection2_sqr = bone2query.square_magnitude() - _sqr(axe_projection);
    if (axe_projection2_sqr > _sqr(bone.m_radius + query.R))
        return false;

    if (_abs(axe_projection) <= half_height)
        return true;

    if (axe_projection2_sqr <= _sqr(bone.m_radius))
        return true;

    Fvector const center_direction = Fvector(bone.m_direction).mul(axe_projection >= 0.f ? 1.f : -1.f);
    Fvector const circle_center    = Fvector(bone.m_center).mad(center_direction, half_height);
    Fvector const circle2sphere    = Fvector().sub(query.P, circle_center);
    float const   distance2plane   = circle2sphere.dotproduct(center_direction);
    VERIFY(distance2plane > 0.f);
    VERIFY(_sqr(query.R) >= _sqr(distance2plane));
    float const   circle_radius        = _sqrt(_sqr(query.R) - _sqr(distance2plane));
    Fvector const sphere_circle_center = Fvector(query.P).mad(center_direction, -distance2plane);
    float const   distance2center_sqr  = circle_center.distance_to_sqr(sphere_circle_center);
    return distance2center_sqr <= _sqr(bone.m_radius + circle_radius);
}

void CWeaponKnifeHit::GetVictimPos(CEntityAlive* victim, Fvector& pos_dest)
{
    /*VERIFY(victim);
	IKinematics*	tmp_kinem	= smart_cast<IKinematics*>(victim->Visual());
	u16 hit_bone_id				= tmp_kinem->LL_BoneID(m_SplashHitBone);
	if (hit_bone_id != BI_NONE)
	{
		Fmatrix			tmp_matrix;
		tmp_kinem->Bone_GetAnimPos	(tmp_matrix, hit_bone_id, u8(-1), true);
		pos_dest.set(tmp_matrix.c);
		Fmatrix	& tmp_xform			= victim->XFORM();
		tmp_xform.transform_tiny	(pos_dest);
	} else
	{
		Fbox const & tmp_box = tmp_kinem->GetBox();
		Fvector tmp_fake_vec;
		tmp_box.get_CD(pos_dest, tmp_fake_vec);
		pos_dest.add(victim->Position());
	}
	
	CBoneData& tmp_bone_data	= tmp_kinem->LL_GetData(hit_bone_id);
	Fmatrix	& tmp_xform			= victim->XFORM();
	CBoneInstance &bi			= tmp_kinem->LL_GetBoneInstance();

	switch (tmp_bone_data.shape.type)
	{
	case SBoneShape::stBox:
		{
			pos_dest = tmp_bone_data.shape.box.m_translate;
			break;
		};
	case SBoneShape::stSphere:
		{
			pos_dest = tmp_bone_data.shape.sphere.P;
		}break;
	case SBoneShape::stCylinder:
		{
			pos_dest = tmp_bone_data.shape.cylinder.m_center;
		}break;
	};//switch (tmp_bone_data.shape.type)
	tmp_xform.transform_tiny(pos_dest);
	bi.mTransform.transform_tiny(pos_dest);*/
}

u32 CWeaponKnifeHit::get_entity_bones_count(CEntityAlive const* entity)
{
    VERIFY(entity);
    if (!entity)
        return 0;
    IKinematics* tmp_kinem = smart_cast<IKinematics*>(entity->Visual());
    if (!tmp_kinem)
        return 0;

    IKinematics::accel* tmp_accel = tmp_kinem->LL_Bones();
    if (!tmp_accel)
        return 0;

    return tmp_accel->size();
};

void CWeaponKnifeHit::fill_shapes_list(CEntityAlive const* entity, Fvector const& camera_endpos, victims_shapes_list_t& dest_shapes)
{
    VERIFY(entity);
    if (!entity)
        return;

    CCF_Skeleton* tmp_skeleton = smart_cast<CCF_Skeleton*>(entity->GetCForm());
    if (!tmp_skeleton)
        return;

    Fvector basis_vector;
    float   max_dist;
    make_hit_sort_vectors(basis_vector, max_dist);
    Fvector camendpos2;
    camendpos2.set(camera_endpos).mul(basis_vector);

    CCF_Skeleton::ElementVec const& elems_vec = tmp_skeleton->_GetElements();
    for (CCF_Skeleton::ElementVec::const_iterator i = elems_vec.begin(), ie = elems_vec.end(); i != ie; ++i)
    {
        Fvector tmp_pos;
        i->center(tmp_pos);
        tmp_pos.mul(basis_vector);
        //float basis_proj = tmp_pos.dotproduct(basis_vector);
        float bone_dist = tmp_pos.distance_to_sqr(camendpos2);
        if (bone_dist < max_dist)
        {
            victim_bone_data tmp_bone_data;
            tmp_bone_data.m_bone_element = &(*i);
            tmp_bone_data.m_victim_id    = entity->ID();
            tmp_bone_data.m_shots_count  = 0;
            dest_shapes.push_back(std::make_pair(tmp_bone_data, bone_dist));
        }
    }
}

void CWeaponKnifeHit::fill_shots_list(victims_shapes_list_t& victims_shapres, Fsphere const& query, shot_targets_t& dest_shots)
{
    m_victims_hits_count.clear();
    for (victims_shapes_list_t::iterator i = victims_shapres.begin(), ie = victims_shapres.end(); i != ie; ++i)
    {
        if (dest_shots.capacity() <= dest_shots.size())
            return;

        bool    intersect_res = false;
        Fvector target_pos;
#ifdef DEBUG
        m_dbg_data.m_target_boxes.clear();
#endif
        victim_bone_data& curr_bone = i->first;
        switch (curr_bone.m_bone_element->type)
        {
        case SBoneShape::stBox:
        {
            Fobb    tmp_obb;
            Fmatrix tmp_xform;
            if (!tmp_xform.invert_b(curr_bone.m_bone_element->b_IM))
            {
                VERIFY2(false, "invalid bone xform");
                break;
            }
            tmp_obb.xform_set(tmp_xform);
            tmp_obb.m_halfsize = curr_bone.m_bone_element->b_hsize;
#ifdef DEBUG
            m_dbg_data.m_target_boxes.push_back(tmp_obb);
#endif
            intersect_res = intersect(tmp_obb, query);
            break;
        };
        case SBoneShape::stSphere:
        {
            intersect_res = intersect(curr_bone.m_bone_element->s_sphere, query);
            break;
        }
        break;
        case SBoneShape::stCylinder:
        {
            intersect_res = intersect(curr_bone.m_bone_element->c_cylinder, query);
            break;
        }
        break;
        }; //switch (tmp_bone_data.shape.type)*/
        if (intersect_res)
        {
            victims_hits_count_t::iterator tmp_vhits_iter = m_victims_hits_count.find(curr_bone.m_victim_id);
            if (m_SplashPerVictimsHCount && (tmp_vhits_iter == m_victims_hits_count.end()))
            {
                m_victims_hits_count.insert(std::make_pair(curr_bone.m_victim_id, u16(1)));
            }
            else if (m_SplashPerVictimsHCount && (tmp_vhits_iter->second < m_SplashPerVictimsHCount))
            {
                ++tmp_vhits_iter->second;
            }
            else if (m_SplashPerVictimsHCount)
            {
                continue;
            }
            curr_bone.m_bone_element->center(target_pos);
            dest_shots.push_back(target_pos);
        }
    }
}

void CWeaponKnifeHit::create_victims_list(spartial_base_t spartial_result, victims_list_t& victims_dest)
{
    for (spartial_base_t::const_iterator i = spartial_result.begin(), ie = spartial_result.end(); i != ie; ++i)
    {
        IGameObject* tmp_obj = (*i)->dcast_GameObject();
        VERIFY(tmp_obj);
        if (!tmp_obj)
            continue;
        CEntityAlive* tmp_entity = smart_cast<CEntityAlive*>(tmp_obj);
        if (!tmp_entity)
            continue;
        VERIFY(victims_dest.capacity() > victims_dest.size());
        victims_dest.push_back(tmp_entity);
    }
}

void CWeaponKnifeHit::make_hit_sort_vectors(Fvector& basis_hit_specific, float& max_dist)
{
    //basis_hit_specific1.set(-1.f, 0.f, 0.f);
    basis_hit_specific.set(0.f, 1.f, 0.f);
    max_dist = 0.2;
}

static float const spartial_prefetch_radius = 2.0f;

u32 CWeaponKnifeHit::SelectHitsToShot(shot_targets_t& dst_dirs, Fvector const& f_pos)
{
    Fmatrix parent_xform;
    Fvector fendpos;
    Fsphere query_sphere;

    dst_dirs.clear();
    if (!SelectBestHitVictim(f_pos, parent_xform, fendpos, query_sphere))
        return 0;

    victims_list_t tmp_victims_list(_alloca(m_spartial_query_res.size() * sizeof(CEntityAlive*)), m_spartial_query_res.size());

    create_victims_list(m_spartial_query_res, tmp_victims_list);

    u32 summ_shapes_count = 0;
    for (victims_list_t::const_iterator i = tmp_victims_list.begin(), ie = tmp_victims_list.end(); i != ie; ++i)
    {
        summ_shapes_count += get_entity_bones_count(*i);
    }
    victims_shapes_list_t tmp_shapes_list(_alloca(summ_shapes_count * sizeof(victims_shapes_list_t::value_type)), summ_shapes_count);

    parent_xform.transform_dir(hit_basis_vector);
    hit_basis_vector.normalize();

    for (victims_list_t::const_iterator i = tmp_victims_list.begin(), ie = tmp_victims_list.end(); i != ie; ++i)
    {
        fill_shapes_list(*i, fendpos, tmp_shapes_list);
    }
    std::sort(tmp_shapes_list.begin(), tmp_shapes_list.end(), shapes_compare_predicate);
    fill_shots_list(tmp_shapes_list, query_sphere, dst_dirs);

    return static_cast<u32>(dst_dirs.size());
}

bool CWeaponKnifeHit::SelectBestHitVictim(Fvector const& f_pos, Fmatrix& parent_xform, Fvector& fendpos_dest, Fsphere& query_sphere)
{
    CActor* tmp_parent = smart_cast<CActor*>(m_owner->H_Parent());
    VERIFY(tmp_parent);
    if (!tmp_parent)
        return false;

    if (m_owner->GetHUDmode())
        tmp_parent->Cameras().hud_camera_Matrix(parent_xform);
    else
        return false;

    parent_xform.transform_dir(m_HitSpashDir);
    fendpos_dest.set(f_pos).mad(m_HitSpashDir, m_HitDistance);
    query_sphere.set(fendpos_dest, m_HitSplashRadius);

#ifdef DEBUG
    m_dbg_data.m_spheres.push_back(std::make_pair(fendpos_dest, m_HitSplashRadius));
#endif

    m_spartial_query_res.clear();
    g_SpatialSpace->q_sphere(m_spartial_query_res, 0, STYPE_COLLIDEABLE, fendpos_dest, m_HitSplashRadius);

    if (!m_spartial_query_res.empty())
    {
        spartial_base_t::value_type tmp_best_victim = NULL;
        best_victim_selector        tmp_selector(tmp_parent->ID(), fendpos_dest, spartial_prefetch_radius, tmp_best_victim);
        std::for_each(m_spartial_query_res.begin(), m_spartial_query_res.end(), tmp_selector);
        m_spartial_query_res.clear();
        if (tmp_best_victim)
            m_spartial_query_res.push_back(tmp_best_victim);
    }
    else
    {
        victim_filter tmp_filter(tmp_parent->ID(), fendpos_dest, spartial_prefetch_radius);
        m_spartial_query_res.erase(std::remove_if(m_spartial_query_res.begin(), m_spartial_query_res.end(), tmp_filter), m_spartial_query_res.end());
    }
    return !m_spartial_query_res.empty();
}

BOOL CWeaponKnifeHit::RayQueryCallback(collide::rq_result& result, LPVOID this_ptr)
{
    CWeaponKnifeHit* me = static_cast<CWeaponKnifeHit*>(this_ptr);
    if (result.O && (result.O->ID() != me->m_except_id))
    {
        me->m_last_picked_obj = result.O;
        return FALSE; //first hit
    }
    return TRUE;
}

IGameObject* CWeaponKnifeHit::TryPick(Fvector const& start_pos, Fvector const& dir, float const dist)
{
    collide::ray_defs tmp_rdefs(start_pos, dir, dist, CDB::OPT_FULL_TEST, collide::rqtObject);
    m_ray_query_results.r_clear();
    m_last_picked_obj = NULL;
    VERIFY(m_owner->H_Parent());
    m_except_id = m_owner->H_Parent()->ID();
    Level().ObjectSpace.RayQuery(m_ray_query_results, tmp_rdefs, &CWeaponKnifeHit::RayQueryCallback, static_cast<LPVOID>(this), NULL, NULL);
    return m_last_picked_obj;
}

//predicates implementation

CWeaponKnifeHit::victim_filter::victim_filter(u16 except_id, Fvector const& pos, float query_distance)
    : m_except_id(except_id), m_start_pos(pos), m_query_distance(query_distance)
{
}

CWeaponKnifeHit::victim_filter::victim_filter(victim_filter const& copy)
    : m_except_id(copy.m_except_id), m_start_pos(copy.m_start_pos), m_query_distance(copy.m_query_distance)
{
}

bool CWeaponKnifeHit::victim_filter::operator()(spartial_base_t::value_type const& left) const
{
    IGameObject* const tmp_obj = left->dcast_GameObject();
    VERIFY(tmp_obj);
    if (!tmp_obj)
        return true;

    if (tmp_obj->ID() == m_except_id)
        return true;

    CEntityAlive* const tmp_actor = smart_cast<CEntityAlive*>(tmp_obj);
    if (!tmp_actor)
        return true;

    Fvector obj_pos;
    tmp_actor->Center(obj_pos);

    Fvector     tmp_dir  = Fvector(obj_pos).sub(m_start_pos);
    float const tmp_dist = tmp_dir.magnitude();

    if (tmp_dist > m_query_distance)
        return true;

    return false;
}

CWeaponKnifeHit::best_victim_selector::best_victim_selector(
    u16 except_id, Fvector const& pos, float const query_distance, spartial_base_t::value_type& dest_result)
    : m_except_id(except_id), m_start_pos(pos), m_query_distance(query_distance), m_dest_result(dest_result)
{
    m_dest_result = NULL;
}

CWeaponKnifeHit::best_victim_selector::best_victim_selector(best_victim_selector const& copy)
    : m_min_dist(copy.m_min_dist), m_except_id(copy.m_except_id), m_start_pos(copy.m_start_pos), m_query_distance(copy.m_query_distance),
      m_dest_result(copy.m_dest_result)
{
}

void CWeaponKnifeHit::best_victim_selector::operator()(spartial_base_t::value_type const& left)
{
    IGameObject* const tmp_obj = left->dcast_GameObject();
    VERIFY(tmp_obj);
    if (!tmp_obj)
        return;

    if (tmp_obj->ID() == m_except_id)
        return;

    CEntityAlive* const tmp_actor = smart_cast<CEntityAlive*>(tmp_obj);
    if (!tmp_actor)
        return;

    Fvector obj_pos;
    tmp_actor->Center(obj_pos);
    //m_owner->GetVictimPos	(tmp_actor, obj_pos);

    Fvector     tmp_dir  = Fvector(obj_pos).sub(m_start_pos);
    float const tmp_dist = tmp_dir.magnitude();
    tmp_dir.normalize();

    if (tmp_dist > m_query_distance)
        return;

    if (!m_dest_result || (m_min_dist > tmp_dist))
    {
        m_dest_result = left;
        m_min_dist    = tmp_dist;
        return;
    }
}

#ifdef DEBUG
void CWeaponKnifeHit::OnRender()
{
    CDebugRenderer& renderer = Level().debug_renderer();
    if (g_bDrawBulletHit)
    {
        for (dbg_draw_data::spheres_t::const_iterator i = m_dbg_data.m_spheres.begin(), ie = m_dbg_data.m_spheres.end(); i != ie; ++i)
        {
            float   sc_r   = i->second;
            Fmatrix sphere = Fmatrix().scale(sc_r, sc_r, sc_r);
            sphere.c       = i->first;
            renderer.draw_ellipse(sphere, color_xrgb(100, 255, 0));
        }
    }
    float hit_power = 1.f;
    for (dbg_draw_data::targets_t::const_iterator i = m_dbg_data.m_targets_vectors.begin(), ie = m_dbg_data.m_targets_vectors.end(); i != ie; ++i)
    {
        Fmatrix sphere = Fmatrix().scale(0.05f, 0.05f, 0.05f);
        sphere.c       = *i;
        u8 hit_color   = u8(255 * hit_power);
        hit_power *= m_NextHitDivideFactor;
        renderer.draw_ellipse(sphere, color_xrgb(hit_color, 50, 0));
    }

    for (dbg_draw_data::obbes_t::const_iterator i = m_dbg_data.m_target_boxes.begin(), ie = m_dbg_data.m_target_boxes.end(); i != ie; ++i)
    {
        Fmatrix tmp_matrix;
        tmp_matrix.set(i->m_rotate.i, i->m_rotate.j, i->m_rotate.k, i->m_translate);
        renderer.draw_obb(tmp_matrix, i->m_halfsize, color_xrgb(0, 255, 0));
    }
}
#endif
