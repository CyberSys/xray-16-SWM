/********************************/
/***** Состояние "Стрельба" *****/ //--#SM+#--
/********************************/

#include "StdAfx.h"
#include "Weapon.h"

#include "reward_event_generator.h"
#include "HUDManager.h"
#include "ExplosiveRocket.h"
#include "MPPlayersBag.h"

#ifdef DEBUG
#include "object_handler_planner.h"
#endif

// Пробуем начать стрельбу на клиенте
bool CWeapon::Try2Fire(bool bCheckOnlyMode)
{
    if (m_bKnifeMode || !IsShootingAllowed())
        return false;
    if (!bCheckOnlyMode && GetState() == eFire)
        return false;
    if (IsPending() == true)
        return false;

    if (IsAmmoBeltReloadNow())
        return false;

    if (IsMagazine())
        return false;

    if (!IsMisfire())
    {
        // Оружие может стрелять
        if (iAmmoElapsed > 0)
        {
            // Есть патроны
            if (!IsWorking())
            {
                if (GetState() == eReload)
                    return false;
                if (GetState() == eShowing)
                    return false;
                if (GetState() == eHiding)
                    return false;
                if (GetState() == eMisfire)
                    return false;

                R_ASSERT(H_Parent());
                if (!bCheckOnlyMode)
                    SwitchState(eFire);
                return true;
            }
        }
        else
        {
            // Патронов нет
            if (!bCheckOnlyMode)
                OnEmptyClick();
            return false;
        }
    }
    else
    { // У оружия осечка
        if (!bCheckOnlyMode)
            Need2Misfire();
    }

    return false;
}

// Пробуем начать удар ножом на клиенте
bool CWeapon::Try2Knife(bool bAltAttack)
{
    if (!IsShootingAllowed())
        return false;
    if (m_bKnifeMode == false)
        return false;
    if (IsPending() == true)
        return false;

    if (!bAltAttack)
        SwitchState(eFire);
    else
        SwitchState(eFire2);

    return true;
}

// Нужно остановить стрельбу на клиенте
void CWeapon::Need2Stop_Fire()
{
    if (GetState() == eFire || GetState() == eFire2)
        Need2Idle();
}

// Переключение стэйта на "Стрельба"
void CWeapon::switch2_Fire()
{
    if (!IsShootingAllowed())
        return;

    // Режим ножа
    if (m_bKnifeMode == true)
    {
        CShootingObject::FireStart();
        PlayAnimKnifeAttack();
        SetPending(TRUE);
        return;
    }

    if (!Try2Fire(true)) //--> Повторная проверка для МП, где вызов стэйтов идёт в обход Try-функций
    {
        Need2Idle();
        return;
    }

    // Обычная стрельба
    CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
    CInventoryItem*  ii = smart_cast<CInventoryItem*>(this);

    if (!io)
    {
        Need2Idle();
        return;
    }

#ifdef DEBUG
    if (ii != io->inventory().ActiveItem())
        Msg("! not an active item, item %s, owner %s, active item %s",
            *cName(),
            *H_Parent()->cName(),
            io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");

    if (!(io && (ii == io->inventory().ActiveItem())))
    {
        CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(H_Parent());
        if (stalker)
        {
            stalker->planner().show();
            stalker->planner().show_current_world_state();
            stalker->planner().show_target_world_state();
        }
    }
#endif // DEBUG

    m_bStopedAfterQueueFired = false;
    m_bFireSingleShot        = true;
    m_iShotNum               = 0;

    CShootingObject::FireStart();
}

// Переключение на другой стэйт из стэйта "Стрельба"
void CWeapon::switchFrom_Fire(u32 newS)
{
    if (IsWorking())
    {
        StopShotEffector();

        if (m_pFlameParticles && m_pFlameParticles->IsLooped())
            StopFlameParticles();

        CShootingObject::FireEnd();
    }

    m_iShotNum = 0;
    if (m_fInitialBaseBulletSpeed != 0.f)
        SetBulletSpeed(m_fInitialBaseBulletSpeed);
}

// Обновление оружия в состоянии "Стрельба"
void CWeapon::state_Fire(float dt)
{
    if (m_bKnifeMode)
        return;

    R_ASSERT(IsShootingAllowed());

    UpdShellShowTimer();

    if (iAmmoElapsed > 0 && m_bNeed2Pump == false)
    {
        VERIFY(fOneShotTime > 0.f);

        if (!H_Parent())
            return;

        if (smart_cast<CMPPlayersBag*>(H_Parent()) != NULL)
        {
            Msg("! WARNING: state_Fire of object [%d][%s] while parent is CMPPlayerBag...", ID(), cNameSect().c_str());
            return;
        }

        CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
        if (NULL == io->inventory().ActiveItem())
        {
            Log("current_state", GetState());
            Log("next_state", GetNextState());
            Log("item_sect", cNameSect().c_str());
            Log("H_Parent", H_Parent()->cNameSect().c_str());
            return; // Alundaio: This is not supposed to happen but it does. GSC was aware but why no return here? Known
                    // to cause crash on game load if NPC immediately enters combat.
        }

        if (m_bGrenadeMode || fShotTimeCounter < 0)
        {
            // Если владелец не может стрелять, то отменяем стрельбу
            CEntity* E = smart_cast<CEntity*>(H_Parent());
            if (!E->g_stateFire())
            {
                Need2Idle();
                return;
            }

            // Изначальная позиция выстрела
            Fvector p1, d;
            p1.set(get_LastFP());
            d.set(get_LastFD());

            // Обрабатываем позицию и направление выстрела через владельца оружия
            E->g_fireParams(this, p1, d);

            R_ASSERT(!m_magazine.empty());

            // Стреляем пока есть патроны и число выстрелов не превысило размер очереди
            while (!m_magazine.empty() && (m_bGrenadeMode || fShotTimeCounter < 0) && (IsWorking() || m_bFireSingleShot) &&
                   (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
            {
                m_bFireSingleShot = false;
                fShotTimeCounter += (m_bGrenadeMode ? 0.0f : fOneShotTime); //--> Регулируем скорострельность
                ++m_iShotNum;

                // Сперва пробуем стрельнуть патроном как гранатой\ракетой
                if (!SpawnAndLaunchRocket())
                { // Иначе стреляем обычной пулей
                    if (m_iShotNum == 1 && m_iBaseDispersionedBulletsCount > 0)
                    { //--> Для Абакана - делаем несколько выстрелов за один кадр
                        for (int i = 0; i < m_iBaseDispersionedBulletsCount; i++)
                        {
                            //--> Со второй пули начинаем повторно проверять возможность стрельбы
                            if (i > 0)
                            {
                                // С каждым "выстрелом" повторно проверяем что мы ещё можем стрелять
                                if (!m_magazine.empty() && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
                                    ++m_iShotNum;
                                else
                                    break;
                            }

                            FireTrace(p1, d);
                            OnShot(false, true);
                        }
                    }
                    else
                    { //--> Обычный одинарный выстрел
                        FireTrace(p1, d);
                        OnShot(false);
                    }
                }

                if (!m_bGrenadeMode)
                {
                    // После выстрела тестируем на осечку
                    bool bIsMisfife = CheckForMisfire();

                    // Передёргиваем помпу
                    if (m_bUsePumpMode)
                    {
                        m_bNeed2Pump = true;

                        if (!bIsMisfife)
                            Try2Pump();

                        break;
                    }
                }
            }

            if (!m_bStopedAfterQueueFired && m_iShotNum == m_iQueueSize)
            {
                m_bStopedAfterQueueFired = true;
            }
        }
    }

    // Таймер скорострельности
    if (fShotTimeCounter < 0)
    {
        // Если опустился ниже 0, то мы отстрелялись
        if (iAmmoElapsed == 0)
            Need2Empty();
        else
            Need2Idle();
    }
    else
    {
        fShotTimeCounter -= dt;
    }
}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

float _nrand(float sigma)
{
#define ONE_OVER_SIGMA_EXP (1.0f / 0.7975f)

    if (sigma == 0)
        return 0;

    float y;
    do
    {
        y = -logf(Random.randF());
    } while (Random.randF() > expf(-_sqr(y - 1.0f) * 0.5f));
    if (rand() & 0x1)
        return y * sigma * ONE_OVER_SIGMA_EXP;
    else
        return -y * sigma * ONE_OVER_SIGMA_EXP;
}

void random_dir(Fvector& tgt_dir, const Fvector& src_dir, float dispersion)
{
    float   sigma = dispersion / 3.f;
    float   alpha = clampr(_nrand(sigma), -dispersion, dispersion);
    float   theta = Random.randF(0, PI);
    float   r     = tan(alpha);
    Fvector U, V, T;
    Fvector::generate_orthonormal_basis(src_dir, U, V);
    U.mul(r * _sin(theta));
    V.mul(r * _cos(theta));
    T.add(U, V);
    tgt_dir.add(src_dir, T).normalize();
}

// Переопределяем метод из CShootingObject (Начать стрельбу)
void CWeapon::FireStart() { Try2Fire(); }

// Переопределяем метод из CShootingObject (Остановить стрельбу)
void CWeapon::FireEnd() { Need2Stop_Fire(); }

// Калбэк на попытку выстрела при пустом или заклинившем магазине
void CWeapon::OnEmptyClick(bool bFromMisfire)
{
    if (GetState() == eIdle || bFromMisfire)
        PlayAnimEmptyClick();

    if (!bFromMisfire)
        Try2AutoReload();
}

// Калбэк на выстрел
void CWeapon::OnShot(bool bIsRocket, bool bIsBaseDispersionedBullet)
{
    Fvector vel;
    PHGetLinearVell(vel);

    // Партиклы, звуки, анимации, эффекты
    if (bIsBaseDispersionedBullet == false || m_iShotNum == 1)
    {
        PlayAnimShoot();   // Анимация выстрела
        AddShotEffector(); // Эффекты экрана (отдача от выстрела)

        // Звук
        if (m_bGrenadeMode)
            PlaySound("sndShotG", get_LastFP2());
        else
            PlaySound(m_sSndShotCurrent.c_str(), get_LastFP());

        // Партиклы
        if (m_bGrenadeMode)
        {
            // Огонь из ствола
            StartFlameParticles2();
        }
        else
        {
            // Огонь из ствола
            StartFlameParticles();

            // Дым из ствола
            ForceUpdateFireParticles();
            StartSmokeParticles(get_LastFP(), vel);
        }
    }

    // Гильзы
    if (m_bGrenadeMode == false)
    {
        // 2D-Гильзы
        if (!m_bUsePumpMode)
            LaunchShell2D(&vel);

        // 3D-Гильзы
        if (ParentIsActor())
        {
            for (int _idx = 1; _idx <= GetLPCount(); _idx++)
                LaunchShell3D(_idx, (_idx == 1 ? m_sCurShell3DSect.c_str() : nullptr));
        }
    }

    // Эффект сдвига (отдача)
    AddHUDShootingEffect();
}

// Колбек на вылет ракеты
void CWeapon::OnRocketLaunch(u16 rocket_id) { OnShot(true); }

// Переключение на следующий режим стрельбы
void CWeapon::OnNextFireMode()
{
    if (!m_bHasDifferentFireModes)
        return;
    if (GetState() != eIdle)
        return;
    m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
};

// Переключение на предыдущий режим стрельбы
void CWeapon::OnPrevFireMode()
{
    if (!m_bHasDifferentFireModes)
        return;
    if (GetState() != eIdle)
        return;
    m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
    SetQueueSize(GetCurrentFireMode());
};

// Установить размер очереди
void CWeapon::SetQueueSize(int size) { m_iQueueSize = size; };

// Выстрелить первой гранатой из очереди (вызывается на апдейте)
void CWeapon::LaunchGrenade()
{
    if (getRocketCount() == 0)
        return;

    Fvector p1, d1, p;
    Fvector p2, d2, d;
    p1.set(get_LastFP());
    d1.set(get_LastFD());
    p = p1;
    d = d1;

    CEntity* E = smart_cast<CEntity*>(H_Parent());
    if (E)
    {
        E->g_fireParams(this, p2, d2);
        p = p2;
        d = d2;

        if (IsHudModeNow())
        {
            Fvector p0;
            float   dist = HUD().GetCurrentRayQuery().range;
            p0.mul(d2, dist);
            p0.add(p1);
            p = p1;
            d.sub(p0, p1);
            d.normalize_safe();
        }
    }

    Fmatrix launch_matrix;
    launch_matrix.identity();
    launch_matrix.k.set(d);
    Fvector::generate_orthonormal_basis(launch_matrix.k, launch_matrix.j, launch_matrix.i);
    launch_matrix.c.set(p);

    // В режиме прицеливания пускаем гранату ровно в перекрестие
    if (getCurrentRocket()->IsEnginePresent() == false)
    {
        if (IsGameTypeSingle() && IsZoomed() && smart_cast<CActor*>(H_Parent()))
        {
            H_Parent()->setEnabled(FALSE);
            setEnabled(FALSE);

            collide::rq_result RQ;
            BOOL               HasPick = Level().ObjectSpace.RayPick(p1, d, 300.0f, collide::rqtStatic, RQ, this);

            setEnabled(TRUE);
            H_Parent()->setEnabled(TRUE);

            if (HasPick)
            {
                Fvector Transference;
                Transference.mul(d, RQ.range);
                Fvector res[2];

                u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference, CRocketLauncher::m_fLaunchSpeed, EffectiveGravity(), res);
                if (canfire0 != 0)
                    d = res[0];
            }
        };
    }

    d.normalize();
    d.mul(m_fLaunchSpeed);

    CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

    CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
    VERIFY(pGrenade);
    pGrenade->SetInitiator(H_Parent()->ID());

    if (OnServer())
    {
        NET_Packet P;
        u_EventGen(P, GE_LAUNCH_ROCKET, ID());
        P.w_u16(u16(getCurrentRocket()->ID()));
        u_EventSend(P);
    }
}

// Произвести выстрел пулей текущем патроном в текущую дирекцию
void CWeapon::FireTrace(const Fvector& P, const Fvector& D)
{
    VERIFY(m_magazine.size());

    CCartridge* l_cartridge = m_magazine.back();

    VERIFY(u16(-1) != l_cartridge->bullet_material_idx);
    //-------------------------------------------------------------
    u8 m_old_u8TracerColorID = u8(-1);

    bool is_tracer = m_bHasTracers && !!l_cartridge->m_flags.test(CCartridge::cfTracer);
    if (is_tracer && !IsGameTypeSingle())
        is_tracer = is_tracer /*&& (m_magazine.size() % 3 == 0)*/ && !IsSilencerAttached();

    l_cartridge->m_flags.set(CCartridge::cfTracer, is_tracer);
    if (m_u8TracerColorID != u8(-1))
    {
        m_old_u8TracerColorID          = l_cartridge->param_s.u8ColorID;
        l_cartridge->param_s.u8ColorID = m_u8TracerColorID;
    }
    //-------------------------------------------------------------

    // Повысить изношенность оружия с учетом влияния конкретного патрона
    ChangeCondition(-GetWeaponDeterioration() * l_cartridge->param_s.impair);

    float   fire_disp = 0.f;
    CActor* tmp_actor = NULL;

    if (!IsGameTypeSingle())
    {
        tmp_actor = smart_cast<CActor*>(Level().CurrentControlEntity());
        if (tmp_actor)
        {
            CEntity::SEntityState state;
            tmp_actor->g_State(state);
            if (m_first_bullet_controller.is_bullet_first(state.fVelocity))
            {
                fire_disp = m_first_bullet_controller.get_fire_dispertion();
                m_first_bullet_controller.make_shot();
            }
        }
        game_cl_mp* tmp_mp_game = smart_cast<game_cl_mp*>(&Game());
        VERIFY(tmp_mp_game);
        if (tmp_mp_game->get_reward_generator())
            tmp_mp_game->get_reward_generator()->OnWeapon_Fire(H_Parent()->ID(), ID());
    }

    if (fsimilar(fire_disp, 0.f))
    {
        if (H_Parent() && (H_Parent() == tmp_actor))
        {
            fire_disp = tmp_actor->GetFireDispertion();
        }
        else
        {
            fire_disp = GetFireDispersion(true);
        }
    }

    bool SendHit = SendHitAllowed(H_Parent()); //--> Требуется-ли регистрировать попадание по сети

    // Выстерлить пулю (с учетом возможной стрельбы дробью)
    UpdateLastBulletInfo(l_cartridge);

    for (int i = 0; i < l_cartridge->param_s.buckShot; ++i)
    {
        FireBullet(P, D, fire_disp, *l_cartridge, H_Parent()->ID(), ID(), SendHit);
    }

    if (m_old_u8TracerColorID != u8(-1))
        l_cartridge->param_s.u8ColorID = m_old_u8TracerColorID;

    StartShotParticles();

    if (m_bLightShotEnabled)
        Light_Start();

    // Убираем патрон
    m_magazine.pop_back();
    --iAmmoElapsed;

    VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

// Выстрел пулей с заданными параметрами
void CWeapon::FireBullet(
    const Fvector& pos, const Fvector& shot_dir, float fire_disp, const CCartridge& cartridge, u16 parent_id, u16 weapon_id, bool send_hit)
{
    // Для Абакана - пускаем первые пули с немного другой скоростью, чтобы они не сливались вместе во время полёта
    if (m_iBaseDispersionedBulletsCount)
    {
        if (m_iShotNum <= m_iBaseDispersionedBulletsCount)
        { // "Сверхточные пули"
            if (m_iShotNum == 1)
            { //--> При первом выстреле запоминаем текущую скорость
                m_fInitialBaseBulletSpeed = GetBulletSpeed();
                SetBulletSpeed(m_fBaseDispersionedBulletsSpeed);
            }
            else
            { //--> Линейно интерполируем скорость пули с каждым следующим выстрелом
                float fSpeedStep = (m_fBaseDispersionedBulletsSpeed - m_fInitialBaseBulletSpeed) / (float)m_iBaseDispersionedBulletsCount;
                SetBulletSpeed(GetBulletSpeed() - fSpeedStep);
            }
        }
        else
        { // Обычные пули
            SetBulletSpeed(m_fInitialBaseBulletSpeed);
        }
    }
    CShootingObject::FireBullet(pos, shot_dir, fire_disp, cartridge, parent_id, weapon_id, send_hit);
}

// Выпустить 2D-гильзу
void CWeapon::LaunchShell2D(Fvector* pVel)
{
    Fvector vel;
    if (pVel != NULL)
        vel = *pVel;
    else
        PHGetLinearVell(vel);

    OnShellDrop(get_LastSP(), vel);
}

// Можно-ли в данный момент запускать анимированные 3D-гильзы
bool CWeapon::CanPlay3DShellAnim() const
{
    if (CShellLauncher::CanPlay3DShellAnim() == false)
        return false;

    switch (GetState())
    {
        case eIdle:
        case eFire:
        case eFire2:
        case eMisfire:
        case eMagEmpty:
        case ePump:
        {
            return true;
        }
    }

    return false; //--> Гильза будет выкинута сразу
}

// Можно-ли стрелять из оружия
bool CWeapon::IsShootingAllowed() const
{
    if (m_bDisableFire)
        return false;

    if (m_bDisableFireWhileZooming && (GetZRotatingFactor() > 0.0f && IsRotatingToZoom()))
        return false;

    return true;
}

// Обновить информацию о последнем выстрелянном патроне
void CWeapon::UpdateLastBulletInfo(CCartridge* pBullet)
{
    m_sCurAnimatedShellHudVisual = pBullet->m_sShellHudVisual;
    m_sCurShell3DSect = pBullet->m_sShell3DSect;
}
