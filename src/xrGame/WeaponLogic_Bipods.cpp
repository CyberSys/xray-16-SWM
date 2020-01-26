/*******************************/
/***** Сошки для оружия *****/ //--#SM+#--
/*******************************/

#include "StdAfx.h"
#include "Weapon.h"
#include "HUDManager.h"
#include "player_hud.h"
#include "PHCollideHelper.h"
#include "xrPhysics/ActorCameraCollision.h"
#include "xrPhysics/ExtendedGeom.h"
#include "debug_renderer.h"

//#define DEBUG_DISABLE_COLLIDE_CHECK // Отключить проверку на коллизию камеры
//#define DEBUG_DISABLE_BBOX_CHECK // Отключить проверку на коллизию BBox-а сошек (рейкаст)
//#define DEBUG_DISABLE_MODEL_ANIMATED // Отключить анимированное движение сошек
//#define DEBUG_DISABLE_LEGS_CHECK // Отключить проверку места под ножки

#define BONE_MAIN "joint" // Кость основания
#define BONE_LEG_L "leg_l" // Кость левой ножки
#define BONE_LEG_R "leg_r" // Кость правой ножки

#define BIPODS_BODY_SIZE_DEF 0.15f, 0.1f, 0.35f // Размер тела сошек по умолчанию (используется при проверке коллизии)
#define BIPODS_BODY_OFFS_DEF 0.0f, 0.1f, 0.0f // Смещение тела сошек по умолчанию (используется при проверке коллизии)
#define BIPODS_LEGS_SIZE_DEF 0.05f, 0.2f, 0.05f // Размер тела ножек по умолчанию (используется при установке сошек)
#define BIPODS_LEGS_DIST_DEF 0.25f // Расстояние между ножками сошек

#define BIPODS_GRND_SRCH_FACTOR 0.9f // Фактор макс. расстояния установки сошек при поиске позиции ниже прицела
#define BIPODS_GRND_SRCH_CNT 10 // Кол-во попыток поиска места установки ниже прицела (с каждой всё ближе к камере)
#define BIPODS_GRND_RANGE_MOD 1.5f // Модификатор расстояния установки,при поиске точки ниже прицела

#define BIPODS_CAM_CLOSE_FACTOR 0.7f // Фактор макс. допустимого приближения камеры к месту установки сошек

#define BIPODS_CAM_HEIGHT_FACTOR 1.0f // Модификатор высоты камеры ГГ (Y), выше которой сошки ставить нельзя
#define BIPODS_LOWGROUND_VALUE -0.15f // Максимально допустимое расстояние установки сошек ниже ног ГГ

#define BIPODS_MAX_WALK_DIST 0.1f // Как далеко можно "отойти" от установленных сошек

#define BIPODS_LEGS_INERTION 6.f // Скорость движения ножек сошек при поворотах (больше -> быстрее)

#define BIPODS_BODY_HEIGHT_CROUCH 1.35f // Высота поверхности установки сошек, ниже которой ГГ будет в приседе
#define BIPODS_BODY_HEIGHT_LYING 1.1f // Высота поверхности установки сошек, ниже которой ГГ будет в низком приседе

// Колбэки на контакт сошек с геометрией
static bool bIsShellBodyCollided = false;
static void bipods_body_contact_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
static BOOL bipods_body_ray_callback(collide::rq_result& result, LPVOID params);

static bool bIsShellLegLCollided = false;
static void bipods_legL_contact_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
static BOOL bipods_legL_ray_callback(collide::rq_result& result, LPVOID params);

static bool bIsShellLegRCollided = false;
static void bipods_legR_contact_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2);
static BOOL bipods_legR_ray_callback(collide::rq_result& result, LPVOID params);

//============== Код сошек как аддона ==============//

// Вызывается при присоединении сошек к оружию
void CWeapon::OnBipodsAttach(EAddons iSlot, const shared_str& sAddonDataName)
{
    m_BipodsSlot = iSlot;

    m_bipods.m_fTranslationFactor = 0.f;
    m_bipods.m_vBipodInitPos = {0, 0, 0};
    m_bipods.m_vBipodInitDir = {0, 0, 0};
    m_bipods.m_vBipodInitNormal = {0, 0, 0};

    m_bipods.m_bInstalled = true;
    m_bipods.m_bCamLimitsWasSaved = false;
    m_bipods.m_bBipodsIntroEffPlayed = false;

    m_bipods.dbg_mPHCamBody.identity();
    m_bipods.dbg_vPHCamBodyHalfsize = {0, 0, 0};
    m_bipods.dbg_CollideType = bipods_data::EDBGCollideType::ctNoCollide;
}

// Вызывается при отсоединении сошек от оружия
void CWeapon::OnBipodsDetach(const shared_str& sAddonDataName)
{
    UndeployBipods(true);

    m_BipodsSlot = eNotExist;

    m_bipods.sBipodsHudSect = nullptr;
    m_bipods.sBipodsVisSect = nullptr;
    m_bipods.m_bInstalled = false;
    m_bipods.m_bBipodsIntroEffPlayed = false;

    R_ASSERT(m_bipods.m_bCamLimitsWasSaved == false);
}

// Загрузка параметров из конфига
void CWeapon::LoadBipodsParams()
{
    if (!IsBipodsAttached())
        return;

    SAddonData* pAddonBipods = GetAddonBySlot(m_BipodsSlot);

    m_bipods.sBipodsHudSect = pAddonBipods->GetVisuals("visuals_hud", true);
    m_bipods.sBipodsVisSect = pAddonBipods->GetVisuals("visuals_world", true);

    Fvector2 vNoLimits = {-PI, PI};
    Fvector vZero = {0.f, 0.f, 0.f};

    // clang-format off
    m_bipods.bDbgDraw = READ_ADDON_DATA(r_bool, "bipods_draw_dbg", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.bAnimatedLegs = READ_ADDON_DATA(r_bool, "bipods_animated", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.sInstCamAnm = READ_ADDON_DATA(r_string, "bipods_inst_cam_anm", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), nullptr);
    m_bipods.fCamAnmSpeed = READ_ADDON_DATA(r_float, "bipods_inst_cam_anm_speed", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.0f);
    m_bipods.fInstRangeMax = READ_ADDON_DATA(r_float, "bipods_inst_range", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.8f);
    m_bipods.fInstAngleMax = READ_ADDON_DATA(r_float, "bipods_inst_angle_f", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 25.f);
    m_bipods.bEnableWpnTilt = READ_ADDON_DATA(r_bool, "bipods_enable_tilt", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.fCamDistMax = READ_ADDON_DATA(r_float, "bipods_cam_dist", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.2f);
    m_bipods.fCamYOffset = READ_ADDON_DATA(r_float, "bipods_cam_y_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.fHudZOffset = READ_ADDON_DATA(r_float, "bipods_hud_z_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.fPitch2LegsTiltFactor = READ_ADDON_DATA(r_float, "bipods_pitch_vis_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 2.f);
    m_bipods.fInertiaMod = READ_ADDON_DATA(r_float, "bipods_inertia_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.5f);
    m_bipods.fShotEffMod = READ_ADDON_DATA(r_float, "bipods_shot_eff_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.0f);
    m_bipods.fPosZOffset = READ_ADDON_DATA(r_float, "bipods_pos_displ", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.f);
    m_bipods.fDeployTime = READ_ADDON_DATA(r_float, "bipods_deploy_time", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.5f);
    m_bipods.fUndeployTime = READ_ADDON_DATA(r_float, "bipods_undeploy_time", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 0.5f);
    m_bipods.fZoomFOV = READ_ADDON_DATA(r_float, "bipods_zoom_fov", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), g_fov * 0.5f);
    m_bipods.fHudFOVFactor = READ_ADDON_DATA(r_float, "bipods_hud_fov", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), psHUD_FOV_def);
    m_bipods.fDispersionMod = READ_ADDON_DATA(r_float, "bipods_dispersion_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.fRecoilMod = READ_ADDON_DATA(r_float, "bipods_recoil_factor", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), 1.f);
    m_bipods.vCamYawLimits = READ_ADDON_DATA(r_fvector2, "bipods_yaw_limit", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vNoLimits);
    m_bipods.vCamPitchLimits = READ_ADDON_DATA(r_fvector2, "bipods_pitch_limit", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vNoLimits);
    m_bipods.vTorchOffset = READ_ADDON_DATA(r_fvector3, "bipods_torch_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.vBoneMDeployPosOffs = READ_ADDON_DATA(r_fvector3, "bipods_deploy_pos", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.vBoneMDeployRotOffs = READ_ADDON_DATA(r_fvector3, "bipods_deploy_rot", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.vBoneLDeployRotOffs = READ_ADDON_DATA(r_fvector3, "bipods_legs_rot", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vZero);
    m_bipods.bInvertBodyYaw = READ_ADDON_DATA(r_bool, "bipods_invert_yaw_body", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.bInvertBodyPitch = READ_ADDON_DATA(r_bool, "bipods_invert_pitch_body", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.bInvertLegsPitch = READ_ADDON_DATA(r_bool, "bipods_invert_pitch_legs", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.bDeployWhenBayonetInst = READ_ADDON_DATA(r_bool, "bipods_bayonet_deploy", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    m_bipods.bAlwaysDeployed = READ_ADDON_DATA(r_bool, "bipods_always_deployed", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), false);
    // clang-format on

    // Создаём физическую оболочку
    if (m_bipods.m_pPHWpnBody == nullptr)
    {
        //--> Тело сошек
        //--> Его размеры
        constexpr Fvector vBodySizeDef = {BIPODS_BODY_SIZE_DEF};
        Fvector vBodySize = READ_ADDON_DATA(
            r_fvector3, "bipods_body_size", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vBodySizeDef);
        Fvector vBodyHalfsize = {vBodySize.x / 2.0f, vBodySize.y / 2.0f, vBodySize.z / 2.0f};

        //--> Его локальное смещение
        constexpr Fvector vBodyOffsDef = {BIPODS_BODY_OFFS_DEF};
        Fvector vBodyOffset = READ_ADDON_DATA(
            r_fvector3, "bipods_body_offset", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vBodyOffsDef);
        Fvector vBodyCenter = { //--> Приподымаем тело над землёй
            0.0f + vBodyOffset.x, vBodySize.y + vBodyOffset.y, 0.0f + vBodyOffset.z};

        m_bipods.m_pPHWpnBody = new CPHCollideHelper(this, bipods_body_contact_callback);
        m_bipods.m_pPHWpnBody->AddBox(vBodyCenter, vBodyHalfsize);

        //--> Ножки сошек
        constexpr Fvector vLegsSizeDef = {BIPODS_LEGS_SIZE_DEF};
        Fvector vLegsSize = READ_ADDON_DATA(
            r_fvector3, "bipods_legs_size", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), vLegsSizeDef);
        Fvector vLegsHalfsize = {vLegsSize.x / 2.0f, vLegsSize.y / 2.0f, vLegsSize.z / 2.0f};

        float fLegsWidth = READ_ADDON_DATA(
            r_float, "bipods_legs_dist", pAddonBipods->GetName(), pAddonBipods->GetAddonName(), BIPODS_LEGS_DIST_DEF);

        Fvector vLegLOfs = {-fLegsWidth / 2.0f, 0.0f, 0.0f};
        m_bipods.m_pPHBipodsLegL = new CPHCollideHelper(this, bipods_legL_contact_callback);
        m_bipods.m_pPHBipodsLegL->AddBox(vLegLOfs, vLegsHalfsize);

        Fvector vLegROfs = {fLegsWidth / 2.0f, 0.0f, 0.0f};
        m_bipods.m_pPHBipodsLegR = new CPHCollideHelper(this, bipods_legR_contact_callback);
        m_bipods.m_pPHBipodsLegR->AddBox(vLegROfs, vLegsHalfsize);
    }
};

//============== Отрисовка ==============//

void CWeapon::BipodsOnRender(bool bHudMode)
{
#ifdef DEBUG
    // Отрисовка отладочной информации
    if (m_bipods.bDbgDraw && IsHudModeNow() && IsBipodsAttached())
    {
        // R1-R2 only ...
        CDebugRenderer& render = Level().debug_renderer();

        u32 iBodyColor = 0;
        switch (m_bipods.dbg_CollideType)
        {
        case bipods_data::EDBGCollideType::ctNoCollide:
        { //--> Коллизии тела нет
            iBodyColor = color_xrgb(0, 255, 0);
            m_bipods.m_pPHWpnBody->DBG_DrawData(iBodyColor);
            break;
        }
        case bipods_data::EDBGCollideType::ctBox:
        { //--> Коллизия тела сошек
            iBodyColor = color_xrgb(255, 255, 0);
            m_bipods.m_pPHWpnBody->DBG_DrawData(iBodyColor);
            break;
        }
        case bipods_data::EDBGCollideType::ctCamera:
        { //--> Коллизия камеры
            iBodyColor = color_xrgb(255, 0, 0);
            render.draw_obb(m_bipods.dbg_mPHCamBody, m_bipods.dbg_vPHCamBodyHalfsize, iBodyColor);
            break;
        }
        };

        // Рисуем шейп ножек сошек
        iBodyColor = color_xrgb(0, 0, 255);
        m_bipods.m_pPHBipodsLegL->DBG_DrawData(iBodyColor);
        m_bipods.m_pPHBipodsLegR->DBG_DrawData(iBodyColor);
    }
#endif
}

//============== Уничтожение родительского объекта ==============//

// Вызывается при net_Destroy() родительского объекта
void CWeapon::BipodsOnDestroy()
{
    xr_delete(m_bipods.m_pPHWpnBody);
    xr_delete(m_bipods.m_pPHBipodsLegL);
    xr_delete(m_bipods.m_pPHBipodsLegR);
}

//============== Сохранение и загрузка ==============//

// Сохранение оружия
void CWeapon::BipodsOnSave(NET_Packet& output_packet)
{
    save_data(m_bipods.m_iBipodState, output_packet);

    if (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON)
    {
        // TODO: Сейчас не сохраняем направление взгляда
        save_data(m_bipods.m_vBipodInitPos, output_packet);
        save_data(m_bipods.m_vBipodInitDir, output_packet);
        save_data(m_bipods.m_vBipodInitNormal, output_packet);
    }
}

// Загрузка оружия
void CWeapon::BipodsOnLoad(IReader& input_packet)
{
    bBipodsUseSavedData = false;

    int iBipodsState = bipods_data::eBS_SwitchedOFF;
    load_data(iBipodsState, input_packet);

    if (iBipodsState == bipods_data::eBS_SwitchedON)
    {
        load_data(vBipodsSavedIPos, input_packet);
        load_data(vBipodsSavedIDir, input_packet);
        load_data(vBipodsSavedINormal, input_packet);

        bBipodsUseSavedData = true;
    }
}

//============== Разложение и убирание сошек ==============//

// Нажатие на клавишу установки сошек
bool CWeapon::Try2DeployBipods()
{
    //--> Сошки присоединены
    if (!IsBipodsAttached())
        return false;
    //--> Владелец оружия - игрок
    if (!ParentIsActor())
        return false;
    //--> В режиме худа
    if (!IsHudModeNow())
        return false;

    bool bDeployed = IsBipodsDeployed();

    //--> Сошки не разложены, но мы в зуме
    if (!bDeployed && IsZoomed())
        return false;
    //--> Сошки не разложены, но оружие занято
    if (!bDeployed && IsPending())
        return false;

    CActor* pActor = H_Parent()->cast_actor();
    R_ASSERT(pActor);

    //--> Игрок не на лестнице
    bool bIsClimbing = ((pActor->MovingState() & mcClimb) != 0);
    if (bIsClimbing)
        return false;

    //--> Игрок играет не от 1-го лица
    CCameraBase* pCamera = pActor->cam_Active();
    if (pCamera != pActor->cam_FirstEye())
        return false;

    // Если игрок уже в режиме сошек - пробуем их снять
    if (m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
    {
        UndeployBipods();
        return false;
    }

    // Рассчитываем параметры и место установки
    collide::rq_result RQ = HUD().GetCurrentRayQuery();

    //--> Позиция установки
    //--> Сперва тестируем точку прямо в прицеле ГГ
    Fvector vInstPos = pCamera->vPosition;
    vInstPos.add(Fvector(pCamera->vDirection).mul(RQ.range));

    //--> Проверка дистанции установки
    float fDisXZ = vInstPos.distance_to_xz(pCamera->vPosition);
    if (fDisXZ > m_bipods.fInstRangeMax)
    {
        //--> Если точка в прицеле слишком далеко, то пробуем найти точку ниже него
        //--> Находим промежуточную позицию, откуда будем пускать луч вниз
        //--> Делаем несколько иттераций поиска
        for (u8 iItt = BIPODS_GRND_SRCH_CNT; iItt >= 1; iItt--)
        {
            Fvector vRayPos = pCamera->vPosition;
            Fvector vRayDir = {0.f, -1.0f, 0.0f};
            float fSearchDist = _min(RQ.range, m_bipods.fInstRangeMax) * BIPODS_GRND_SRCH_FACTOR;
            fSearchDist *= (float)iItt / (float)BIPODS_GRND_SRCH_CNT;

            vRayPos.add(Fvector(pCamera->vDirection).mul(fSearchDist));

            //--> Запускаем луч вниз
            float fRayLength = m_bipods.fInstRangeMax * BIPODS_GRND_RANGE_MOD;

            // clang-format off
            g_pGameLevel->ObjectSpace.RayPick(
                vRayPos,
                vRayDir,
                fRayLength,
                collide::rqtBoth,
                RQ,
                pActor
            );
            // clang-format on

            //--> Получаем и проверяем новые параметры
            vInstPos = vRayPos;
            vInstPos.add(Fvector(vRayDir).mul(RQ.range));

            fDisXZ = vInstPos.distance_to_xz(pCamera->vPosition);
            if (RQ.range >= fRayLength || fDisXZ > m_bipods.fInstRangeMax)
            {
                if (iItt > 1)
                    continue;

#ifdef DEBUG
                Log("~Bipods: Can't deploy bipods, ground too far");
#endif
                return false; //--> Не смогли найти
            }
            else
            {
                break; //--> Смогли найти
            }
        }
    }

    //--> Нормаль места установки (Вектор наклона поверхности)
    Fvector vInstNormal;
    Fvector* pVerts = Level().ObjectSpace.GetStaticVerts();
    CDB::TRI* pTri = Level().ObjectSpace.GetStaticTris() + RQ.element;
    vInstNormal.mknormal(pVerts[pTri->verts[0]], pVerts[pTri->verts[1]], pVerts[pTri->verts[2]]);

    //--> Проверка силы наклона поверхности
    float fMaxYNormal = (90.0f - m_bipods.fInstAngleMax) / 90.f; //--> Переводим углы в наклон нормали
    if (vInstNormal.y < fMaxYNormal)
    {
#ifdef DEBUG
        Log("~Bipods: Can't deploy bipods, ground not enough flat");
#endif
        return false;
    }

    //--> Если у сошек отключён наклон, то оружие всегда ставим ровно
    if (m_bipods.bEnableWpnTilt == false)
    {
        vInstNormal.set(0.0f, 1.0f, 0.0f);
    }

    //--> Дирекция установки
    Fvector vInstDir = pCamera->vDirection;
    float fInstYaw, fInstPitch; // Влево\вправо | Вверх\вниз
    vInstDir.getHP(fInstYaw, fInstPitch);
    vInstDir.setHP(fInstYaw, 0.0f);
    vInstDir.slide(vInstDir, vInstNormal); //--> Наклоняем дирекцию взгляда по нормали целевой поверхности
    vInstDir.normalize_safe();

    // Сошки нельзя установить выше головы
    if (vInstPos.y > pCamera->vPosition.y * BIPODS_CAM_HEIGHT_FACTOR)
    {
#ifdef DEBUG
        Log("~Bipods: Can't deploy bipods, ground too high");
#endif
        return false;
    }

    // Сошки нельзя установить слишком ниже чем тело
    if (vInstPos.y - pActor->Position().y < BIPODS_LOWGROUND_VALUE)
    {
#ifdef DEBUG
        Log("~Bipods: Can't deploy bipods, ground too low");
#endif
        return false;
    }

    // Есть-ли пространство под "ножки"
    R_ASSERT(m_bipods.m_pPHBipodsLegL != nullptr);
    R_ASSERT(m_bipods.m_pPHBipodsLegR != nullptr);

    Fmatrix mPHLegs; //--> XFORM ножек
    mPHLegs.mk_xform_from_vec(vInstDir, vInstNormal, vInstPos);

    m_bipods.m_pPHBipodsLegL->DoCollideAt(mPHLegs);
    m_bipods.m_pPHBipodsLegR->DoCollideAt(mPHLegs);

#ifndef DEBUG_DISABLE_BBOX_CHECK
    //--> Если коллизии нету, дополнительно проверяем bounding-box (прямоугольник)
    if (bIsShellLegLCollided != true)
    {
        m_bipods.m_pPHBipodsLegL->DoBBoxCollide(
            mPHLegs, CPHCollideHelper::EBBoxTestMode::eCentreUpOnly, bipods_legL_ray_callback);
    }

    if (bIsShellLegRCollided != true)
    {
        m_bipods.m_pPHBipodsLegR->DoBBoxCollide(
            mPHLegs, CPHCollideHelper::EBBoxTestMode::eCentreUpOnly, bipods_legR_ray_callback);
    }
#endif

#ifndef DEBUG_DISABLE_LEGS_CHECK
    if (bIsShellLegLCollided != true || bIsShellLegRCollided != true)
    {
#ifdef DEBUG
        Log("~Bipods: Can't find ground for both legs");
#endif
        return false;
    }
#endif

    bIsShellLegLCollided = false;
    bIsShellLegRCollided = false;

    // Двигаем финальную позицию установки для лучшего вида
    vInstPos.add(Fvector().mul(vInstDir, m_bipods.fPosZOffset));

    // Вызываем функцию установки (проверки на наличие свободного места будут в процессе установки)
    return DeployBipods(vInstPos, vInstDir, vInstNormal);
}

// Разложить сошки в указанной позиции
bool CWeapon::DeployBipods(Fvector vDeployPos, Fvector vDeployDir, Fvector vDeployNormal, bool bInstantly)
{
    R_ASSERT(IsBipodsAttached());

    if (IsBipodsDeployed())
        return false;

    IGameObject* pParent = H_Parent();
    R_ASSERT2(pParent, "Can't deploy bipods, weapon parent is null");

    CActor* pActor = pParent->cast_actor();
    R_ASSERT2(pActor, "Can't deploy bipods, parent is not actor");

    // Инициализируем данные сошек
    if (bInstantly)
    { //--> Мгновенная установка
        m_bipods.m_fTranslationFactor = 1.f;
        m_bipods.m_iBipodState = bipods_data::eBS_SwitchedON;
    }
    else
    { //--> Анимированная установка
        m_bipods.m_fTranslationFactor = 0.f;
        m_bipods.m_iBipodState = bipods_data::eBS_TranslateInto;
    }

    m_bipods.m_bZoomMode = false;
    m_bipods.m_vBipodInitPos = vDeployPos;
    m_bipods.m_vBipodInitDir = vDeployDir;
    m_bipods.m_vBipodInitNormal = vDeployNormal;
    m_bipods.m_bFirstCamUpdate = true;
    m_bipods.m_vPrevYP = {0.f, 0.f};
    m_bipods.m_vPrevLegsXYZ = {0.f, 0.f, 0.f};

    // Зумим
    OnZoomIn();

    // Модифицируем камеру от первого лица
    CCameraBase* pFPCam = pActor->cam_FirstEye();
    R_ASSERT(pFPCam != nullptr);

    pFPCam->m_bInputDisabled = !bInstantly; // Блокируем управление камерой

    // Нормализуем текущий Yaw\Pitch\Roll
    pFPCam->yaw = angle_normalize_signed(pFPCam->yaw);
    pFPCam->pitch = angle_normalize_signed(pFPCam->pitch);
    pFPCam->roll = angle_normalize_signed(pFPCam->roll);

    // Сохраняем текущие параметры камеры
    m_bipods.m_vOldCamPos = pFPCam->vPosition;
    m_bipods.m_vOldCamYP.x = pFPCam->yaw;
    m_bipods.m_vOldCamYP.y = pFPCam->pitch;

    // Сохраняем позицию игрока в момент установки
    m_bipods.m_vParentInitPos = pActor->Position();

    return true;
}

// Снять оружие с сошек
void CWeapon::UndeployBipods(bool bInstantly)
{
    // Если мы не в режиме сошек, или уже в процессе выхода - игнорируем вызов
    if (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedOFF ||
        (bInstantly == false && m_bipods.m_iBipodState == bipods_data::eBS_TranslateOutro))
        return;

    // Проверяем что владелец игрок
    IGameObject* pParent = H_Parent();
    R_ASSERT2(pParent, "Can't undeploy bipods, weapon parent is null");

    CActor* pActor = pParent->cast_actor();
    R_ASSERT2(pActor, "Can't undeploy bipods, parent is not actor");

    // Выключаем сошки
    if (bInstantly)
    { //--> Мгновенное снятие
        m_bipods.m_fTranslationFactor = 0.f;
        m_bipods.m_iBipodState = bipods_data::eBS_SwitchedOFF;
    }
    else
    { //--> Анимированное снятие
        m_bipods.m_iBipodState = bipods_data::eBS_TranslateOutro;
    }

    // Возвращаем модель сошек в сложенное положение
    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != nullptr)
    {
        attachable_hud_item* bipods_item = hud_item->FindChildren(m_bipods.sBipodsHudSect);
        if (bipods_item != nullptr)
            bipods_item->m_model->LL_ClearAdditionalTransform(BI_NONE, KinematicsABT::SourceID::WPN_BIPODS_DEPLOY);
    }

    // Убираем зум
    OnZoomOut();
    m_bipods.m_bZoomMode = false;

    // Восстанавливаем параметры камеры от 1-го лица
    CCameraBase* pFPCam = pActor->cam_FirstEye();
    R_ASSERT(pFPCam != nullptr);

    pFPCam->m_bInputDisabled = false;

    if (m_bipods.m_bCamLimitsWasSaved)
        BipodsSetCameraLimits(pFPCam, false);
}

//============== Прицеливание при активных сошках ==============//

// Вызывается при нажатии клавиши зума в режиме сошек
void CWeapon::BipodsZoom(u32 flags)
{
    //--> Во время перезарядки зумить нельзя
    if (IsReloading())
        return;

    if (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON)
    {
        if (b_toggle_weapon_aim)
        {
            // Режим "Нажать раз"
            if (flags & CMD_START)
                m_bipods.m_bZoomMode = !m_bipods.m_bZoomMode;
        }
        else
        {
            // Режим "Пока зажато"
            if (flags & CMD_START)
                m_bipods.m_bZoomMode = true;
            else
                m_bipods.m_bZoomMode = false;
        }
    }
}

//============== Обновление сошек (UpdateCL) ==============//

// Обновление сошек (вызывается на каждом кадре)
void CWeapon::UpdateBipods()
{
    // Обновляем только при наличии сошек
    if (!IsBipodsAttached())
    {
        //--> В конфигах могут отключить сошки у оружия, но они останутся в сохранении
        VERIFY(bBipodsUseSavedData == false);
        bBipodsUseSavedData = false;
        return;
    }

    // Разложить сошки от бедра ...
    if (IsBipodsDeployed() == false)
    {
        do
        {
            // ... если так указано в конфиге
            if (m_bipods.bAlwaysDeployed)
            {
                UpdateBipodsHUD(1.0f, 0.0f, 0.0f, 0.0f, false);
                break;
            }

            // ... при установленном штык-ноже
            if (m_bipods.bDeployWhenBayonetInst)
            {
                UpdateBipodsHUD((IsBayonetAttached() ? 1.0f : 0.0f), 0.0f, 0.0f, 0.0f, false);
                break;
            }
        } while (0);
    }

    // Если грузим сохранение с разложеными сошками, то восстанавливаем их
    if (bBipodsUseSavedData)
    {
        DeployBipods(vBipodsSavedIPos, vBipodsSavedIDir, vBipodsSavedINormal, true);
        bBipodsUseSavedData = false;
    }

    // При сложенных сошках не обновляем
    if (IsBipodsDeployed() == false)
        return;

    // Если сошки разложены, но владелец оружия - не игрок, это значит где-то ошибка
    R_ASSERT2(ParentIsActor(), "Can't update bipods, parent is not actor");

    // Проигрываем эффекты снятия\установки сошек
    if (m_bipods.m_fTranslationFactor >= 0.2)
    {
        if (m_bipods.m_bBipodsIntroEffPlayed == false)
        {
            BipodsPlayEffects(true);
            m_bipods.m_bBipodsIntroEffPlayed = true;
        }
    }
    else
    {
        if (m_bipods.m_bBipodsIntroEffPlayed == true)
        {
            BipodsPlayEffects(false);
            m_bipods.m_bBipodsIntroEffPlayed = false;
        }
    }

    // Регулируем положение тела игрока
    CActor* pActor = H_Parent()->cast_actor();

    float fBaseActorY = m_bipods.m_vParentInitPos.y;
    float fCurCamY = pActor->cam_FirstEye()->vPosition.y;
    float fCamDif = fCurCamY - fBaseActorY;

    u32 iWishState = pActor->get_state_wishful();
    if (fCamDif <= BIPODS_BODY_HEIGHT_LYING)
    { // Низкая присядь
        iWishState |= mcCrouch;
        iWishState |= mcAccel;
    }
    else if (fCamDif <= BIPODS_BODY_HEIGHT_CROUCH)
    { // Присядь
        iWishState |= mcCrouch;
        iWishState &= ~mcAccel;
    }
    else
    { // Стоя
        iWishState &= ~mcCrouch;
        iWishState &= ~mcAccel;
    }
    pActor->set_state_wishful(iWishState);
}

// Обновить худовую модель сошек
void CWeapon::UpdateBipodsHUD(float fDeployFactor, float fHBody, float fPBody, float fPLegs, bool bUseInertion)
{
    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != nullptr)
    {
        attachable_hud_item* bipods_item = hud_item->FindChildren(m_bipods.sBipodsHudSect);
        if (bipods_item != nullptr)
        {
            // Основание сошек
            KinematicsABT::additional_bone_transform offsets(KinematicsABT::SourceID::WPN_BIPODS_DEPLOY);
            float fToRad = PI / 180.f;

            float fX_joint = (0.f + (m_bipods.vBoneMDeployRotOffs.x * fToRad)) * fDeployFactor;
            float fY_joint = (-fHBody + (m_bipods.vBoneMDeployRotOffs.y * fToRad)) * fDeployFactor *
                (m_bipods.vBoneMDeployRotOffs.z < 0.f ? -1.f : 1.f);
            float fZ_joint = (-fPBody + (m_bipods.vBoneMDeployRotOffs.z * fToRad)) * fDeployFactor;

            shared_str str_bone = BONE_MAIN;
            offsets.m_bone_id = bipods_item->m_model->LL_BoneID(str_bone);
            offsets.setRotLocal(fX_joint, fY_joint, fZ_joint);
            offsets.setPosOffsetGlobal(Fvector().mul(m_bipods.vBoneMDeployPosOffs, fDeployFactor));

            bipods_item->m_model->LL_ClearAdditionalTransform(
                offsets.m_bone_id, KinematicsABT::SourceID::WPN_BIPODS_DEPLOY);
            bipods_item->m_model->LL_AddTransformToBone(offsets);

            // Ножки сошек
            float fX_legs = (-(fPLegs / m_bipods.fPitch2LegsTiltFactor) + (m_bipods.vBoneLDeployRotOffs.x * fToRad)) *
                fDeployFactor;
            float fY_legs = (0.f + (m_bipods.vBoneLDeployRotOffs.y * fToRad)) * fDeployFactor;
            float fZ_legs = (0.f + (m_bipods.vBoneLDeployRotOffs.z * fToRad)) * fDeployFactor;

            //--> Добавляем инерцию чтобы смягчить их движения
            if (bUseInertion && m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON)
            {
                m_bipods.m_vPrevLegsXYZ.inertion(
                    {fX_legs, fY_legs, fZ_legs}, clampr(1.0f - Device.fTimeDelta * BIPODS_LEGS_INERTION, 0.0f, 0.99f));
                fX_legs = m_bipods.m_vPrevLegsXYZ.x;
                fY_legs = m_bipods.m_vPrevLegsXYZ.y;
                fZ_legs = m_bipods.m_vPrevLegsXYZ.z;
            }

            //--> Левая ножка
            shared_str noga_1 = BONE_LEG_L;
            offsets.m_bone_id = bipods_item->m_model->LL_BoneID(noga_1);
            offsets.setRotLocal(-fX_legs, -fY_legs, fZ_legs);

            bipods_item->m_model->LL_ClearAdditionalTransform(
                offsets.m_bone_id, KinematicsABT::SourceID::WPN_BIPODS_DEPLOY);
            bipods_item->m_model->LL_AddTransformToBone(offsets);

            //--> Правая ножка
            shared_str noga_2 = BONE_LEG_R;
            offsets.m_bone_id = bipods_item->m_model->LL_BoneID(noga_2);
            offsets.setRotLocal(fX_legs, fY_legs, fZ_legs);

            bipods_item->m_model->LL_ClearAdditionalTransform(
                offsets.m_bone_id, KinematicsABT::SourceID::WPN_BIPODS_DEPLOY);
            bipods_item->m_model->LL_AddTransformToBone(offsets);
        }
    }
}

//============== Эффекты сошек ==============//

// Отыграть эффекты установки \ снятия сошек
void CWeapon::BipodsPlayEffects(bool bIsIntro)
{
    if (bIsIntro)
    { //--> Анимация установки
        // Играем звук установки сошек
        m_sounds.StopSound("sndBipodsU");
        PlaySound("sndBipodsD", get_LastFP());

        // Запускаем анимацию камеры
        attachable_hud_item* pHudItem = HudItemData();
        if (pHudItem != nullptr)
        {
            if (m_bipods.sInstCamAnm != nullptr)
            {
                pHudItem->anim_play_cam_eff(m_bipods.sInstCamAnm.c_str(), m_bipods.fCamAnmSpeed);
            }
        }
    }
    else
    { //--> Анимация снятия
        // Играем звук снятия сошек
        m_sounds.StopSound("sndBipodsD");
        PlaySound("sndBipodsU", get_LastFP());
    }
}

//============== Обновление камеры игрока ==============//

// Каллбэк на повороты камеры игроком при включённых сошках
void CWeapon::OnOwnedCameraMoveWhileBipods(CCameraBase* pCam, float fOldYaw, float fOldPitch) {}

// Ежекадровое обновление позиции камеры игрока от активных сошек
bool CWeapon::UpdateCameraFromBipods(IGameObject* pCameraOwner, Fvector noise_dangle)
{
    R_ASSERT(pCameraOwner != nullptr);

    CActor* pActor = pCameraOwner->cast_actor();
    R_ASSERT(pActor != nullptr);
    R_ASSERT(m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF);

    // Получаем текущую камеру и делаем различные проверки
    CCameraBase* C = pActor->cam_Active();
    bool bFar = (m_bipods.m_vParentInitPos.distance_to(pActor->Position()) > BIPODS_MAX_WALK_DIST) &&
        m_bipods.m_iBipodState != bipods_data::eBS_TranslateOutro;
    if (C != pActor->cam_FirstEye() || !ParentIsActor() || bFar)
    { //--> Игрок не от 1-го лица, не является владельцем или ушёл слишком далеко от места установки
        UndeployBipods(!bFar);
        return false;
    }

    // Убираем зум сошек при перезарядке
    bool bIsReload = IsReloading();
    if (bIsReload)
        m_bipods.m_bZoomMode = false;

    // Восстанавливаем режим Зума после перезарядки
    if (m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON && !IsZoomed() && !bIsReload)
        OnZoomIn();

    // Находим новую позицию камеры
    Fvector vCalcCamPos, vCalcCamDir; // Рассчитанная позиция\дирекция камеры

    Fvector vCalcNormal; // Рассчитанная нормаль камеры (Roll)
    vCalcNormal.lerp({0, 1, 0}, m_bipods.m_vBipodInitNormal, m_bipods.m_fTranslationFactor);

    // Рассчитывем итоговую позицию камеры при установленных сошках
    if (m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF)
    {
        // Должны-ли мы обновить отладочную инфу на этом кадре
        bool bUpdDbgInfo = BipodsCanDrawDbg();

        if (bUpdDbgInfo)
            m_bipods.dbg_CollideType = bipods_data::EDBGCollideType::ctNoCollide;

        // Вычисляем текущую дирекцию камеры
        vCalcCamDir =
            C->Direction(); // <!> Для Y\P камеры getHP() нужно инвертировать <!> + он также не нормализирован (0..2P)

        // Если eBS_SwitchedON выпал на первый апдейт после установки (bInstantly), то форсируем yaw\pitch
        if (m_bipods.m_iBipodState == bipods_data::eBS_TranslateInto ||
            (m_bipods.m_bFirstCamUpdate && m_bipods.m_iBipodState == bipods_data::eBS_SwitchedON))
        {
            vCalcCamDir = m_bipods.m_vBipodInitDir;
        }

        // Проверяем что тело сошек\камера игрока не уприаются в геометрию, корректируем позицию
        BipodsDoCollide(vCalcCamPos, vCalcCamDir, vCalcNormal, pCameraOwner, noise_dangle);
    }

    // Обрабатываем различные состояния
    switch (m_bipods.m_iBipodState)
    {
    case bipods_data::eBS_TranslateInto:
    { //--> Установка сошек
        m_bipods.m_fTranslationFactor += Device.fTimeDelta / m_bipods.fDeployTime;
        clamp(m_bipods.m_fTranslationFactor, 0.f, 1.f);

        // Плавный переход камеры
        //--> POS
        C->vPosition.lerp(m_bipods.m_vOldCamPos, vCalcCamPos, m_bipods.m_fTranslationFactor);

        //--> DIR (Yaw + Pitch)
        float fTYaw, fTPitch;
        vCalcCamDir.getHP(fTYaw, fTPitch);
        fTYaw = angle_normalize_signed(fTYaw);
        fTPitch = angle_normalize_signed(fTPitch);

        float fYawDiff = angle_difference_signed(fTYaw, -m_bipods.m_vOldCamYP.x);

        C->yaw = fTYaw - (fYawDiff * (1.f - m_bipods.m_fTranslationFactor));
        C->yaw *= -1.f;

        float fPitchDiff = angle_difference_signed(fTPitch, -m_bipods.m_vOldCamYP.y);
        C->pitch = fTPitch - (fPitchDiff * (1.f - m_bipods.m_fTranslationFactor));
        C->pitch *= -1.f;

        //--> Roll
        C->vNormal = vCalcNormal;

        if (m_bipods.m_fTranslationFactor == 1.f)
        {
            // Переход завершён
            C->m_bInputDisabled = false;
            m_bipods.m_iBipodState = bipods_data::eBS_SwitchedON;

            // Ограничиваем камеру
            BipodsSetCameraLimits(C, true);
        }
    }
    break;
    case bipods_data::eBS_SwitchedON:
    { //--> Сошки включены
        m_bipods.m_fTranslationFactor = 1.f;

        // Заменяем позицию и дирекцию камеры
        C->vPosition = vCalcCamPos;
        C->vDirection = vCalcCamDir;
        C->vNormal = m_bipods.m_vBipodInitNormal;

        // Если это первый апдейт камеры после установки сошек, то форсим поворот камеры для резкой установки.
        if (m_bipods.m_bFirstCamUpdate)
        {
            vCalcCamDir.getHP(C->yaw, C->pitch);
            C->yaw = angle_normalize_signed(C->yaw) * -1.f;
            C->pitch = angle_normalize_signed(C->pitch) * -1.f;

            // Ограничиваем камеру
            BipodsSetCameraLimits(C, true);
        }
    }
    break;
    case bipods_data::eBS_TranslateOutro:
    { //--> Снятие сошек
        m_bipods.m_fTranslationFactor -= Device.fTimeDelta / m_bipods.fUndeployTime;
        clamp(m_bipods.m_fTranslationFactor, 0.f, 1.f);

        // Плавный переход камеры
        //--> POS
        C->vPosition.lerp(C->vPosition, vCalcCamPos, m_bipods.m_fTranslationFactor);

        if (m_bipods.m_fTranslationFactor == 0.f)
        {
            // Переход завершён
            m_bipods.m_iBipodState = bipods_data::eBS_SwitchedOFF;
        }
    }
    break;
    case bipods_data::eBS_SwitchedOFF:
    { //--> Сошки выключены
        R_ASSERT(false); //--> Этот код не должен вызываться
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
    if (m_bipods.bAnimatedLegs && !bDisableAnimUpd) // В процессе перезарядки кости не двигаем <!>
    {
        //*******************************************//

        // Двигаем кости сошек
        Fquaternion Q;

        float yaw = (-m_bipods.m_vBipodInitDir.getH()); // Y
        float& cam_yaw = C->yaw;
        float fHBody = angle_difference_signed(yaw, cam_yaw); // Разница текущего Yaw от Yaw при установке
        if (m_bipods.bInvertBodyYaw)
        {
            fHBody *= -1.f;
        }

        float pitch = (-m_bipods.m_vBipodInitDir.getP()); // X
        float& cam_pitch = C->pitch;
        float fPBody = angle_difference_signed(pitch, cam_pitch); // Разница текущего Pitch от Pitch при установке
        float fPLegs = fPBody;
        if (m_bipods.bInvertBodyPitch)
        {
            fPBody *= -1.f;
        }
        if (m_bipods.bInvertLegsPitch)
        {
            fPLegs *= -1.f;
        }

        //--> В мировой модели
        // TODO: При необходимости

        //--> В худе
        float fDeployFactor = m_bipods.m_fTranslationFactor;
        if (m_bipods.bAlwaysDeployed || (m_bipods.bDeployWhenBayonetInst && IsBayonetAttached()))
        {
            fDeployFactor = 1.0f;
        }
        UpdateBipodsHUD(fDeployFactor,
            fHBody * m_bipods.m_fTranslationFactor, fPBody * m_bipods.m_fTranslationFactor,
            fPLegs * m_bipods.m_fTranslationFactor, true);

        //*******************************************//
    }
#endif // ! DEBUG_DISABLE_MODEL_ANIMATED

    return true; //--> Отключаем стандартную коллизию камеры
}

// Ограничить или восстановить лимиты камеры при активных сошках
void CWeapon::BipodsSetCameraLimits(CCameraBase* pCam, bool bLimit)
{
    float fToRad = PI / 180.f;

    if (bLimit == true)
    { //=== Установка лимитов ===//
        R_ASSERT(m_bipods.m_bCamLimitsWasSaved == false);

        // Сохраняем текущие лимиты
        m_bipods.m_fOldYawLimit = pCam->lim_yaw;
        m_bipods.m_fOldPitchLimit = pCam->lim_pitch;
        m_bipods.m_bOldClampYaw = pCam->bClampYaw;
        m_bipods.m_bOldClampPitch = pCam->bClampPitch;

        ////////////// YAW (Y) //////////////
        float yaw = (-m_bipods.m_vBipodInitDir.getH());
        float& cam_yaw = pCam->yaw;
        float delta_yaw = angle_difference_signed(yaw, cam_yaw);

        yaw = cam_yaw + delta_yaw;
        pCam->lim_yaw[0] = (yaw + (m_bipods.vCamYawLimits.x * fToRad));
        pCam->lim_yaw[1] = (yaw + (m_bipods.vCamYawLimits.y * fToRad));
        pCam->bClampYaw = true;

        ////////////// PITCH (X) //////////////
        float pitch = (-m_bipods.m_vBipodInitDir.getP());
        float& cam_pitch = pCam->pitch;
        float delta_pitch = angle_difference_signed(pitch, cam_pitch);

        pitch = cam_pitch + delta_pitch;
        pCam->lim_pitch[0] = (pitch + (-1 * m_bipods.vCamPitchLimits.y * fToRad));
        pCam->lim_pitch[1] = (pitch + (-1 * m_bipods.vCamPitchLimits.x * fToRad));
        pCam->bClampPitch = true;

        m_bipods.m_bCamLimitsWasSaved = true;
    }
    else
    { //=== Снятие лимитов ===
        R_ASSERT(m_bipods.m_bCamLimitsWasSaved == true);

        ////////////// YAW (Y) //////////////
        pCam->lim_yaw = m_bipods.m_fOldYawLimit;
        pCam->bClampYaw = m_bipods.m_bOldClampYaw;
        pCam->CheckLimYaw();

        ////////////// PITCH (X) //////////////
        pCam->lim_pitch = m_bipods.m_fOldPitchLimit;
        pCam->bClampPitch = m_bipods.m_bOldClampPitch;
        pCam->CheckLimPitch();

        m_bipods.m_bCamLimitsWasSaved = false;
    }
}

//============== Физика сошек ==============//

// Обработка коллизии сошек и камеры с объектами
bool CWeapon::BipodsDoCollide(
    Fvector& vCalcCamPos, Fvector& vCalcCamDir, Fvector& vCalcNormal, IGameObject* pCameraOwner, Fvector noise_dangle)
{
    R_ASSERT(pCameraOwner != nullptr);

    CActor* pActor = pCameraOwner->cast_actor();
    R_ASSERT(pActor != nullptr);
    R_ASSERT(m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF);

    // Получаем текущую камеру и делаем различные проверки
    CCameraBase* C = pActor->cam_Active();

    // Добавляем смещение камеры от центра установки
    float covariance = VIEWPORT_NEAR * 6.f;
    u8 iCollideCheckStage = 0;

    float fCurYaw, fCurPitch; /// Текущие Yaw\Pitch

    std::function<bool()> fnDoCollide = [&]() -> bool {
        if (iCollideCheckStage == 0)
        {
            fCurYaw = C->yaw;
            fCurPitch = C->pitch;
        }

        // Заменяем позицию камеры на позицию установки
        vCalcCamPos = m_bipods.m_vBipodInitPos;

        // Добавляем смещение камеры вверх по нормали
        vCalcCamPos.mad(m_bipods.m_vBipodInitNormal, m_bipods.fCamYOffset);

        // Рассчитываем смещение от центра
        // clang-format off
        collide::rq_result R;
        g_pGameLevel->ObjectSpace.RayPick(
            vCalcCamPos,
            Fvector().invert(vCalcCamDir),
            m_bipods.fCamDistMax + covariance,
            collide::rqtBoth,
            R,
            pActor
        );
        // clang-format on

        float fZOffset = (R.range - covariance);
        vCalcCamPos.add(Fvector().mul(vCalcCamDir, -(fZOffset - VIEWPORT_NEAR))); //--> Камера находится сзади

        // Проверяем необходимость обновления отладочной инфы
        bool bUpdDbgInfo = BipodsCanDrawDbg();

        // Проверяем коллизию камеры
        C->SaveCamVec();
        C->Set(vCalcCamPos, vCalcCamDir, vCalcNormal);

        Fmatrix pOutCamXFORM;
        Fvector pOutCamBox;
        Fvector vCamPosOfsView = {0.0f, 0.025f, 0.1f};
        Fvector vCamPosOfsWorld = {0.0f, 0.0f, 0.0f};
        bool bIsCollided = test_camera_collide(
            *C, VIEWPORT_NEAR, pActor, vCamPosOfsView, vCamPosOfsWorld, 0.9f, &pOutCamXFORM, &pOutCamBox);

        if (bIsCollided)
        { //--> Камера игрока ударяется об геометрию
#ifdef DEBUG
            Log("~Bipods: Collided by camera!");
#endif

            if (bUpdDbgInfo)
            {
                m_bipods.dbg_CollideType = bipods_data::EDBGCollideType::ctCamera;
                m_bipods.dbg_mPHCamBody = pOutCamXFORM;
                m_bipods.dbg_vPHCamBodyHalfsize.mul(pOutCamBox, 0.5f);
            }
        }

        C->RestoreCamVec();

        // Если с колизией камеры проблем нет, то проверяем коллизию "тела" сошек
        if (bIsCollided == false)
        {
            R_ASSERT(m_bipods.m_pPHWpnBody != nullptr);

            Fmatrix mPHBody; //--> XFORM тела сошек
            mPHBody.mk_xform_from_vec(vCalcCamDir, vCalcNormal, m_bipods.m_vBipodInitPos);

            //--> Сперва тестируем физ. оболочку (не надёжная)
            m_bipods.m_pPHWpnBody->DoCollideAt(mPHBody);

#ifndef DEBUG_DISABLE_BBOX_CHECK
            //--> Если коллизии нету, дополнительно проверяем bounding-box (прямоугольник)
            if (bIsShellBodyCollided == false)
            {
                if (m_bipods.m_iBipodState == bipods_data::eBS_TranslateInto)
                { //--> Только в стадии установки (защита от установки сквозь стены)
                    m_bipods.m_pPHWpnBody->DoBBoxCollide(
                        mPHBody, CPHCollideHelper::EBBoxTestMode::eEdgesOnly, bipods_body_ray_callback);
                }
            }
#endif

            if (bIsShellBodyCollided)
            { //--> Тело сошек ударяется об геометрию
#ifdef DEBUG
                Log("~Bipods: Collided by body!");
#endif

                if (bUpdDbgInfo)
                    m_bipods.dbg_CollideType = bipods_data::EDBGCollideType::ctBox;

                bIsCollided = true;
            }
            bIsShellBodyCollided = false;
        }

        //--> Необходимость пропуска проверки коллизии (например во время анимации снятия)
        bool bSkipCollideCheck = (m_bipods.m_iBipodState == bipods_data::eBS_TranslateOutro);

#ifdef DEBUG_DISABLE_COLLIDE_CHECK
        bSkipCollideCheck = true;
#endif
        //--> Ближе этого расстояния не можем приближать камеру к месту установка
        float fCamTooCloseDist = m_bipods.fCamDistMax * BIPODS_CAM_CLOSE_FACTOR;

        //--> Проверка на колизию камеры (во время анимации снятия - не производим)
        if (bSkipCollideCheck == true || (bIsCollided == false && fZOffset >= fCamTooCloseDist))
        {
            // Камера не колизится - сохраняем текущие Yaw\Pitch
            m_bipods.m_vPrevYP = {C->yaw, C->pitch};
            return true;
        }
        else
        {
            // Камера колизится - пытаемся откатить камеру к предыдущему кадру в 3 попытки
            //--> Во время анимации установки - не пытаемся корректировать
            if (iCollideCheckStage != 3 && (m_bipods.m_iBipodState != bipods_data::eBS_TranslateInto))
            { //--> Если мы уже разложили сошки, то пытаемся корректировать камеру, чтобы не влезать в стены
                if (iCollideCheckStage == 0)
                { // Откат только Yaw
                    C->yaw = m_bipods.m_vPrevYP[0];
                    C->pitch = fCurPitch;
                    C->Update(C->Position(), noise_dangle);
                }
                else if (iCollideCheckStage == 1)
                { // Откат только Pitch
                    C->yaw = fCurYaw;
                    C->pitch = m_bipods.m_vPrevYP[1];
                    C->Update(C->Position(), noise_dangle);
                }
                else if (iCollideCheckStage == 2)
                { // Откат Yaw и Pitch
                    C->yaw = m_bipods.m_vPrevYP[0];
                    C->pitch = m_bipods.m_vPrevYP[1];
                    C->Update(C->Position(), noise_dangle);
                }

                vCalcCamDir = C->Direction();

                iCollideCheckStage++; //--> Несколько попыток

                return fnDoCollide();
            }
            else
            {
                // Мы не смогли успешно откатиться к позиции с прошлого кадра - снимаем сошки.
                UndeployBipods();
                return false;
            }
        }
    };

    return fnDoCollide();
}

// Колбэк на контакт "тела" сошек с геометрией
//--> Физическая оболочка
static void bipods_body_contact_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (!do_collide || bIsShellBodyCollided)
        return;

    do_collide = false;

    // Игнорируем поверхности, через которые можно ходить
    SGameMtl* oposite_matrial = bo1 ? material_1 : material_2;
    if (oposite_matrial->Flags.test(SGameMtl::flPassable))
        return;

    dxGeomUserData* my_data = PHRetrieveGeomUserData(bo1 ? c.geom.g1 : c.geom.g2);
    dxGeomUserData* oposite_data = PHRetrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);

    VERIFY(my_data);

    // Игнорируем коллизию со своим родительским объектом (оружием)
    if (oposite_data && oposite_data->ph_ref_object == my_data->ph_ref_object)
        return;

    // Игнорируем коллизию с игроком
    if (oposite_data && oposite_data->ph_ref_object->IsActor())
        return;

    bIsShellBodyCollided = true;
}

//--> Bounding box (прямоугольники, которые описывают все элементы физ. оболочки)
static BOOL bipods_body_ray_callback(collide::rq_result& result, LPVOID params)
{
    phcollider_bbox_callback_data& data = *(phcollider_bbox_callback_data*)params;
    Fvector collide_position = Fvector().mad(data.vRayPos, data.vRayDirN, result.range);

    if (!result.O)
    { //--> Статический объект
        // Получить треугольник и узнать его материал
        CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;

        SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
        if (mtl->Flags.is(SGameMtl::flPassable))
        { //--> С бестелесными объектами коллизии нету
            return TRUE;
        }
        else
        {
#ifdef DEBUG
            Log("~Bipods: (BBox) Collide with geometry");
#endif
        }
    }
    else
    { //--> Динамический объект (родительское оружие исключено по умолчанию)
        if (result.O->cast_actor() != nullptr)
        { //--> С игроком коллизии нету
            return TRUE;
        }
        else
        {
#ifdef DEBUG
            Log("~Bipods: (BBox) Collide with din. object -", result.O->cNameSect_str());
#endif
        }
    }

    // Столкнулись с чем-то непреодолимым - заканчиваем поиск
    bIsShellBodyCollided = true;
    data.bStopTesting = true;

    return FALSE;
}

// Колбэк на контакт ножек сошек с геометрией
//--> Физическая оболочка
static void bipods_legs_contact_callback(
    bool& bClbkFlag, bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (!do_collide || bClbkFlag)
        return;

    do_collide = false;

    // Игнорируем поверхности, через которые можно ходить
    SGameMtl* oposite_matrial = bo1 ? material_1 : material_2;
    if (oposite_matrial->Flags.test(SGameMtl::flPassable))
        return;

    dxGeomUserData* my_data = PHRetrieveGeomUserData(bo1 ? c.geom.g1 : c.geom.g2);
    dxGeomUserData* oposite_data = PHRetrieveGeomUserData(bo1 ? c.geom.g2 : c.geom.g1);

    VERIFY(my_data);

    // Игнорируем коллизию со своим родительским объектом (оружием)
    if (oposite_data && oposite_data->ph_ref_object == my_data->ph_ref_object)
        return;

    // Игнорируем коллизию с игроком
    if (oposite_data && oposite_data->ph_ref_object->IsActor())
        return;

    bClbkFlag = true;
}

static void bipods_legL_contact_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    bipods_legs_contact_callback(bIsShellLegLCollided, do_collide, bo1, c, material_1, material_2);
}

static void bipods_legR_contact_callback(
    bool& do_collide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    bipods_legs_contact_callback(bIsShellLegRCollided, do_collide, bo1, c, material_1, material_2);
}

//--> Bounding box (прямоугольники, которые описывают все элементы физ. оболочки)
static BOOL bipods_body_legs_callback(bool& bClbkFlag, collide::rq_result& result, LPVOID params)
{
    phcollider_bbox_callback_data& data = *(phcollider_bbox_callback_data*)params;
    Fvector collide_position = Fvector().mad(data.vRayPos, data.vRayDirN, result.range);

    if (!result.O)
    { //--> Статический объект
        // Получить треугольник и узнать его материал
        CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;

        SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
        if (mtl->Flags.is(SGameMtl::flPassable))
        { //--> С бестелесными объектами коллизии нету
            return TRUE;
        }
    }
    else
    { //--> Динамический объект (родительское оружие исключено по умолчанию)
        if (result.O->cast_actor() != nullptr)
        { //--> С игроком коллизии нету
            return TRUE;
        }
    }

    // Столкнулись с чем-то непреодолимым - заканчиваем поиск
    bClbkFlag = true;
    data.bStopTesting = true;

    return FALSE;
}

static BOOL bipods_legL_ray_callback(collide::rq_result& result, LPVOID params)
{
    return bipods_body_legs_callback(bIsShellLegLCollided, result, params);
}

static BOOL bipods_legR_ray_callback(collide::rq_result& result, LPVOID params)
{
    return bipods_body_legs_callback(bIsShellLegRCollided, result, params);
}

//============== Работа с фонариком игрока ==============//

// Обновление базовой позиции света от фонарика игрока
void CWeapon::UpdateActorTorchLightPosFromBipods(Fvector* pTorchPos)
{
    R_ASSERT(m_bipods.m_iBipodState != bipods_data::eBS_SwitchedOFF);

    //--> Мы должны быть в режиме худа
    if (HudItemData() == nullptr)
        return;

    //--> Корректируем позицию фонарика для более качественного освещения при активных сошках
    Fmatrix m_attach_offset;
    m_attach_offset.setHPB(0, 0, 0);
    m_attach_offset.translate_over(m_bipods.vTorchOffset);

    Fmatrix result;
    result.mul(HudItemData()->m_item_transform, m_attach_offset);

    //--> Плавно смещаем позицию фонарика
    pTorchPos->lerp(*pTorchPos, result.c, m_bipods.m_fTranslationFactor);
}
