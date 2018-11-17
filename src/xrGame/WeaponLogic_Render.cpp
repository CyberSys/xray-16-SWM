/*************************************************/
/***** Код для рендеринга оружия *****/ //--#SM+#--
/*************************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"
#include "attachable_visual.h"

#define BULLET_BONE_NAME "wpn_bullet" // Имя кости, вариации которой будут показаны\скрыты в зависимости от числа патронов в оружии

extern u32 hud_adj_mode;

// Проверка, что сейчас включён вид от 1-го лица (худ показывается)
bool CWeapon::IsHudModeNow() { return (HudItemData() != NULL); }

// Требуется-ли отрисовывать перекрестие
bool CWeapon::show_crosshair()
{
    if (hud_adj_mode)
        return true;

    if (GetState() == eKick)
        return true;

    return !IsPending() && (!IsZoomed() || !ZoomHideCrosshair());
}

// Требуется-ли отображать интерфейс игрока
bool CWeapon::show_indicators() { return !(IsZoomed() && ZoomTexture()); }

// Получить текущий износ оружия для UI
float CWeapon::GetConditionToShow() const
{
    return (GetCondition()); //powf(GetCondition(),4.0f));
}

// Получить индекс текущих координат худа
u8 CWeapon::GetCurrentHudOffsetIdx()
{
    CActor* pActor = smart_cast<CActor*>(H_Parent());
    if (!pActor)
        return 0;

    bool b_aiming = ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f));

    if (!b_aiming)
        return 0;
    else if (m_bGrenadeMode)
        return 2;
    else
        return 1;
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

    // Поворот ствола во время аима
    if ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) || (!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f))
    {
        Fvector curr_offs, curr_rot;
        curr_offs = hi->m_measures.m_hands_offset[0][idx]; //pos,aim
        curr_rot  = hi->m_measures.m_hands_offset[1][idx]; //rot,aim

        if (IsBipodsDeployed())
        {
            curr_offs.z += m_bipods.m_hud_z_offset;
        }

        curr_offs.mul(m_zoom_params.m_fZoomRotationFactor);
        curr_rot.mul(m_zoom_params.m_fZoomRotationFactor);

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
            m_zoom_params.m_fZoomRotationFactor += Device.fTimeDelta / m_zoom_params.m_fZoomRotateTime;
        else
            m_zoom_params.m_fZoomRotationFactor -= Device.fTimeDelta / m_zoom_params.m_fZoomRotateTime;

        clamp(m_zoom_params.m_fZoomRotationFactor, 0.f, 1.f);
    }

    // Боковой стрейф с оружием
    clamp(idx, u8(0), u8(1));

    // Рассчитываем фактор боковой ходьбы
    float fStrafeMaxTime = hi->m_measures.m_strafe_offset[2][idx].y; // Макс. время в секундах, за которое мы наклонимся из центрального положения
    if (fStrafeMaxTime <= EPS)
        fStrafeMaxTime = 0.01f;

    float fStepPerUpd = Device.fTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота

    u32 iMovingState = pActor->MovingState();
    if ((iMovingState & mcLStrafe) != 0)
    { // Движемся влево
        float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * 3 : fStepPerUpd);
        m_fLR_MovingFactor -= fVal;
    }
    else if ((iMovingState & mcRStrafe) != 0)
    { // Движемся вправо
        float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * 3 : fStepPerUpd);
        m_fLR_MovingFactor += fVal;
    }
    else
    { // Двигаемся в любом другом направлении
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

    // Производим наклон ствола для нормального режима и аима
    for (int _idx = 0; _idx <= 1; _idx++)
    {
        bool bEnabled = hi->m_measures.m_strafe_offset[2][_idx].x;
        if (!bEnabled)
            continue;

        Fvector curr_offs, curr_rot;

        // Смещение позиции худа в стрейфе
        curr_offs = hi->m_measures.m_strafe_offset[0][_idx]; //pos
        curr_offs.mul(m_fLR_MovingFactor);                   // Умножаем на фактор стрейфа

        // Поворот худа в стрейфе
        curr_rot = hi->m_measures.m_strafe_offset[1][_idx]; //rot
        curr_rot.mul(-PI / 180.f);                          // Преобразуем углы в радианы
        curr_rot.mul(m_fLR_MovingFactor);                   // Умножаем на фактор стрейфа

        if (_idx == 0)
        { // От бедра
            curr_offs.mul(1.f - m_zoom_params.m_fZoomRotationFactor);
            curr_rot.mul(1.f - m_zoom_params.m_fZoomRotationFactor);
        }
        else
        { // Во время аима
            curr_offs.mul(m_zoom_params.m_fZoomRotationFactor);
            curr_rot.mul(m_zoom_params.m_fZoomRotationFactor);
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
}

// Получить текущий FOV для зума
float CWeapon::CurrentZoomFactor()
{
    if (IsGrenadeLauncherAttached() && m_bGrenadeMode)
        return m_zoom_params.m_fIronSightZoomFactor;

    return IsScopeAttached() ? m_zoom_params.m_fScopeZoomFactor : m_zoom_params.m_fIronSightZoomFactor;
};

// Получить FOV текущего оружия
float CWeapon::GetFov()
{
    if (IsBipodsDeployed() && !ZoomTexture())
    { // FOV при сошках
        if (m_bipods.m_bUseZoomFov)
        {
            return (IsScopeAttached() && m_bipods.m_fCurScopeZoomFov > 0.0f ? m_bipods.m_fCurScopeZoomFov : m_bipods.m_fZoomFOV);
        }
        else
            return g_fov;
    }
    else
    { // FOV при обычном зуме
        if (IsZoomed() && (!ZoomTexture() || (!IsRotatingToZoom() && ZoomTexture())))
        {
            return GetZoomFactor() * (0.75f);
        }
    }

    // Дефолт
    return g_fov;
}

// Получить HUD FOV текущего оружия
float CWeapon::GetHudFov()
{
    if (IsBipodsDeployed())
    { // При разложенных сошках
        return m_bipods.m_fHUD_FOV;
    }

    if (m_zoom_params.m_fZoomRotationFactor > 0.0f)
    {
        // В процессе зума
        float fDiff = psHUD_FOV_def - m_zoom_params.m_fZoomHudFov;
        return m_zoom_params.m_fZoomHudFov + (fDiff * (1 - m_zoom_params.m_fZoomRotationFactor));
    }
    else
    {
        // От бедра
        return psHUD_FOV_def;
    }
}

// Получить фактор силы инерции
float CWeapon::GetInertionPowerFactor()
{
    if (IsBipodsDeployed())
    {
        return 1.f - ((1.f - m_bipods.m_inertia_power) * GetZRotatingFactor());
    }

    return 1.f;
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
    if (m_zoom_params.m_pVision != NULL)
        m_zoom_params.m_pVision->Draw();

    ZoomTexture()->Update();
    ZoomTexture()->Draw();
}

// Рендеринг в режиме худа
void CWeapon::render_hud_mode() { RenderLight(); }

// Апдейт во время рендера оружия (вызывается только для оружия в зоне видимости игрока)
// НЕ ВЫЗЫВАЕТСЯ когда оружие в руках от первого лица, т.к его мировая модель в этот момент не рендерится <!>
void CWeapon::renderable_Render()
{
    // Обновляем XForm от третьего лица
    UpdateXForm();

    // Рисуем подсветку
    RenderLight();

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
            render.draw_aabb(get_LastFP(), 0.005f, 0.005f, 0.005f, D3DCOLOR_XRGB(255, 0, 0));

        if (hud_adj_mode == 6)
            render.draw_aabb(get_LastFP2(), 0.005f, 0.005f, 0.005f, D3DCOLOR_XRGB(0, 0, 255));

        if (hud_adj_mode == 7)
            render.draw_aabb(get_LastSP(), 0.005f, 0.005f, 0.005f, D3DCOLOR_XRGB(0, 255, 0));
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
void CWeaponKnife::OnRender()
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

// Проверить текущие партиклы пламя на их присутствие в конфигах
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
bool CWeapon::IsMovementEffectorAllowed()
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
                if (m_dwHideBulletVisual <= Device.dwTimeGlobal) // "fix" for blinking bullets
                    iAmmoIdx--;

            bool         bVisible = iAmmoIdx < iAmmoCnt;
            AmmoBeltData data     = m_AmmoBeltData[idx];

            // Msg("[%d]:bVisible = [%d] (iAmmoIdx = %d iAmmoCnt = %d GetState() = %d GetReloadState() = %d IsTriStateReload() = %d, m_bGrenadeMode = %d)", idx, bVisible, iAmmoIdx, iAmmoCnt, GetState() == eReload, GetReloadState() == eSubstateReloadInProcess, m_bGrenadeMode, IsTriStateReload());

            // HUD
            if (GetHUDmode() == true && m_sAB_hud != NULL)
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
                                        sBulletHud = cartridges->at(iType).m_sHudVisual;
                                }
                                else
                                {
                                    CCartridge* pCartridge = magazine->at(idx);
                                    if (pCartridge != NULL)
                                        sBulletHud = pCartridge->m_sHudVisual;
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
                                    sBulletVis = cartridges->at(iType).m_sWorldVisual;
                            }
                            else
                            {
                                CCartridge* pCartridge = magazine->at(idx);
                                if (pCartridge != NULL)
                                    sBulletVis = pCartridge->m_sWorldVisual;
                            }

                            //bullet_item->SetVisual(sBulletVis); // SM_TODO: Это рабочий код?
                        }
                    }
                    //*************************************************//
                }
            }
            ////////////////////////////////////////////////////////////////////////////
        }
    }
}

// Патрон, который сейчас заряжаем
void CWeapon::UpdateBulletVisual()
{
    if (!GetHUDmode())
        return;
    if (m_sBulletVisual == NULL)
        return;

    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != NULL)
    {
        bool bVisible = true;

        // Отображаем\скрываем патрон
        hud_item->UpdateChildrenList(m_sBulletVisual, bVisible);

        // Обновляем его визуал
        attachable_hud_item* bullet_item = hud_item->FindChildren(m_sBulletVisual);
        if (bullet_item != NULL)
        {
            u8 iType = (m_set_next_ammoType_on_reload == undefined_ammo_type ? m_ammoType : m_set_next_ammoType_on_reload);
            if (iType < m_AmmoCartidges.size())
                bullet_item->UpdateVisual(m_AmmoCartidges[iType].m_sHudVisual);
        }
    }
}

// Гильза после выстрела
void CWeapon::UpdateShellVisual()
{
    if (!GetHUDmode())
        return;
    if (m_sShellVisual == NULL)
        return;

    attachable_hud_item* hud_item = HudItemData();
    if (hud_item != NULL)
    {
        bool bVisible = (m_dwShowShellVisual >= Device.dwTimeGlobal && GetState() != eReload);

        // Отображаем\скрываем гильзу
        hud_item->UpdateChildrenList(m_sShellVisual, bVisible);

        // Обновляем её визуал
        attachable_hud_item* shell_item = hud_item->FindChildren(m_sShellVisual);
        if (shell_item != NULL)
            shell_item->UpdateVisual(m_sCurrentShellModel);
    }
}

// Обновляем все связанные с текущим оружием визуалы
void CWeapon::UpdateWpnVisuals()
{
    if (GetHUDmode() == true)
    { // Обновляем визуалы худа
        attachable_hud_item* hud_item = HudItemData();
        if (hud_item != NULL)
        {
            // Перебираем все активные атачи худа и скрываем\раскрываем кости в зависимости от числа патронов
            for (u32 i = 0; i < hud_item->m_socket_items.size(); i++)
            {
                attachable_hud_item* item = hud_item->m_socket_items[i];
                if (item != NULL)
                {
                    IRenderVisual* pVis = item->m_model->dcast_RenderVisual();
                    if (pVis != NULL)
                    {
                        vis_object_data* object_data = pVis->getVisData().obj_data;

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
                vis_object_data* object_data = pVis->getVisData().obj_data;

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
                            if (pModel->LL_GetBoneVisible(bone_id) != bShow)
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

    bool bCond_1 = m_zoom_params.m_fZoomRotationFactor > 0.05f;    // Мы должны целиться
    bool bCond_2 = m_zoom_params.m_fSecondVP_FovFactor > 0.0f;     // В конфиге должен быть прописан фактор зума (scope_lense_fov_factor) больше чем 0
    bool bCond_3 = pActor->cam_Active() == pActor->cam_FirstEye(); // Мы должны быть от 1-го лица

    Device.m_SecondViewport.SetSVPActive(bCond_1 && bCond_2 && bCond_3);
}

// Статическая функция. Считывает из модели число костей с привязкой к числу патронов
// Если в любой модели (худовой или мировой) есть кости вида bullet_1, bullet_2, ... bullet_x, то они будут скрываться
// если число патронов в основном магазине меньше, чем числовой индекс кости
void CWeapon::ReadMaxBulletBones(IKinematics* pModel)
{
    R_ASSERT(pModel != NULL);

    vis_object_data* object_data    = pModel->dcast_RenderVisual()->getVisData().obj_data;
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