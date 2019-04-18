/*********************************************************/
/** Вспомогательный класс для теста коллизии примитивов **/ //--#SM+#--
/*********************************************************/

#include "StdAfx.h"
#include "PHCollideHelper.h"
#include "xrPhysics/Geometry.h"
#include "debug_renderer.h"

#ifdef DEBUG
//#define DEBUG_DRAW_BBOX_COLLIDE_DATA //--> Отрисовывать BBox при использовании DoBBoxCollide() - R1/R2 only
#endif

CPHCollideHelper::CPHCollideHelper(IPhysicsShellHolder* pOwnerObj, ObjectContactCallbackFun* fnContactClbk)
{
    R_ASSERT(pOwnerObj != nullptr);
    R_ASSERT(fnContactClbk != nullptr);

    m_bIgnoreStatic = false;
    m_bIgnoreDynamic = false;

    // Запоминаем родительский объект и калбэк
    m_pOwnerObj = pOwnerObj;
    m_fnContactClbk = fnContactClbk;

    // Создаём физическую оболочку
    m_pPHBody = P_create_Shell();
    R_ASSERT(m_pPHBody);

    m_pPHBody->DisableCollision();
}

CPHCollideHelper::~CPHCollideHelper() { DestroyPHShell(); }

// Корректное удаление физической оболочки коллайдера
void CPHCollideHelper::DestroyPHShell()
{
    if (m_pPHBody != nullptr)
        destroy_physics_shell(m_pPHBody);

    m_pPHBody = nullptr;
}

// Должна-ли физ. оболочка игнорировать статику
void CPHCollideHelper::SetIgnoreStatic(bool bVal)
{
    R_ASSERT(m_pPHBody != nullptr);

    if (bVal)
        m_pPHBody->SetIgnoreStatic();
    else
        m_pPHBody->SetCollideStatic();
}

// Должна-ли физ. оболочка игнорировать динамику
void CPHCollideHelper::SetIgnoreDynamic(bool bVal)
{
    R_ASSERT(m_pPHBody != nullptr);

    if (bVal)
        m_pPHBody->SetIgnoreDynamic();
    else
        m_pPHBody->SetCollideDynamic();
}

// Корректная регистрация нового элемента у физ. оболочки
void CPHCollideHelper::RegisterElement(CPhysicsElement* pPHElem)
{
    m_pPHBody->add_Element(pPHElem);

    m_pPHBody->set_PhysicsRefObject(m_pOwnerObj);
    m_pPHBody->set_ApplyByGravity(false);
    m_pPHBody->setMass(0.1f);
    m_pPHBody->setDensity(0.1f);
    m_pPHBody->SetAirResistance(0.f, 0.f);
    m_pPHBody->set_ObjectContactCallback(m_fnContactClbk);
    m_pPHBody->set_ContactCallback(0);
}

// Получить XFORM элемента в мировых координатах (с учётом всех трансформаций родителя)
void CPHCollideHelper::GetElementWorldXFORM(u16 iElemIdx, Fmatrix& pMOut)
{
    u16 iTotalElements = m_pPHBody->get_ElementsNumber();
    R_ASSERT2(iElemIdx < iTotalElements,
        make_string("iElemIdx [%d] is less then iTotalElements [%d]", iElemIdx, iTotalElements).c_str());

    m_pPHBody->get_ElementByStoreOrder(iElemIdx)->geometry(0)->get_xform(pMOut);
}

#ifdef DEBUG
// Собрать отладочную инфу для отрисовки
void CPHCollideHelper::DBG_CollectData()
{
    m_dbg_DrawData.clear();

    u16 iTotalElements = m_pPHBody->get_ElementsNumber();
    for (u8 iElemIdx = 0; iElemIdx < iTotalElements; iElemIdx++)
    {
        CODEGeom* pGeom = m_pPHBody->get_ElementByStoreOrder(iElemIdx)->geometry(0);
        if (pGeom != nullptr)
        {
            Fmatrix mXFORM;
            Fvector vSize;
            pGeom->get_Box(mXFORM, vSize);
            m_dbg_DrawData.push_back(std::make_pair(mXFORM, vSize.mul(0.5f)));
        }
    }
}

// Отрисовать боксы вокруг физических оболочек, используя ранее собранные данные
void CPHCollideHelper::DBG_DrawData(u32 iColor)
{
    // R1-R2 only ...
    CDebugRenderer& render = Level().debug_renderer();

    for (u8 i = 0; i < m_dbg_DrawData.size(); i++)
    {
        render.draw_obb(m_dbg_DrawData[i].first, m_dbg_DrawData[i].second, iColor);
    }
}
#endif

// Добавить параллелепипед\куб к физ. оболочке
CPhysicsElement* CPHCollideHelper::AddBox(const Fmatrix& mLocalXFORM, const Fvector& vHalfsize)
{
    R_ASSERT(m_pPHBody != nullptr);

    CPhysicsElement* pPHElem = P_create_Element();
    R_ASSERT(pPHElem);

    Fobb Box;

    Box.m_rotate.i = mLocalXFORM.i;
    Box.m_rotate.j = mLocalXFORM.j;
    Box.m_rotate.k = mLocalXFORM.k;

    Box.m_translate.set(mLocalXFORM.c);

    Box.m_halfsize.set(vHalfsize.x, vHalfsize.y, vHalfsize.z);

    pPHElem->add_Box(Box);

    RegisterElement(pPHElem);

    return pPHElem;
}

// Добавить параллелепипед\куб к физ. оболочке (только сдвиг позиции)
CPhysicsElement* CPHCollideHelper::AddBox(const Fvector& vLocalCenter, const Fvector& vHalfsize)
{
    Fmatrix mBox;
    mBox.identity();
    mBox.c.set(vLocalCenter);

    return AddBox(mBox, vHalfsize);
}

// Добавить параллелепипед\куб к физ. оболочке (без смещения)
CPhysicsElement* CPHCollideHelper::AddBox(const Fvector& vHalfsize)
{
    Fmatrix mBox;
    mBox.identity();
    mBox.c.set(0.0f, 0.0f, 0.0f);

    return AddBox(mBox, vHalfsize);
}

// Добавить цилиндр к физ. оболочке
CPhysicsElement* CPHCollideHelper::AddCylinder(
    const Fvector& vLocalCenter, const Fvector& vLocalDir, float fHeight, float fRadius)
{
    R_ASSERT(m_pPHBody != nullptr);

    CPhysicsElement* pPHElem = P_create_Element();
    R_ASSERT(pPHElem);

    Fcylinder Cyl;
    Cyl.m_center = vLocalCenter;
    Cyl.m_direction = vLocalDir;
    Cyl.m_height = fHeight;
    Cyl.m_radius = fRadius;

    pPHElem->add_Cylinder(Cyl);

    RegisterElement(pPHElem);

    return pPHElem;
}

// Добавить цилиндр к физ. оболочке (без смещения)
CPhysicsElement* CPHCollideHelper::AddCylinder(const Fvector& vLocalDir, float fHeight, float fRadius)
{
    return AddCylinder({0.0f, 0.0f, 0.0f}, vLocalDir, fHeight, fRadius);
}

// Добавить сферу к физ. оболочке
CPhysicsElement* CPHCollideHelper::AddSphere(const Fvector& vLocalCenter, float fRadius)
{
    R_ASSERT(m_pPHBody != nullptr);

    CPhysicsElement* pPHElem = P_create_Element();
    R_ASSERT(pPHElem);

    Fsphere Sphere;
    Sphere.P = vLocalCenter;
    Sphere.R = fRadius;

    pPHElem->add_Sphere(Sphere);

    RegisterElement(pPHElem);

    return pPHElem;
}

// Добавить сферу к физ. оболочке (без смещения)
CPhysicsElement* CPHCollideHelper::AddSphere(float fRadius) { return AddSphere({0.0f, 0.0f, 0.0f}, fRadius); }

/* Протестировать физ. оболочку на коллизию с объектами \ геометрией
   Коллизия может не всегда срабатывать, если центр оболочки "провален" внутрь геометрии

   XFORM - матрица трансформации, содержит поворот и мировые координаты размещения оболочки

    m_bipods.m_pPHBipodsLegR->DoCollideAt(mPHBody, &(OnBeforeCollide)([&](CPHCollideHelper* pPHHelper) {
        // Доступна окружающая область видимости
        pPHHelper->GetElementWorldXFORM(m_bipods.dbg_mPHLeg2);
    }));
*/
void CPHCollideHelper::DoCollideAt(Fmatrix mXFORM, OnBeforeCollide* pFnClbk)
{
    R_ASSERT2(IsReady(), "Can't test collide, add elements first!");

    Fvector vZero = {0.0f, 0.0f, 0.0f};

    mXFORM.c.y += 0.001f; //--> Хак чтобы слегка сдвинуть шейп, без него коллизия иногда не срабатывает

    m_pPHBody->Activate(mXFORM, vZero, vZero);
    m_pPHBody->EnableCollision();

    if (pFnClbk != nullptr)
    {
        (*pFnClbk)(this);
    }

#ifdef DEBUG
    DBG_CollectData();
#endif

    m_pPHBody->CollideAll();

    m_pPHBody->DisableCollision();
    m_pPHBody->Deactivate();
    m_pPHBody->Disable();
}

// Протестировать физ. оболочку на коллизию с объектами \ геометрией
// Размещает оболочку в указанной позиции с дефолтным поворотом
void CPHCollideHelper::DoCollideAt(const Fvector& vPos, OnBeforeCollide* pFnClbk)
{
    Fmatrix mXFORM;
    mXFORM.translate(vPos);

    DoCollideAt(mXFORM, pFnClbk);
}

// Протестировать грани bounding-box (прямоугольник) всех элементов на коллизию с объектами \ геометрией
// Тяжёлая функция. В отличии от DoCollideAt - сработает когда центр физ. оболочки "провален" внутрь
// геометрии, при условии что грани bbox её зацепят. Лучше всего использовать совместно с DoCollideAt.
// testMode - влияет на кол-во лучей и достоверность проверки
// pFnCollideClbk - вызывается при коллизии с любым объектом, если вернёт false - трассировка луча остановится 
// pFnTestClbk - вызывается при коллизии с дин. объектом, если вернёт false - объект будет проигнорирован
void CPHCollideHelper::DoBBoxCollide(Fmatrix mXFORM, EBBoxTestMode testMode, collide::rq_callback* pFnCollideClbk,
    collide::test_callback* pFnTestClbk, OnBeforeCollide* pFnClbk)
{
    Fvector vZero = {0.0f, 0.0f, 0.0f};

    m_pPHBody->Activate(mXFORM, vZero, vZero);
    m_pPHBody->EnableCollision();

    if (pFnClbk != nullptr)
    {
        (*pFnClbk)(this);
    }

#ifdef DEBUG
    DBG_CollectData();
#endif

#ifdef DEBUG_DRAW_BBOX_COLLIDE_DATA
    CDebugRenderer& render = Level().debug_renderer();
#endif

    // Перебираем все элементы
    u16 iTotalElements = m_pPHBody->get_ElementsNumber();
    for (u8 iElemIdx = 0; iElemIdx < iTotalElements; iElemIdx++)
    {
        CODEGeom* pGeom = m_pPHBody->get_ElementByStoreOrder(iElemIdx)->geometry(0);
        if (pGeom != nullptr)
        {
            // Получаем XFORM и Size элемента (world)
            Fmatrix mElXFORM;
            Fvector vElSize;
            pGeom->get_Box(mElXFORM, vElSize);

            // Пускаем луч по всем граням коробки + в центр если нужно
            Fmatrix mScaledXFORM, mScaleTransform;
            mScaleTransform.scale({vElSize.x * 0.5f, vElSize.y * 0.5f, vElSize.z * 0.5f});
            mScaledXFORM.mul_43(mElXFORM, mScaleTransform);

#ifdef DEBUG_DRAW_BBOX_COLLIDE_DATA
            //--> Рисуем центр BBox-а
            render.draw_aabb(mScaledXFORM.c, 0.025, 0.025, 0.025, color_xrgb(125, 125, 0));
#endif

            int iIdxFrom = 0;
            int iIdxTo = 0;
            Fvector aabb[14];

            // Рассчитываем грани BBox
            if (testMode == eAll || testMode == eEdgesOnly)
            { // 0 - 7 / iIdx 0 - 12
                mScaledXFORM.transform_tiny(aabb[0], Fvector().set(-1, -1, -1)); // 0
                mScaledXFORM.transform_tiny(aabb[1], Fvector().set(-1, +1, -1)); // 1
                mScaledXFORM.transform_tiny(aabb[2], Fvector().set(+1, +1, -1)); // 2
                mScaledXFORM.transform_tiny(aabb[3], Fvector().set(+1, -1, -1)); // 3
                mScaledXFORM.transform_tiny(aabb[4], Fvector().set(-1, -1, +1)); // 4
                mScaledXFORM.transform_tiny(aabb[5], Fvector().set(-1, +1, +1)); // 5
                mScaledXFORM.transform_tiny(aabb[6], Fvector().set(+1, +1, +1)); // 6
                mScaledXFORM.transform_tiny(aabb[7], Fvector().set(+1, -1, +1)); // 7

                iIdxFrom = 1;
                iIdxTo = 12;
            }

            // Рассчитываем линии через центр BBox-а (перекрестие)
            if (testMode == eAll || testMode == eCentreOnly)
            { // 8 - 13 / iIdx 13 - 15
                mScaledXFORM.transform_tiny(aabb[8], Fvector().set(+0, +0, -1)); // 8 Fwd Из
                mScaledXFORM.transform_tiny(aabb[9], Fvector().set(+0, +0, +1)); // 9 Fwd В
                mScaledXFORM.transform_tiny(aabb[10], Fvector().set(+0, -1, +0)); // 10 Up Из
                mScaledXFORM.transform_tiny(aabb[11], Fvector().set(+0, +1, +0)); // 11 Up В
                mScaledXFORM.transform_tiny(aabb[12], Fvector().set(-1, +0, +0)); // 12 Right Из
                mScaledXFORM.transform_tiny(aabb[13], Fvector().set(+1, +0, +0)); // 13 Right В

                iIdxFrom = 13;
                iIdxTo = 15;
            }

            // Тестируем линию через центр BBox-а (вертикальный луч через центр)
            if (testMode == eCentreUpOnly)
            { // iIdx 14
                iIdxFrom = 14;
                iIdxTo = 14;
            }

            // Тестируем все линии
            if (testMode == eAll)
            {
                iIdxFrom = 1;
                iIdxTo = 15;
            }

            // Производим RayCast всех линий, с целью найти пересечения с геометрией
            for (int iIdx = iIdxFrom; iIdx <= iIdxTo; iIdx++)
            {
                Fvector vRayPos, vRayDir;
                int iDbgColor;

                // Перебираем все возможные пути
                switch (iIdx)
                {
                case 1:
                {
                    vRayPos = aabb[0];
                    vRayDir = (aabb[1] - aabb[0]);
                    iDbgColor = color_xrgb(255, 255, 255);
                    break;
                }
                case 2:
                {
                    vRayPos = aabb[1];
                    vRayDir = (aabb[2] - aabb[1]);
                    iDbgColor = color_xrgb(255, 255, 180);
                    break;
                }
                case 3:
                {
                    vRayPos = aabb[2];
                    vRayDir = (aabb[3] - aabb[2]);
                    iDbgColor = color_xrgb(255, 255, 90);
                    break;
                }
                case 4:
                {
                    vRayPos = aabb[3];
                    vRayDir = (aabb[0] - aabb[3]);
                    iDbgColor = color_xrgb(255, 255, 0);
                    break;
                }
                case 5:
                {
                    vRayPos = aabb[4];
                    vRayDir = (aabb[5] - aabb[4]);
                    iDbgColor = color_xrgb(255, 180, 255);
                    break;
                }
                case 6:
                {
                    vRayPos = aabb[5];
                    vRayDir = (aabb[6] - aabb[5]);
                    iDbgColor = color_xrgb(255, 90, 255);
                    break;
                }
                case 7:
                {
                    vRayPos = aabb[6];
                    vRayDir = (aabb[7] - aabb[6]);
                    iDbgColor = color_xrgb(255, 0, 255);
                    break;
                }
                case 8:
                {
                    vRayPos = aabb[7];
                    vRayDir = (aabb[4] - aabb[7]);
                    iDbgColor = color_xrgb(180, 255, 255);
                    break;
                }
                case 9:
                {
                    vRayPos = aabb[0];
                    vRayDir = (aabb[4] - aabb[0]);
                    iDbgColor = color_xrgb(90, 255, 255);
                    break;
                }
                case 10:
                {
                    vRayPos = aabb[3];
                    vRayDir = (aabb[7] - aabb[3]);
                    iDbgColor = color_xrgb(0, 255, 255);
                    break;
                }
                case 11:
                {
                    vRayPos = aabb[1];
                    vRayDir = (aabb[5] - aabb[1]);
                    iDbgColor = color_xrgb(255, 0, 0);
                    break;
                }
                case 12:
                {
                    vRayPos = aabb[2];
                    vRayDir = (aabb[6] - aabb[2]);
                    iDbgColor = color_xrgb(0, 255, 0);
                    break;
                }
                case 13:
                { //--> По центру (Fwd)
                    vRayPos = aabb[8];
                    vRayDir = (aabb[9] - aabb[8]);
                    iDbgColor = color_xrgb(0, 0, 255);
                    break;
                }
                case 14:
                { //--> По центру (Up)
                    vRayPos = aabb[10];
                    vRayDir = (aabb[11] - aabb[10]);
                    iDbgColor = color_xrgb(0, 0, 125);
                    break;
                }
                case 15:
                { //--> По центру (Right)
                    vRayPos = aabb[12];
                    vRayDir = (aabb[13] - aabb[12]);
                    iDbgColor = color_xrgb(0, 0, 60);
                    break;
                }
                default: R_ASSERT2(false, "Broken switch sequence!");
                }

                Fvector vRayDirN = vRayDir;
                vRayDirN.normalize();

                collide::rq_results RQ;
                collide::ray_defs RD(vRayPos, vRayDirN, vRayDir.magnitude(), CDB::OPT_FULL_TEST, collide::rqtBoth);

                collide::rq_callback* cb = pFnCollideClbk;
                collide::test_callback* tb = pFnTestClbk;

                phcollider_bbox_callback_data data;
                data.vRayPos = vRayPos;
                data.vRayDirN = vRayDirN;
                data.bStopTesting = false;

#ifdef DEBUG_DRAW_BBOX_COLLIDE_DATA
                //--> Рисуем все лучи внутри BBox-а
                float fDbgDrawSizeMod = 0.5f + (0.5f * ((float)iIdx) / 12.f); //--> Без этого сливаются

                //--> Точка запуска луча (крупная)
                Fvector vStartSize = {0.025f, 0.025f, 0.025f};
                vStartSize.mul(fDbgDrawSizeMod);
                render.draw_aabb(vRayPos, vStartSize.x, vStartSize.y, vStartSize.z, iDbgColor);

                //--> Точка конца луча (мелкая)
                Fvector vEndSize = {0.01f, 0.01f, 0.01f};
                vEndSize.mul(fDbgDrawSizeMod);
                render.draw_aabb(Fvector().add(vRayPos, vRayDir), vEndSize.x, vEndSize.y, vEndSize.z, iDbgColor);
#endif

                Level().ObjectSpace.RayQuery(RQ, RD, cb, &data, tb, m_pOwnerObj->IObject());

                // Проверяем нужно-ли тестировать дальше
                if (data.bStopTesting)
                {
                    //--> goto т.к у нас вложенные циклы
                    goto label_DoBBoxCollide_exit;
                }
            } //--> Цикл по BBox
        }
    } //--> Цикл по элементам

label_DoBBoxCollide_exit:
    m_pPHBody->DisableCollision();
    m_pPHBody->Deactivate();
    m_pPHBody->Disable();
}

// Протестировать грани bounding-box всех элементов на коллизию с объектами \ геометрией
// Размещает bbox в указанной позиции с дефолтным поворотом
void CPHCollideHelper::DoBBoxCollide(const Fvector& vPos, EBBoxTestMode testMode, collide::rq_callback* pFnCollideClbk,
    collide::test_callback* pFnTestClbk, OnBeforeCollide* pFnClbk)
{
    Fmatrix mXFORM;
    mXFORM.translate(vPos);

    DoBBoxCollide(mXFORM, testMode, pFnCollideClbk, pFnTestClbk, pFnClbk);
}
