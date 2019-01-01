/**************************/
/***** Инерция у худа *****/ //--#SM+#--
/**************************/

#include "stdafx.h"
#include "player_hud_inertion.h"
#include "player_hud.h"
#include "HudItem.h"

// Проверка разрешена-ли инерция худа
bool player_hud::inertion_allowed()
{
    attachable_hud_item* hi = m_attached_items[0];
    if (hi)
    {
        bool res = (hi->m_parent_hud_item->HudInertionEnabled() && hi->m_parent_hud_item->HudInertionAllowed());
        return res;
    }
    return true;
}

// Обновление инерции худа --#SM+#--
void player_hud::update_inertion(Fmatrix& trans)
{
    if (inertion_allowed())
    {
        attachable_hud_item* pMainHud = m_attached_items[0];

        Fmatrix  xform;
        Fvector& origin = trans.c;
        xform           = trans;

        static Fvector st_last_dir = {0, 0, 0};

        // load params
        hud_item_measures::inertion_params inertion_data;
        if (pMainHud != NULL)
        { // Загружаем параметры инерции из основного худа
            inertion_data.m_pitch_offset_r    = pMainHud->m_measures.m_inertion_params.m_pitch_offset_r;
            inertion_data.m_pitch_offset_n    = pMainHud->m_measures.m_inertion_params.m_pitch_offset_n;
            inertion_data.m_pitch_offset_d    = pMainHud->m_measures.m_inertion_params.m_pitch_offset_d;
            inertion_data.m_pitch_low_limit   = pMainHud->m_measures.m_inertion_params.m_pitch_low_limit;
            inertion_data.m_origin_offset     = pMainHud->m_measures.m_inertion_params.m_origin_offset;
            inertion_data.m_origin_offset_aim = pMainHud->m_measures.m_inertion_params.m_origin_offset_aim;
            inertion_data.m_tendto_speed      = pMainHud->m_measures.m_inertion_params.m_tendto_speed;
            inertion_data.m_tendto_speed_aim  = pMainHud->m_measures.m_inertion_params.m_tendto_speed_aim;
        }
        else
        { // Загружаем дефолтные параметры инерции
            inertion_data.m_pitch_offset_r    = PITCH_OFFSET_R;
            inertion_data.m_pitch_offset_n    = PITCH_OFFSET_N;
            inertion_data.m_pitch_offset_d    = PITCH_OFFSET_D;
            inertion_data.m_pitch_low_limit   = PITCH_LOW_LIMIT;
            inertion_data.m_origin_offset     = ORIGIN_OFFSET;
            inertion_data.m_origin_offset_aim = ORIGIN_OFFSET_AIM;
            inertion_data.m_tendto_speed      = TENDTO_SPEED;
            inertion_data.m_tendto_speed_aim  = TENDTO_SPEED_AIM;
        }

        // calc difference
        Fvector diff_dir;
        diff_dir.sub(xform.k, st_last_dir);

        // clamp by PI_DIV_2
        Fvector last;
        last.normalize_safe(st_last_dir);
        float dot = last.dotproduct(xform.k);
        if (dot < EPS)
        {
            Fvector v0;
            v0.crossproduct(st_last_dir, xform.k);
            st_last_dir.crossproduct(xform.k, v0);
            diff_dir.sub(xform.k, st_last_dir);
        }

        // tend to forward
        float _tendto_speed, _origin_offset;
        if (pMainHud != NULL && pMainHud->m_parent_hud_item->GetCurrentHudOffsetIdx() > 0)
        { // Худ в режиме "Прицеливание"
            float factor   = pMainHud->m_parent_hud_item->GetInertionAimFactor();
            _tendto_speed  = inertion_data.m_tendto_speed_aim - (inertion_data.m_tendto_speed_aim - inertion_data.m_tendto_speed) * factor;
            _origin_offset = inertion_data.m_origin_offset_aim - (inertion_data.m_origin_offset_aim - inertion_data.m_origin_offset) * factor;
        }
        else
        { // Худ в режиме "От бедра"
            _tendto_speed  = inertion_data.m_tendto_speed;
            _origin_offset = inertion_data.m_origin_offset;
        }

        // Фактор силы инерции
        if (pMainHud != NULL)
        {
            float power_factor = pMainHud->m_parent_hud_item->GetInertionPowerFactor();
            _tendto_speed *= power_factor;
            _origin_offset *= power_factor;
        }

        st_last_dir.mad(diff_dir, _tendto_speed * Device.fTimeDelta);
        origin.mad(diff_dir, _origin_offset);

        // pitch compensation
        float pitch = angle_normalize_signed(xform.k.getP());

        if (pMainHud != NULL)
            pitch *= pMainHud->m_parent_hud_item->GetInertionAimFactor();

        // Отдаление\приближение
        origin.mad(xform.k, -pitch * inertion_data.m_pitch_offset_d);

        // Сдвиг в противоположную часть экрана
        origin.mad(xform.i, -pitch * inertion_data.m_pitch_offset_r);

        // Подьём\опускание
        clamp(pitch, inertion_data.m_pitch_low_limit, PI);
        origin.mad(xform.j, -pitch * inertion_data.m_pitch_offset_n);
    }
}