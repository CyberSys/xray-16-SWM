//////////////////////////////////////////////////////////////////////
// ShellLauncher.cpp:	интерфейс для семейства объектов
//						стреляющих гранатами и ракетами
//////////////////////////////////////////////////////////////////////

/* SM_TODO:M
// ЗНАК SM + ПОМЕНЯТЬ ОПИСАНИЕ !!!!  //--#SM_TODO+#--
// extract_type
// time_based, anim_bases
// on_shot, on_reload, on_pump
// shell section
// Убедиться что гильза спавнится локально, и если это так, то значит в оружии гранаты спавнятся только у клиента ?
// bUseBoneDir реализовать
// Учитывать вектор худа (превращать в ускорение гильзы)
*/

#include "stdafx.h"
#include "ShellLauncher.h"
#include "CustomShell.h"
#include "xrserver_objects_alife_items.h"
#include "Level.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrEngine/IGame_Persistent.h"
#include "Common/object_broker.h"
#include "Include/xrRender/Kinematics.h"
#include "weapon.h"

CShellLauncher::CShellLauncher(CGameObject* parent)
{
    m_params_sect           = NULL; // Не инициализировать здесь <!>
    m_params_hud_sect       = NULL;
    m_parent_shell_launcher = parent;

    m_launch_points_count = 0;
}

CShellLauncher::~CShellLauncher() { delete_data(m_launch_points); }

CShellLauncher::_launch_point::_launch_point()
{
    sBoneName   = NULL;
    bUseBoneDir = false;

    vOfsPos.set(0, 0, 0);
    vOfsDir.set(0, 0, 0);
    vOfsDirRnd.set(0, 0, 0);
    vVelocity.set(0, 0, 0);
    vVelocityRnd.set(0, 0, 0);

    vLaunchVel.set(0, 0, 0);
    vLaunchMatrix.identity();

    iBoneID  = BI_NONE;
    bEnabled = false;
}

CShellLauncher::_launch_point::_launch_point(const shared_str& sect_data, u32 _idx) : _launch_point()
{
    string64 sLine;

    xr_sprintf(sLine, "shells_3d_bone_name_%d", _idx);
    sBoneName = READ_IF_EXISTS(pSettings, r_string, sect_data, sLine, "wpn_body");

    xr_sprintf(sLine, "shells_3d_use_bone_dir_%d", _idx);
    bUseBoneDir = READ_IF_EXISTS(pSettings, r_bool, sect_data, sLine, false);

    xr_sprintf(sLine, "shells_3d_pos_%d", _idx);
    vOfsPos = READ_IF_EXISTS(pSettings, r_fvector3, sect_data, sLine, vOfsPos);

    xr_sprintf(sLine, "shells_3d_dir_%d", _idx);
    vOfsDir = READ_IF_EXISTS(pSettings, r_fvector3, sect_data, sLine, vOfsDir);

    xr_sprintf(sLine, "shells_3d_dir_disp_%d", _idx);
    vOfsDirRnd = READ_IF_EXISTS(pSettings, r_fvector3, sect_data, sLine, vOfsDirRnd);

    xr_sprintf(sLine, "shells_3d_vel_%d", _idx);
    vVelocity = READ_IF_EXISTS(pSettings, r_fvector3, sect_data, sLine, vVelocity);

    xr_sprintf(sLine, "shells_3d_vel_disp_%d", _idx);
    vVelocityRnd = READ_IF_EXISTS(pSettings, r_fvector3, sect_data, sLine, vVelocityRnd);

    bEnabled = true;
}

CShellLauncher::launch_points::launch_points(const shared_str& sWorldSect, const shared_str& sHudSect, u32 _idx)
{
    R_ASSERT(pSettings != NULL);

    // Грузим переопределённую секцию гильз (если есть)
    string64 sLine;
    xr_sprintf(sLine, "shells_3d_overridden_sect_%d", _idx);
    sShellOverSect = READ_IF_EXISTS(pSettings, r_string, sWorldSect, sLine, "none");
    if (sShellOverSect == "none")
        sShellOverSect = NULL;

    // Параметры худового FOV
    xr_sprintf(sLine, "shells_3d_fov_transl_time_%d", _idx);
    dwFOVTranslateTime = READ_IF_EXISTS(pSettings, r_u32, sWorldSect, sLine, 0);

    xr_sprintf(sLine, "shells_3d_fov_stable_time_%d", _idx);
    dwFOVStableTime = READ_IF_EXISTS(pSettings, r_u32, sWorldSect, sLine, 0);

    // Грузим точки запуска для мировой и худовой модели
    point_world = _launch_point(sWorldSect, _idx);
    if (sHudSect != NULL)
        point_hud = _launch_point(sHudSect, _idx);
}

// ПереЗагрузить параметры гильз
void CShellLauncher::ReLoadShellData(const shared_str& sWorldSect, const shared_str& sHudSect)
{
    // Считываем число точек для гильз
    m_launch_points_count = READ_IF_EXISTS(pSettings, r_u32, sWorldSect, "shells_3d_lp_count", 0);
    clamp(m_launch_points_count, u32(0), u32(-1));

    // Очищаем прошлые данные
    delete_data(m_launch_points);

    // Грузим новые для мировой и худовой модели (если есть)
    for (int _idx = 1; _idx <= m_launch_points_count; _idx++)
        m_launch_points.push_back(launch_points(sWorldSect, sHudSect, _idx));

    // Запоминаем новые секции
    m_params_sect     = sWorldSect;
    m_params_hud_sect = sHudSect;
}

// Запустить одну партию гильз
void CShellLauncher::LaunchShell(u32 launch_point_idx, LPCSTR sOverriddenShellSect)
{
    // Индекс точки запуска должен быть задан верно
    R_ASSERT(launch_point_idx > 0 && launch_point_idx <= m_launch_points_count);

    // У нас должен быть родитель и загруженные данные
    R_ASSERT(m_parent_shell_launcher != NULL);
    R_ASSERT(m_params_sect != NULL);

    // Получаем точки запуска с данным индексом
    launch_points& lp = m_launch_points[launch_point_idx - 1];

    // Достаём из них секцию гильзы
    R_ASSERT(lp.sShellOverSect != NULL || sOverriddenShellSect != NULL);
    LPCSTR shell_sect = (lp.sShellOverSect != NULL ? lp.sShellOverSect.c_str() : sOverriddenShellSect);

    // Спавним гильзу
    CSE_Abstract* pSO = Level().spawn_item(shell_sect, m_parent_shell_launcher->Position(), u32(-1), m_parent_shell_launcher->ID(), true);

    CSE_Temporary* l_tpTemporary = smart_cast<CSE_Temporary*>(pSO);
    R_ASSERT(l_tpTemporary);

    // PS: Значит custom_data (m_ini_string) мы определяем в CSE_Abstract, зато её сохранение реализовываем лишь в CSE_AlifeObject ._.
    // => мы не можем передать данные через spawn_ini() => играем грязно
    // client_data использовать в теории можно, но разработчиками это не предусмотренно
    // добавлять новое поле в CSE_Temporary ради одних гильз - расточительно

    // pSO->spawn_ini().w_u32("shell_data", "lp_idx", launch_point_idx);

    string64 sLP_idx;
    xr_sprintf(sLP_idx, "%d", launch_point_idx);
    pSO->set_name_replace(sLP_idx);

    NET_Packet P;
    pSO->Spawn_Write(P, TRUE);
    Level().Send(P, net_flags(TRUE));
    F_entity_Destroy(pSO);
}

// Зарегестрировать заспавненную гильзу
void CShellLauncher::RegisterShell(u16 shell_id, CGameObject* parent_shell_launcher)
{
    // Регистратор гильзы должен быть нашим владельцем
    R_ASSERT(parent_shell_launcher == m_parent_shell_launcher);

    // Если это не гильза, то не регистрируем её
    CCustomShell* pShell = smart_cast<CCustomShell*>(Level().Objects.net_Find(shell_id));
    if (!pShell)
        return;

    // Выставляем у гильзы владельца - нас
    pShell->H_SetParent(m_parent_shell_launcher);
}

// Обновить параметры для точек вылета
void CShellLauncher::RebuildLaunchParams(const Fmatrix& mTransform, IKinematics* pModel, bool bIsHud)
{
    // У нас должен быть родитель, загруженные данные и визуал
    R_ASSERT(m_parent_shell_launcher != NULL);
    R_ASSERT(m_params_sect != NULL);
    R_ASSERT(pModel != NULL);

    // Если точек запуска гильз нет, то и считать нечего
    if (m_launch_points_count == 0)
        return;

    // Если у нас сменилась худовая секция, то требуется перезагрузить данные
    CWeapon* pWpn = m_parent_shell_launcher->cast_weapon();
    if (pWpn != NULL && pWpn->GetHUDmode() == true)
        if (pWpn->HudSection() != m_params_hud_sect)
            ReLoadShellData(m_params_sect, pWpn->HudSection());

    // Обновляем все точки запуска
    R_ASSERT(m_launch_points.size() == m_launch_points_count);

    for (int i = 0; i < m_launch_points.size(); i++)
    {
        _launch_point& point = (bIsHud ? m_launch_points[i].point_hud : m_launch_points[i].point_world);

        // Если точка не активна - не обновляем её
        if (point.bEnabled == false)
            continue;

        // Если ещё нету BoneID, то находим его
        if (point.iBoneID == BI_NONE)
            point.iBoneID = pModel->LL_BoneID(point.sBoneName);

        R_ASSERT2(point.iBoneID != BI_NONE,
            make_string("Model from [%s] has no bone [%s]", (bIsHud ? m_params_hud_sect.c_str() : m_params_sect.c_str()), point.sBoneName.c_str())
                .c_str());

        // Матрица трансформации кости
        Fmatrix& mBoneTransform = pModel->LL_GetTransform(point.iBoneID);

        // Вектор поворота
        Fvector vRotate = Fvector(point.vOfsDir);
        vRotate.x += Random.randFs(point.vOfsDirRnd.x); // +\-
        vRotate.y += Random.randFs(point.vOfsDirRnd.y);
        vRotate.z += Random.randFs(point.vOfsDirRnd.z);
        vRotate.mul(PI / 180.f); // Переводим углы в радианы

        // Строим матрицу поворота
        point.vLaunchMatrix.setXYZ(VPUSH(vRotate));
        point.vLaunchMatrix.c.set(point.vOfsPos);

        // Применяем её к матрице кости и самого предмета
        point.vLaunchMatrix.mulA_43(mBoneTransform);
        point.vLaunchMatrix.mulA_43(mTransform);

        // Вектор полёта (не учитывает кость)
        point.vLaunchVel.set(point.vVelocity);
        point.vLaunchVel.x += Random.randFs(point.vVelocityRnd.x); // +\-
        point.vLaunchVel.y += Random.randFs(point.vVelocityRnd.y);
        point.vLaunchVel.z += Random.randFs(point.vVelocityRnd.z);
        mTransform.transform_dir(point.vLaunchVel);
    }
}