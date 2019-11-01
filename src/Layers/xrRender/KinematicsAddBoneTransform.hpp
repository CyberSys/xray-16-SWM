#pragma once //--#SM+#--

#include "xrCore/_quaternion.h"

namespace KinematicsABT
{
// Идентификаторы источников трансформации - для возможности их выборочной отмены
enum SourceID
{
    _ANY = u8(0),           //--> Использовать в LL_ClearAdditionalTransform для очистки всех трансформаций с выбранной кости
    
    WPN_HOLOMARK,           //--> Метка голографа на прицелах оружия (положение на модели)
    WPN_BIPODS_DEPLOY,      //--> Тело и ножки сошек (сложено \ разложено)
    WPN_PRIKLAD_DEPLOY,     //--> Приклад оружия

    HUD_HANDS_OFFSETS,      //--> Сдвиги костей рук, управляемые через конфиг худа
    HUD_ITEM_OFFSETS,       //--> Сдвиги костей предмета, управляемые через конфиг худа

    AVIS_ITEM_OFFSETS,      //--> Сдвиги костей attachable_visual, управляемые через конфиг
};

// Описывает постоянное смещение для указанной кости --#SM+#--
struct additional_bone_transform
{
    SourceID m_source_id; // ID of transform source
    bool m_bRotGlobal; // Use XYZ axis instead of HPB
    u16 m_bone_id; // Bone ID
    Fmatrix m_transform; // Rotation + Position

    additional_bone_transform(SourceID srcID) : m_bRotGlobal(false), m_bone_id(-1)
    {
        VERIFY(srcID != SourceID::_ANY);
        m_source_id = srcID;
        m_transform.identity();
    }

    // Установить поворот кости (статичные оси)
    IC void setRotGlobal(const float _x, const float _y, const float _z)
    {
        //--> Выставляем флаг глобального поворота
        m_bRotGlobal = true;

        //--> Запоминаем текущее смещение кости - оно будет сброшено
        Fvector3 vSavedPos = Fvector3().set(m_transform.c.x, m_transform.c.y, m_transform.c.z);

        //--> Делаем поворот
        m_transform.setXYZ(_x, _y, _z);

        //--> Восстанавливаем смещение кости
        setPosOffset(vSavedPos);
    }

    IC void setRotGlobal(Fvector& vRotOfs)
    {
        setRotGlobal(vRotOfs.x, vRotOfs.y, vRotOfs.z);
    }

    // Установить поворот кости (кость крутится вместе с осями)
    IC void setRotLocal(const float _x, const float _y, const float _z)
    {
        //--> Выставляем флаг глобального поворота
        m_bRotGlobal = false;

        //--> Запоминаем текущее смещение кости - оно будет сброшено
        Fvector3 vSavedPos = Fvector3().set(m_transform.c.x, m_transform.c.y, m_transform.c.z);

        //--> Делаем поворот
        Fquaternion Q;
        Q.rotationYawPitchRoll(_y, _x, _z);
        m_transform.rotation(Q);

        //--> Восстанавливаем смещение кости
        setPosOffset(vSavedPos);
    }

    IC void setRotLocal(Fvector& vRotOfs)
    {
        setRotLocal(vRotOfs.x, vRotOfs.y, vRotOfs.z);
    }

    // Установить смещение позиции кости
    IC void setPosOffset(const float _x, const float _y, const float _z)
    {
        m_transform.translate_over(_x, _y, _z);
    }

    IC void setPosOffset(Fvector& vPosOfs)
    {
        setPosOffset(vPosOfs.x, vPosOfs.y, vPosOfs.z);
    }
};
}
