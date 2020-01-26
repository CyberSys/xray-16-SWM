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
    u16 m_bone_id; // Bone ID
    Fmatrix m_transform_local; // Local position \ rotation (HPB)
    Fmatrix m_transform_global; // Global position \ rotation (XYZ)

    class fade_effect
    {
    private:
        bool m_bEnabled;
        bool m_bInverseMode;
        Fvector2 m_vIntervalFadeIn; // Fade In Begin <-> End, < m_vIntervalFadeOut
        Fvector2 m_vIntervalFadeOut; // Fade Out Begin <-> End, > m_vIntervalFadeOut

    public:
        fade_effect() : m_bEnabled(false), m_bInverseMode(false) {
            m_vIntervalFadeIn.set(0.0f, 0.5f);
            m_vIntervalFadeOut.set(0.5f, 1.0f);
        }

        IC bool IsEnabled() { return m_bEnabled; }
        IC bool IsInverted() { return m_bInverseMode; }
        IC Fvector2 GetIntervalFadeIn() { return m_vIntervalFadeIn; }
        IC Fvector2 GetIntervalFadeOut() { return m_vIntervalFadeOut; }

        IC void DisableEffect() { m_bEnabled = false; }
        void SetupEffect(const Fvector2& vIntervalFadeIn, const Fvector2& vIntervalFadeOut, bool bInverted)
        {
            R_ASSERT2(m_vIntervalFadeIn.y > m_vIntervalFadeIn.x,
                make_string(
                    "Wrong fade effect params, Fade In End [%f] <= Fade In Start [%f]",
                    vIntervalFadeIn.y, vIntervalFadeIn.x));
            m_vIntervalFadeIn.x = clampr(vIntervalFadeIn.x, 0.0f, 1.0f);
            m_vIntervalFadeIn.y = clampr(vIntervalFadeIn.y, 0.0f, 1.0f);

            R_ASSERT2(m_vIntervalFadeOut.y > m_vIntervalFadeOut.x,
                make_string(
                    "Wrong fade effect params, Fade Out End [%f] <= Fade Out Start [%f]",
                    m_vIntervalFadeOut.y, m_vIntervalFadeOut.x));
            m_vIntervalFadeOut.x = clampr(vIntervalFadeOut.x, 0.0f, 1.0f);
            m_vIntervalFadeOut.y = clampr(vIntervalFadeOut.y, 0.0f, 1.0f);

            m_bInverseMode = bInverted;
            m_bEnabled = true;

            R_ASSERT2(m_vIntervalFadeOut.x > m_vIntervalFadeIn.y,
                make_string("Wrong fade effect params, Fade Out Start [%f] <= Fade In End [%f] or not inside [0.0f - 1.0f]",
                    vIntervalFadeOut.x, vIntervalFadeIn.y));
        }
    } m_fade; // For fade in\out effect (CKinematics::CalculateBonesAdditionalTransforms)

    additional_bone_transform(SourceID srcID) : m_bone_id(-1)
    {
        VERIFY(srcID != SourceID::_ANY);
        m_source_id = srcID;
        m_transform_local.identity();
        m_transform_global.identity();
    }

    // Установить глобальный поворот кости (без учёта направления кости)
    IC void setRotGlobal(const float _x, const float _y, const float _z)
    {
        //--> Запоминаем текущее смещение кости - оно будет сброшено
        Fvector3 vSavedPos = Fvector3().set(m_transform_global.c.x, m_transform_global.c.y, m_transform_global.c.z);

        //--> Делаем поворот
        m_transform_global.setXYZ(_x, _y, _z);

        //--> Восстанавливаем смещение кости
        setPosOffsetGlobal(vSavedPos);
    }

    IC void setRotGlobal(Fvector& vRotOfs)
    {
        setRotGlobal(vRotOfs.x, vRotOfs.y, vRotOfs.z);
    }

    // Установить глобальное смещение позиции кости (без учёта направления кости)
    IC void setPosOffsetGlobal(const float _x, const float _y, const float _z)
    {
        m_transform_global.translate_over(_x, _y, _z);
    }

    IC void setPosOffsetGlobal(Fvector& vPosOfs)
    {
        setPosOffsetGlobal(vPosOfs.x, vPosOfs.y, vPosOfs.z);
    }

    // Установить поворот кости (с учётом направления кости)
    IC void setRotLocal(const float _x, const float _y, const float _z)
    {
        //--> Запоминаем текущее смещение кости - оно будет сброшено
        Fvector3 vSavedPos = Fvector3().set(m_transform_local.c.x, m_transform_local.c.y, m_transform_local.c.z);

        //--> Делаем поворот
        Fquaternion Q;
        Q.rotationYawPitchRoll(_y, _x, _z);
        m_transform_local.rotation(Q);

        //--> Восстанавливаем смещение кости
        setPosOffsetLocal(vSavedPos);
    }

    IC void setRotLocal(Fvector& vRotOfs)
    {
        setRotLocal(vRotOfs.x, vRotOfs.y, vRotOfs.z);
    }

    // Установить локальное смещение позиции кости (с учётом направления кости)
    IC void setPosOffsetLocal(const float _x, const float _y, const float _z)
    {
        m_transform_local.translate_over(_x, _y, _z);
    }

    IC void setPosOffsetLocal(Fvector& vPosOfs)
    {
        setPosOffsetLocal(vPosOfs.x, vPosOfs.y, vPosOfs.z);
    }
};
}
