#pragma once

/***************************************************/
/***** Класс для связи CWeapon с Lua-скриптами *****/ //--#SM+#--
/***************************************************/

class CLuaWpnAdapter
{
public:
    static bool bNeed2Reload; //--> True если требуется переопределить значения

private:
    // Калбэк на перегрузку Lua-скриптов игрой
    class CResetEventCb : public CEventNotifierCallbackWithCid
    {
    public:
        CResetEventCb(CID cid) : CEventNotifierCallbackWithCid(cid) {}
        void ProcessEvent() override
        {
            CLuaWpnAdapter::bNeed2Reload = true; //--> Нужно привязать скрипты заного
            ai().Unsubscribe(GetCid(), CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
        }
    };

    // Привязываем Lua-скрипты к движковым функциям
    void ReloadFunctors()
    {
        if (bNeed2Reload == true)
        {
            // Привязываем Lua-функции
            m_bIsFastReloadAllowedFnExist =
                GEnv.ScriptEngine->functor("swm_wpn_adapter.IsMagazineFastReloadAllowed", m_lfnIsFastReloadAllowed);

            // Подписываемся на перезагрузку скриптов
            ai().template Subscribe<CLuaWpnAdapter::CResetEventCb>(CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
        }
        bNeed2Reload = false;
    }

    CLuaWpnAdapter()
    {
        bNeed2Reload = true;
        ReloadFunctors();
    }
    ~CLuaWpnAdapter(){};

public:
    CLuaWpnAdapter(CLuaWpnAdapter const&) = delete;
    void operator=(CLuaWpnAdapter const&) = delete;

    //--> Проверка возможности перезарядить магазин из инвентаря
    luabind::functor<bool> m_lfnIsFastReloadAllowed;
    bool m_bIsFastReloadAllowedFnExist;

    // Получить экземпляр CLuaWpnAdapter
    static CLuaWpnAdapter& Get()
    {
        static CLuaWpnAdapter* instance = new CLuaWpnAdapter(); //--> Вызывается один раз за запуск, удаляется при выходе из игры
        instance->ReloadFunctors();
        return *instance;
    }
};

bool CLuaWpnAdapter::bNeed2Reload = false;
