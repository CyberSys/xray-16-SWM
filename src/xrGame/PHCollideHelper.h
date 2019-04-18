#pragma once

/*********************************************************/
/** Вспомогательный класс для теста коллизии примитивов **/ //--#SM+#--
/*********************************************************/

class CPhysicsShell;
class CPhysicsElement;

typedef std::function<void(CPHCollideHelper*)> OnBeforeCollide;

// Данные из DoBBoxCollide, передаюстя в функцию рейтрейсинга
struct phcollider_bbox_callback_data
{
    Fvector vRayPos; //--> Точка запуска луча
    Fvector vRayDirN; //--> Нормализованная дирекция луча
    bool bStopTesting; //--> Если true - трассировка следующих лучей не произойдёт
};

class CPHCollideHelper
{
private:
    bool m_bIgnoreStatic; //-> True если физ. оболочка игнорирует статику
    bool m_bIgnoreDynamic; //-> True если физ. оболочка игнорирует динамику

protected:
    CPhysicsShell* m_pPHBody; //-> Физическая оболочка
    IPhysicsShellHolder* m_pOwnerObj; //-> Владелец нашей физической оболочки
    ObjectContactCallbackFun* m_fnContactClbk; //-> Функия, будет вызвана при коллизии объекта во время DoCollideAt

    void RegisterElement(CPhysicsElement* pPHElem);

#ifdef DEBUG
    using PHCDataVector = xr_vector<std::pair<Fmatrix, Fvector>>;
    PHCDataVector m_dbg_DrawData;

    void DBG_CollectData();
#endif

public:
    CPHCollideHelper(IPhysicsShellHolder* pOwnerObj, ObjectContactCallbackFun* fnContactClbk);
    ~CPHCollideHelper();

    // Получить физ. оболочку коллайдера
    IC CPhysicsShell* GetPHShell() { return m_pPHBody; }

    // True когда к оболочке добавлен хоть один элемент
    IC bool IsReady() { return m_pPHBody->get_ElementsNumber() > 0; }

    void DestroyPHShell();

    void SetIgnoreStatic(bool bVal);
    IC bool GetIgnoreStatic() { return m_bIgnoreStatic; } //-> Объект игнорирует статику

    void SetIgnoreDynamic(bool bVal);
    IC bool GetIgnoreDynamic() { return m_bIgnoreDynamic; } //-> Объект игнорирует динамику

    IC const u16 GetElementsCnt() //-> Узнать кол-во добавленных элементов (примитивов)
    {
        return m_pPHBody->get_ElementsNumber();
    }

    void GetElementWorldXFORM(u16 iElemIdx, Fmatrix& pMOut);
    void GetElementWorldXFORM(Fmatrix& pMOut) { GetElementWorldXFORM(0, pMOut); }

#ifdef DEBUG
    void DBG_DrawData(u32 iColor = color_xrgb(0, 255, 0));
#endif

    CPhysicsElement* AddBox(const Fmatrix& mLocalXFORM, const Fvector& vHalfsize);
    CPhysicsElement* AddBox(const Fvector& vLocalCenter, const Fvector& vHalfsize);
    CPhysicsElement* AddBox(const Fvector& vHalfsize);

    CPhysicsElement* AddCylinder(const Fvector& vLocalCenter, const Fvector& vLocalDir, float fHeight, float fRadius);
    CPhysicsElement* AddCylinder(const Fvector& vLocalDir, float fHeight, float fRadius);

    CPhysicsElement* AddSphere(const Fvector& vLocalCenter, float fRadius);
    CPhysicsElement* AddSphere(float fRadius);

    void DoCollideAt(Fmatrix mXFORM, OnBeforeCollide* pFnClbk = nullptr);
    void DoCollideAt(const Fvector& vPos, OnBeforeCollide* pFnClbk = nullptr);

    enum EBBoxTestMode
    {
        eEdgesOnly = 0, //--> Тестировать только грани
        eCentreOnly, //--> Тестировать только центральную полость (перекрестием, быстро)
        eCentreUpOnly, //--> Тестировать только центр (вертикальный луч, быстро)
        eAll, //--> Тестировать всё
    };

    void DoBBoxCollide(Fmatrix mXFORM, EBBoxTestMode testMode, collide::rq_callback* pFnCollideClbk = nullptr,
        collide::test_callback* pFnTestClbk = nullptr, OnBeforeCollide* pFnClbk = nullptr);
    void DoBBoxCollide(const Fvector& vPos, EBBoxTestMode testMode, collide::rq_callback* pFnCollideClbk = nullptr,
        collide::test_callback* pFnTestClbk = nullptr, OnBeforeCollide* pFnClbk = nullptr);
};
