/************************************/
/***** Работа со звуками оружия *****/ //--#SM+#--
/************************************/

#include "stdafx.h"
#include "Weapon_Shared.h"

// Перезагрузка всех звуков, с учётом текущих аддонов
void CWeapon::ReloadAllSounds()
{
    // clang-format off
	//		<Имя звука из конфига>		  <Движковый SID звука> <Флаг эксклюзивности>  <Тип звука для НПС>
	//		Одновременно на оружии может играться только один эксклюзивный звук - другие экслюзивные будут остановленны <!>
	ReloadSound( "snd_draw",					"sndShow"				, true,		SOUND_TYPE_ITEM_TAKING				);
	ReloadSound( "snd_holster",					"sndHide"				, true,		SOUND_TYPE_ITEM_HIDING				);
	ReloadSound( "snd_shoot",					"sndShot"				, false,	SOUND_TYPE_WEAPON_SHOOTING			);
	ReloadSound( "snd_knife",					"sndKnife"				, false,	SOUND_TYPE_WEAPON_SHOOTING			);
	ReloadSound( "snd_empty",					"sndEmptyClick"			, false,	SOUND_TYPE_WEAPON_EMPTY_CLICKING	);
	ReloadSound( "snd_reload",					"sndReload"				, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_empty",			"sndReloadEmpty"		, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_w_gl",				"sndReloadWGL"			, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_empty_w_gl",		"sndReloadEmptyWGL"		, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab",				"sndReloadAB"			, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab_empty",			"sndReloadABEmpty"		, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab_w_gl",			"sndReloadABWGL"		, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_ab_empty_w_gl",	"sndReloadABEmptyWGL"	, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab",			"sndReloadFrAB"			, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab_empty",		"sndReloadFrABEmpty"	, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab_w_gl",		"sndReloadFrABWGL"		, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_reload_fr_ab_empty_w_gl",	"sndReloadFrABEmptyWGL"	, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_bore",					"sndBore"				, true,		SOUND_TYPE_IDLE						);
	ReloadSound( "snd_shoot_grenade",			"sndShotG"				, false,	SOUND_TYPE_WEAPON_SHOOTING			);
	ReloadSound( "snd_reload_grenade",			"sndReloadG"			, true,		SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_switch",					"sndSwitch"				, true,		SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_kick",					"sndKick"				, false,	SOUND_TYPE_IDLE						);
	ReloadSound( "snd_zoomin",					"sndZoomIn"				, true,		SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_zoomout",					"sndZoomOut"			, true,		SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_pump",					"sndPump"				, false,    SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_pump_w_gl",				"sndPumpWGL"			, false,    SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_pump_aim",				"sndPumpAim"			, false,    SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_pump_aim_w_gl",			"sndPumpAimWGL"			, false,    SOUND_TYPE_WEAPON_RECHARGING		);
	ReloadSound( "snd_bipods_deploy",			"sndBipodsD"			, false,	SOUND_TYPE_ITEM_USING				);
	ReloadSound( "snd_bipods_retract",			"sndBipodsU"			, false,	SOUND_TYPE_ITEM_USING				);

	// Звук глушителя
	if ( get_SilencerStatus() != ALife::eAddonDisabled )
	{
		ReloadSound( "snd_silncer_shot", "sndSilencerShot", false, SOUND_TYPE_WEAPON_SHOOTING );
	}

	// Звуки перезарядки в три стадии
	if(m_bTriStateReload_main)
	{
		ReloadSound( "snd_open_weapon",						"sndOpen"							, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon",				"sndOpenEmpty"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge",					"sndAddCartridge"					, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_empty",				"sndAddCartridgeEmpty"				, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon",					"sndClose"							, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon",				"sndCloseEmpty"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon",				"sndCloseFE"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon",		"sndCloseEmptyFE"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
	}

	if(m_bTriStateReload_gl)
	{
		ReloadSound( "snd_open_weapon_g",					"sndOpenG"							, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_weapon_w_gl",				"sndOpenWGL"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_g",				"sndOpenEmptyG"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_w_gl",			"sndOpenEmptyWGL"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_g",					"sndAddCartridgeG"					, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_w_gl",				"sndAddCartridgeWGL"				, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_empty_g",			"sndAddCartridgeEmptyG"				, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_empty_w_gl",		"sndAddCartridgeEmptyWGL"			, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_g",					"sndCloseG"							, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_w_gl",				"sndCloseWGL"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_g",			"sndCloseEmptyG"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_w_gl",			"sndCloseEmptyWGL"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_g",			"sndCloseFEG"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_w_gl",			"sndCloseFEWGL"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_g",		"sndCloseEmptyFEG"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_w_gl",	"sndCloseEmptyFEWGL"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
	}

	if(m_bTriStateReload_ab)
	{
		ReloadSound( "snd_open_weapon_ab",					"sndOpenAB"							, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_weapon_ab_w_gl",				"sndOpenABWGL"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_ab",				"sndAddCartridgeAB"					, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_ab_w_gl",			"sndAddCartridgeABWGL"				, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_ab",					"sndCloseAB"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_ab_w_gl",			"sndCloseABWGL"						, true,		SOUND_TYPE_WEAPON_RECHARGING	);
	}

	if(m_bTriStateReload_frab)
	{
		ReloadSound( "snd_open_weapon_fr_ab",					"sndOpenFrAB"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_fr_ab",				"sndOpenFrABEmpty"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_weapon_fr_ab_w_gl",				"sndOpenFrABWGL"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_open_empty_weapon_fr_ab_w_gl",		"sndOpenFrABEmptyWGL"			, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab",					"sndAddCartridgeFrAB"			, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab_empty",			"sndAddCartridgeFrABEmpty"		, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab_w_gl",			"sndAddCartridgeFrABWGL"		, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_add_cartridge_fr_ab_empty_w_gl",		"sndAddCartridgeFrABEmptyWGL"	, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_fr_ab",					"sndCloseFrAB"					, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_fr_ab",			"sndCloseFrABEmpty"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_weapon_fr_ab_w_gl",				"sndCloseFrABWGL"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_empty_weapon_fr_ab_w_gl",		"sndCloseFrABEmptyWGL"			, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_fr_ab",			"sndCloseFrABFE"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_fr_ab",		"sndCloseFrABEmptyFE"			, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_weapon_fr_ab_w_gl",		"sndCloseFrABFEWGL"				, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_close_fempt_empty_weapon_fr_ab_w_gl",	"sndCloseFrABEmptyFEWGL"		, true,		SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge",				"sndSwAddCartridge"				, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge_empty",			"sndSwAddCartridgeEmpty"		, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge_w_gl",			"sndSwAddCartridgeWGL"			, false,	SOUND_TYPE_WEAPON_RECHARGING	);
		ReloadSound( "snd_switch_add_cartridge_empty_w_gl",		"sndSwAddCartridgeEmptyWGL"		, false,	SOUND_TYPE_WEAPON_RECHARGING	);
	}

    // Звуки перезарядки для НПС / Третьего лица
    ReloadSound("snd_reload_npc", "sndReloadNPC", true, SOUND_TYPE_WEAPON_RECHARGING);

    // clang-format on

    // Звуки с привязкой к anm_
    CInifile::Sect&   _sect = pSettings->r_section(this->cNameSect());
    CInifile::SectCIt _b    = _sect.Data.begin();
    CInifile::SectCIt _e    = _sect.Data.end();

    for (; _b != _e; ++_b)
    {
        if (strstr(_b->first.c_str(), "snd_anm_") == _b->first.c_str())
        {
            ReloadSound(_b->first, _b->first, true, sg_SourceType);
        }
    }
}

// Перезагрузить конкретный звук, с учётом текущих аддонов
void CWeapon::ReloadSound(shared_str const& strName, shared_str const& strAlias, bool exclusive, int type)
{
    shared_str strSection;

    // Специальный аддон 1
    if (IsSpecial_1_Attached() && get_Special_1_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_1_SetSect(), strName))
        {
            strSection = GetSpecial_1_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Специальный аддон 2
    if (IsSpecial_2_Attached() && get_Special_2_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_2_SetSect(), strName))
        {
            strSection = GetSpecial_2_SetSect();
            goto label_ReloadSound_reload;
        }
    }
    // Специальный аддон 3
    if (IsSpecial_3_Attached() && get_Special_3_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_3_SetSect(), strName))
        {
            strSection = GetSpecial_3_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Специальный аддон 4
    if (IsSpecial_4_Attached() && get_Special_4_Status() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSpecial_4_SetSect(), strName))
        {
            strSection = GetSpecial_4_SetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Магазин
    if (IsMagazineAttached() && get_MagazineStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetMagazineSetSect(), strName))
        {
            strSection = GetMagazineSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Подствол
    if (IsGrenadeLauncherAttached() && get_GrenadeLauncherStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetGrenadeLauncherSetSect(), strName))
        {
            strSection = GetGrenadeLauncherSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Глушитель
    if (IsSilencerAttached() && get_SilencerStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetSilencerSetSect(), strName))
        {
            strSection = GetSilencerSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Прицел
    if (IsScopeAttached() && get_ScopeStatus() == ALife::eAddonAttachable)
    {
        if (pSettings->line_exist(GetScopeSetSect(), strName))
        {
            strSection = GetScopeSetSect();
            goto label_ReloadSound_reload;
        }
    }

    // Если ничего не нашли - пробуем грузить из секции оружия
    strSection = this->cNameSect();

// Пере-загружаем звук
label_ReloadSound_reload:

    bool bOrigSndType = READ_IF_EXISTS(pSettings, r_bool, strSection.c_str(), make_string("%s_use_src_sndtype", strName.c_str()).c_str(), false);
    if (bOrigSndType == true)
        type = sg_SourceType;

    m_sounds.ReLoadSound(strSection.c_str(), strName.c_str(), strAlias.c_str(), exclusive, type);
}

// Обновление позиции звуков
void CWeapon::UpdateSounds()
{
    if (Device.dwFrame == dwUpdateSounds_Frame)
        return;

    dwUpdateSounds_Frame = Device.dwFrame;

    Fvector P = get_LastFP();

    m_sounds.UpdateAllSounds(P);
}

////////////////////////////////////////////////////////////////////
// ************************************************************** //
////////////////////////////////////////////////////////////////////

// Звук входа в прицеливание
void CWeapon::PlaySoundZoomIn()
{
    IGameObject* pParent = H_Parent();
    if (pParent == NULL)
        return;

    m_sounds.StopSound("sndZoomOut");
    PlaySound("sndZoomIn", pParent->Position());
}

// Звук выхода из прицеливания
void CWeapon::PlaySoundZoomOut()
{
    IGameObject* pParent = H_Parent();
    if (pParent == NULL)
        return;

    m_sounds.StopSound("sndZoomIn");
    PlaySound("sndZoomOut", pParent->Position());
}
