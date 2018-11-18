/****************************/
/***** Худовые анимации *****/ //--#SM+#--
/****************************/

#include "stdafx.h"
#include "player_hud_motion.h"

player_hud_motion* player_hud_motion_container::find_motion(const shared_str& name)
{
    xr_vector<player_hud_motion>::iterator it   = m_anims.begin();
    xr_vector<player_hud_motion>::iterator it_e = m_anims.end();
    for (; it != it_e; ++it)
    {
        const shared_str& s = (true) ? (*it).m_alias_name : (*it).m_base_name;
        if (s == name)
            return &(*it);
    }
    return NULL;
}

void player_hud_motion_container::load(IKinematicsAnimated* model, const shared_str& sect) //--#SM+#--
{
    CInifile::Sect&    _sect = pSettings->r_section(sect);
    CInifile::SectCIt  _b    = _sect.Data.begin();
    CInifile::SectCIt  _e    = _sect.Data.end();
    player_hud_motion* pm    = NULL;

    string512 buff;
    MotionID  motion_ID;

    for (; _b != _e; ++_b)
    {
        if (strstr(_b->first.c_str(), "anm_") == _b->first.c_str())
        {
            const shared_str& anm = _b->second;
            m_anims.resize(m_anims.size() + 1);
            pm = &m_anims.back();
            //base and alias name
            pm->m_alias_name = _b->first;

            if (_GetItemCount(anm.c_str()) == 1)
            {
                pm->m_base_name       = anm;
                pm->m_additional_name = anm;
                pm->m_child_name      = "_";
            }
            else
            {
                if (_GetItemCount(anm.c_str()) == 3)
                {
                    string512 str_item;
                    _GetItem(anm.c_str(), 0, str_item);
                    pm->m_base_name = str_item;

                    _GetItem(anm.c_str(), 1, str_item);
                    pm->m_additional_name = str_item;

                    _GetItem(anm.c_str(), 2, str_item);
                    pm->m_child_name = str_item;
                }
                else
                {
                    R_ASSERT2(_GetItemCount(anm.c_str()) == 2, anm.c_str());
                    string512 str_item;
                    _GetItem(anm.c_str(), 0, str_item);
                    pm->m_base_name = str_item;

                    _GetItem(anm.c_str(), 1, str_item);
                    pm->m_additional_name = str_item;

                    pm->m_child_name = "_";
                }
            }

            //and load all motions for it
            if (!pm->m_base_name.equal("_"))
            {
                for (u32 i = 0; i <= 8; ++i)
                {
                    if (i == 0)
                        xr_strcpy(buff, pm->m_base_name.c_str());
                    else
                        xr_sprintf(buff, "%s%d", pm->m_base_name.c_str(), i);

                    motion_ID = model->ID_Cycle_Safe(buff);
                    if (motion_ID.valid())
                    {
                        pm->m_animations.resize(pm->m_animations.size() + 1);
                        pm->m_animations.back().mid  = motion_ID;
                        pm->m_animations.back().name = buff;
#ifdef DEBUG
                        //						Msg(" alias=[%s] base=[%s] name=[%s]",pm->m_alias_name.c_str(), pm->m_base_name.c_str(), buff);
#endif // #ifdef DEBUG
                    }
                }
                R_ASSERT2(pm->m_animations.size(), make_string("motion not found [%s]", pm->m_base_name.c_str()).c_str());
            }
        }
    }
}

// Модифицировать параметры анимации (скорость и стартовую секунду) --#SM+#--
//-> Значение fSpeed может быть положительным и отрицательным (анимация будет идти реверсивно)
//--> Однако для отрицательного значения обязательно требуется указать fStartFromTime - время старта анимации
//-> Значение fStartFromTime в пределах ...
//--> [0.0    ... float(max)] - время старта анимации указано в виде конкретной секунды
//--> [-0.001 ... -1.0]       - время старта анимации указано в виде процентов от её всей длины (0% - 100%)
float ModifyBlendParams(CBlend* B, float fSpeed, float fStartFromTime)
{
    R_ASSERT(B);

    // Модифицируем скорость анимации
    B->speed *= fSpeed;

    // Устанавливаем время начала анимации
    B->timeCurrent = CalculateMotionStartSeconds(fStartFromTime, B->timeTotal);

    return B->timeCurrent;
}

// Рассчитать стартовую секунду анимации --#SM+#--
float CalculateMotionStartSeconds(float fStartFromTime, float fMotionLength)
{
    R_ASSERT(fStartFromTime >= -1.0f);

    if (fStartFromTime >= 0.0f)
    { // Выставляем время в точных значениях
        clamp(fStartFromTime, 0.0f, fMotionLength);
        return abs(fStartFromTime);
    }
    else
    { // Выставляем время в процентных значениях (от всей длины анимации)
        return (abs(fStartFromTime) * fMotionLength);
    }
}