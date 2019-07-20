#pragma once

/***********************************************/
/***** Структура, описывающая аддон оружия *****/ //--#SM+#--
/***********************************************/

// Системные индексы set-секций аддонов в списке
enum
{
    empty_addon_idx = u8(-1), //--> Аддон не установлен
    first_addon_idx = u8(0) //--> Индекс первого аддона
};

// clang-format off
// Прочитать данные сперва из секции аддона внутри оружия, потом самого аддона, и если там тоже нету - из секции оружия.
#define READ_ADDON_DATA(DEF_method,DEF_Name,DEF_fnc_1,DEF_fnc_2,DEF_def_val) \
			READ_IF_EXISTS(pSettings, DEF_method, DEF_fnc_1,	DEF_Name,	 \
			READ_IF_EXISTS(pSettings, DEF_method, DEF_fnc_2,	DEF_Name,	 \
			READ_IF_EXISTS(pSettings, DEF_method, cNameSect(),	DEF_Name,	 \
		DEF_def_val)));

#define READ_ADDON_DATA_NO_WPN(DEF_method,DEF_Name,DEF_fnc_1,DEF_fnc_2,DEF_def_val) \
			READ_IF_EXISTS(pSettings, DEF_method, DEF_fnc_1,	DEF_Name,	 \
			READ_IF_EXISTS(pSettings, DEF_method, DEF_fnc_2,	DEF_Name,	 \
		DEF_def_val));
// clang-format on

// Вектор, хранящий в себе все set-секции аддонов данного типа у оружия
DEFINE_VECTOR(shared_str, ADDONS_VECTOR, ADDONS_VECTOR_IT);

struct SAddonData
{
public:
    SAddonData();
    ~SAddonData();

public:
    bool bActive; //--> Флаг активности аддона (Установлен \ Не установлен)
    u8 addon_idx; //--> Индекс текущего активного аддона из m_addons_list (empty_addon_idx если пусто)

    ADDONS_VECTOR m_addons_list; //--> Список всех set-секций аддона данного типа у оружия
    shared_str m_addon_alias;    //--> Название строки из set-секции аддона, где указана секция предмета аддона
    shared_str m_addon_bone;     //--> Название кости на оружии, отображение которой привязано к аддону
    shared_str m_parent_sect;    //--> Название секции оружия, которой была инициализирована текущая SAddonData

    // Возможность присоединения аддонов (0 - отсутствует, 1 - встроен, 2 - присоединяем)
    ALife::EWeaponAddonStatus m_attach_status;

    bool bHideVis3p; //--> Нужно-ли скрыть доп. модель аддона на мировой модели

    // Коэфиценты оружейных характеристик от текущего аддона
    float m_kHitPower;
    float m_kImpulse;
    float m_kBulletSpeed;
    float m_kFireDispersion;
    float m_kCamVDispersion;
    float m_kCamHDispersion;
    float m_kShootingShake;

    //==============================================================//

    /* Инициализировать аддон параметрами из конфига
       section - секция оружия, которому принадлежит аддон;
       sAddonsList - название строки из конфига, где перечислены все set-секции адона данного типа;
       sAttachAlias - название строки из конфига, где прописывается имя предмета-аддона;
       sAddonBone - название кости в модели, отображение которой привязано к аддону;
    */
    void Initialize(LPCSTR section, LPCSTR sAddonsList, LPCSTR sAddonAlias, LPCSTR sAttachAlias, LPCSTR sAddonBone);

    // Получить кол-во доступных аддонов данного типа
    IC const u8 SAddonData::AddonsCount() const { return m_addons_list.size(); }

    // Получить текущую Set-секцию аддона
    IC const shared_str SAddonData::GetName() const
    {
        VERIFY(addon_idx != empty_addon_idx);
        VERIFY(bActive == true);

        return m_addons_list[addon_idx];
    }

    // Получить Set-секцию аддона по его индексу в списке
    IC const shared_str SAddonData::GetNameByIdx(u8 idx) const
    {
        VERIFY(idx <= AddonsCount());

        return m_addons_list[idx];
    }

    // Получить текущую секцию предмета аддона
    IC const shared_str SAddonData::GetAddonName() const
    {
        return pSettings->r_string(GetName().c_str(), m_addon_alias.c_str());
    }

    // Получить секцию предмета аддона по его индексу в списке
    IC const shared_str SAddonData::GetAddonNameByIdx(u8 idx) const
    {
        return pSettings->r_string(GetNameByIdx(idx).c_str(), m_addon_alias.c_str());
    }

    // Получить список всех визуалов (мировых или худовых) аддона (текущего или по его idx)
    const shared_str GetVisuals(LPCSTR vis_alias, bool bOnlyFirst = false, u8 idx = empty_addon_idx) const;

    // Применить оружейные коэфиценты текущего аддона
    void ApplyWeaponKoefs();

    // Сбросить оружейные коэфиценты текущего аддона
    void ResetWeaponKoefs();
};
