#pragma once

/******************************************************/
/***** Общие заголовочные файлы для всего оружия  *****/ //--#SM+#--
/******************************************************/

// SM_TODO: Превратить в предкомпилированный?

#include "Weapon.h"

/////////////////////////

///////////////////////////

#include "ParticlesObject.h"
#include "entity_alive.h"
#include "inventory_item_impl.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "actor.h"
#include "actoreffector.h"
#include "level.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "Include/xrRender/Kinematics.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "xrPhysics/mathutils.h"
#include "Common/object_broker.h"
#include "player_hud.h"
#include "gamepersistent.h"
#include "effectorFall.h"
#include "debug_renderer.h"
#include "static_cast_checked.hpp"
#include "clsid_game.h"

//#include "pch_script.h"

//#include "WeaponMagazined.h"

#include "WeaponAddon.h" //--#SM+#--

#include "InventoryOwner.h"

#include "EffectorZoomInertion.h"

#include "UIGameCustom.h"

#include "string_table.h"
#include "MPPlayersBag.h"

#include "ui/UIStatic.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif

// WGrenade
#include "entity.h"

#include "GrenadeLauncher.h"

#include "ExplosiveRocket.h" // Перетащить в стэйт файр ???

#ifdef DEBUG
#include "phdebug.h"
#endif

extern BOOL    b_toggle_weapon_aim;
extern CUIXml* pWpnScopeXml;
extern u32     hud_adj_mode;

ENGINE_API extern float psHUD_FOV;     //--#SM+#--
ENGINE_API extern float psHUD_FOV_def; //--#SM+#--
