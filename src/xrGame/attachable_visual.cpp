#include "stdafx.h"
#include "GameObject.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Common/object_broker.h"
#include "attachable_visual.h"

/**********************************/
/***** Присоединяемые визуалы *****/ //--#SM+#--
/**********************************/

// Конструктор
attachable_visual::attachable_visual(CGameObject* _parent, const shared_str& sect_name, attachable_visual* _avisual)
{
    R_ASSERT(_parent != NULL);
    R_ASSERT(sect_name != NULL);

    m_parent      = _parent;
    m_parent_aVis = _avisual;
    m_model       = NULL;

    m_sect_name    = sect_name;
    m_cfg_vis_name = NULL;
    m_cur_vis_name = NULL;

    m_pExtRenderTransform = NULL;

    m_anims.clear();

    ReLoad(m_sect_name);
};
// Деструктор
attachable_visual::~attachable_visual()
{
    // Удаляем модель
    if (m_model != NULL)
    {
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GEnv.Render->model_Delete(v);
        m_model = NULL;
    }

    // Удаляем визуалы, присоединённые к нам
    delete_data(m_attached_visuals);
};

// Пере-Загрузить в объект настройки из указанной секции конфигов
void attachable_visual::ReLoad(const shared_str& sect_name)
{
    // Позиция
    Fvector pos = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, "visual_attach_offset", Fvector().set(0, 0, 0));

    // Поворот
    Fvector ypr = READ_IF_EXISTS(pSettings, r_fvector3, sect_name, "visual_attach_YPR", Fvector().set(0, 0, 0));
    ypr.mul(PI / 180.f);

    // Преобразуем в матрицу смещения
    m_attach_offset.setHPB(VPUSH(ypr));
    m_attach_offset.c = pos;

    // Считываем кость на родительской модели, к которой будем цепляться
    m_bone_name = READ_IF_EXISTS(pSettings, r_string, sect_name, "visual_attach_bone", NULL);

    // Устанавливаем визуал
    shared_str visual = READ_IF_EXISTS(pSettings, r_string, sect_name, "visual", NULL);
    if (visual == NULL || visual.equal("_dont_change_") == false)
    {
        m_cfg_vis_name = visual;
        SetVisual(m_cfg_vis_name);
    }
};

// Установить новый визуал и загрузить из него все доступные анимации, указанные в конфиге
void attachable_visual::SetVisual(shared_str vis_name)
{
    if (m_cur_vis_name.equal(vis_name))
        return;

    m_cur_vis_name = (vis_name != nullptr ? vis_name : m_cfg_vis_name);

    // Удаляем старую модель
    if (m_model != nullptr)
    {
        IRenderVisual* v = m_model->dcast_RenderVisual();
        GEnv.Render->model_Delete(v);
        m_model = nullptr;
    }

    // Устанавливаем новую
    if (m_cur_vis_name != nullptr)
        m_model = smart_cast<IKinematics*>(GEnv.Render->model_Create(m_cur_vis_name.c_str()));

    if (m_model)
    {
        // Ищем в конфиге визуала параметры смещения \ скрытия костей и применяем их
        m_model->LL_ClearAdditionalTransform(BI_NONE, KinematicsABT::SourceID::AVIS_ITEM_OFFSETS); //--> Удаляем старые

        u32 lines_count = pSettings->line_count(m_sect_name);
        for (u32 i = 0; i < lines_count; ++i)
        {
            LPCSTR line_name = nullptr;
            LPCSTR line_value = nullptr;
            pSettings->r_line(m_sect_name, i, &line_name, &line_value);

            if (line_name && xr_strlen(line_name))
            {
                // Сдвиги костей
                if (nullptr != strstr(line_name, "visual_bone_offset"))
                {
                    string128 str_bone;
                    _GetItem(line_name, 1, str_bone, '|'); //--> Считываем имя кости
                    string128 str_pos;
                    _GetItem(line_value, 0, str_pos, '|'); //--> Считываем позицию
                    string128 str_rot;
                    _GetItem(line_value, 1, str_rot, '|'); //--> Считываем поворот

                    //--> Передаём данные в модель
                    KinematicsABT::additional_bone_transform offsets(KinematicsABT::SourceID::AVIS_ITEM_OFFSETS);
                    Fvector vPos, vRot;

                    offsets.m_bone_id = m_model->LL_BoneID(str_bone);
                    sscanf(str_pos, "%f,%f,%f", &vPos.x, &vPos.y, &vPos.z);
                    sscanf(str_rot, "%f,%f,%f", &vRot.x, &vRot.y, &vRot.z);
                    vRot.mul(PI / 180.f); //--> Преобразуем углы в радианы

                    offsets.setRotLocal(vRot);
                    offsets.setPosOffset(vPos);

                    m_model->LL_AddTransformToBone(offsets);
                }

                // Скрытие костей
                if (nullptr != strstr(line_name, "visual_bone_hide"))
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

        // Загружаем анимации из секции визуала
        do
        {
            CInifile::Sect& _sect = pSettings->r_section(m_sect_name);
            auto _b = _sect.Data.begin();
            auto _e = _sect.Data.end();

            m_anims.clear(); //--> Очищаем текущие

            //--> Ищем новые анимации в секции визуала
            for (; _b != _e; ++_b)
            {
                if (strstr(_b->first.c_str(), "anm_world_") == _b->first.c_str())
                {
                    const shared_str& sAnmAlias = _b->first;
                    const shared_str& sMotionName = _b->second;

                    //--> Проверяем что модель вообще имеет анимации
                    IKinematicsAnimated* pModelAnimated = m_model->dcast_PKinematicsAnimated();
                    if (pModelAnimated == false)
                    {
                        break;
                    }

                    //--> Проверяем что модель имеет конкретно указанную анимацию
                    MotionID motion_ID = pModelAnimated->ID_Cycle_Safe(sMotionName);
                    if (motion_ID.valid())
                    {
                        m_anims[sAnmAlias.c_str()] = sMotionName.c_str(); //--> Заносим связку в список анимаций визуала
                    }
                }
            }
        } while (false);
    }

    // Сообщеаем о смене модели
    m_parent->OnAdditionalVisualModelChange(this);
}

// Попытаться отыграть анимацию по её алиасу из конфига ("anm_world_XXX")
void attachable_visual::Try2PlayMotionByAlias(const shared_str& sAnmAlias, bool bMixIn, bool bChilds)
{
    // Сперва играем у себя
    if (m_model != nullptr)
    {
        auto _it = m_anims.find(sAnmAlias.c_str());
        if (_it != m_anims.end())
        {
            const xr_string& sMotionName = ((*_it).second);
            m_model->dcast_PKinematicsAnimated()->PlayCycle(sMotionName.c_str(), bMixIn);
        }
    }

    // Потом у всех детей
    if (bChilds)
    {
        for (u32 i = 0; i < m_attached_visuals.size(); i++)
            m_attached_visuals[i]->Try2PlayMotionByAlias(sAnmAlias, bMixIn, bChilds);
    }
}

// Обновление
void attachable_visual::Update()
{
    // Обновляем присоединённые визуалы
    for (u32 i = 0; i < m_attached_visuals.size(); i++)
        m_attached_visuals[i]->Update();
};

// Отрисовка модели
void attachable_visual::Render()
{
    if (m_model != NULL)
    {
        IKinematics* pParentKVis = NULL;

        if (m_parent_aVis == NULL)
        {
            // Мы не присоединены к другому attachable_visual, тогда пробуем взять модель родителя
            pParentKVis = (m_parent->Visual() != NULL ? m_parent->Visual()->dcast_PKinematics() : NULL);
        }
        else
        {
            // Иначем используем модель от родительского attachable_visual
            if (m_parent_aVis->m_model != NULL)
            {
                pParentKVis = m_parent_aVis->m_model;
            }
        }

        // Обновляем индекс кости, к которой цепляемся
        if (pParentKVis != NULL && m_bone_name != NULL)
        {
            m_bone_id = pParentKVis->LL_BoneID(m_bone_name);
            R_ASSERT3(m_bone_id != BI_NONE,
                make_string("model [%s] has no bone [%s]",
                    (m_parent_aVis == NULL ? m_parent->cNameVisual().c_str() : m_parent_aVis->m_cur_vis_name.c_str()),
                    m_bone_name.c_str())
                    .c_str(),
                m_sect_name.c_str());
        }
        else
            m_bone_id = BI_NONE;

        // Обновляем позицию для рендеринга
        Fmatrix* m_pRenderTransform = NULL;
        if (m_pExtRenderTransform != NULL)
        { // Используем матрицу трансформации, переданную извне
            m_pRenderTransform = m_pExtRenderTransform;
        }
        else
        { // Считываем матрицу трансформации на основе родительского объекта
            if (pParentKVis != NULL && m_bone_id != BI_NONE)
            {
                // Цепляем предмет относительно родительской кости
                m_render_transform.mul_43(pParentKVis->LL_GetBoneInstance(m_bone_id).mTransform, m_attach_offset);
                m_render_transform.mulA_43(GetParentXFORM());
            }
            else
            {
                // Цепляем предмет относительно центра объекта
                m_render_transform.mul_43(GetParentXFORM(), m_attach_offset);
            }

            m_pRenderTransform = &m_render_transform;
        }

        GEnv.Render->set_Transform(m_pRenderTransform);
        GEnv.Render->add_Visual(m_model->dcast_RenderVisual());
    }

    // Отрисовываем присоединённые визуалы
    for (u32 i = 0; i < m_attached_visuals.size(); i++)
        m_attached_visuals[i]->Render();
};

// Получаем XFORM родителя
const Fmatrix& attachable_visual::GetParentXFORM()
{
    if (m_parent_aVis != NULL)
        return m_parent_aVis->m_render_transform;
    else
        return m_parent->XFORM();
}

// Найти присоединённый визуал
attachable_visual* attachable_visual::FindChildren(const shared_str& children_name, int* idx)
{
    for (u32 i = 0; i < m_attached_visuals.size(); i++)
    {
        attachable_visual* item = m_attached_visuals[i];
        if (item != NULL && item->m_sect_name.equal(children_name))
        {
            if (idx != NULL)
                *idx = i;
            return item;
        }
    }

    return NULL;
}

// Присоединить или отсоединить другой attachable_visual к нашему
void attachable_visual::SetChildrenVisual(const shared_str& attach_name, bool bAttach)
{
    int                idx  = 0;
    attachable_visual* item = FindChildren(attach_name, &idx);

    if (bAttach)
    { // Присоединяем аттач
        if (item == NULL)
        {
            attachable_visual* res = new attachable_visual(this->m_parent, attach_name, this);
            m_attached_visuals.push_back(res);
        }
    }
    else
    { // Отсоединяем аттач
        if (item != NULL)
        {
            // Отсоединяем аттач
            m_attached_visuals.erase(m_attached_visuals.begin() + idx);

            // Уничтожаем
            xr_delete(item);
        }
    }
}

// Получить список абсолютно всех attachable_visual, которые привязаны к нам
void attachable_visual::GetAllInheritedAVisuals(xr_vector<attachable_visual*>& tOutAVisList)
{
    // Заносим себя
    tOutAVisList.push_back(this);

    // Заносим своих детей с ихними детьми ...
    for (u32 i = 0; i < m_attached_visuals.size(); i++)
    {
        attachable_visual* item = m_attached_visuals[i];
        if (item != NULL)
        {
            item->GetAllInheritedAVisuals(tOutAVisList);
        }
    }
}
