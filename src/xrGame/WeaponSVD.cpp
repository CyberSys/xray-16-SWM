#include "pch_script.h"
#include "WeaponSVD.h"
#include "xrScriptEngine/ScriptExporter.hpp"

//--#SM+#--
CWeaponSVD::CWeaponSVD() {}
CWeaponSVD::~CWeaponSVD() {}

using namespace luabind;

SCRIPT_EXPORT(CWeaponSVD, (CGameObject), { module(luaState)[class_<CWeaponSVD, CGameObject>("CWeaponSVD").def(constructor<>())]; });
