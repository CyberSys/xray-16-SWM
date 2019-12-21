#pragma once

/**********************************/
/***** Присоединяемые визуалы *****/ //--#SM+#--
/**********************************/

class attachable_visual
{
public:
    typedef xr_unordered_map<xr_string, xr_string> AnmPairs;

    CGameObject*                  m_parent;           // Родительский объект, к которому мы прикреплены
    attachable_visual*            m_parent_aVis;      // *Родительский attachable_visual к которому мы можем быть прикреплены
    xr_vector<attachable_visual*> m_attached_visuals; // Визуалы, присоединённые к нам

    shared_str   m_cfg_vis_name; // Визуал из конфига (если есть)
    shared_str   m_cur_vis_name; // Текущий визуал
    IKinematics* m_model;        // *Наша модель
    AnmPairs     m_anims;        // Доступные анимации модели (anm_world_XXX = название анимации)

    shared_str m_sect_name; // Секция с параметрами из конфига

    shared_str m_bone_name; // *Имя кости в модели родителя, относительно которой цепляемся
    u16        m_bone_id;   // ID этой кости

    Fmatrix m_attach_offset;    // Матрица смещений нашей модели относительно родительского объекта
    Fmatrix m_render_transform; // Матрица, используемая для отрисовки модели

    Fmatrix* m_pExtRenderTransform; // Указатель на матрицу, которая если есть, то будет использована вместо m_render_transform

    void Render();
    void Update();

    const Fmatrix& GetParentXFORM();

    attachable_visual(CGameObject* _parent, const shared_str& sect_name, attachable_visual* _avisual = NULL);
    ~attachable_visual();

    // Пере-Загрузить настройки из секции в конфигах
    void ReLoad(const shared_str& sect_name);

    // Установить новый визуал и загрузить из него все доступные анимации, указанные в конфиге
    void SetVisual(shared_str vis_name);

    // Попытаться отыграть анимацию по её алиасу из конфига ("anm_world_XXX")
    void Try2PlayMotionByAlias(const shared_str& sAnmAlias, bool bMixIn = false, bool bChilds = false);

    // Найти присоединённый визуал
    attachable_visual* FindChildren(const shared_str& children_name, int* idx = NULL);

    // Присоединить\отсоединить другой attachable_visual к этому
    void SetChildrenVisual(const shared_str& attach_name, bool bAttach);

    // Получить список абсолютно всех attachable_visual, которые привязаны к нам
    void GetAllInheritedAVisuals(xr_vector<attachable_visual*>& tOutAVisList);

    // Использовать
    IC void UseThisRenderTransform(Fmatrix* pTransform = NULL) { m_pExtRenderTransform = pTransform; }
};
