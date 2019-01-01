/*******************************/
/***** Сошки для оружия *****/ //--#SM+#--
/*******************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

#include "attachable_visual.h"
#include "CameraFirstEye.h"
#include "HUDManager.h"
#include "xrPhysics/ActorCameraCollision.h"

//#define DEBUG_DISABLE_COLLIDE_CHECK		// Отключить проверку на коллизию камеры
//#define DEBUG_DISABLE_MODEL_ANIMATED		// Отключить анимированное движение сошек

#define BONE_MAIN "joint"  // Кость основания
#define BONE_LEG_L "leg_l" // Кость левой ножки
#define BONE_LEG_R "leg_r" // Кость правой ножки

/**********************************/
/***** Логика сошек на оружии *****/ //--#SM+#--
/**********************************/

//--#SM+#-- SM_TODO --SM_CAMERA -> Проведи поиск по этим словам
/*
2) Проверка что есть место под сошки (она частично есть сейчас, но проверяет лишь что камера влезает)
10) Сошки на динамической поверхности
11) Плавные движения костей после перезарядки?
12) Что будет, если бросить оружие?
13) Сохранение состояния тела игрока, положение "лёжа"
14) С включённой физикой камеры есть баг - подойди к окну где тайник монолита, попытайся установить
камеру в угол - теперь в этом месте ГГ ВСЕГДА будет разворачивать, хотя он не в режиме сошек.
15) Если во время Outro начать перезарядку или наоборот, то сошки до конца не согнутся
16) Баг с нормалью и инвертированием дирекции
17) Звук сошек
18) FOV влияет на размер камеры?
19) FOV под разные прицелы
20) SkeletonRigid.cpp
21) Установка как в арме https://www.youtube.com/watch?v=J7knaQ0mdMo&feature=youtu.be&t=13
22) Обнулять у дирекции X, чтобы slide не инвертировал <!>
*/

void CWeapon::OnBipodsAttach(EAddons iSlot, const shared_str& sAddonDataName)
{
    m_BipodsSlot = iSlot;

    m_bipods.m_translation_factor = 0.f;
    m_bipods.m_vBipodInitPos = {0, 0, 0};
    m_bipods.m_vBipodInitDir = {0, 0, 0};
    m_bipods.m_vBipodInitNormal = {0, 0, 0};
    m_bipods.m_bInstalled = true;
}

void CWeapon::OnBipodsDetach(const shared_str& sAddonDataName)
{
    m_BipodsSlot = eNotExist;

    m_bipods.m_sBipod_hud = NULL;
    m_bipods.m_sBipod_vis = NULL;
    m_bipods.m_bInstalled = false;
}

void CWeapon::LoadBipodsParams()
{
    if (!IsBipodsAttached())
        return;

    m_bipods.m_translation_factor = 0.f;

    SAddonData* pAddonBipods = GetAddonBySlot(m_BipodsSlot);

    m_bipods.m_sBipod_hud = pAddonBipods->GetVisuals("visuals_hud", true);
    m_bipods.m_sBipod_vis = pAddonBipods->GetVisuals("visuals_world", true);

    Fvector2 vNoLimits = {-PI, PI};
    Fvector  vZero     = {0.f, 0.f, 0.f};

    m_bipods.m_bAnimated        = READ_ADDON_DATA(r_bool, "bipods_animated", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.m_i_range          = READ_ADDON_DATA(r_float, "bipods_inst_range", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.m_i_angle_f        = READ_ADDON_DATA(r_float, "bipods_inst_angle_f", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.m_cam_dist         = READ_ADDON_DATA(r_float, "bipods_cam_dist", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.m_cam_y_offset     = READ_ADDON_DATA(r_float, "bipods_cam_y_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.m_hud_z_offset     = READ_ADDON_DATA(r_float, "bipods_hud_z_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.m_pitch_vis_factor = READ_ADDON_DATA(r_float, "bipods_pitch_vis_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 2.f);
    m_bipods.m_inertia_power    = READ_ADDON_DATA(r_float, "bipods_inertia_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.m_pos_displ        = READ_ADDON_DATA(r_float, "bipods_pos_displ", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.m_deploy_time      = READ_ADDON_DATA(r_float, "bipods_deploy_time", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.m_undeploy_time    = READ_ADDON_DATA(r_float, "bipods_undeploy_time", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.5f);
    m_bipods.m_fZoomFOV         = READ_ADDON_DATA(r_float, "bipods_zoom_fov", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), g_fov);
    m_bipods.m_fHUD_FOV         = READ_ADDON_DATA(r_float, "bipods_hud_fov", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), psHUD_FOV_def);
    m_bipods.m_fDispersionMod   = READ_ADDON_DATA(r_float, "bipods_dispersion_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.m_fRecoilMod       = READ_ADDON_DATA(r_float, "bipods_recoil_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.m_max_dist         = READ_ADDON_DATA(r_float, "bipods_max_dist", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.2f);
    m_bipods.m_yaw_limit        = READ_ADDON_DATA(r_fvector2, "bipods_yaw_limit", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vNoLimits);
    m_bipods.m_pitch_limit      = READ_ADDON_DATA(r_fvector2, "bipods_pitch_limit", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vNoLimits);
    m_bipods.m_torch_offset     = READ_ADDON_DATA(r_fvector3, "bipods_torch_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.m_deploy_pos       = READ_ADDON_DATA(r_fvector3, "bipods_deploy_pos", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.m_deploy_rot       = READ_ADDON_DATA(r_fvector3, "bipods_deploy_rot", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.m_legs_rot         = READ_ADDON_DATA(r_fvector3, "bipods_legs_rot", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
};

void CWeapon::Try2DeployBipods() // Переименовать в OnBipodsButton ?
{
    bool bDeployed = IsBipodsDeployed();

    if (!IsBipodsAttached())
        return;
    if (!ParentIsActor())
        return;
    if (!IsHudModeNow())
        return;
    if (!bDeployed && IsZoomed())
        return;
    if (!bDeployed && IsPending())
        return;

    /*
	CCameraBase* C = pActor->cam_Active();
	if (C != pActor->cam_FirstEye())
	SM_TODO: Не пускать
	*/

    // на лестнице не пускай <!>

    if (IsBipodsDeployed())
    {
        UndeployBipods();
        return;
    }

    // Рассчитываем параметры установки
    CCameraBase*        pCam = g_actor->cam_Active(); // g_actor не использовать <!> !!!
    collide::rq_result& RQ   = HUD().GetCurrentRayQuery();

    // Проверка дистанции установки
    float dist = RQ.range;
    if (dist > m_bipods.m_i_range)
        return;

    // Позиция установки
    Fvector vInstPos = pCam->vPosition;
    vInstPos.add(Fvector(pCam->vDirection).mul(dist));

    // Нормаль места установки (Roll, наклон поверхности)
    Fvector   vInstNormal;
    Fvector*  pVerts = Level().ObjectSpace.GetStaticVerts();
    CDB::TRI* pTri   = Level().ObjectSpace.GetStaticTris() + RQ.element;
    vInstNormal.mknormal(pVerts[pTri->verts[0]], pVerts[pTri->verts[1]], pVerts[pTri->verts[2]]);

    // Проверка наклона поверхности
    if (vInstNormal.y < m_bipods.m_i_angle_f)
        return;

    // Дирекция установки
    Fvector vInstDir = pCam->vDirection;
    vInstDir.slide(vInstDir, vInstNormal); //--> Наклоняем дирекцию по нормали целевой поверхности
    vInstDir.normalize_safe();

    /*
	https://stackoverflow.com/questions/5188561/signed-angle-between-two-3d-vectors-with-same-origin-within-the-same-plane
	Log("X = ",
		acos(pCam->vDirection.dotproduct(vInstNormal)) * 180.f / PI
		);
	В данном варианте у угла нету знака SM_TODO
	*/

    // Двигаем стартовую позицию сошек для лучшего вида
    vInstPos.add(Fvector().mul(vInstDir, m_bipods.m_pos_displ));

    // Вызываем функцию установки
    DeployBipods(vInstPos, vInstDir, vInstNormal);
}

void CWeapon::DeployBipods(Fvector vDeployPos, Fvector vDeployDir, Fvector vDeployNormal, bool bInstantly)
{
    R_ASSERT(IsBipodsAttached());

    if (IsBipodsDeployed())
        return;

    // Инициализируем данные сошек
    if (bInstantly)
    {
        m_bipods.m_translation_factor = 1.f;
        m_bipods.m_iBipodState        = bipods_data::eBS_SwitchedON;
    }
    else
    {
        m_bipods.m_translation_factor = 0.f;
        m_bipods.m_iBipodState        = bipods_data::eBS_TranslateInto;

        // Играем звук установки сошек
        m_sounds.StopSound("sndBipodsU");
        PlaySound("sndBipodsD", get_LastFP());
    }
    m_bipods.m_bUseZoomFov      = false;
    m_bipods.m_vBipodInitPos    = vDeployPos;
    m_bipods.m_vBipodInitDir    = vDeployDir;
    m_bipods.m_vBipodInitNormal = vDeployNormal;
    m_bipods.m_bFirstCamUpdate  = true;
    m_bipods.m_vPrevYP          = {0.f, 0.f};

    // Зумим
    OnZoomIn();

    // Модифицируем камеру от первого лица
    CCameraBase* pFPCam = g_actor->cam_FirstEye();
    R_ASSERT(pFPCam != NULL);

    pFPCam->m_bInputDisabled = !bInstantly; // Блокируем управление камерой

    // Нормализуем текущий Yaw\Pitch\Roll
    pFPCam->yaw   = angle_normalize_signed(pFPCam->yaw);
    pFPCam->pitch = angle_normalize_signed(pFPCam->pitch);
    pFPCam->roll  = angle_normalize_signed(pFPCam->roll);

    // Сохраняем текущие параметры камеры
    m_bipods.m_vOldCamPos  = pFPCam->vPosition;
    m_bipods.m_vOldCamYP.x = pFPCam->yaw;
    m_bipods.m_vOldCamYP.y = pFPCam->pitch;

    // Сохраняем позицию игрока в момент установки
    m_bipods.m_vParentInitPos = g_actor->Position(); // g_actor не использовать <!> !!! SM_TODO
}

void CWeapon::UndeployBipods(bool bInstantly)
{
    if (!IsBipodsDeployed())
        return;

    // Выключаем сошки
    if (bInstantly)
    {
        m_bipods.m_translation_factor = 0.f;
        m_bipods.m_iBipodState        = bipods_data::eBS_SwitchedOFF;
    }
    else
    {
        m_bipods.m_iBipodState = bipods_data::eBS_TranslateOutro;

        // Играем звук снятия сошек
        m_sounds.StopSound("sndBipodsD");
        PlaySound("sndBipodsU", get_LastFP());
    }

    // Возвращаем их в дефолтное положение
    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != NULL)
    {
        attachable_hud_item* bipods_item = hud_item->FindChildren(m_bipods.m_sBipod_hud);
        if (bipods_item != NULL) // R_ASSERT(bipods_item)
            bipods_item->m_model->LL_ClearAdditionalTransform(BI_NONE);
    }

    // Убираем зум
    OnZoomOut();
    m_bipods.m_bUseZoomFov = false;

    // Восстанавливаем параметры камеры
    CCameraBase* pFPCam = g_actor->cam_FirstEye();
    R_ASSERT(pFPCam != NULL);

    pFPCam->m_bInputDisabled = false;

    BipodsSetCameraLimits(pFPCam, false);
}

void CWeapon::Need2UndeployBipods(bool bInstantly) {}

void CWeapon::BipodsZoom(u32 flags)
{
    if (IsReloading())
        return;

    if (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON)
    {
        if (b_toggle_weapon_aim)
        {
            // Режим "Нажать раз"
            if (flags & CMD_START)
                m_bipods.m_bUseZoomFov = !m_bipods.m_bUseZoomFov;
        }
        else
        {
            // Режим "Пока зажато"
            if (flags & CMD_START)
                m_bipods.m_bUseZoomFov = true;
            else
                m_bipods.m_bUseZoomFov = false;
        }
    }
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void CWeapon::UpdateBipods()
{
    if (!IsBipodsAttached())
        return;

    //SM_TODO: В зуме юзай этот фактор
    //SM_TODO: Вылет, если сошки включены, но владелец - сталкер
}

// Каллбэк на повороты камеры игроком
void CWeapon::OnOwnedCameraMoveWhileBipods(CCameraBase* pCam, float fOldYaw, float fOldPitch) {}

// Обновление позиции камеры игрока от сошек
bool CWeapon::UpdateCameraFromBipods(IGameObject* pCameraOwner, Fvector noise_dangle)
{
    CActor* pActor = pCameraOwner->cast_actor();
    R_ASSERT(pActor != NULL);
    R_ASSERT(m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF);

    // Получаем текущую камеру и делаем различные проверки
    CCameraBase* C    = pActor->cam_Active();
    bool         bFar = (m_bipods.m_vParentInitPos.distance_to(g_actor->Position()) > m_bipods.m_max_dist) &&
                m_bipods.m_iBipodState != bipods_data::eBS_TranslateOutro;
    if (C != pActor->cam_FirstEye() || !ParentIsActor() || bFar)
    {
        UndeployBipods(!bFar);
        return false;
    }

    // Убираем зум сошек при перезарядке
    bool bIsReload = IsReloading();
    if (bIsReload)
        m_bipods.m_bUseZoomFov = false;

    // Восстанавливаем режим Зума после перезарядки
    if (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON && !IsZoomed() && !bIsReload)
        OnZoomIn();

    // Находим новую позицию камеры
    Fvector vTemp;                    // Вектор для хранения результатов векторных операций
    Fvector vCalcCamPos, vCalcCamDir; // Рассчитанная позиция\дирекция камеры

    Fvector vCalcNormal; // Рассчитанная нормаль камеры (Roll)
    vCalcNormal.lerp({0, 1, 0}, m_bipods.m_vBipodInitNormal, m_bipods.m_translation_factor);

    // Рассчитывем итоговую позицию камеры при установленных сошках
    if (m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
    {
        vCalcCamDir = C->Direction(); // <!> Для Y\P камеры getHP() нужно инвертировать <!> + он также не нормализирован (0..2P)

        // Если eBS_SwitchedON выпал на первый апдейт после установки (bInstantly), то форсируем yaw\pitch
        if (m_bipods.m_iBipodState == bipods_data::eBS_TranslateInto ||
            (m_bipods.m_bFirstCamUpdate && m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON))
        {
            vCalcCamDir = m_bipods.m_vBipodInitDir;
        }

        // Добавляем смещение камеры от центра установки
        float covariance         = VIEWPORT_NEAR * 6.f;
        u8    iCollideCheckStage = 0;

        float fCurYaw, fCurPitch; /// Текущие Yaw\Pitch

    UpdateCameraFromBipods_collide_check:

        if (iCollideCheckStage == 0)
        {
            fCurYaw   = C->yaw;
            fCurPitch = C->pitch;
        }

        // Заменяем позицию камеры на позицию установки
        vCalcCamPos = m_bipods.m_vBipodInitPos;

        // Добавляем смещение камеры вверх по нормали
        vCalcCamPos.mad(m_bipods.m_vBipodInitNormal, m_bipods.m_cam_y_offset);

        // Рассчитываем смещение от центра
        vTemp.invert(vCalcCamDir);

        collide::rq_result R;
        g_pGameLevel->ObjectSpace.RayPick(vCalcCamPos, vTemp, m_bipods.m_cam_dist + covariance, collide::rqtBoth, R, pActor);

        float fZOffset = (R.range - covariance);
        vCalcCamPos.add(vTemp.mul(vCalcCamDir, -fZOffset - VIEWPORT_NEAR));

        // Проверяем коллизию камеры
        C->SaveCamVec();
        C->Set(vCalcCamPos, vCalcCamDir, vCalcNormal);

        Fvector vCamPosOfs  = {0.f, 0.05f, 0.05f};
        bool    bIsCollided = test_camera_collide(*C, VIEWPORT_NEAR, pActor, vCamPosOfs, 0.9f);

        C->RestoreCamVec();

#ifndef DEBUG_DISABLE_COLLIDE_CHECK
        if (bIsCollided == false && fZOffset >= (m_bipods.m_cam_dist * 0.7f))
        {
            // Камера не колизится - сохраняем текущие Yaw\Pitch
            m_bipods.m_vPrevYP = {C->yaw, C->pitch};
        }
        else
        {
            // Камера колизится - пытаемся откатить камеру к предыдущему кадру в 3 попытки
            if (iCollideCheckStage != 3)
            {
                if (iCollideCheckStage == 0)
                { // Откат только Yaw
                    C->yaw   = m_bipods.m_vPrevYP[0];
                    C->pitch = fCurPitch;
                    C->Update(C->Position(), noise_dangle);
                }
                else if (iCollideCheckStage == 1)
                { // Откат только Pitch
                    C->yaw   = fCurYaw;
                    C->pitch = m_bipods.m_vPrevYP[1];
                    C->Update(C->Position(), noise_dangle);
                }
                else if (iCollideCheckStage == 2)
                { // Откат Yaw и Pitch
                    C->yaw   = m_bipods.m_vPrevYP[0];
                    C->pitch = m_bipods.m_vPrevYP[1];
                    C->Update(C->Position(), noise_dangle);
                }

                vCalcCamDir = C->Direction();

                iCollideCheckStage++;

                goto UpdateCameraFromBipods_collide_check;
            }
            else
            {
                // Мы не смогли успешно откатиться к прошлому кадру - резко снимаем сошки.
                UndeployBipods(true);
                return false;
            }
        }
#endif // ! DEBUG_DISABLE_COLLIDE_CHECK
    }

    // Обрабатываем различные состояния
    switch (m_bipods.m_iBipodState)
    {
    case bipods_data::eBS_TranslateInto:
    { //--> Установка сошек
        m_bipods.m_translation_factor += Device.fTimeDelta / m_bipods.m_deploy_time;
        clamp(m_bipods.m_translation_factor, 0.f, 1.f);

        // Плавный переход камеры
        //--> POS
        vTemp.sub(vCalcCamPos, m_bipods.m_vOldCamPos);
        vTemp.mul(m_bipods.m_translation_factor);
        C->vPosition.add(m_bipods.m_vOldCamPos, vTemp);

        //--> DIR (Yaw + Pitch)
        float fTYaw, fTPitch;
        vCalcCamDir.getHP(fTYaw, fTPitch);
        fTYaw   = angle_normalize_signed(fTYaw);
        fTPitch = angle_normalize_signed(fTPitch);

        float fYawDiff = angle_difference_signed(fTYaw, -m_bipods.m_vOldCamYP.x);

        C->yaw = fTYaw - (fYawDiff * (1.f - m_bipods.m_translation_factor));
        C->yaw *= -1.f;

        float fPitchDiff = angle_difference_signed(fTPitch, -m_bipods.m_vOldCamYP.y);
        C->pitch         = fTPitch - (fPitchDiff * (1.f - m_bipods.m_translation_factor));
        C->pitch *= -1.f;

        //--> Roll
        C->vNormal = vCalcNormal;

        if (m_bipods.m_translation_factor == 1.f)
        {
            // Переход завершён
            C->m_bInputDisabled    = false;
            m_bipods.m_iBipodState = bipods_data::eBS_SwitchedON;

            // Ограничиваем камеру
            BipodsSetCameraLimits(C, true);
        }
    }
    break;
    case bipods_data::eBS_SwitchedON:
    { //--> Сошки включены
        m_bipods.m_translation_factor = 1.f;

        // Заменяем позицию и дирекцию камеры
        C->vPosition  = vCalcCamPos;
        C->vDirection = vCalcCamDir;
        C->vNormal    = m_bipods.m_vBipodInitNormal;

        // Если это первый апдейт камеры после установки сошек, то форсим поворот камеры для резкой установки.
        if (m_bipods.m_bFirstCamUpdate)
        {
            vCalcCamDir.getHP(C->yaw, C->pitch);
            C->yaw   = angle_normalize_signed(C->yaw) * -1.f;
            C->pitch = angle_normalize_signed(C->pitch) * -1.f;

            // Ограничиваем камеру
            BipodsSetCameraLimits(C, true);
        }
    }
    break;
    case bipods_data::eBS_TranslateOutro:
    { //--> Снятие сошек
        m_bipods.m_translation_factor -= Device.fTimeDelta / m_bipods.m_undeploy_time;
        clamp(m_bipods.m_translation_factor, 0.f, 1.f);

        // Плавный переход камеры
        //--> POS
        vTemp.sub(vCalcCamPos, C->vPosition);
        vTemp.mul(m_bipods.m_translation_factor);
        C->vPosition.add(C->vPosition, vTemp);

        if (m_bipods.m_translation_factor == 0.f)
        {
            // Переход завершён
            m_bipods.m_iBipodState = bipods_data::eBS_SwitchedOFF;
        }
    }
    break;
    case bipods_data::eBS_SwitchedOFF:
    {                    //--> Сошки выключены
        R_ASSERT(false); // Этот код не должен вызываться
        return false;
    }
    break;
    default: { R_ASSERT2(false, m_bipods.m_iBipodState);
    }
    break;
    }

    // Сбрасываем флаг первого апдейта
    if (m_bipods.m_bFirstCamUpdate && m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
    {
        m_bipods.m_bFirstCamUpdate = false;
    }

#ifndef DEBUG_DISABLE_MODEL_ANIMATED
    // Анимируем движения сошек
    bool bDisableAnimUpd = bIsReload && m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON;
    if (m_bipods.m_bAnimated && !bDisableAnimUpd) // В процессе перезарядки кости не двигаем <!>
    {
        //*******************************************//

        // Двигаем кости сошек
        Fquaternion Q;
        float       fToRad = PI / 180.f;

        float  yaw     = (-m_bipods.m_vBipodInitDir.getH()); // Y
        float& cam_yaw = C->yaw;
        float  fH      = angle_difference_signed(yaw, cam_yaw); // Разница текущего Yaw от Yaw при установке

        float  pitch     = (-m_bipods.m_vBipodInitDir.getP()); // X
        float& cam_pitch = C->pitch;
        float  fP        = angle_difference_signed(pitch, cam_pitch); // Разница текущего Pitch от Pitch при установке

        //--> В мировой модели
        // SM_TODO: Запилить

        //--> В худе
        attachable_hud_item* hud_item = HudItemData();
        if (hud_item != NULL)
        {
            attachable_hud_item* bipods_item = hud_item->FindChildren(m_bipods.m_sBipod_hud);
            if (bipods_item != NULL)
            {
                // Основание сошек
                KinematicsABT::additional_bone_transform offsets;

                float fX_joint = (0.f + (m_bipods.m_deploy_rot.x * fToRad)) * m_bipods.m_translation_factor;
                float fY_joint =
                    (-fH + (m_bipods.m_deploy_rot.y * fToRad)) * m_bipods.m_translation_factor * (m_bipods.m_deploy_rot.z < 0.f ? -1.f : 1.f);
                float fZ_joint = (-fP + (m_bipods.m_deploy_rot.z * fToRad)) * m_bipods.m_translation_factor;

                shared_str str_bone = BONE_MAIN;
                offsets.m_bone_id   = bipods_item->m_model->LL_BoneID(str_bone);
                offsets.setRotLocal(fX_joint, fY_joint, fZ_joint);
                offsets.setPosOffset(Fvector().mul(m_bipods.m_deploy_pos, m_bipods.m_translation_factor));

                bipods_item->m_model->LL_ClearAdditionalTransform(offsets.m_bone_id);
                bipods_item->m_model->LL_AddTransformToBone(offsets);

                // Ножки сошек
                float fX_legs = (-(fP / m_bipods.m_pitch_vis_factor) + (m_bipods.m_legs_rot.x * fToRad)) * m_bipods.m_translation_factor;
                float fY_legs = (0.f + (m_bipods.m_legs_rot.y * fToRad)) * m_bipods.m_translation_factor;
                float fZ_legs = (0.f + (m_bipods.m_legs_rot.z * fToRad)) * m_bipods.m_translation_factor;

                //--> Левая ножка
                shared_str noga_1 = BONE_LEG_L;
                offsets.m_bone_id = bipods_item->m_model->LL_BoneID(noga_1);
                offsets.setRotLocal(-fX_legs, -fY_legs, fZ_legs);

                bipods_item->m_model->LL_ClearAdditionalTransform(offsets.m_bone_id);
                bipods_item->m_model->LL_AddTransformToBone(offsets);

                //--> Правая ножка
                shared_str noga_2 = BONE_LEG_R;
                offsets.m_bone_id = bipods_item->m_model->LL_BoneID(noga_2);
                offsets.setRotLocal(fX_legs, fY_legs, fZ_legs);

                bipods_item->m_model->LL_ClearAdditionalTransform(offsets.m_bone_id);
                bipods_item->m_model->LL_AddTransformToBone(offsets);
            }
        }
        ///////////////////////////
    }
#endif // ! DEBUG_DISABLE_MODEL_ANIMATED

    return true; // Отключаем проверку коллизии камеры
}

// Ограничить или восстановить лимиты камеры при активных сошках
void CWeapon::BipodsSetCameraLimits(CCameraBase* pCam, bool bLimit)
{
    float fToRad = PI / 180.f;

    if (bLimit == true)
    { //=== Установка лимитов ===//
        // Сохраняем текущие лимиты
        m_bipods.m_fOldYawLimit   = pCam->lim_yaw;
        m_bipods.m_fOldPitchLimit = pCam->lim_pitch;
        m_bipods.m_bOldClampYaw   = pCam->bClampYaw;
        m_bipods.m_bOldClampPitch = pCam->bClampPitch;

        ////////////// YAW (Y) //////////////
        float  yaw       = (-m_bipods.m_vBipodInitDir.getH());
        float& cam_yaw   = pCam->yaw;
        float  delta_yaw = angle_difference_signed(yaw, cam_yaw);

        yaw              = cam_yaw + delta_yaw;
        pCam->lim_yaw[0] = (yaw + (m_bipods.m_yaw_limit.x * fToRad));
        pCam->lim_yaw[1] = (yaw + (m_bipods.m_yaw_limit.y * fToRad));
        pCam->bClampYaw  = true;

        ////////////// PITCH (X) //////////////
        float  pitch       = (-m_bipods.m_vBipodInitDir.getP());
        float& cam_pitch   = pCam->pitch;
        float  delta_pitch = angle_difference_signed(pitch, cam_pitch);

        pitch              = cam_pitch + delta_pitch;
        pCam->lim_pitch[0] = (pitch + (-1 * m_bipods.m_pitch_limit.y * fToRad));
        pCam->lim_pitch[1] = (pitch + (-1 * m_bipods.m_pitch_limit.x * fToRad));
        pCam->bClampPitch  = true;
    }
    else
    { //=== Снятие лимитов ===
        ////////////// YAW (Y) //////////////
        pCam->lim_yaw   = m_bipods.m_fOldYawLimit;
        pCam->bClampYaw = m_bipods.m_bOldClampYaw;
        pCam->CheckLimYaw();

        ////////////// PITCH (X) //////////////
        pCam->lim_pitch   = m_bipods.m_fOldPitchLimit;
        pCam->bClampPitch = m_bipods.m_bOldClampPitch;
        pCam->CheckLimPitch();
    }
}

// Обновление базовой позиции света от фонарика игрока
void CWeapon::UpdateActorTorchLightPosFromBipods(Fvector* pTorchPos)
{
    R_ASSERT(m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF);

    if (HudItemData() == NULL)
        return;

    Fmatrix m_attach_offset;
    m_attach_offset.setHPB(0, 0, 0);
    m_attach_offset.translate_over(m_bipods.m_torch_offset);

    Fmatrix result;
    result.mul(HudItemData()->m_item_transform, m_attach_offset);

    pTorchPos->set(result.c);
}
