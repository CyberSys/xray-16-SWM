#pragma once

/**********************************/
/***** Присоединяемые визуалы *****/ //--#SM+#--
/**********************************/
// SM_TODO:M ^^^

class CCustomShell;
class CGameObject;

class CShellLauncher
{
public:
    struct _launch_point
    {
    private:
        friend CShellLauncher;

    public:
        shared_str sBoneName;    // Кость, относительно которой спавним гильзу
        bool       bUseBoneDir;  // Использовать дирекцию кости при спавне гильзы
        Fvector    vOfsPos;      // Смещение начальной позиции гильзы относительно кости
        Fvector    vOfsDir;      // Смещение начального поворота гильзы относительно кости (в градусах)
        Fvector    vOfsDirRnd;   // Лимиты случайного отклонения начального поворота (в градусах)
        Fvector    vVelocity;    // Вектор полёта после спавна относительно кости
        Fvector    vVelocityRnd; // Лимиты случайного отклонения начального вектора полёта
                                 /*===================*/
    private:
        bool    bEnabled;      // Флаг активности для данной точки
        u16     iBoneID;       // ID кости, относительно которой спавним гильзу
        Fmatrix vLaunchMatrix; // Матрица трансформации при спавне, содержимт текущий поворот и позицию
        Fvector vLaunchVel;    // Текущий вектор полёта при спавне
    public:
        ICF bool IsEnabled() { return bEnabled; }
        ICF u16   GetBoneID() { return iBoneID; }
        ICF const Fmatrix& GetLaunchMatrix() { return vLaunchMatrix; }
        ICF const Fvector& GetLaunchVel() { return vLaunchVel; }

        _launch_point();
        _launch_point(const shared_str& sect_data, u32 _idx);
    };

    struct launch_points
    {
        shared_str sShellOverSect;     // Переопределённая секция гильзы (NULL - возмёт секцию из LaunchShell)
        u32        dwFOVTranslateTime; // Время (в м\сек) за которое FOV гильзы переходит из худового в мировой
        u32        dwFOVStableTime;    // Время (в м\сек) в течении которого FOV гильзы ещё не меняется

        _launch_point point_world; // Параметры гильзы для мировой модели
        _launch_point point_hud;   // Параметры гильзы для худовой модели

        launch_points(const shared_str& sWorldSect, const shared_str& sHudSect, u32 _idx);
    };

    CShellLauncher(CGameObject* parent);
    ~CShellLauncher();

    DEFINE_VECTOR(launch_points, LAUNCH_VECTOR, LAUNCH_VECTOR_IT);
    LAUNCH_VECTOR m_launch_points; // Содержит все доступные точки запуска гильзы (для оружия с несколькими гильзами, например БМ-16)

    void LaunchShell(
        u32 launch_point_idx, LPCSTR sOverriddenShellSect = NULL); // Запустить гильзу из указанного launch_point (с возможностью указать свою секцию)
    u32 GetLPCount() { return m_launch_points_count; }             // Узнать число точек запуска
protected:
    void ReLoadShellData(const shared_str& sWorldSect, const shared_str& sHudSect = NULL); // ПереЗагрузить параметры гильз
    void RegisterShell(u16 shell_id, CGameObject* parent_shell_launcher);                  // Зарегестрировать заспавненную гильзу

    void RebuildLaunchParams(const Fmatrix& mTransform, IKinematics* pModel, bool bIsHud); // Обновить параметры для точек вылета

private:
    shared_str   m_params_sect;           // Секция с параметрами
    shared_str   m_params_hud_sect;       // Худовая секция с параметрами (если есть)
    CGameObject* m_parent_shell_launcher; // Владелец данного CShellLauncher
    u32          m_launch_points_count;   // Число точек запуска
};