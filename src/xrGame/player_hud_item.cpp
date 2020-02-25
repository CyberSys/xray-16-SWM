/*********************************/
/***** Худовая модель оружия *****/ //--#SM+#--
/*********************************/

#include "StdAfx.h"
#include "player_hud.h"
#include "xrUICore/ui_base.h"
#include "static_cast_checked.hpp"
#include "HudItem.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "Weapon.h" //--#SM+#--

extern BOOL g_LogHUDAnims; //--#SM+#--

attachable_hud_item::~attachable_hud_item() //--#SM+#--
{
    IRenderVisual* v = m_model->dcast_RenderVisual();
    GEnv.Render->model_Delete(v);
    m_model = NULL;
    delete_data(m_child_items);
}

void attachable_hud_item::load(const shared_str& sect_name) //--#SM+#--
{
    m_sect_name = sect_name;

    // Visual
    m_model        = NULL;
    m_def_vis_name = pSettings->r_string(m_sect_name, "item_visual");
    m_cur_vis_name = NULL;
    UpdateVisual(m_def_vis_name);

    // Motions
    m_def_motion   = READ_IF_EXISTS(pSettings, r_string, m_sect_name, "default_motion", HUD_ITEM_DEF_MOTION);

    // Measures
    m_attach_place_idx = pSettings->r_u16(m_sect_name, "attach_place_idx");
    m_measures.load(m_sect_name, m_model);

    // Socket data
    m_socket_bone_name = READ_IF_EXISTS(pSettings, r_string, m_sect_name, "socket_bone_name", NULL);
    m_socket_bone_id   = NULL;
    m_parent_aitem     = NULL;
}

void attachable_hud_item::update(bool bForce) //--#SM+#--
{
    if (!bForce && m_upd_firedeps_frame == Device.dwFrame)
        return;

    bool is_16x9        = UI().is_widescreen();
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

    if (!IsChildHudItem())
        m_parent->calc_transform(m_attach_place_idx, m_attach_offset, m_item_transform);
    else
        m_parent_aitem->calc_child_transform(m_attach_offset, m_socket_bone_id, m_item_transform);
    m_upd_firedeps_frame = Device.dwFrame;

    IKinematicsAnimated* ka = m_model->dcast_PKinematicsAnimated();
    if (ka)
    {
        ka->UpdateTracks();
        ka->dcast_PKinematics()->CalculateBones_Invalidate();
        ka->dcast_PKinematics()->CalculateBones(TRUE);
    }

    // Обновляем потомков
    for (u32 i = 0; i < m_child_items.size(); i++)
    {
        if (m_child_items[i])
        {
            if (m_child_items[i]->m_parent_hud_item != this->m_parent_hud_item)
                m_child_items[i]->m_parent_hud_item = this->m_parent_hud_item;

            m_child_items[i]->update(bForce);
        }
    }

    if (bUpdateHudData)
        UpdateHudFromChildren();
}

// Загружаем и обновляем визуал --#SM+#--
void attachable_hud_item::UpdateVisual(shared_str new_visual)
{
    if (new_visual == NULL)
        new_visual = m_def_vis_name;

    if (m_cur_vis_name.equal(new_visual) == false)
    {
        //=======================================================//
        // Удаляем старую
        if (m_model != NULL)
        {
            IRenderVisual* v = m_model->dcast_RenderVisual();
            GEnv.Render->model_Delete(v);
            m_model = NULL;
        }

        // Устанавливаем новую
        m_cur_vis_name = new_visual;
        m_model        = smart_cast<IKinematics*>(GEnv.Render->model_Create(m_cur_vis_name.c_str()));

        // Считываем число костей с привязкой к числу патронов
        CWeapon::ReadMaxBulletBones(m_model);

        // Ищем в конфиге худа предмета параметры смещения \ скрытия костей и применяем их
        u32 lines_count = pSettings->line_count(m_sect_name);
        for (u32 i = 0; i < lines_count; ++i)
        {
            LPCSTR line_name = nullptr;
            LPCSTR line_value = nullptr;
            pSettings->r_line(m_sect_name, i, &line_name, &line_value);

            if (line_name && xr_strlen(line_name))
            {
                // Сдвиги костей
                if (nullptr != strstr(line_name, "item_bone_offset"))
                {
                    string16 str_mode;
                    _GetItem(line_name, 1, str_mode, '|'); //--> Считываем локальность \ глобальность изменений
                    string128 str_bone;
                    _GetItem(line_name, 2, str_bone, '|'); //--> Считываем имя кости
                    _GetItem(str_bone, 0, str_bone, ':');  //    Отрезаем от неё :alias (если был)

                    string128 str_pos;
                    _GetItem(line_value, 0, str_pos, '|'); //--> Считываем позицию
                    string128 str_rot;
                    _GetItem(line_value, 1, str_rot, '|'); //--> Считываем поворот

                    //--> Передаём данные в модель
                    KinematicsABT::additional_bone_transform offsets(KinematicsABT::SourceID::HUD_ITEM_OFFSETS);
                    Fvector vPos, vRot;

                    //--> ID Кости
                    offsets.m_bone_id = m_model->LL_BoneID(str_bone);

                    //--> Позиция и поворот
                    sscanf(str_pos, "%f,%f,%f", &vPos.x, &vPos.y, &vPos.z);
                    sscanf(str_rot, "%f,%f,%f", &vRot.x, &vRot.y, &vRot.z);
                    vRot.mul(PI / 180.f); //--> Преобразуем углы в радианы

                    bool bIsPosLocal = (str_mode[0] == 'L');
                    bIsPosLocal ? offsets.setPosOffsetLocal(vPos) : offsets.setPosOffsetGlobal(vPos);
                    
                    bool bIsRotLocal = (str_mode[1] == 'L');
                    bIsRotLocal ? offsets.setRotLocal(vRot) : offsets.setRotGlobal(vRot);

                    //--> Применяем смещение
                    m_model->LL_AddTransformToBone(offsets);
                }

                // Скрытие костей
                if (nullptr != strstr(line_name, "item_bone_hide"))
                {
                    string128 str_bone;
                    _GetItem(line_name, 1, str_bone, '|'); //--> Считываем имя кости

                    //--> Скрываем кость
                    u16 iBoneID = m_model->LL_BoneID(str_bone);
                    if (iBoneID != BI_NONE)
                    {
                        m_model->LL_SetBoneVisible(iBoneID, false, true);
                    }
                }
            }
        }
        //=======================================================//
    }
}

bool attachable_hud_item::need_renderable() { return m_parent_hud_item->need_renderable(); }

void attachable_hud_item::render() //--#SM+#--
{
    GEnv.Render->set_Transform(&m_item_transform);
    GEnv.Render->add_Visual(m_model->dcast_RenderVisual());

    if (!IsChildHudItem())
    {
        debug_draw_firedeps();
        m_parent_hud_item->render_hud_mode();
    }

    // Рендерим визуалы потомков
    for (u32 i = 0; i < m_child_items.size(); i++)
    {
        if (m_child_items[i])
        {
            m_child_items[i]->render();
        }
    }
}

bool attachable_hud_item::render_item_ui_query() //--#SM+#--
{
    bool bRes = m_parent_hud_item->render_item_3d_ui_query();

    // Обновляем UI у потомков
    for (u32 i = 0; i < m_child_items.size(); i++)
    {
        if (m_child_items[i])
        {
            m_child_items[i]->render_item_ui_query();
        }
    }

    return bRes;
}

void attachable_hud_item::render_item_ui() //--#SM+#--
{
    m_parent_hud_item->render_item_3d_ui();

    // Обновляем UI в потомках
    for (u32 i = 0; i < m_child_items.size(); i++)
    {
        if (m_child_items[i])
        {
            m_child_items[i]->render_item_ui();
        }
    }
}

void attachable_hud_item::set_bone_visible(const shared_str& bone_names, BOOL bVisibility, BOOL bSilent, BOOL bCheckVisibility) //--#SM+#--
{
    string128 _bone_name;
    int       count = _GetItemCount(bone_names.c_str());
    for (int it = 0; it < count; ++it)
    {
        _GetItem(bone_names.c_str(), it, _bone_name);
        const shared_str& bone_name = _bone_name;

        u16 bone_id;
        bone_id = m_model->LL_BoneID(bone_name);
        if (bone_id == BI_NONE)
        {
            if (bSilent)
                return;
            R_ASSERT2(0, make_string("model [%s] has no bone [%s]", pSettings->r_string(m_sect_name, "item_visual"), bone_name.c_str()).c_str());
        }

        if (bCheckVisibility == FALSE || m_model->LL_GetBoneVisible(bone_id) != bVisibility)
        {
            m_model->LL_SetBoneVisible(bone_id, bVisibility, TRUE);
        }

        /* --#SM+#-- SM_TODO:L
        Если к кости привязана геометрия с разными текстурами (surface), то при раскрытии кости
        иногда лишь часть геометрии (с одной текстурой) становится видимой, а другая нет.
        Стабильно это можно увидеть на М4A3 (из SWM 3.0), если одеть прицел и снять, то ручка для
        переноски будет раз через раз невидимой. Притом что LL_GetBoneVisible будет возвращать
        true.
        В качестве "костыльного" решения проблемы была убрана проверка, что кость уже сокрыта + само скрытие худовых
        костей стоит на апдейте. Этот "баг" (?) тянется с оригинал.

        UPD: Возможно причина кроется в том, что в движке одна модель разбивается на несколько составных, по одной на каждый материал (FHierarchyVisual)
        */
    }
}

// Ищем анимации для всех моделей (руки, оружие, аддоны) по её алиасу --#SM+#--
attachable_hud_item::anim_find_result attachable_hud_item::anim_find(
    const shared_str& sAnmAliasBase, bool bNoChilds, u8& rnd_idx, bool bAssert)
{
    R_ASSERT(strstr(sAnmAliasBase.c_str(), "anm_") == sAnmAliasBase.c_str());

    bool bIsChild = IsChildHudItem();

    // Некоторые предметы поддерживают отдельные анимации для _16x9
    string256 sAnmAlias;
    bool      is_16x9 = UI().is_widescreen() && !bIsChild;
    xr_sprintf(sAnmAlias, "%s%s", sAnmAliasBase.c_str(), ((m_attach_place_idx == 1) && is_16x9) ? "_16x9" : "");

    // Создаём дефолтный результат поиска
    anim_find_result result;
    result.anm_alias        = sAnmAlias; //--> Алиас анимации
    result.handsMotionDescr = NULL;      //--> Анимация рук (+ тех. данные)
    result.item_motion_name = NULL;      //--> Анимация предмета
    result.child_motions_map.clear();    //--> Анимации предметов-потомков (attachable_hud_item* потомок => shared_str анимация)

    // Теперь пробуем найти актуальные анимации
    //--> Сперва ищем у себя
    player_hud_motion* pHandsMotions = m_hand_motions.find_motion(sAnmAlias);
    player_hud_motion* pItemMotions  = pHandsMotions;

    //--> Запоминаем какая анимация рук была найдена без учёта аддонов
    player_hud_motion* pOrigHandsMotions = nullptr;
    if (g_LogHUDAnims)
    {
        pOrigHandsMotions = pHandsMotions;
    }

    //--> Потом у своих потомков (они приоритетней)
    if (bNoChilds == false)
    {
        xr_vector<attachable_hud_item*>::iterator it = m_child_items.begin();
        while (it != m_child_items.end())
        {
            attachable_hud_item*         pChildItem                 = (*it);
            player_hud_motion_container* pChildItemMotionsContainer = &pChildItem->m_hand_motions;
            player_hud_motion*           pChildItemMotions          = pChildItemMotionsContainer->find_motion(sAnmAlias);

            if (pChildItemMotions != NULL)
            {
                // Анимация рук и предмета
                if (pChildItemMotions->m_base_name != "_")
                    pHandsMotions = pChildItemMotions;

                if (pChildItemMotions->m_additional_name != "_")
                    pItemMotions = pChildItemMotions;

                // Анимации предметов-потомков - заносим в результат сразу
                if (pChildItemMotions->m_child_name != "_")
                    result.child_motions_map.emplace(pChildItem, pChildItemMotions->m_child_name);
            }

            // Следующий потомок
            ++it;
        }
    }

    if (pHandsMotions == NULL)
    { //--> Не смогли найти анимацию рук с таким алиасом
        if (bAssert == true)
        {
            Msg("[PRINT CHILD'S LIST]");
            xr_vector<attachable_hud_item*>::iterator it = m_child_items.begin();
            while (it != m_child_items.end())
            {
                Msg("%s\n", (*it)->m_sect_name);
                ++it;
            }

            R_ASSERT2(pHandsMotions != NULL,
                make_string("hudItem model [%s] and childs: can't find any motion with alias [%s]", m_sect_name.c_str(),
                    sAnmAlias)
                    .c_str());
        }

        // Возвращаем пустой результат
        return result;
    }

    // Заполняем результат поиска актуальными данными
    //--> Руки
    rnd_idx                 = (u8)Random.randI(pHandsMotions->m_animations.size());
    result.handsMotionDescr = &pHandsMotions->m_animations[rnd_idx];

    //--> Предмет
    if (pItemMotions != NULL)
        result.item_motion_name =
            (pHandsMotions->m_base_name != pItemMotions->m_additional_name) ? pItemMotions->m_additional_name : result.handsMotionDescr->name;

    // Проверяем совпадает ли имя анимации предмета с именем анимации модели рук до аддонов
    if (g_LogHUDAnims && pOrigHandsMotions != nullptr && pOrigHandsMotions->m_base_name == result.item_motion_name)
    {
        //--> Если совпало, значит скорее всего анимация предмета не была явно указана в конфиге,
        //    поэтому её имя автоматически стало равно имени анимации рук (player_hud_motion_container::load)
        //    В таком случае учитываем это при выводе ошибок в лог об отсутствующей анимации - скрываем ошибки про неё
        result.mute_item_motion_not_found_err = true;
    }

    return result;
}

// Проиграть анимацию одновременно у худовой модели предмета и худовой модели рук --#SM+#--
u32 attachable_hud_item::anim_play_both(anim_find_result& anim_to_play, bool bMixIn, bool bNoChilds, float fSpeed, float fStartFromTime)
{
    // Анимация рук
    u32 ret = anim_play_hands(anim_to_play, bMixIn, fSpeed, fStartFromTime);

    // Анимация предмета
    anim_play_item(anim_to_play, bMixIn, bNoChilds, fSpeed, fStartFromTime);

    return ret;
}

// Проиграть анимацию у худовой модели рук --#SM+#--
u32 attachable_hud_item::anim_play_hands(anim_find_result& anim_to_play, bool bMixIn, float fSpeed, float fStartFromTime)
{
    // Запускаем анимацию у рук
    u32 ret = g_player_hud->anim_play(m_attach_place_idx, anim_to_play.handsMotionDescr->mid, bMixIn, fSpeed, fStartFromTime);

    // Проигрываем эффект камеры, привязанный к имени анимации
    R_ASSERT2(m_parent_hud_item, "parent hud item is NULL");
    CPhysicItem& parent_object = m_parent_hud_item->object();

    if (IsGameTypeSingle() && parent_object.H_Parent() == Level().CurrentControlEntity() && !IsChildHudItem())
    {
        string_path anm_name;
        strconcat(sizeof(anm_name), anm_name, "camera_effects\\weapon\\", anim_to_play.handsMotionDescr->name.c_str(), ".anm");
        anim_play_cam_eff(anm_name);
    }

    return ret;
}

// Проиграть анимацию у худовой модели предмета (1) --#SM+#--
void attachable_hud_item::anim_play_item(
    shared_str& sMotionName, bool bMixIn, bool bNoChilds, float fSpeed, float fStartFromTime, float _fRootStartTime)
{
    anim_find_result anim_data;
    anim_data.handsMotionDescr = NULL;
    anim_data.anm_alias        = NULL;
    anim_data.item_motion_name = sMotionName;
    anim_data.child_motions_map.clear();
    anim_play_item(anim_data, bMixIn, bNoChilds, fSpeed, fStartFromTime, _fRootStartTime);
}

// Проиграть анимацию у худовой модели предмета (2) --#SM+#--
void attachable_hud_item::anim_play_item(
    anim_find_result& anim_to_play, bool bMixIn, bool bNoChilds, float fSpeed, float fStartFromTime, float _fRootStartTime)
{
    // Если анимация предмета не указана, то отыгрываем дефолтную ("idle")
    if (anim_to_play.item_motion_name == NULL)
        anim_to_play.item_motion_name = m_def_motion;

    // Если наша анимация была запущена из другого attachable_hud_item, то _fRootStartTime будет содержать
    // стартовую секунду анимации нашего самого первого корневого родителя, запустившего всю цепочку вызовов anim_play_item
    if (_fRootStartTime < 0.0f)
        _fRootStartTime = fStartFromTime;

    // Играем свою анимацию (X в названии анимации означает оставить текущую)
    if (m_model->dcast_PKinematicsAnimated() && anim_to_play.item_motion_name != "X")
    {
        IKinematicsAnimated* ka = m_model->dcast_PKinematicsAnimated();

        MotionID M2 = ka->ID_Cycle_Safe(anim_to_play.item_motion_name);
        if (!M2.valid())
        { // Не нашли указанной анимации у предмета
            //--> Выводим ошибку
            if (g_LogHUDAnims && (anim_to_play.item_motion_name != anim_to_play.handsMotionDescr->name) &&
                anim_to_play.mute_item_motion_not_found_err == false)
            {
                Msg("! item model [%s] has no motion [%s], playing [idle] instead",
                    pSettings->r_string(m_sect_name, "item_visual"), anim_to_play.item_motion_name.c_str());
            }

            //--> Пытаемся проиграть дефолтную ("idle")
            M2 = ka->ID_Cycle_Safe(m_def_motion);
        }
        else
        { // Анимация найдена и успешно запущена
            if (bDebug)
            {
                Msg("playing item animation [%s]", anim_to_play.item_motion_name.c_str());
            }
        }
        R_ASSERT3(M2.valid(), "item model has no motion [idle] ", pSettings->r_string(m_sect_name, "item_visual"));

        u16            root_id    = m_model->LL_GetBoneRoot();
        CBoneInstance& root_binst = m_model->LL_GetBoneInstance(root_id);
        root_binst.set_callback_overwrite(TRUE);
        root_binst.mTransform.identity();

        u16 pc = ka->partitions().count();
        for (u16 pid = 0; pid < pc; ++pid)
        {
            CBlend* B     = ka->PlayCycle(pid, M2, bMixIn);
            float   fTime = ModifyBlendParams(B, fSpeed, fStartFromTime);
        }

        m_model->CalculateBones_Invalidate();
    }

    // Запускаем анимации у потомков
    if (bNoChilds == false)
    {
        xr_vector<attachable_hud_item*>::iterator it = m_child_items.begin();
        while (it != m_child_items.end())
        {
            attachable_hud_item*   pChildItem = (*it);
            ChildMotions::iterator pRes       = anim_to_play.child_motions_map.find(pChildItem);
            shared_str sChildAnm              = (pRes != anim_to_play.child_motions_map.end() ?
                                                    pRes->second :
                                                    pChildItem->m_def_motion
                                                );
            pChildItem->anim_play_item(sChildAnm, bMixIn, false, fSpeed, fStartFromTime, _fRootStartTime);

            // Следующий потомок
            ++it;
        }
    }
}

// Запустить худовой эффект камеры --#SM+#--
void attachable_hud_item::anim_play_cam_eff(LPCSTR sAnmPath, float fSpeed)
{
    CActor* current_actor = static_cast_checked<CActor*>(Level().CurrentControlEntity());
    VERIFY(current_actor);
    CEffectorCam* ec = current_actor->Cameras().GetCamEffector(eCEWeaponAction);
    if (NULL == ec)
    {
        string_path ce_path;
        if (FS.exist(ce_path, "$game_anims$", sAnmPath))
        {
            CAnimatorCamEffector* e = new CAnimatorCamEffector();
            e->SetType(eCEWeaponAction);
            e->SetHudAffect(false);
            e->SetCyclic(false);
            e->SetSpeed(fSpeed);
            e->Start(sAnmPath);
            current_actor->Cameras().AddCamEffector(e);
        }
    }
}

// Поиск потомка --#SM+#--
attachable_hud_item* attachable_hud_item::FindChildren(const shared_str& sect_name, int* idx)
{
    if (idx != NULL)
        *idx = 0;

    for (u32 i = 0; i < m_child_items.size(); i++)
    {
        attachable_hud_item* item = m_child_items[i];
        if (item != NULL && item->m_sect_name.equal(sect_name))
        {
            if (idx != NULL)
                *idx = i;
            return item;
        }
    }
    return NULL;
}

// Присоединение потомка --#SM+#--
void attachable_hud_item::AddChildren(const shared_str& sect_name, bool bRecalcBonesOffsets)
{
    // Проверка что такого потомка у нас ещё нету
    if (FindChildren(sect_name) != NULL)
        return;

    // Добавляем новый
    attachable_hud_item* res = new attachable_hud_item(this->m_parent);
    res->load(sect_name);
    res->m_hand_motions.load(m_parent->get_model(), sect_name);

    res->m_parent_aitem    = this;
    res->m_parent          = this->m_parent;
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

        MotionID M2 = ka->ID_Cycle_Safe(res->m_def_motion);
        R_ASSERT4(M2.valid(),
            "model has no default motion",
            pSettings->r_string(res->m_sect_name, "item_visual"),
            res->m_def_motion.c_str()
        );

        u16            root_id    = res->m_model->LL_GetBoneRoot();
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
    m_child_items.push_back(res);

    // Вызываем событие изменения потомков
    OnChildrenChange(res, false);

    // Пересчитываем смещения костей
    if (bRecalcBonesOffsets)
        m_parent->RecalculateBonesOffsets();

    // Переопределяем параметры худа
    UpdateHudFromChildren();
}

// Отсоединяем потомка --#SM+#--
void attachable_hud_item::RemoveChildren(const shared_str& sect_name, bool bRecalcBonesOffsets)
{
    int                  idx  = 0;
    attachable_hud_item* item = FindChildren(sect_name, &idx);

    if (item != NULL)
    {
        // Вызываем событие изменения потомков
        OnChildrenChange(item, true);

        // Удаляем потомка
        xr_delete(item);
        m_child_items.erase(m_child_items.begin() + idx);
        UpdateHudFromChildren();

        // Пересчитываем смещения костей
        if (bRecalcBonesOffsets)
            m_parent->RecalculateBonesOffsets();
    }
}

// Добавляем\убираем потомков (можно перечеслять несколько через запятую) --#SM+#--
void attachable_hud_item::UpdateChildrenList(const shared_str& addons_list, bool bShow, bool bUseParser)
{
    if (bUseParser)
    { //--> Это список секций
        if (addons_list.size())
        {
            string128 _hudName;
            LPCSTR    _addons_list = addons_list.c_str();

            int count = _GetItemCount(_addons_list);
            for (int it = 0; it < count; ++it)
            {
                _GetItem(_addons_list, it, _hudName);
                if (bShow)
                    this->AddChildren(_hudName, false);
                else
                    this->RemoveChildren(_hudName, false);
            }
        }
    }
    else
    { //--> Это одна секция
        if (bShow)
            this->AddChildren(addons_list, false);
        else
            this->RemoveChildren(addons_list, false);
    }

    // Пересчитываем смещения костей
    m_parent->RecalculateBonesOffsets();
}

// Коллбэк на добавление \ удаление потомка
void attachable_hud_item::OnChildrenChange(attachable_hud_item* pChild, bool bOnRemove)
{

}

// Расчитываем позицию потомка относительно его ролителя --#SM+#--
void attachable_hud_item::calc_child_transform(const Fmatrix& offset, u16 bone_id, Fmatrix& result)
{
    Fmatrix ancor_m = m_model->LL_GetTransform(bone_id);
    result.mul(m_item_transform, ancor_m);
    result.mulB_43(offset);
}

// Считываем смещения для костей из худовых секций потомков, и регистрируем эту информацию в текущей модели рук --#SM+#--
void attachable_hud_item::ReadBonesOffsetsToHands(bool bNoHands)
{
    // Лямбда-функция для считывания смещений из переданной секции
    auto fnReadHandsOffsets = [&](shared_str& section) {
        u32 lines_count = pSettings->line_count(section);
        for (u32 i = 0; i < lines_count; ++i)
        {
            LPCSTR line_name = NULL;
            LPCSTR line_value = NULL;
            pSettings->r_line(section, i, &line_name, &line_value);

            if (line_name && xr_strlen(line_name))
            {
                if (NULL != strstr(line_name, "hand_bone_offset"))
                {
                    string16 str_mode;
                    _GetItem(line_name, 1, str_mode, '|'); //--> Считываем локальность \ глобальность изменений
                    string128 str_bone;
                    _GetItem(line_name, 2, str_bone, '|'); //--> Считываем имя кости
                    _GetItem(str_bone, 0, str_bone, ':');  //    Отрезаем от неё :alias (если был)

                    string128 str_pos;
                    _GetItem(line_value, 0, str_pos, '|'); //--> Считываем позицию
                    string128 str_rot;
                    _GetItem(line_value, 1, str_rot, '|'); //--> Считываем поворот

                    bool bFadeExist = false;
                    string128 str_fade;
                    if (_GetItemCount(line_value, '|') > 2)
                    {
                        bFadeExist = true;
                        _GetItem(line_value, 2, str_fade, '|'); //--> Считываем параметры Fade In\Out эффекта
                    }

                    // Передаём данные в модель
                    KinematicsABT::additional_bone_transform offsets(KinematicsABT::SourceID::HUD_HANDS_OFFSETS);
                    Fvector vPos, vRot;

                    // ID Кости
                    offsets.m_bone_id = m_parent->get_model()->dcast_PKinematics()->LL_BoneID(str_bone);

                    //--> Позиция и поворот
                    sscanf(str_pos, "%f,%f,%f", &vPos.x, &vPos.y, &vPos.z);
                    sscanf(str_rot, "%f,%f,%f", &vRot.x, &vRot.y, &vRot.z);
                    vRot.mul(PI / 180.f); //--> Преобразуем углы в радианы

                    bool bIsPosLocal = (str_mode[0] == 'L');
                    bIsPosLocal ? offsets.setPosOffsetLocal(vPos) : offsets.setPosOffsetGlobal(vPos);

                    bool bIsRotLocal = (str_mode[1] == 'L');
                    bIsRotLocal ? offsets.setRotLocal(vRot) : offsets.setRotGlobal(vRot);

                    //--> Fade In\Out эффект
                    if (bFadeExist)
                    {
                        string16 sFadeMode = "00";
                        Fvector2 vFadeIn, vFadeOut;
                        sscanf(str_fade, "%2s,%f,%f,%f,%f", sFadeMode, &vFadeIn.x, &vFadeIn.y, &vFadeOut.x, &vFadeOut.y);

                        offsets.m_fade.SetupEffect(vFadeIn, vFadeOut, sFadeMode[1] == 'I');
                    }

                    //--> Применяем смещение
                    m_parent->get_model()->LL_AddTransformToBone(offsets);
                    if (m_parent->get_model_twin() != NULL)
                        m_parent->get_model_twin()->LL_AddTransformToBone(offsets);
                }
            }
        }
    };

    // Считываем смещения из присоединённых потомков
    xr_vector<attachable_hud_item*>::iterator it = m_child_items.begin();
    while (it != m_child_items.end())
    {
        // На случаи если к потомку тоже что-то присоединено
        (*it)->ReadBonesOffsetsToHands(true); //--> Но уже игнорируем смещение из секции рук

        // Ищем смещения в худовой секции потомка
        fnReadHandsOffsets((*it)->m_sect_name);

        // next
        ++it;
    }

    // Ищем смещения в худовой секции текущего предмета
    if (bNoHands == false)
    {
        fnReadHandsOffsets(this->m_sect_name);
    }
}
