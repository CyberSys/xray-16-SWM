#include "pch_script.h"
#include "WeaponWalther.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponWalther::CWeaponWalther() {}
CWeaponWalther::~CWeaponWalther() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponWalther, (CGameObject), { module(luaState)[class_<CWeaponWalther, CGameObject>("CWeaponWalther").def(constructor<>())]; });