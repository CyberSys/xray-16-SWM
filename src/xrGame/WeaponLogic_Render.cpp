/*************************************************/
/***** Код для рендеринга оружия *****/ //--#SM+#--
/*************************************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "WeaponKnifeHit.h"
#include "WeaponBinocularsVision.h"

#include "xrEngine/vis_object_data.h" //--#SM+#--
#include "attachable_visual.h"
#include "HUDManager.h"
#include "player_hud.h"

#ifdef DEBUG
#include "debug_renderer.h"
#endif

#define BULLET_BONE_NAME "wpn_bullet" // Имя кости, вариации которой будут показаны\скрыты в зависимости от числа патронов в оружии

extern u32 hud_adj_mode;

// Проверка, что сейчас включён вид от 1-го лица (худ показывается)
bool CWeapon::IsHudModeNow() { return (HudItemData() != NULL); }

// Требуется-ли отрисовывать перекрестие
bool CWeapon::show_crosshair()
{
    if (hud_adj_mode)
        return true;

    if (!IsZoomed() && m_bHideCrosshair)
        return false;

    if (GetState() == eKick)
        return true;

    if (IsMagazine())
        return false;

    return !IsPending() && (!IsZoomed() || !ZoomHideCrosshair());
}

// Требуется-ли отображать интерфейс игрока
bool CWeapon::show_indicators() { return !(IsZoomed() && ZoomTexture()); }

// Получить текущий износ оружия для UI
float CWeapon::GetConditionToShow() const
{
    return (GetCondition()); //powf(GetCondition(),4.0f));
}

// Получить индекс для текущих координат худа (от бедра, от прицела, ...)
u8 CWeapon::GetCurrentHudOffsetIdx()
{
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return 0;

    bool b_aiming = ((IsZoomed() && m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_fZoomRotationFactor > 0.f));

    if (!b_aiming)
        return 0;
    else if (GetZoomType() == eZoomMain)
        return 1;
    else if (GetZoomType() == eZoomGL)
        return 2;
    else if (GetZoomType() == eZoomAlt)
        return 3;
    else
        R_ASSERT2("Unknown zoom type", GetZoomType());

    return 0;
}

// Обновление координат текущего худа
void CWeapon::UpdateHudAdditonal(Fmatrix& trans)
{
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return;

    attachable_hud_item* hi = HudItemData();
    R_ASSERT(hi);

    u8 idx = GetCurrentHudOffsetIdx();

    //============= Поворот ствола во время аима =============//
    if ((IsZoomed() && m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_fZoomRotationFactor > 0.f))
    {
        Fvector curr_offs, curr_rot;
        curr_offs = hi->m_measures.m_hands_offset[0][idx]; //pos,aim
        curr_rot  = hi->m_measures.m_hands_offset[1][idx]; //rot,aim

        if (IsBipodsDeployed())
        {
            curr_offs.z += (m_bipods.fHudZOffset * GetBipodsTranslationFactor());
        }

        curr_offs.mul(m_fZoomRotationFactor);
        curr_rot.mul(m_fZoomRotationFactor);

        Fmatrix hud_rotation;
        hud_rotation.identity();
        hud_rotation.rotateX(curr_rot.x);

        Fmatrix hud_rotation_y;
        hud_rotation_y.identity();
        hud_rotation_y.rotateY(curr_rot.y);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation_y.identity();
        hud_rotation_y.rotateZ(curr_rot.z);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation.translate_over(curr_offs);
        trans.mulB_43(hud_rotation);

        if (pActor->IsZoomAimingMode())
            m_fZoomRotationFactor += Device.fTimeDelta / m_fZoomRotateTime;
        else
            m_fZoomRotationFactor -= Device.fTimeDelta / m_fZoomRotateTime;

        clamp(m_fZoomRotationFactor, 0.f, 1.f);
    }

    //============= Подготавливаем общие переменные =============//
    clamp(idx, u8(0), u8(1));
    bool bForAim = (idx == 1);

    float fInertiaPower = GetInertionPowerFactor();

    float fYMag = pActor->fFPCamYawMagnitude;
    float fPMag = pActor->fFPCamPitchMagnitude;

    static float fAvgTimeDelta = Device.fTimeDelta;
    _inertion(fAvgTimeDelta, Device.fTimeDelta, 0.8f);

    //============= Сдвиг оружия при стрельбе =============//
    // Параметры сдвига
    float fShootingReturnSpeedMod = _lerp(
        hi->m_measures.m_shooting_params.m_ret_speed,
        hi->m_measures.m_shooting_params.m_ret_speed_aim,
        m_fZoomRotationFactor);
    
    float fShootingBackwOffset = _lerp(
        hi->m_measures.m_shooting_params.m_shot_offset_BACKW.x,
        hi->m_measures.m_shooting_params.m_shot_offset_BACKW.y,
        m_fZoomRotationFactor);
  
    Fvector4 vShOffsets; // x = L, y = R, z = U, w = D
    vShOffsets.x = _lerp(
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.x,
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.x,
        m_fZoomRotationFactor);
    vShOffsets.y = _lerp(
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.y,
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.y,
        m_fZoomRotationFactor);
    vShOffsets.z = _lerp(
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.z,
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.z,
        m_fZoomRotationFactor);
    vShOffsets.w = _lerp(
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD.w,
        hi->m_measures.m_shooting_params.m_shot_max_offset_LRUD_aim.w,
        m_fZoomRotationFactor);

    // Плавное затухание сдвига от стрельбы (основное, но без линейной никогда не опустит до полного 0.0f)
    m_fLR_ShootingFactor    *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);
    m_fUD_ShootingFactor    *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);
    m_fBACKW_ShootingFactor *= clampr(1.f - fAvgTimeDelta * fShootingReturnSpeedMod, 0.0f, 1.0f);

    // Минимальное линейное затухание сдвига от стрельбы при покое (горизонталь)
    {
        float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
        if (m_fLR_ShootingFactor < 0.0f)
        {
            m_fLR_ShootingFactor += fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fLR_ShootingFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fLR_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fLR_ShootingFactor, 0.0f, 1.0f);
        }
    }

    // Минимальное линейное затухание сдвига от стрельбы при покое (вертикаль)
    {
        float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
        if (m_fUD_ShootingFactor < 0.0f)
        {
            m_fUD_ShootingFactor += fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fUD_ShootingFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fUD_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fUD_ShootingFactor, 0.0f, 1.0f);
        }
    }

    // Минимальное линейное затухание сдвига от стрельбы при покое (вперёд\назад)
    {
        float fRetSpeedMod = fShootingReturnSpeedMod * 0.125f;
        m_fBACKW_ShootingFactor -= fAvgTimeDelta * fRetSpeedMod;
        clamp(m_fBACKW_ShootingFactor, 0.0f, 1.0f);
    }

    // Применяем сдвиг от стрельбы к худу
    {
        float fLR_lim = (m_fLR_ShootingFactor < 0.0f ? vShOffsets.x : vShOffsets.y);
        float fUD_lim = (m_fUD_ShootingFactor < 0.0f ? vShOffsets.z : vShOffsets.w);

        Fvector curr_offs;
        curr_offs = {fLR_lim * m_fLR_ShootingFactor, fUD_lim * -1.f * m_fUD_ShootingFactor, -1.f * fShootingBackwOffset * m_fBACKW_ShootingFactor};

        //--> Глушители могут изменять силу тряски
        curr_offs.mul(cur_silencer_koef.shooting_shake);

        //--> Прочие generic-аддоны могут изменять силу тряски
        float fTrgtShakeKoef = 1.0f;
        for (int iSlot = 0; iSlot < EAddons::eAddonsSize; iSlot++)
        {
            SAddonData* pAddon = GetAddonBySlot((CWeapon::EAddons)iSlot);
            if (pAddon->bActive)
            {
                fTrgtShakeKoef *= pAddon->m_kShootingShake;
            }
        }
        curr_offs.mul(fTrgtShakeKoef);

        Fmatrix hud_rotation;
        hud_rotation.identity();
        hud_rotation.translate_over(curr_offs);
        trans.mulB_43(hud_rotation);
    }

    //======== Проверяем доступность инерции и стрейфа ========//
    if (!g_player_hud->inertion_allowed())
        return;

    //============= Боковой стрейф с оружием =============//
    // Рассчитываем фактор боковой ходьбы
    float fStrafeMaxTime = hi->m_measures.m_strafe_offset[2][idx].y; // Макс. время в секундах, за которое мы наклонимся из центрального положения
    if (fStrafeMaxTime <= EPS)
        fStrafeMaxTime = 0.01f;

    float fStepPerUpd = fAvgTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота

    // Добавляем боковой наклон от движения камеры
    float fCamReturnSpeedMod = 1.5f; // Восколько ускоряем нормализацию наклона, полученного от движения камеры (только от бедра)

    // Высчитываем минимальную скорость поворота камеры для начала инерции
    float fStrafeMinAngle = _lerp(
        hi->m_measures.m_strafe_offset[3][0].y,
        hi->m_measures.m_strafe_offset[3][1].y,
        m_fZoomRotationFactor);

    // Высчитываем мксимальный наклон от поворота камеры
    float fCamLimitBlend = _lerp(
        hi->m_measures.m_strafe_offset[3][0].x,
        hi->m_measures.m_strafe_offset[3][1].x,
        m_fZoomRotationFactor);

    // Считаем стрейф от поворота камеры
    if (abs(fYMag) > (m_fLR_CameraFactor == 0.0f ? fStrafeMinAngle : 0.0f))
    { //--> Камера крутится по оси Y
        m_fLR_CameraFactor -= (fYMag * fAvgTimeDelta * 0.75f);
        clamp(m_fLR_CameraFactor, -fCamLimitBlend, fCamLimitBlend);
    }
    else
    { //--> Камера не поворачивается - убираем наклон
        if (m_fLR_CameraFactor < 0.0f)
        {
            m_fLR_CameraFactor += fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
            clamp(m_fLR_CameraFactor, -fCamLimitBlend, 0.0f);
        }
        else
        {
            m_fLR_CameraFactor -= fStepPerUpd * (bForAim ? 1.0f : fCamReturnSpeedMod);
            clamp(m_fLR_CameraFactor, 0.0f, fCamLimitBlend);
        }
    }

    // Добавляем боковой наклон от ходьбы вбок
    float fChangeDirSpeedMod = 3; // Восколько быстро меняем направление направление наклона, если оно в другую сторону от текущего
    u32 iMovingState = pActor->MovingState();
    if ((iMovingState & mcLStrafe) != 0)
    { // Движемся влево
        float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
        m_fLR_MovingFactor -= fVal;
    }
    else if ((iMovingState & mcRStrafe) != 0)
    { // Движемся вправо
        float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * fChangeDirSpeedMod : fStepPerUpd);
        m_fLR_MovingFactor += fVal;
    }
    else
    { // Двигаемся в любом другом направлении - плавно убираем наклон
        if (m_fLR_MovingFactor < 0.0f)
        {
            m_fLR_MovingFactor += fStepPerUpd;
            clamp(m_fLR_MovingFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fLR_MovingFactor -= fStepPerUpd;
            clamp(m_fLR_MovingFactor, 0.0f, 1.0f);
        }
    }
    clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

    // Вычисляем и нормализируем итоговый фактор наклона
    float fLR_Factor = m_fLR_MovingFactor + (m_fLR_CameraFactor * fInertiaPower);
    clamp(fLR_Factor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты

    // Производим наклон ствола для нормального режима и аима
    for (int _idx = 0; _idx <= 1; _idx++) //<-- Для плавного перехода
    {
        bool bEnabled = (hi->m_measures.m_strafe_offset[2][_idx].x != 0.0f);
        if (!bEnabled)
            continue;

        Fvector curr_offs, curr_rot;

        // Смещение позиции худа в стрейфе
        curr_offs = hi->m_measures.m_strafe_offset[0][_idx]; // pos
        curr_offs.mul(fLR_Factor); // Умножаем на фактор стрейфа

        // Поворот худа в стрейфе
        curr_rot = hi->m_measures.m_strafe_offset[1][_idx]; // rot
        curr_rot.mul(-PI / 180.f); // Преобразуем углы в радианы
        curr_rot.mul(fLR_Factor); // Умножаем на фактор стрейфа

        // Мягкий переход между бедром \ прицелом
        if (_idx == 0)
        { // От бедра
            curr_offs.mul(1.f - m_fZoomRotationFactor);
            curr_rot.mul(1.f - m_fZoomRotationFactor);
        }
        else
        { // Во время аима
            curr_offs.mul(m_fZoomRotationFactor);
            curr_rot.mul(m_fZoomRotationFactor);
        }

        Fmatrix hud_rotation;
        Fmatrix hud_rotation_y;

        hud_rotation.identity();
        hud_rotation.rotateX(curr_rot.x);

        hud_rotation_y.identity();
        hud_rotation_y.rotateY(curr_rot.y);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation_y.identity();
        hud_rotation_y.rotateZ(curr_rot.z);
        hud_rotation.mulA_43(hud_rotation_y);

        hud_rotation.translate_over(curr_offs);
        trans.mulB_43(hud_rotation);
    }

    //============= Инерция оружия =============//
    // Параметры инерции
    float fInertiaSpeedMod = _lerp(
        hi->m_measures.m_inertion_params.m_tendto_speed,
        hi->m_measures.m_inertion_params.m_tendto_speed_aim,
        m_fZoomRotationFactor);
    
    float fInertiaReturnSpeedMod = _lerp(
        hi->m_measures.m_inertion_params.m_tendto_ret_speed,
        hi->m_measures.m_inertion_params.m_tendto_ret_speed_aim,
        m_fZoomRotationFactor);

    float fInertiaMinAngle = _lerp(
        hi->m_measures.m_inertion_params.m_min_angle,
        hi->m_measures.m_inertion_params.m_min_angle_aim,
        m_fZoomRotationFactor);

    Fvector4 vIOffsets; // x = L, y = R, z = U, w = D
    vIOffsets.x = _lerp(
        hi->m_measures.m_inertion_params.m_offset_LRUD.x,
        hi->m_measures.m_inertion_params.m_offset_LRUD_aim.x,
        m_fZoomRotationFactor) * fInertiaPower;
    vIOffsets.y = _lerp(
        hi->m_measures.m_inertion_params.m_offset_LRUD.y,
        hi->m_measures.m_inertion_params.m_offset_LRUD_aim.y,
        m_fZoomRotationFactor) * fInertiaPower;
    vIOffsets.z = _lerp(
        hi->m_measures.m_inertion_params.m_offset_LRUD.z,
        hi->m_measures.m_inertion_params.m_offset_LRUD_aim.z,
        m_fZoomRotationFactor) * fInertiaPower;
    vIOffsets.w = _lerp(
        hi->m_measures.m_inertion_params.m_offset_LRUD.w,
        hi->m_measures.m_inertion_params.m_offset_LRUD_aim.w,
        m_fZoomRotationFactor) * fInertiaPower;

    // Высчитываем инерцию из поворотов камеры
    bool bIsInertionPresent = m_fLR_InertiaFactor != 0.0f || m_fUD_InertiaFactor != 0.0f;
    if (abs(fYMag) > fInertiaMinAngle || bIsInertionPresent)
    {
        float fSpeed = fInertiaSpeedMod;
        if (fYMag > 0.0f && m_fLR_InertiaFactor > 0.0f ||
            fYMag < 0.0f && m_fLR_InertiaFactor < 0.0f)
        {
            fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
        }

        m_fLR_InertiaFactor -= (fYMag * fAvgTimeDelta * fSpeed); // Горизонталь (м.б. > |1.0|)
    }

    if (abs(fPMag) > fInertiaMinAngle || bIsInertionPresent)
    { 
        float fSpeed = fInertiaSpeedMod;
        if (fPMag > 0.0f && m_fUD_InertiaFactor > 0.0f ||
            fPMag < 0.0f && m_fUD_InertiaFactor < 0.0f)
        {
            fSpeed *= 2.f; //--> Ускоряем инерцию при движении в противоположную сторону
        }

        m_fUD_InertiaFactor -= (fPMag * fAvgTimeDelta * fSpeed); // Вертикаль (м.б. > |1.0|)
    }

    clamp(m_fLR_InertiaFactor, -1.0f, 1.0f);
    clamp(m_fUD_InertiaFactor, -1.0f, 1.0f);

    // Плавное затухание инерции (основное, но без линейной никогда не опустит инерцию до полного 0.0f)
    m_fLR_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);
    m_fUD_InertiaFactor *= clampr(1.f - fAvgTimeDelta * fInertiaReturnSpeedMod, 0.0f, 1.0f);

    // Минимальное линейное затухание инерции при покое (горизонталь)
    if (fYMag == 0.0f)
    {
        float fRetSpeedMod = (fYMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
        if (m_fLR_InertiaFactor < 0.0f)
        {
            m_fLR_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fLR_InertiaFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fLR_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fLR_InertiaFactor, 0.0f, 1.0f);
        }
    }

    // Минимальное линейное затухание инерции при покое (вертикаль)
    if (fPMag == 0.0f)
    {
        float fRetSpeedMod = (fPMag == 0.0f ? 1.0f : 0.75f) * (fInertiaReturnSpeedMod * 0.075f);
        if (m_fUD_InertiaFactor < 0.0f)
        {
            m_fUD_InertiaFactor += fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fUD_InertiaFactor, -1.0f, 0.0f);
        }
        else
        {
            m_fUD_InertiaFactor -= fAvgTimeDelta * fRetSpeedMod;
            clamp(m_fUD_InertiaFactor, 0.0f, 1.0f);
        }
    }

    // Применяем инерцию к худу
    float fLR_lim = (m_fLR_InertiaFactor < 0.0f ? vIOffsets.x : vIOffsets.y);
    float fUD_lim = (m_fUD_InertiaFactor < 0.0f ? vIOffsets.z : vIOffsets.w);

    Fvector curr_offs;
    curr_offs = {fLR_lim * -1.f * m_fLR_InertiaFactor, fUD_lim * m_fUD_InertiaFactor, 0.0f};

    Fmatrix hud_rotation;
    hud_rotation.identity();
    hud_rotation.translate_over(curr_offs);
    trans.mulB_43(hud_rotation);
}

// Добавить эффект сдвига оружия от выстрела
void CWeapon::AddHUDShootingEffect() 
{
    if (IsHidden() || ParentIsActor() == false)
        return;

    // Отдача назад
    m_fBACKW_ShootingFactor = 1.0f;

    // Отдача в бока
    float fPowerMin = 0.0f;
    attachable_hud_item* hi = HudItemData();
    if (hi != nullptr)
    {
        fPowerMin = clampr(hi->m_measures.m_shooting_params.m_min_LRUD_power, 0.0f, 0.99f);
    }

    float fPowerRnd = 1.0f - fPowerMin;

    m_fLR_ShootingFactor = ::Random.randF(-fPowerRnd, fPowerRnd);
    m_fLR_ShootingFactor += (m_fLR_ShootingFactor >= 0.0f ? fPowerMin : -fPowerMin);
    
    m_fUD_ShootingFactor = ::Random.randF(-fPowerRnd, fPowerRnd);
    m_fUD_ShootingFactor += (m_fUD_ShootingFactor >= 0.0f ? fPowerMin : -fPowerMin);
}

// Получить мировой FOV от текущего оружия игрока
float CWeapon::GetFov() const
{
    if (IsBipodsDeployed() && !ZoomTexture())
    { // FOV при сошках
        if (m_bipods.m_bZoomMode)
        {
            return (IsScopeAttached() && GetZoomParams().m_fScopeBipodsZoomFov > 0.0f ?
                    GetZoomParams().m_fScopeBipodsZoomFov :
                    m_bipods.fZoomFOV);
        }
        else
            return g_fov;
    }
    else
    { // FOV при обычном зуме
        if (IsZoomed() && (!ZoomTexture() || (!IsRotatingToZoom() && ZoomTexture())))
        {
            float fCurZoomFactor = (GetZoomParams().m_bUseDynamicZoom && !IsSecondVPZoomPresent()) ?
                GetZoomParams().m_fRTZoomFactor : //--> Динамический зум, только если прицел без двойного вьюпорта
                GetAimZoomFactor();

            return fCurZoomFactor;
        }
    }

    // Дефолт (от бедра)
    return g_fov;
}

// Получить линзовый FOV от текущего оружия игрока для второго вьюпорта
float CWeapon::GetFovSVP() const
{
    if (GetZoomParams().m_bUseDynamicZoom && IsSecondVPZoomPresent())
        return GetZoomParams().m_fRTZoomFactor;

    return GetAimZoomFactor(true);
}

// Получить HUD FOV от текущего оружия игрока
float CWeapon::GetHudFov()
{
    // Рассчитываем HUD FOV от бедра (с учётом упирания в стены)
    if (ParentIsActor() && Level().CurrentViewEntity() == H_Parent())
    {
        // Получаем расстояние от камеры до точки в прицеле
        CCameraBase*        pCam = H_Parent()->cast_actor()->cam_Active();
        collide::rq_result& RQ   = HUD().GetCurrentRayQuery();
        float               dist = RQ.range;

        // Интерполируем расстояние в диапазон от 0 (min) до 1 (max)
        clamp(dist, m_nearwall_dist_min, m_nearwall_dist_max);
        float fDistanceMod = ((dist - m_nearwall_dist_min) / (m_nearwall_dist_max - m_nearwall_dist_min)); // 0.f ... 1.f

        // Рассчитываем базовый HUD FOV от бедра
        float fBaseFov = psHUD_FOV_def + m_HudFovAddition;
        clamp(fBaseFov, 0.0f, FLT_MAX);

        // Плавно высчитываем итоговый FOV от бедра
        float src = m_nearwall_speed_mod * Device.fTimeDelta;
        clamp(src, 0.f, 1.f);

        float fTrgFov           = m_nearwall_target_hud_fov + fDistanceMod * (fBaseFov - m_nearwall_target_hud_fov);
        m_nearwall_last_hud_fov = m_nearwall_last_hud_fov * (1 - src) + fTrgFov * src;
    }

    // Возвращаем итоговый HUD FOV
    float fRes = 1.0f;

    if (m_fZoomRotationFactor > 0.0f)
    {
        //--> В процессе зума
        float fDiff = m_nearwall_last_hud_fov - GetZoomParams().m_fZoomHudFov;
        fRes = GetZoomParams().m_fZoomHudFov + (fDiff * (1 - m_fZoomRotationFactor));
    }
    else
    {
        //--> От бедра
        fRes = m_nearwall_last_hud_fov;
    }
    
    if (IsBipodsDeployed())
    { 
        // При разложенных сошках
        fRes = _lerpc(fRes, m_bipods.fHudFOVFactor, GetBipodsTranslationFactor());
    }
    
    return fRes;
}

// Получить фактор силы инерции и cam-стрейфа (насколько далеко и быстро влияет на оружие)
float CWeapon::GetInertionPowerFactor()
{
    if (IsBipodsDeployed())
    {
        return 1.0f - ((1.0f - m_bipods.fInertiaMod) * GetZRotatingFactor());
    }

    return 1.0f;
}

// Требуется-ли отрисовывать худ
bool CWeapon::need_renderable() { return !Device.m_SecondViewport.IsSVPFrame() && !(IsZoomed() && ZoomTexture() && !IsRotatingToZoom()); }

// Требуется-ли отрисовывать UI-элементы от оружия (эффекты прицела)
bool CWeapon::render_item_ui_query()
{
    bool b_is_active_item = (m_pInventory->ActiveItem() == this);
    bool res              = b_is_active_item && IsZoomed() && ZoomHideCrosshair() && ZoomTexture() && !IsRotatingToZoom();
    return res;
}

// Отрисовать UI оружия
void CWeapon::render_item_ui()
{
    if (GetZoomParams().m_pVision != NULL)
        GetZoomParams().m_pVision->Draw();

    ZoomTexture()->Update();
    ZoomTexture()->Draw();
}

// Рендеринг в режиме худа
void CWeapon::render_hud_mode()
{
    // Рисуем подсветку
    RenderLight();

    // Рисуем инфу от сошек
    BipodsOnRender(true);
}

// Апдейт во время рендера оружия (вызывается только для оружия в зоне видимости игрока)
// НЕ ВЫЗЫВАЕТСЯ когда оружие в руках от первого лица, т.к его мировая модель в этот момент не рендерится <!>
void CWeapon::renderable_Render()
{
    // Обновляем XForm от третьего лица
    UpdateXForm();

    // Рисуем подсветку
    RenderLight();

    // Рисуем инфу от сошек
    BipodsOnRender(false);

    inherited::renderable_Render();
}

// Отрисовывать спец-фигуры при дебаге
void CWeapon::debug_draw_firedeps()
{
#ifdef DEBUG
    if (hud_adj_mode == 5 || hud_adj_mode == 6 || hud_adj_mode == 7)
    {
        CDebugRenderer& render = Level().debug_renderer();

        if (hud_adj_mode == 5)
            render.draw_aabb(get_LastFP(), 0.005f, 0.005f, 0.005f, color_xrgb(255, 0, 0));

        if (hud_adj_mode == 6)
            render.draw_aabb(get_LastFP2(), 0.005f, 0.005f, 0.005f, color_xrgb(0, 0, 255));

        if (hud_adj_mode == 7)
            render.draw_aabb(get_LastSP(), 0.005f, 0.005f, 0.005f, color_xrgb(0, 255, 0));
    }
#endif // DEBUG
}

// Перерасчёт позиции точек стрельбы\гильз по текущим смещениям
void CWeapon::UpdateFireDependencies_internal()
{
    if (Device.dwFrame != dwFP_Frame)
    {
        dwFP_Frame = Device.dwFrame;

        UpdateXForm();

        if (GetHUDmode())
        {
            attachable_hud_item* pHudItem = HudItemData();
            pHudItem->setup_firedeps(m_current_firedeps);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));

            CShellLauncher::RebuildLaunchParams(pHudItem->m_item_transform, pHudItem->m_model, true); // Hud
        }
        else
        {
            // 3rd person or no parent
            Fmatrix& parent = XFORM();
            Fvector& fp     = vLoadedFirePoint;
            Fvector& fp2    = vLoadedFirePoint2;
            Fvector& sp     = vLoadedShellPoint;

            parent.transform_tiny(m_current_firedeps.vLastFP, fp);
            parent.transform_tiny(m_current_firedeps.vLastFP2, fp2);
            parent.transform_tiny(m_current_firedeps.vLastSP, sp);

            m_current_firedeps.vLastFD.set(0.f, 0.f, 1.f);
            parent.transform_dir(m_current_firedeps.vLastFD);

            m_current_firedeps.m_FireParticlesXForm.set(parent);
            VERIFY(_valid(m_current_firedeps.m_FireParticlesXForm));

            CShellLauncher::RebuildLaunchParams(parent, Visual()->dcast_PKinematics(), false); // World
        }
    }
}

// Обновление текущей позиции оружия
void CWeapon::UpdateXForm()
{
    if (Device.dwFrame == dwXF_Frame)
        return;

    dwXF_Frame = Device.dwFrame;

    if (!H_Parent())
        return;

    // Get access to entity and its visual
    CEntityAlive* E = smart_cast<CEntityAlive*>(H_Parent());

    if (!E)
    {
        if (!IsGameTypeSingle())
            UpdatePosition(H_Parent()->XFORM());

        return;
    }

    const CInventoryOwner* parent = smart_cast<const CInventoryOwner*>(E);
    if (parent && parent->use_simplified_visual())
        return;

    if (parent->attached(this))
        return;

    IKinematics* V = smart_cast<IKinematics*>(E->Visual());
    VERIFY(V);

    // Get matrices
    int boneL = -1, boneR = -1, boneR2 = -1;

    // this ugly case is possible in case of a CustomMonster, not a Stalker, nor an Actor
    E->g_WeaponBones(boneL, boneR, boneR2);

    if (boneR == -1)
        return;

    if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
        boneL = boneR2;

    V->CalculateBones();
    Fmatrix& mL = V->LL_GetTransform(u16(boneL));
    Fmatrix& mR = V->LL_GetTransform(u16(boneR));
    // Calculate
    Fmatrix mRes;
    Fvector R, D, N;
    D.sub(mL.c, mR.c);

    if (fis_zero(D.magnitude()))
    {
        mRes.set(E->XFORM());
        mRes.c.set(mR.c);
    }
    else
    {
        D.normalize();
        R.crossproduct(mR.j, D);

        N.crossproduct(D, R);
        N.normalize();

        mRes.set(R, N, D, mR.c);
        mRes.mulA_43(E->XFORM());
    }

    UpdatePosition(mRes);
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
    Position().set(trans.c);
    XFORM().mul(trans, m_strapped_mode ? m_StrapOffset : m_Offset);
    VERIFY(!fis_zero(DET(renderable.xform)));
}

#ifdef DEBUG
void CWeapon::OnRender()
{
    if (m_first_attack != NULL)
        m_first_attack->OnRender();
    if (m_second_attack != NULL)
        m_second_attack->OnRender();
}
#endif

//*****  Партиклы и эффекты *****//

// Обновление партиклов стрельбы (от 3-его лица)
void CWeapon::ForceUpdateFireParticles()
{
    if (!GetHUDmode())
    { // Update particles XFORM real bullet direction

        if (!H_Parent())
            return;

        Fvector p, d;
        smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

        Fmatrix _pxf;
        _pxf.k = d;
        _pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
        _pxf.j.crossproduct(_pxf.k, _pxf.i);
        _pxf.c = XFORM().c;

        m_current_firedeps.m_FireParticlesXForm.set(_pxf);
    }
}

// Отыграть партикл альтернативного выстрела
void CWeapon::StartFlameParticles2() { CShootingObject::StartParticles(m_pFlameParticles2, *m_sFlameParticles2, get_LastFP2()); }

// Остановить партикл альтернативного выстрела
void CWeapon::StopFlameParticles2() { CShootingObject::StopParticles(m_pFlameParticles2); }

// Обновить позицию партиклов альтернативного выстрела
void CWeapon::UpdateFlameParticles2()
{
    if (m_pFlameParticles2)
        CShootingObject::UpdateParticles(m_pFlameParticles2, get_LastFP2());
}

// Остановить абсолютно все эффекты оружия
void CWeapon::StopAllEffects()
{
    StopFlameParticles();
    StopFlameParticles2();
    StopLight();
    Light_Destroy();
    RemoveShotEffector();
}

// Загрузить параметры света во время стрельбы
void CWeapon::LoadLights(LPCSTR section, LPCSTR prefix)
{
    // Загружаем из секции оружия
    CShootingObject::LoadLights(section, prefix);

    // Если одет глушитель, то пробуем загрузить параметры из секции глушителя в конфиге оружия
    if (IsSilencerAttached())
    {
        string256  full_name;
        shared_str m_sil_set_sect = GetSilencerSetSect();

        strconcat(sizeof(full_name), full_name, prefix, "light_color");
        if (pSettings->line_exist(m_sil_set_sect, full_name))
        {
            Fvector clr = pSettings->r_fvector3(m_sil_set_sect, full_name);
            light_base_color.set(clr.x, clr.y, clr.z, 1);
        }

        strconcat(sizeof(full_name), full_name, prefix, "light_range");
        if (pSettings->line_exist(m_sil_set_sect, full_name))
            light_base_range = pSettings->r_float(m_sil_set_sect, full_name);

        strconcat(sizeof(full_name), full_name, prefix, "light_var_color");
        if (pSettings->line_exist(m_sil_set_sect, full_name))
            light_var_color = pSettings->r_float(m_sil_set_sect, full_name);

        strconcat(sizeof(full_name), full_name, prefix, "light_var_range");
        if (pSettings->line_exist(m_sil_set_sect, full_name))
            light_var_range = pSettings->r_float(m_sil_set_sect, full_name);

        strconcat(sizeof(full_name), full_name, prefix, "light_time");
        if (pSettings->line_exist(m_sil_set_sect, full_name))
            light_lifetime = pSettings->r_float(m_sil_set_sect, full_name);
    }
}

// Проверить партиклы пламя\дыма на их присутствие в конфигах, иначе отключить их
void CWeapon::CheckFlameParticles(LPCSTR section, LPCSTR prefix)
{
    // Если одет глушитель, то возможно нам нужно отключить партиклы стрельбы
    if (IsSilencerAttached())
    {
        shared_str m_sil_set_sect = GetSilencerSetSect();

        //**** Проверяем партикл огня ****//
        string256 full_name;
        strconcat(sizeof(full_name), full_name, prefix, "flame_particles");

        // Если этой строки нет ни в конфиге оружия, ни в сэт-секции глушителя -> отключаем партикл
        if (!pSettings->line_exist(section, full_name) && !pSettings->line_exist(m_sil_set_sect, full_name))
        {
            m_sFlameParticlesCurrent = NULL;
        }

        //**** Проверяем партикл дыма ****//
        strconcat(sizeof(full_name), full_name, prefix, "smoke_particles");
        // Если этой строки нет ни в конфиге оружия, ни в сэт-секции глушителя -> отключаем партикл
        if (!pSettings->line_exist(section, full_name) && !pSettings->line_exist(m_sil_set_sect, full_name))
        {
            m_sSmokeParticlesCurrent = NULL;
        }
    }
}

//*****  Для эффекта отдачи оружия *****//
void CWeapon::AddShotEffector() { inventory_owner().on_weapon_shot_start(this); }

void CWeapon::RemoveShotEffector()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
    if (pInventoryOwner)
        pInventoryOwner->on_weapon_shot_remove(this);
}

void CWeapon::ClearShotEffector()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
    if (pInventoryOwner)
        pInventoryOwner->on_weapon_hide(this);
}

void CWeapon::StopShotEffector()
{
    CInventoryOwner* pInventoryOwner = smart_cast<CInventoryOwner*>(H_Parent());
    if (pInventoryOwner)
        pInventoryOwner->on_weapon_shot_stop();
}

// Разрешено ли нам играть эффекты камеры игрока при движении (Actor_Movement.cpp)
bool CWeapon::IsMovementEffectorAllowed() const
{
    if (m_bDisableMovEffAtZoom)
        return !IsZoomed();

    return inherited::IsMovementEffectorAllowed();
}

//*****  Визуализация патронов *****//

// Патронташ
void CWeapon::UpdateAmmoBelt()
{
    if (IsAmmoBeltAttached())
    {
        SAddonData* pAddonAB = GetAddonBySlot(m_AmmoBeltSlot);

        xr_vector<CCartridge*>* magazine   = (m_bGrenadeMode ? &m_magazine : &m_magazine2);
        xr_vector<CCartridge>*  cartridges = (m_bGrenadeMode ? &m_AmmoCartidges : &m_AmmoCartidges2);
        int                     iAmmoCnt   = magazine->size();

        for (int idx = 0; idx < m_AmmoBeltData.size(); idx++)
        {
            ////////////////////////////////////////////////////////////////////////////
            int iAmmoIdx = idx;
            if (IsTriStateReload() && m_bGrenadeMode && GetState() == eReload && GetReloadState() == eSubstateReloadInProcess)
                if (m_dwABHideBulletVisual <= Device.dwTimeGlobal) // "fix" for blinking bullets
                    iAmmoIdx--;

            bool         bVisible = iAmmoIdx < iAmmoCnt;
            AmmoBeltData data     = m_AmmoBeltData[idx];

            // Msg("[%d]:bVisible = [%d] (iAmmoIdx = %d iAmmoCnt = %d GetState() = %d GetReloadState() = %d IsTriStateReload() = %d, m_bGrenadeMode = %d)", idx, bVisible, iAmmoIdx, iAmmoCnt, GetState() == eReload, GetReloadState() == eSubstateReloadInProcess, m_bGrenadeMode, IsTriStateReload());

            // HUD
            if ((bool)GetHUDmode() == true && m_sAB_hud != NULL)
            {
                attachable_hud_item* hud_item = HudItemData();
                if (hud_item != NULL && data.first != NULL)
                {
                    attachable_hud_item* ammo_belt_item = hud_item->FindChildren(m_sAB_hud);
                    if (ammo_belt_item != NULL)
                    {
                        //*************************************************//
                        // Цепляем патрон к патронташу
                        ammo_belt_item->UpdateChildrenList(data.first, bVisible);

                        // Устанавливаем визуал патрона
                        if (bVisible)
                        {
                            attachable_hud_item* bullet_item = ammo_belt_item->FindChildren(data.first);
                            if (bullet_item != NULL)
                            {
                                shared_str sBulletHud = NULL;

                                if (idx >= iAmmoCnt)
                                {
                                    u8 iType = (m_bGrenadeMode ? m_ammoType : m_ammoType2);
                                    if (m_set_next_ammoType_on_reload != undefined_ammo_type)
                                        iType = m_set_next_ammoType_on_reload;

                                    if (iType < cartridges->size())
                                        sBulletHud = cartridges->at(iType).m_sBulletHudVisual;
                                }
                                else
                                {
                                    CCartridge* pCartridge = magazine->at(idx);
                                    if (pCartridge != NULL)
                                        sBulletHud = pCartridge->m_sBulletHudVisual;
                                }

                                bullet_item->UpdateVisual(sBulletHud);
                            }
                        }
                        //*************************************************//
                    }
                }
            }

            // VIS
            if (m_sAB_vis != NULL && data.second != NULL)
            {
                attachable_visual* ammo_belt_item = FindAdditionalVisual(m_sAB_vis);
                if (ammo_belt_item != NULL)
                {
                    //*************************************************//
                    // Цепляем патрон к патронташу
                    ammo_belt_item->SetChildrenVisual(data.second, bVisible);

                    // Устанавливаем визуал патрона
                    if (bVisible)
                    {
                        attachable_visual* bullet_item = ammo_belt_item->FindChildren(data.second);
                        if (bullet_item != NULL)
                        {
                            shared_str sBulletVis = NULL;

                            if (idx >= iAmmoCnt)
                            {
                                u8 iType = (m_bGrenadeMode ? m_ammoType : m_ammoType2);
                                if (m_set_next_ammoType_on_reload != undefined_ammo_type)
                                    iType = m_set_next_ammoType_on_reload;

                                if (iType < cartridges->size())
                                    sBulletVis = cartridges->at(iType).m_sBulletWorldVisual;
                            }
                            else
                            {
                                CCartridge* pCartridge = magazine->at(idx);
                                if (pCartridge != NULL)
                                    sBulletVis = pCartridge->m_sBulletWorldVisual;
                            }

                            bullet_item->SetVisual(sBulletVis);
                        }
                    }
                    //*************************************************//
                }
            }
            ////////////////////////////////////////////////////////////////////////////
        }
    }
}

// Обновить визуал текущего патрона в руках, который сейчас заряжаем
void CWeapon::UpdateBulletHUDVisual()
{
    if (!GetHUDmode())
        return;
    if (m_sBulletHUDVisSect == NULL)
        return;

    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != NULL)
    {
        for (int i = 0, iCnt = _GetItemCount(m_sBulletHUDVisSect.c_str()); i < iCnt; ++i)
        {
            // Получаем очередную HUD-секцию из списка
            string128 sBulletHudSect;
            _GetItem(m_sBulletHUDVisSect.c_str(), i, sBulletHudSect);

            // Отображаем\скрываем патрон
            bool bVisible = true;
            hud_item->UpdateChildrenList(sBulletHudSect, bVisible);

            // Обновляем его визуал
            attachable_hud_item* bullet_item = hud_item->FindChildren(sBulletHudSect);
            if (bullet_item != NULL)
            {
                u8 iType = (m_set_next_ammoType_on_reload == undefined_ammo_type ? m_ammoType : m_set_next_ammoType_on_reload);
                if (iType < m_AmmoCartidges.size())
                    bullet_item->UpdateVisual(m_AmmoCartidges[iType].m_sBulletHudVisual);
            }
        }
    }
}

// Обновить анимированную худовую (нет физики) гильзу после выстрела
void CWeapon::UpdateAnimatedShellVisual()
{
    if (!GetHUDmode())
        return;
    if (m_sAnimatedShellHUDVisSect == NULL)
        return;

    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != NULL)
    {
        bool bVisible = (m_dwShowAnimatedShellVisual >= Device.dwTimeGlobal && GetState() != eReload);

        // Отображаем\скрываем гильзу
        hud_item->UpdateChildrenList(m_sAnimatedShellHUDVisSect, bVisible);

        // Обновляем её визуал
        attachable_hud_item* shell_item = hud_item->FindChildren(m_sAnimatedShellHUDVisSect);
        if (shell_item != NULL)
            shell_item->UpdateVisual(m_sCurAnimatedShellHudVisual);
    }
}

// Обновляем все связанные с текущим оружием доп. визуалы (не связаны напрямую с аддонами)
void CWeapon::UpdateWpnExtraVisuals()
{
    if ((bool)GetHUDmode() == true)
    { // Обновляем визуалы худа
        attachable_hud_item* hud_item = HudItemData();
        if (hud_item != NULL)
        {
            // Перебираем все активные атачи худа
            for (u32 i = 0; i < hud_item->m_child_items.size(); i++)
            {
                attachable_hud_item* item = hud_item->m_child_items[i];
                if (item != NULL)
                {
                    IRenderVisual* pVis = item->m_model->dcast_RenderVisual();
                    if (pVis != NULL)
                    {
                        vis_object_data* object_data = pVis->getObjectData();

                        // Скрываем кости голографа вне зума, двигаем их в зуме
                        if (IsScopeAttached() && m_sHolographBone != NULL)
                        {
                            xr_string hud_vis = GetAddonBySlot(eScope)->GetVisuals("visuals_hud").c_str();
                            if (hud_vis.find(item->m_sect_name.c_str()) != xr_string::npos)
                            {
                                // Скрываем \ показываем метку
                                bool bShow =
                                    (m_bGrenadeMode == false && GetZRotatingFactor() >= m_fHolographRotationFactor);
                                item->set_bone_visible(m_sHolographBone, bShow, TRUE);

                                // Сдвигаем её на произвольное расстояние
                                item->m_model->LL_ClearAdditionalTransform(BI_NONE); //--> Очищаем старые смещения

                                KinematicsABT::additional_bone_transform holoOffsets;
                                holoOffsets.m_bone_id = item->m_model->LL_BoneID(m_sHolographBone);
                                holoOffsets.setPosOffset(m_vHolographOffset);

                                item->m_model->LL_AddTransformToBone(holoOffsets);
                            }
                        }

                        // Скрываем \ показываем кости в зависимости от числа патронов
                        // Получаем число костей, привязанных к числу патронов
                        int max_bullet_bones = object_data->m_max_bullet_bones;
                        if (max_bullet_bones <= 0)
                            continue;

                        // Перебираем соответствующее число костей и скрываем\раскрываем их
                        for (int i = 1; i <= max_bullet_bones; i++)
                        {
                            string256 sBulletBone;
                            xr_sprintf(sBulletBone, "%s_%d", BULLET_BONE_NAME, i);

                            item->set_bone_visible(sBulletBone, i <= GetMainAmmoElapsed(), FALSE);
                        }
                    }
                }
            }
        }
    }
    else
    { // Обновляем визуалы мировых моделей
        // Строим список всех наших визуалов
        xr_vector<IRenderVisual*> tVisList;
        GetAllInheritedVisuals(tVisList);

        // Теперь скрываем\раскрываем нужные кости
        for (u32 i = 0; i < tVisList.size(); i++)
        {
            IRenderVisual* pVis = tVisList[i];
            if (pVis != NULL)
            {
                vis_object_data* object_data = pVis->getObjectData();

                // Скрываем \ показываем кости в зависимости от числа патронов
                // Получаем число костей, привязанных к числу патронов
                int max_bullet_bones = object_data->m_max_bullet_bones;
                if (max_bullet_bones <= 0)
                    continue;

                IKinematics* pModel = pVis->dcast_PKinematics();
                if (pModel != NULL)
                {
                    // Перебираем соответствующее число костей и скрываем\раскрываем их
                    for (int i = 1; i <= max_bullet_bones; i++)
                    {
                        string256 sBulletBone;
                        xr_sprintf(sBulletBone, "%s_%d", BULLET_BONE_NAME, i);

                        bool bShow = (i <= GetMainAmmoElapsed());

                        u16 bone_id = pModel->LL_BoneID(sBulletBone);
                        if (bone_id != BI_NONE)
                            if ((bool)pModel->LL_GetBoneVisible(bone_id) != bShow)
                                pModel->LL_SetBoneVisible(bone_id, bShow, TRUE);
                    }
                }
            }
        }
    }
}

// Каллбэк на смену визуала мировой модели
void CWeapon::OnChangeVisual()
{
    inherited::OnChangeVisual();

    // Считываем число костей с привязкой к числу патронов
    if (renderable.visual != NULL && renderable.visual->dcast_PKinematics() != NULL)
    {
        CWeapon::ReadMaxBulletBones(renderable.visual->dcast_PKinematics());
    }
}

// Каллбэк на смену визуала в одном из присоединённых доп. визуалов  --#SM+#--
void CWeapon::OnAdditionalVisualModelChange(attachable_visual* pChangedVisual)
{
    inherited::OnAdditionalVisualModelChange(pChangedVisual);

    // Считываем число костей с привязкой к числу патронов
    if (pChangedVisual->m_model != NULL)
    {
        CWeapon::ReadMaxBulletBones(pChangedVisual->m_model);
    }

    // Проигрываем у аддона мировые анимации
    if (pSettings->line_exist(pChangedVisual->m_sect_name.c_str(), "anm_world_idle"))
    {
        IKinematicsAnimated* pWeaponVisual = pChangedVisual->m_model->dcast_PKinematicsAnimated();
        if (pWeaponVisual)
        {
            pWeaponVisual->PlayCycle(pSettings->r_string(pChangedVisual->m_sect_name.c_str(), "anm_world_idle"), TRUE);
        }
    }
}

// Обновление необходимости включения второго вьюпорта +SecondVP+
// Вызывается только для активного оружия игрока
void CWeapon::UpdateSecondVP()
{
    // + CActor::UpdateCL();
    bool b_is_active_item = (m_pInventory != NULL) && (m_pInventory->ActiveItem() == this);
    R_ASSERT(ParentIsActor() && b_is_active_item); // Эта функция должна вызываться только для оружия в руках нашего игрока

    CActor* pActor = H_Parent()->cast_actor();

    bool bCond_1 = m_fZoomRotationFactor > 0.05f; // Мы должны целиться
    bool bCond_2 = IsSecondVPZoomPresent();       // В конфиге должен быть прописан FOV для второго вьюпорта
    bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye(); // Мы должны быть от 1-го лица

    Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_2 && bCond_3);
}

// Получить степень видимости содержимого второго вьюпорта (линзы) +SecondVP+
float CWeapon::GetSVPVisibilityFactor()
{ 
    if (Device.m_SecondViewport.IsSVPActive() == false)
    {
        return 0.0f;
    }

    return GetZRotatingFactor();
}

// Статическая функция. Считывает из модели число костей с привязкой к числу патронов
// Если в любой модели (худовой или мировой) есть кости вида bullet_1, bullet_2, ... bullet_x, то они будут скрываться
// если число патронов в основном магазине меньше, чем числовой индекс кости
void CWeapon::ReadMaxBulletBones(IKinematics* pModel)
{
    R_ASSERT(pModel != NULL);

    vis_object_data* object_data    = pModel->dcast_RenderVisual()->getObjectData();
    object_data->m_max_bullet_bones = 0;

    while (true)
    {
        string256 sBulletBone;
        xr_sprintf(sBulletBone, "%s_%d", BULLET_BONE_NAME, object_data->m_max_bullet_bones + 1);

        int bone_id = pModel->LL_BoneID(sBulletBone);
        if (bone_id != BI_NONE)
        {
            object_data->m_max_bullet_bones++;
        }
        else
        {
            break;
        }
    }
}

// Каллбэк на повороты камеры игроком
void CWeapon::OnOwnedCameraMove(CCameraBase* pCam, float fOldYaw, float fOldPitch)
{
    R_ASSERT(ParentIsActor());

    if (m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
        OnOwnedCameraMoveWhileBipods(pCam, fOldYaw, fOldPitch);
}

// Обновление камеры текущего игрока
bool CWeapon::UpdateCameraFromHUD(IGameObject* pCameraOwner, Fvector noise_dangle)
{
    R_ASSERT(ParentIsActor());

    bool bDisableCamCollision = false;

    if (m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
        return UpdateCameraFromBipods(pCameraOwner, noise_dangle);

    return bDisableCamCollision;
}

// Обновление базовой позиции света от фонарика игрока
void CWeapon::UpdateActorTorchLightPosFromHUD(Fvector* pTorchPos)
{
    R_ASSERT(ParentIsActor());

    if (m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
        UpdateActorTorchLightPosFromBipods(pTorchPos);
}

// Обновление магазина на мировой модели во время перезарядки
void CWeapon::UpdateMagazine3p(bool bForceUpdate)
{
    SAddonData*  pMagaz       = GetAddonBySlot(eMagaz);
    IGameObject* pParent      = H_Parent();
    bool         bValidState  = (GetState() == eReload || GetState() == eSwitchMag);
    bool         bValidTime   = CurrStateTime() > m_iMagaz3pHideStartTime && CurrStateTime() <= m_iMagaz3pHideEndTime;
    bool         bValidParent = pParent == NULL ? false : (!!pParent->cast_stalker() || !!pParent->cast_actor());
    bool         bIsHidden    = IsHidden();

    if (!bIsHidden && m_bMagaz3pHideWhileReload && pMagaz->bActive && bValidState && bValidParent && bValidTime)
    {
        if (!m_bMagaz3pIsHidden || bForceUpdate)
        {
            // Скрываем магазин на самом оружии
            pMagaz->bHideVis3p = true;

            // Цепляем магазин к NPC
            LPCSTR world_vis = pMagaz->GetVisuals("visuals_world", true).c_str();

            if (bForceUpdate)
                H_Parent()->cast_game_object()->DetachAdditionalVisual("attachable_magazin_3p");

            if (world_vis != NULL)
            {
                string256 str_buffer;
                xr_sprintf(str_buffer, "%s__reload", world_vis);

                if (pSettings->section_exist(str_buffer))
                {
                    attachable_visual* pOut          = NULL;
                    bool               bAttachResult = H_Parent()->cast_game_object()->AttachAdditionalVisual("attachable_magazin_3p", &pOut);

                    // Загружаем параметры аттача магазина в руках НПС
                    if (pOut != NULL && (bAttachResult == true))
                    {
                        pOut->ReLoad(str_buffer);
                    }
                }
            }

            // Устанавливаем флаг
            m_bMagaz3pIsHidden = true;
        }
    }
    else
    {
        if (m_bMagaz3pIsHidden)
        {
            // Отображаем магазин на оружии
            pMagaz->bHideVis3p = false;

            // Скрываем его у НПС
            if (bValidParent)
                H_Parent()->cast_game_object()->DetachAdditionalVisual("attachable_magazin_3p");

            // Снимаем флаг
            m_bMagaz3pIsHidden = false;
        }
    }
}

// Получить текущую целевую позицию 3D-гильзы на оружии
void CWeapon::Update3DShellTransform()
{
    UpdateFireDependencies_internal();
}
