/*
SDLPoP, a port/conversion of the DOS game Prince of Persia.
Copyright (C) 2013-2025  Dávid Nagy

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

The authors of this program may be contacted at https://forum.princed.org
*/

#include "common.h"

#ifdef USE_MENU

byte arrowhead_up_image_data[];
byte arrowhead_down_image_data[];
byte arrowhead_left_image_data[];
byte arrowhead_right_image_data[];
image_type* arrowhead_up_image;
image_type* arrowhead_down_image;
image_type* arrowhead_left_image;
image_type* arrowhead_right_image;

void load_arrowhead_images(void) {
	// Make a dummy palette for decode_image().
	dat_pal_type dat_pal;
	memset(&dat_pal, 0, sizeof(dat_pal));
	dat_pal.vga[1].r = dat_pal.vga[1].g = dat_pal.vga[1].b = 0x3F; // white
	if (arrowhead_up_image == NULL) {
		arrowhead_up_image = decode_image((image_data_type*) arrowhead_up_image_data, &dat_pal);
	}
	if (arrowhead_down_image == NULL) {
		arrowhead_down_image = decode_image((image_data_type*) arrowhead_down_image_data, &dat_pal);
	}
//	dat_pal.vga[1] = vga_palette[color_7_lightgray];
	if (arrowhead_left_image == NULL) {
		arrowhead_left_image = decode_image((image_data_type*) arrowhead_left_image_data, &dat_pal);
	}
	if (arrowhead_right_image == NULL) {
		arrowhead_right_image = decode_image((image_data_type*) arrowhead_right_image_data, &dat_pal);
	}
}

#define MAX_MENU_ITEM_LENGTH 32

typedef struct pause_menu_item_type pause_menu_item_type;
struct pause_menu_item_type {
	int id;
	pause_menu_item_type* previous;
	pause_menu_item_type* next;
	void* required;
	char text[MAX_MENU_ITEM_LENGTH];
};

enum pause_menu_item_ids {
	PAUSE_MENU_RESUME,
	PAUSE_MENU_CHEATS,
	PAUSE_MENU_SAVE_GAME,
	PAUSE_MENU_LOAD_GAME,
	PAUSE_MENU_RESTART_LEVEL,
	PAUSE_MENU_SETTINGS,
	PAUSE_MENU_RESTART_GAME,
	PAUSE_MENU_QUIT_GAME,
	SETTINGS_MENU_GENERAL,
	SETTINGS_MENU_GAMEPLAY,
	SETTINGS_MENU_VISUALS,
	SETTINGS_MENU_MODS,
	SETTINGS_MENU_LEVEL_CUSTOMIZATION,
	SETTINGS_MENU_BACK,
	SETTINGS_MENU_CONTROLS,
};

pause_menu_item_type pause_menu_items[] = {
		{.id = PAUSE_MENU_RESUME,        .text = "CONTINUAR"},
		// TODO: Add a cheats menu, where you can choose a cheat from a list?
		/*{.id = PAUSE_MENU_CHEATS,        .text = "CHEATS", .required = &cheats_enabled},*/
#ifdef USE_QUICKSAVE // TODO: If quicksave is disabled, show regular save/load instead?
		{.id = PAUSE_MENU_SAVE_GAME,     .text = "GUARDAR (F6)"},
		{.id = PAUSE_MENU_LOAD_GAME,     .text = "CARGAR (F9)"},
#endif
		{.id = PAUSE_MENU_RESTART_LEVEL, .text = "REINICIAR NIVEL"},
		{.id = PAUSE_MENU_SETTINGS,      .text = "OPCIONES"},
		{.id = PAUSE_MENU_RESTART_GAME,  .text = "REINICIAR JUEGO"},
		{.id = PAUSE_MENU_QUIT_GAME,     .text = "SALIR DEL JUEGO"},
};

int hovering_pause_menu_item = PAUSE_MENU_RESUME;
pause_menu_item_type* next_pause_menu_item;
pause_menu_item_type* previous_pause_menu_item;
int drawn_menu;
byte pause_menu_alpha;
int current_dialog_box;
const char* current_dialog_text;
word menu_current_level = 1;
bool need_close_menu;

enum menu_dialog_ids {
	DIALOG_NONE,
	DIALOG_RESTORE_DEFAULT_SETTINGS,
	DIALOG_CONFIRM_QUIT,
	DIALOG_SELECT_LEVEL,
};

pause_menu_item_type settings_menu_items[] = {
		{.id = SETTINGS_MENU_GENERAL, .text = "GENERAL"},
		{.id = SETTINGS_MENU_GAMEPLAY, .text = "JUEGO"},
		{.id = SETTINGS_MENU_VISUALS, .text = "GRÁFICOS"},
		{.id = SETTINGS_MENU_MODS, .text = "MODS"},
		{.id = SETTINGS_MENU_CONTROLS, .text = "CONTROLES"},
		{.id = SETTINGS_MENU_BACK, .text = "VOLVER"},
};
int active_settings_subsection = 0;
int highlighted_settings_subsection = 0;
int scroll_position = 0;
int menu_control_y;
int menu_control_x;
int menu_control_back;

enum menu_setting_style_ids {
	SETTING_STYLE_TOGGLE,
	SETTING_STYLE_NUMBER,
	SETTING_STYLE_TEXT_ONLY,
	SETTING_STYLE_KEY,
};

enum menu_setting_number_type_ids {
	SETTING_BYTE  = 0,
	SETTING_SBYTE = 1,
	SETTING_WORD  = 2,
	SETTING_SHORT = 3,
	//SETTING_DWORD = 4,
	SETTING_INT   = 5,
};

enum setting_ids {
	SETTING_RESET_ALL_SETTINGS,
	SETTING_SHOW_MENU_ON_PAUSE,
	SETTING_ENABLE_INFO_SCREEN,
	SETTING_ENABLE_SOUND,
	SETTING_ENABLE_MUSIC,
	SETTING_ENABLE_CONTROLLER_RUMBLE,
	SETTING_JOYSTICK_THRESHOLD,
	SETTING_JOYSTICK_ONLY_HORIZONTAL,
	SETTING_FULLSCREEN,
	SETTING_USE_HARDWARE_ACCELERATION,
	SETTING_USE_CORRECT_ASPECT_RATIO,
	SETTING_USE_INTEGER_SCALING,
	SETTING_SCALING_TYPE,
	SETTING_ENABLE_FADE,
	SETTING_ENABLE_FLASH,
	SETTING_ENABLE_LIGHTING,
	SETTING_ENABLE_CHEATS,
	SETTING_ENABLE_COPYPROT,
	SETTING_ENABLE_QUICKSAVE,
	SETTING_ENABLE_QUICKSAVE_PENALTY,
	SETTING_ENABLE_REPLAY,
	SETTING_USE_FIXES_AND_ENHANCEMENTS,
	SETTING_ENABLE_CROUCH_AFTER_CLIMBING,
	SETTING_ENABLE_FREEZE_TIME_DURING_END_MUSIC,
	SETTING_ENABLE_REMEMBER_GUARD_HP,
	SETTING_FIX_GATE_SOUNDS,
	SETTING_TWO_COLL_BUG,
	SETTING_FIX_INFINITE_DOWN_BUG,
	SETTING_FIX_GATE_DRAWING_BUG,
	SETTING_FIX_BIGPILLAR_CLIMB,
	SETTING_FIX_JUMP_DISTANCE_AT_EDGE,
	SETTING_FIX_EDGE_DISTANCE_CHECK_WHEN_CLIMBING,
	SETTING_FIX_PAINLESS_FALL_ON_GUARD,
	SETTING_FIX_WALL_BUMP_TRIGGERS_TILE_BELOW,
	SETTING_FIX_STAND_ON_THIN_AIR,
	SETTING_FIX_PRESS_THROUGH_CLOSED_GATES,
	SETTING_FIX_GRAB_FALLING_SPEED,
	SETTING_FIX_SKELETON_CHOMPER_BLOOD,
	SETTING_FIX_MOVE_AFTER_DRINK,
	SETTING_FIX_LOOSE_LEFT_OF_POTION,
	SETTING_FIX_GUARD_FOLLOWING_THROUGH_CLOSED_GATES,
	SETTING_FIX_SAFE_LANDING_ON_SPIKES,
	SETTING_FIX_GLIDE_THROUGH_WALL,
	SETTING_FIX_DROP_THROUGH_TAPESTRY,
	SETTING_FIX_LAND_AGAINST_GATE_OR_TAPESTRY,
	SETTING_FIX_UNINTENDED_SWORD_STRIKE,
	SETTING_FIX_RETREAT_WITHOUT_LEAVING_ROOM,
	SETTING_FIX_RUNNING_JUMP_THROUGH_TAPESTRY,
	SETTING_FIX_PUSH_GUARD_INTO_WALL,
	SETTING_FIX_JUMP_THROUGH_WALL_ABOVE_GATE,
	SETTING_FIX_CHOMPERS_NOT_STARTING,
	SETTING_FIX_FEATHER_INTERRUPTED_BY_LEVELDOOR,
	SETTING_FIX_OFFSCREEN_GUARDS_DISAPPEARING,
	SETTING_FIX_MOVE_AFTER_SHEATHE,
	SETTING_FIX_HIDDEN_FLOORS_DURING_FLASHING,
	SETTING_FIX_HANG_ON_TELEPORT,
	SETTING_FIX_EXIT_DOOR,
	SETTING_FIX_QUICKSAVE_DURING_FEATHER,
	SETTING_FIX_CAPED_PRINCE_SLIDING_THROUGH_GATE,
	SETTING_FIX_DOORTOP_DISABLING_GUARD,
	SETTING_FIX_JUMPING_OVER_GUARD,
	SETTING_FIX_DROP_2_ROOMS_CLIMBING_LOOSE_TILE,
	SETTING_FIX_FALLING_THROUGH_FLOOR_DURING_SWORD_STRIKE,
	SETTING_FIX_REGISTER_QUICK_INPUT,
	SETTING_FIX_TURN_RUNNING_NEAR_WALL,
	SETTING_FIX_FEATHER_FALL_AFFECTS_GUARDS,
	SETTING_FIX_ONE_HP_STOPS_BLINKING,
	SETTING_FIX_DEAD_FLOATING_IN_AIR,
	SETTING_ENABLE_SUPER_HIGH_JUMP,
	SETTING_ENABLE_JUMP_GRAB,
	SETTING_USE_CUSTOM_OPTIONS,
	SETTING_START_MINUTES_LEFT,
	SETTING_START_TICKS_LEFT,
	SETTING_START_HITP,
	SETTING_MAX_HITP_ALLOWED,
	SETTING_SAVING_ALLOWED_FIRST_LEVEL,
	SETTING_SAVING_ALLOWED_LAST_LEVEL,
	SETTING_START_UPSIDE_DOWN,
	SETTING_START_IN_BLIND_MODE,
	SETTING_COPYPROT_LEVEL,
	SETTING_DRAWN_TILE_TOP_LEVEL_EDGE,
	SETTING_DRAWN_TILE_LEFT_LEVEL_EDGE,
	SETTING_LEVEL_EDGE_HIT_TILE,
	SETTING_ALLOW_TRIGGERING_ANY_TILE,
	SETTING_ENABLE_WDA_IN_PALACE,
	SETTING_FIRST_LEVEL,
	SETTING_SKIP_TITLE,
	SETTING_SHIFT_L_ALLOWED_UNTIL_LEVEL,
	SETTING_SHIFT_L_REDUCED_MINUTES,
	SETTING_SHIFT_L_REDUCED_TICKS,
	SETTING_DEMO_HITP,
	SETTING_DEMO_END_ROOM,
	SETTING_INTRO_MUSIC_LEVEL,
	SETTING_HAVE_SWORD_FROM_LEVEL,
	SETTING_CHECKPOINT_LEVEL,
	SETTING_CHECKPOINT_RESPAWN_DIR,
	SETTING_CHECKPOINT_RESPAWN_ROOM,
	SETTING_CHECKPOINT_RESPAWN_TILEPOS,
	SETTING_CHECKPOINT_CLEAR_TILE_ROOM,
	SETTING_CHECKPOINT_CLEAR_TILE_COL,
	SETTING_CHECKPOINT_CLEAR_TILE_ROW,
	SETTING_SKELETON_LEVEL,
	SETTING_SKELETON_ROOM,
	SETTING_SKELETON_TRIGGER_COLUMN_1,
	SETTING_SKELETON_TRIGGER_COLUMN_2,
	SETTING_SKELETON_COLUMN,
	SETTING_SKELETON_ROW,
	SETTING_SKELETON_REQUIRE_OPEN_LEVEL_DOOR,
	SETTING_SKELETON_SKILL,
	SETTING_SKELETON_REAPPEAR_ROOM,
	SETTING_SKELETON_REAPPEAR_X,
	SETTING_SKELETON_REAPPEAR_ROW,
	SETTING_SKELETON_REAPPEAR_DIR,
	SETTING_MIRROR_LEVEL,
	SETTING_MIRROR_ROOM,
	SETTING_MIRROR_COLUMN,
	SETTING_MIRROR_ROW,
	SETTING_MIRROR_TILE,
	SETTING_SHOW_MIRROR_IMAGE,

	SETTING_SHADOW_STEAL_LEVEL,
	SETTING_SHADOW_STEAL_ROOM,
	SETTING_SHADOW_STEP_LEVEL,
	SETTING_SHADOW_STEP_ROOM,

	SETTING_FALLING_EXIT_LEVEL,
	SETTING_FALLING_EXIT_ROOM,
	SETTING_FALLING_ENTRY_LEVEL,
	SETTING_FALLING_ENTRY_ROOM,
	SETTING_MOUSE_LEVEL,
	SETTING_MOUSE_ROOM,
	SETTING_MOUSE_DELAY,
	SETTING_MOUSE_OBJECT,
	SETTING_MOUSE_START_X,
	SETTING_LOOSE_TILES_LEVEL,
	SETTING_LOOSE_TILES_ROOM_1,
	SETTING_LOOSE_TILES_ROOM_2,
	SETTING_LOOSE_TILES_FIRST_TILE,
	SETTING_LOOSE_TILES_LAST_TILE,
	SETTING_JAFFAR_VICTORY_LEVEL,
	SETTING_JAFFAR_VICTORY_FLASH_TIME,
	SETTING_HIDE_LEVEL_NUMBER_FIRST_LEVEL,
	SETTING_LEVEL_13_LEVEL_NUMBER,
	SETTING_VICTORY_STOPS_TIME_LEVEL,
	SETTING_WIN_LEVEL,
	SETTING_WIN_ROOM,
	SETTING_LOOSE_FLOOR_DELAY,
	SETTING_BASE_SPEED,
	SETTING_FIGHT_SPEED,
	SETTING_CHOMPER_SPEED,
	SETTING_NO_MOUSE_IN_ENDING,
	SETTING_LEVEL_SETTINGS,
	SETTING_LEVEL_TYPE,
	SETTING_LEVEL_COLOR,
	SETTING_GUARD_TYPE,
	SETTING_GUARD_HP,
	SETTING_CUTSCENE,
	SETTING_ENTRY_POSE,
	SETTING_SEAMLESS_EXIT,
	SETTING_KEY_LEFT,
	SETTING_KEY_RIGHT,
	SETTING_KEY_UP,
	SETTING_KEY_DOWN,
	SETTING_KEY_JUMP_LEFT,
	SETTING_KEY_JUMP_RIGHT,
	SETTING_KEY_ACTION,
	SETTING_KEY_ENTER,
	SETTING_KEY_ESC,
};

typedef struct setting_type {
	int index;
	int id;
	int previous, next;
	byte style;
	byte number_type;
	void* linked;
	void* required;
	int min, max; // for 'number'-style settings
	char text[64];
	char explanation[256];
	names_list_type* names_list;
} setting_type;

setting_type general_settings[] = {
		{.id = SETTING_SHOW_MENU_ON_PAUSE, .style = SETTING_STYLE_TOGGLE, .linked = &enable_pause_menu,
				.text = "Activar menú de pausa",
				.explanation = "Muestra el menú al pausar el juego.\n"
						"Si está desactivado, igual puedes abrirlo con Retroceso."},
		{.id = SETTING_ENABLE_INFO_SCREEN, .style = SETTING_STYLE_TOGGLE, .linked = &enable_info_screen,
				.text = "Pantalla de información al iniciar",
				.explanation = "Muestra la pantalla de información de SDLPoP al arrancar."},
		{.id = SETTING_ENABLE_SOUND, .style = SETTING_STYLE_TOGGLE, .linked = &is_sound_on,
				.text = "Activar sonido",
				.explanation = "Enciende o apaga el sonido."},
		{.id = SETTING_ENABLE_MUSIC, .style = SETTING_STYLE_TOGGLE, .linked = &enable_music,
				.text = "Activar música",
				.explanation = "Enciende o apaga la música."},
		{.id = SETTING_ENABLE_CONTROLLER_RUMBLE, .style = SETTING_STYLE_TOGGLE, .linked = &enable_controller_rumble,
				.text = "Vibración del mando",
				.explanation = "Si el mando tiene vibración, vibra cuando el príncipe recibe daño."},
		{.id = SETTING_JOYSTICK_THRESHOLD, .style = SETTING_STYLE_NUMBER, .number_type = SETTING_INT,
				.linked = &joystick_threshold, .min = 0, .max = INT16_MAX,
				.text = "Umbral del joystick",
				.explanation = "Sensibilidad de la 'zona muerta' del joystick."},
		{.id = SETTING_JOYSTICK_ONLY_HORIZONTAL, .style = SETTING_STYLE_TOGGLE, .linked = &joystick_only_horizontal,
				.text = "Joystick solo horizontal",
				.explanation = "Usa el joystick solo para moverse en horizontal. "
						"Con algunos mandos el juego se controla mejor así."},
		{.id = SETTING_RESET_ALL_SETTINGS, .style = SETTING_STYLE_TEXT_ONLY,
				.text = "Restaurar valores...", .explanation = "Vuelve todas las opciones a sus valores originales."},
};

NAMES_LIST(use_hardware_acceleration_setting_names, {"NO", "SÍ", "AUTO",});
NAMES_LIST(scaling_type_setting_names, {"Nítido", "Suave", "Difuso",});

int integer_scaling_possible =
#if SDL_VERSION_ATLEAST(2,0,5) // SDL_RenderSetIntegerScale
	1
#else
	0
#endif
;

setting_type visuals_settings[] = {
		{.id = SETTING_FULLSCREEN, .style = SETTING_STYLE_TOGGLE, .linked = &start_fullscreen,
				.text = "Iniciar en pantalla completa",
				.explanation = "Inicia el juego en pantalla completa.\nTambién puedes alternarla con Alt+Intro."},
		{.id = SETTING_USE_HARDWARE_ACCELERATION, .style = SETTING_STYLE_NUMBER, .number_type = SETTING_BYTE, .max = 2,
				.linked = &use_hardware_acceleration, .names_list = &use_hardware_acceleration_setting_names_list,
				.text = "Aceleración por hardware",
				.explanation = "Auto - Usa aceleración por hardware si está disponible.\n"
				               "Sí - Fuerza la aceleración por hardware.\n"
				               "No - Desactiva la aceleración por hardware.\n"
				               "Nota: requiere reiniciar el juego."},
		{.id = SETTING_USE_CORRECT_ASPECT_RATIO, .style = SETTING_STYLE_TOGGLE, .linked = &use_correct_aspect_ratio,
				.text = "Usar proporción 4:3",
				.explanation = "Muestra el juego en la proporción 4:3 original."
				               "\nFunciona mejor con alta resolución."},
		{.id = SETTING_USE_INTEGER_SCALING, .style = SETTING_STYLE_TOGGLE, .linked = &use_integer_scaling,
				.required = &integer_scaling_possible,
				.text = "Escalado entero",
				.explanation = "Escalado perfecto: todos los píxeles del mismo tamaño usando factores enteros.\n"
						"Combinado con 4:3 requiere al menos 1600x1200."
						"\nRequiere compilar con SDL 2.0.5 o superior."},
		{.id = SETTING_SCALING_TYPE, .style = SETTING_STYLE_NUMBER, .number_type = SETTING_BYTE, .max = 2,
				.linked = &scaling_type, .names_list = &scaling_type_setting_names_list,
				.text = "Método de escalado",
				.explanation = "Nítido - Píxeles sin suavizar.\n"
						"Suave - Duplica el tamaño y luego suaviza.\n"
						"Difuso - Escalado suavizado."},
#ifdef USE_FADE
		{.id = SETTING_ENABLE_FADE, .style = SETTING_STYLE_TOGGLE, .linked = &enable_fade,
				.text = "Fundidos activados",
				.explanation = "Activa o desactiva los fundidos de pantalla."},
#endif
#ifdef USE_FLASH
		{.id = SETTING_ENABLE_FLASH, .style = SETTING_STYLE_TOGGLE, .linked = &enable_flash,
				.text = "Destellos activados",
				.explanation = "Activa o desactiva los destellos de pantalla."},
#endif
#ifdef USE_LIGHTING
		{.id = SETTING_ENABLE_LIGHTING, .style = SETTING_STYLE_TOGGLE, .linked = &enable_lighting,
				.text = "Sombras de antorchas",
				.explanation = "Oscurece las zonas de la pantalla lejos de las antorchas."},
#endif
};

setting_type gameplay_settings[] = {
		{.id = SETTING_ENABLE_CHEATS, .style = SETTING_STYLE_TOGGLE, .linked = &cheats_enabled,
				.text = "Activar trucos",
				.explanation = "Activa o desactiva los trucos."/*"\nAlso, display the CHEATS option on the pause menu."*/},
#ifdef USE_COPYPROT
		{.id = SETTING_ENABLE_COPYPROT, .style = SETTING_STYLE_TOGGLE, .linked = &enable_copyprot,
				.text = "Nivel de protección anticopia",
				.explanation = "Activa o desactiva el nivel de las pociones (protección anticopia)."},
#endif
#ifdef USE_QUICKSAVE
		{.id = SETTING_ENABLE_QUICKSAVE, .style = SETTING_STYLE_TOGGLE, .linked = &enable_quicksave,
				.text = "Activar guardado rápido",
				.explanation = "Activa el guardado y la carga rápida.\nF6 guarda, F9 carga."},
		{.id = SETTING_ENABLE_QUICKSAVE_PENALTY, .style = SETTING_STYLE_TOGGLE, .linked = &enable_quicksave_penalty,
				.text = "Penalización de tiempo al cargar",
				.explanation = "Hace que el tiempo corra al cargar rápido (como al morir).\n"
						"El 'tiempo restante' se recupera igual, "
						"pero se aplica una penalización (hasta un minuto)."},
#endif
#ifdef USE_REPLAY
		{.id = SETTING_ENABLE_REPLAY, .style = SETTING_STYLE_TOGGLE, .linked = &enable_replay,
				.text = "Activar repeticiones",
				.explanation = "Activa la grabación y reproducción de partidas.\n"
						"Pulsa Ctrl+Tab durante el juego para grabar.\n"
						"Para detener, pulsa Ctrl+Tab otra vez."},
#endif
		{.id = SETTING_USE_FIXES_AND_ENHANCEMENTS, .style = SETTING_STYLE_TOGGLE, .linked = &use_fixes_and_enhancements,
				.text = "Modo mejorado (correcciones)",
				.explanation = "Activa correcciones y mejoras del juego.\n"
						"Abajo puedes activar o desactivar cada corrección.\n"
						"NOTA: algunas correcciones anulan 'trucos' que aprovechan fallos del juego."},
		{.id = SETTING_ENABLE_CROUCH_AFTER_CLIMBING, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.enable_crouch_after_climbing, .required = &use_fixes_and_enhancements,
				.text = "Agacharse tras trepar",
				.explanation = "Permite agacharse justo después de trepar: pulsa abajo y adelante a la vez. "
						"En el original no se podía (abajo siempre hacía descolgarse)."},
		{.id = SETTING_ENABLE_FREEZE_TIME_DURING_END_MUSIC, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.enable_freeze_time_during_end_music, .required = &use_fixes_and_enhancements,
				.text = "Detener el tiempo en la música final",
				.explanation = "El tiempo corre mientras suena la música de fin de nivel; se puede saltar quitando el sonido. "
						"Esta opción detiene el tiempo mientras suena (sin quitar el sonido)."},
		{.id = SETTING_ENABLE_REMEMBER_GUARD_HP, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.enable_remember_guard_hp, .required = &use_fixes_and_enhancements,
				.text = "Recordar la vida de los guardias",
				.explanation = "La vida de los guardias no se recupera al volver a entrar en la sala."},
		{.id = SETTING_ENABLE_SUPER_HIGH_JUMP, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.enable_super_high_jump, .required = &use_fixes_and_enhancements,
				.text = "Súper salto",
				.explanation = "En modo pluma (poción verde) el príncipe salta dos pisos."},
		{.id = SETTING_ENABLE_JUMP_GRAB, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.enable_jump_grab, .required = &use_fixes_and_enhancements,
				.text = "Agarrarse al saltar",
				.explanation = "El príncipe puede agarrarse al piso de arriba al saltar. "
                               "Mantén Shift y arriba, sin la flecha hacia adelante."},
		{.id = SETTING_FIX_GATE_SOUNDS, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_gate_sounds, .required = &use_fixes_and_enhancements,
				.text = "Corregir sonido de rejas",
				.explanation = "Si una sala se enlaza consigo misma a la izquierda, no se oye el cierre de sus rejas."},
		{.id = SETTING_TWO_COLL_BUG, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_two_coll_bug, .required = &use_fixes_and_enhancements,
				.text = "Corregir doble colisión",
				.explanation = "Una reja abierta o una cuchilla pueden dejar atravesar paredes. (Truco 7, 37, 62)"},
		{.id = SETTING_FIX_INFINITE_DOWN_BUG, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_infinite_down_bug, .required = &use_fixes_and_enhancements,
				.text = "Corregir caída infinita",
				.explanation = "Si una sala se enlaza consigo misma abajo y la columna no tiene suelo, el juego se cuelga."},
		{.id = SETTING_FIX_GATE_DRAWING_BUG, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_gate_drawing_bug, .required = &use_fixes_and_enhancements,
				.text = "Corregir dibujo de rejas",
				.explanation = "Con una reja bajo otra, no se ve la parte de arriba de la inferior."},
		{.id = SETTING_FIX_BIGPILLAR_CLIMB, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_bigpillar_climb, .required = &use_fixes_and_enhancements,
				.text = "Corregir trepar junto a columna",
				.explanation = "Al trepar mirando a la derecha junto a una columna grande, se ve a través del suelo."},
		{.id = SETTING_FIX_JUMP_DISTANCE_AT_EDGE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_jump_distance_at_edge, .required = &use_fixes_and_enhancements,
				.text = "Corregir salto en el borde",
				.explanation = "Al trepar dos pisos, girar y saltar hacia arriba, el príncipe se cae. "
						"Esta corrección hace innecesario el Truco 25."},
		{.id = SETTING_FIX_EDGE_DISTANCE_CHECK_WHEN_CLIMBING, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_edge_distance_check_when_climbing, .required = &use_fixes_and_enhancements,
				.text = "Corregir distancia al trepar",
				.explanation = "Al trepar, el juego comprueba sin necesidad la distancia al borde de abajo. "
						"A veces te \"teletransportas\" un poco al trepar desde suelo firme."},
		{.id = SETTING_FIX_PAINLESS_FALL_ON_GUARD, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_painless_fall_on_guard, .required = &use_fixes_and_enhancements,
				.text = "Corregir caída sobre guardia",
				.explanation = "Caer desde muy alto encima de un guardia no hace daño."},
		{.id = SETTING_FIX_WALL_BUMP_TRIGGERS_TILE_BELOW, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_wall_bump_triggers_tile_below, .required = &use_fixes_and_enhancements,
				.text = "Corregir golpe contra pared",
				.explanation = "Chocar con una pared puede soltar una baldosa de abajo sin tocarla. (Truco 18, 34)"},
		{.id = SETTING_FIX_STAND_ON_THIN_AIR, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_stand_on_thin_air, .required = &use_fixes_and_enhancements,
				.text = "Corregir pararse en el aire",
				.explanation = "Al pisar una baldosa suelta y levantarse de agachado, puedes quedar parado en el aire."},
		{.id = SETTING_FIX_PRESS_THROUGH_CLOSED_GATES, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_press_through_closed_gates, .required = &use_fixes_and_enhancements,
				.text = "Corregir pulsar tras reja cerrada",
				.explanation = "Los botones justo a la derecha de una reja se pueden pulsar con la reja cerrada (Truco 1)"},
		{.id = SETTING_FIX_GRAB_FALLING_SPEED, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_grab_falling_speed, .required = &use_fixes_and_enhancements,
				.text = "Corregir velocidad al agarrarse",
				.explanation = "Saltando contra una pared a veces te agarras de un borde dos pisos más abajo (no debería)."},
		{.id = SETTING_FIX_SKELETON_CHOMPER_BLOOD, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_skeleton_chomper_blood, .required = &use_fixes_and_enhancements,
				.text = "Corregir sangre de esqueleto",
				.explanation = "Los esqueletos ensangrientan la cuchilla aunque no tienen sangre."},
		{.id = SETTING_FIX_MOVE_AFTER_DRINK, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_move_after_drink, .required = &use_fixes_and_enhancements,
				.text = "Corregir movimiento tras beber",
				.explanation = "Al beber una poción los controles no se liberan bien y hay movimientos no deseados."},
		{.id = SETTING_FIX_LOOSE_LEFT_OF_POTION, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_loose_left_of_potion, .required = &use_fixes_and_enhancements,
				.text = "Corregir baldosa junto a poción",
				.explanation = "Hay un fallo de dibujo con una baldosa suelta a la izquierda de una poción (o espada)."},
		{.id = SETTING_FIX_GUARD_FOLLOWING_THROUGH_CLOSED_GATES, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_guard_following_through_closed_gates, .required = &use_fixes_and_enhancements,
				.text = "Corregir guardias tras rejas",
				.explanation = "Los guardias pueden \"seguirte\" a otra sala aunque haya una reja cerrada en medio."},
		{.id = SETTING_FIX_SAFE_LANDING_ON_SPIKES, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_safe_landing_on_spikes, .required = &use_fixes_and_enhancements,
				.text = "Corregir caída segura en pinchos",
				.explanation = "Caer en el borde de una baldosa con pinchos se considera seguro. (Truco 65)"},
		{.id = SETTING_FIX_GLIDE_THROUGH_WALL, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_glide_through_wall, .required = &use_fixes_and_enhancements,
				.text = "Corregir deslizarse por paredes",
				.explanation = "El príncipe puede atravesar paredes al girar corriendo (sobre todo en modo pluma)."},
		{.id = SETTING_FIX_DROP_THROUGH_TAPESTRY, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_drop_through_tapestry, .required = &use_fixes_and_enhancements,
				.text = "Corregir caer tras tapices",
				.explanation = "Se puede bajar a través de una reja cerrada si hay un tapiz encima."},
		{.id = SETTING_FIX_LAND_AGAINST_GATE_OR_TAPESTRY, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_land_against_gate_or_tapestry, .required = &use_fixes_and_enhancements,
				.text = "Corregir caída contra reja o tapiz",
				.explanation = "Al caer justo frente a una pared debería verse toda la animación de caída. "
						"Pero contra una reja cerrada o un tapiz (+suelo), la animación se corta."},
		{.id = SETTING_FIX_UNINTENDED_SWORD_STRIKE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_unintended_sword_strike, .required = &use_fixes_and_enhancements,
				.text = "Corregir estocada involuntaria",
				.explanation = "A veces el príncipe ataca solo justo después de desenvainar. "
						"Pasa sobre todo al bajar de un piso y girar hacia el rival."},
		{.id = SETTING_FIX_RETREAT_WITHOUT_LEAVING_ROOM, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_retreat_without_leaving_room, .required = &use_fixes_and_enhancements,
				.text = "Corregir retroceso sin cambiar sala",
				.explanation = "Retrocediendo repetidas veces en combate puedes salir de la sala sin que cambie. (Truco 35)"},
		{.id = SETTING_FIX_RUNNING_JUMP_THROUGH_TAPESTRY, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_running_jump_through_tapestry, .required = &use_fixes_and_enhancements,
				.text = "Corregir salto a través de tapices",
				.explanation = "Se puede atravesar un tapiz con salto largo a la izquierda si hay suelo encima."},
		{.id = SETTING_FIX_PUSH_GUARD_INTO_WALL, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_push_guard_into_wall, .required = &use_fixes_and_enhancements,
				.text = "Corregir empujar guardias a paredes",
				.explanation = "Se puede empujar a los guardias dentro de las paredes porque no se comprueba bien."},
		{.id = SETTING_FIX_JUMP_THROUGH_WALL_ABOVE_GATE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_jump_through_wall_above_gate, .required = &use_fixes_and_enhancements,
				.text = "Corregir salto por pared sobre reja",
				.explanation = "Con un salto largo contra una pared puedes caer tras una reja dos pisos abajo. (p. ej. atajo del nivel 7)"},
		{.id = SETTING_FIX_CHOMPERS_NOT_STARTING, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_chompers_not_starting, .required = &use_fixes_and_enhancements,
				.text = "Corregir cuchillas que no arrancan",
				.explanation = "Si te agarras de un borde uno o más pisos abajo, las cuchillas de esa fila no se activan."},
		{.id = SETTING_FIX_FEATHER_INTERRUPTED_BY_LEVELDOOR, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_feather_interrupted_by_leveldoor, .required = &use_fixes_and_enhancements,
				.text = "Corregir puerta y caída de pluma",
				.explanation = "Cuando la puerta del nivel termina de abrirse se corta el efecto pluma porque se detiene el sonido."},
		{.id = SETTING_FIX_OFFSCREEN_GUARDS_DISAPPEARING, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_offscreen_guards_disappearing, .required = &use_fixes_and_enhancements,
				.text = "Corregir guardias que desaparecen",
				.explanation = "Los guardias empujados fuera de la pantalla a menudo no reaparecen en la otra sala."},
		{.id = SETTING_FIX_MOVE_AFTER_SHEATHE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_move_after_sheathe, .required = &use_fixes_and_enhancements,
				.text = "Corregir movimiento al envainar",
				.explanation = "Al envainar, si pulsas adelante y abajo y sueltas abajo, el príncipe igual se agacha."},
		{.id = SETTING_FIX_HIDDEN_FLOORS_DURING_FLASHING, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_hidden_floors_during_flashing, .required = &use_fixes_and_enhancements,
				.text = "Corregir suelos ocultos",
				.explanation = "Tras unirse con la sombra en el nivel 12, los suelos ocultos no aparecen hasta que cesan los destellos."},
		{.id = SETTING_FIX_HANG_ON_TELEPORT, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_hang_on_teleport, .required = &use_fixes_and_enhancements,
				.text = "Corregir teletransporte al colgarse",
				.explanation = "Saltando a una esquina inferior y agarrándote de un borde puedes pasar a la sala de arriba."},
		{.id = SETTING_FIX_EXIT_DOOR, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_exit_door, .required = &use_fixes_and_enhancements,
				.text = "Corregir puertas de salida",
				.explanation = "Se puede entrar por salidas cerradas tras ver a la sombra, vencer a Jaffar o abrir otra salida."},
		{.id = SETTING_FIX_QUICKSAVE_DURING_FEATHER, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_quicksave_during_feather, .required = &use_fixes_and_enhancements,
				.text = "Corregir guardar en modo pluma",
				.explanation = "No se puede guardar mientras flotas en modo pluma."},
		{.id = SETTING_FIX_CAPED_PRINCE_SLIDING_THROUGH_GATE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_caped_prince_sliding_through_gate, .required = &use_fixes_and_enhancements,
				.text = "Corregir deslizarse por reja",
				.explanation = "Con los gráficos del príncipe con capa, si te agachas de espaldas a una reja cerrada en el borde izquierdo de la sala, la atraviesas."},
		{.id = SETTING_FIX_DOORTOP_DISABLING_GUARD, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_doortop_disabling_guard, .required = &use_fixes_and_enhancements,
				.text = "Corregir guardia sobre puerta",
				.explanation = "Los guardias se desactivan si ellos o el príncipe están sobre un dintel de puerta."},
		{.id = SETTING_FIX_JUMPING_OVER_GUARD, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_jumping_over_guard, .required = &use_fixes_and_enhancements,
				.text = "Corregir saltar sobre guardia",
				.explanation = "El príncipe puede saltar por encima de los guardias con un salto largo a tiempo."},
		{.id = SETTING_FIX_DROP_2_ROOMS_CLIMBING_LOOSE_TILE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_drop_2_rooms_climbing_loose_tile, .required = &use_fixes_and_enhancements,
				.text = "Corregir caer 2 salas con baldosa",
				.explanation = "Se puede caer 2 salas al trepar a una baldosa suelta de la sala de arriba. (Truco 153)"},
		{.id = SETTING_FIX_FALLING_THROUGH_FLOOR_DURING_SWORD_STRIKE, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_falling_through_floor_during_sword_strike, .required = &use_fixes_and_enhancements,
				.text = "Corregir caer al dar estocada",
				.explanation = "El príncipe o un guardia pueden atravesar el suelo durante una estocada."},
		{.id = SETTING_FIX_REGISTER_QUICK_INPUT, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_register_quick_input, .required = &use_fixes_and_enhancements,
				.text = "Corregir pulsaciones rápidas",
				.explanation = "Se ignora una tecla pulsada y soltada entre dos fotogramas del juego."},
		{.id = SETTING_FIX_TURN_RUNNING_NEAR_WALL, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_turn_running_near_wall, .required = &use_fixes_and_enhancements,
				.text = "Corregir giro corriendo junto a pared",
				.explanation = "Asegura pasos seguros cerca de una pared o reja al mirar en sentido contrario."},
		{.id = SETTING_FIX_FEATHER_FALL_AFFECTS_GUARDS, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_feather_fall_affects_guards, .required = &use_fixes_and_enhancements,
				.text = "Corregir pluma en guardias",
				.explanation = "La caída de pluma no debería afectar a los guardias: solo el príncipe bebe esa poción."},
		{.id = SETTING_FIX_ONE_HP_STOPS_BLINKING, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_one_hp_stops_blinking, .required = &use_fixes_and_enhancements,
				.text = "Corregir parpadeo con 1 de vida",
				.explanation = "Si el príncipe vence a Jaffar con 1 de vida, deja de parpadear."},
		{.id = SETTING_FIX_DEAD_FLOATING_IN_AIR, .style = SETTING_STYLE_TOGGLE,
				.linked = &fixes_saved.fix_dead_floating_in_air, .required = &use_fixes_and_enhancements,
				.text = "Corregir cuerpos flotando",
				.explanation = "Si alguien muere al caer sobre una baldosa suelta, la baldosa cae pero el cuerpo queda en el aire."},
};

NAMES_LIST(tile_type_setting_names, {
		"Empty", "Floor", "Spikes", "Pillar", "Gate",                                                    // 0..4
		"Stuck button", "Closer button", "Tapestry/floor", "Big pillar: bottom", "Big pillar: top",      // 5..9
		"Potion", "Loose floor", "Tapestry", "Mirror", "Floor/debris",                                   // 10..14
		"Raise button", "Level door: left", "Level door: right", "Chomper", "Torch",                     // 15..19
		"Wall", "Esqueleto", "Sword", "Balcony: left", "Balcony: right",                                  // 20..24
		"Lattice: pillar", "Lattice: down", "Lattice: small", "Lattice: left", "Lattice: right",         // 25..29
		"Torch/debris", "Tile 31 (unused)" // 30
});
NAMES_LIST(row_setting_names, {"Arriba", "Medio", "Abajo"});
KEY_VALUE_LIST(direction_setting_names, {{"Izquierda", dir_FF_left}, {"Derecha", dir_0_right}});
extern names_list_type never_is_16_list;

setting_type mods_settings[] = {
		{.id = SETTING_USE_CUSTOM_OPTIONS, .style = SETTING_STYLE_TOGGLE, .linked = &use_custom_options,
				.text = "Usar personalización",
				.explanation = "Activa o desactiva la personalización.\n(por defecto = NO)"},
		{.id = SETTING_LEVEL_SETTINGS, .style = SETTING_STYLE_TEXT_ONLY, .required = &use_custom_options,
				.text = "Personalizar nivel...",
				.explanation = "Cambia opciones del nivel (tipo de nivel, tipo de guardia, vida de los guardias)."},
		{.id = SETTING_START_MINUTES_LEFT, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.start_minutes_left, .number_type = SETTING_SHORT, .min = -1, .max = INT16_MAX,
				.text = "Minutos iniciales",
				.explanation = "Minutos al empezar. (por defecto = 60)\n"
						"Para quitar el límite de tiempo, pon -1."},
		{.id = SETTING_START_TICKS_LEFT, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.start_ticks_left, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Segundos iniciales",
				.explanation = "Segundos restantes del primer minuto.\n(por defecto = 59.92)"},
		{.id = SETTING_START_HITP, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.start_hitp, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Vida inicial",
				.explanation = "Puntos de vida al empezar. (por defecto = 3)"},
		{.id = SETTING_MAX_HITP_ALLOWED, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.max_hitp_allowed, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Vida máxima",
				.explanation = "Máximo de puntos de vida posibles. (por defecto = 10)"},
		{.id = SETTING_SAVING_ALLOWED_FIRST_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.saving_allowed_first_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Guardar desde el nivel",
				.explanation = "Primer nivel en que se puede guardar. (por defecto = 3)"},
		{.id = SETTING_SAVING_ALLOWED_LAST_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.saving_allowed_last_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Guardar hasta el nivel",
				.explanation = "Último nivel en que se puede guardar. (por defecto = 13)"},
		{.id = SETTING_START_UPSIDE_DOWN, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.start_upside_down,
				.text = "Empezar con pantalla invertida",
				.explanation = "Empieza con la pantalla boca abajo, como Shift+I (por defecto = NO)"},
		{.id = SETTING_START_IN_BLIND_MODE, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.start_in_blind_mode,
				.text = "Empezar en modo ciego",
				.explanation = "Empieza en modo ciego, como Shift+B (por defecto = NO)"},
		{.id = SETTING_COPYPROT_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.copyprot_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Anticopia antes del nivel",
				.explanation = "El nivel de las pociones aparece antes de este. (por defecto = 2)"},
		{.id = SETTING_DRAWN_TILE_TOP_LEVEL_EDGE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.names_list = &tile_type_setting_names_list,
				.linked = &custom_saved.drawn_tile_top_level_edge, .number_type = SETTING_BYTE, .max = 31,
				.text = "Baldosa en borde superior",
				.explanation = "Baldosa dibujada arriba si no hay sala en esa dirección. (por defecto = suelo)"},
		{.id = SETTING_DRAWN_TILE_LEFT_LEVEL_EDGE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.names_list = &tile_type_setting_names_list,
				.linked = &custom_saved.drawn_tile_left_level_edge, .number_type = SETTING_BYTE, .max = 31,
				.text = "Baldosa en borde izquierdo",
				.explanation = "Baldosa dibujada a la izquierda si no hay sala. (por defecto = pared)"},
		{.id = SETTING_LEVEL_EDGE_HIT_TILE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.names_list = &tile_type_setting_names_list,
				.linked = &custom_saved.level_edge_hit_tile, .number_type = SETTING_BYTE, .max = 31,
				.text = "Baldosa de choque en el borde",
				.explanation = "Comportamiento arriba o a la izquierda si no hay sala (por defecto = pared)"},
		{.id = SETTING_ALLOW_TRIGGERING_ANY_TILE, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.allow_triggering_any_tile,
				.text = "Activar cualquier baldosa",
				.explanation = "Permite activar cualquier baldosa: un botón puede soltar baldosas o arrancar una cuchilla. (por defecto = NO)"},
		{.id = SETTING_ENABLE_WDA_IN_PALACE, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.enable_wda_in_palace,
				.text = "WDA en el palacio",
				.explanation = "Usa el dibujo de paredes de mazmorra en el palacio."
						"\n¡Usar con un VPALACE.DAT modificado con paredes de mazmorra! (por defecto = NO)"},
		{.id = SETTING_FIRST_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.first_level, .number_type = SETTING_WORD, .max = 15,
				.text = "Primer nivel",
				.explanation = "Nivel que se carga al empezar una partida nueva."
						"\n(por defecto = 1)"},
		{.id = SETTING_SKIP_TITLE, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.skip_title,
				.text = "Saltar la presentación",
				.explanation = "Salta siempre la presentación y carga directamente el primer nivel."
						"\n(por defecto = NO)"},
		{.id = SETTING_SHIFT_L_ALLOWED_UNTIL_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shift_L_allowed_until_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Shift+L permitido hasta nivel",
				.explanation = "Primer nivel donde Shift+L no salta niveles sin trucos.\n"
						"(por defecto = 4)"},
		{.id = SETTING_SHIFT_L_REDUCED_MINUTES, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shift_L_reduced_minutes, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Minutos tras usar Shift+L",
				.explanation = "Minutos restantes tras usar Shift+L sin trucos.\n"
						"(por defecto = 15)"},
		{.id = SETTING_SHIFT_L_REDUCED_TICKS, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shift_L_reduced_ticks, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Segundos tras usar Shift+L",
				.explanation = "Segundos restantes tras usar Shift+L sin trucos.\n(por defecto = 59.92)"},
		{.id = SETTING_DEMO_HITP, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.demo_hitp, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Vida en el nivel demo",
				.explanation = "Vida del príncipe en el nivel de demostración.\n(por defecto = 4)"},
		{.id = SETTING_DEMO_END_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.demo_end_room, .number_type = SETTING_WORD, .min = 1, .max = 24,
				.text = "Sala final del nivel demo",
				.explanation = "Sala donde termina el nivel de demostración.\n(por defecto = 24)"},
		{.id = SETTING_INTRO_MUSIC_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.intro_music_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel con música de inicio",
				.explanation = "Nivel donde suena la música de presentación al agacharse. (por defecto = 1)\n"
						"Nota: solo funciona si es el nivel inicial."},
		{.id = SETTING_HAVE_SWORD_FROM_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.have_sword_from_level, .number_type = SETTING_WORD, .min = 1, .max = 16, .names_list = &never_is_16_list,
				.text = "Espada desde el nivel",
				.explanation = "Primer nivel (salvo la demo) en que el príncipe tiene espada.\n(por defecto = 2)\n"},
		{.id = SETTING_CHECKPOINT_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel del punto de control",
				.explanation = "Nivel con punto de control. (por defecto = 3)\n"
						"Se activa al salir de la sala 7 por la izquierda."},
		{.id = SETTING_CHECKPOINT_RESPAWN_DIR, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_respawn_dir, .number_type = SETTING_SBYTE, .min = -1, .max = 0, .names_list = &direction_setting_names_list,
				.text = "Dirección al reaparecer",
				.explanation = "Hacia dónde mira al reaparecer en el punto de control.\n(por defecto = izquierda)"},
		{.id = SETTING_CHECKPOINT_RESPAWN_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_respawn_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala al reaparecer",
				.explanation = "Sala donde reapareces tras el punto de control.\n(por defecto = 2)"},
		{.id = SETTING_CHECKPOINT_RESPAWN_TILEPOS, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_respawn_tilepos, .number_type = SETTING_BYTE, .max = 29,
				.text = "Baldosa al reaparecer",
				.explanation = "Posición (0 a 29) donde reapareces tras el punto de control.\n(por defecto = 6)"},
		{.id = SETTING_CHECKPOINT_CLEAR_TILE_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_clear_tile_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala de baldosa despejada",
				.explanation = "Sala donde se despeja una baldosa al reaparecer.\n(por defecto = 7)"},
		{.id = SETTING_CHECKPOINT_CLEAR_TILE_COL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_clear_tile_col, .number_type = SETTING_BYTE, .max = 9,
				.text = "Columna de baldosa despejada",
				.explanation = "Posición (columna/fila) de la baldosa despejada al reaparecer.\n(por defecto: columna = 4, fila = arriba)"},
		{.id = SETTING_CHECKPOINT_CLEAR_TILE_ROW, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.checkpoint_clear_tile_row, .number_type = SETTING_BYTE, .max = 2, .names_list = &row_setting_names_list,
				.text = "Fila de baldosa despejada",
				.explanation = "Posición (columna/fila) de la baldosa despejada al reaparecer.\n(por defecto: columna = 4, fila = arriba)"},
		{.id = SETTING_SKELETON_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel del esqueleto",
				.explanation = "Nivel y sala donde un esqueleto cobra vida.\n(por defecto: nivel = 3, sala = 1)"},
		{.id = SETTING_SKELETON_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala del esqueleto",
				.explanation = "Nivel y sala donde un esqueleto cobra vida.\n(por defecto: nivel = 3, sala = 1)"},
		{.id = SETTING_SKELETON_TRIGGER_COLUMN_1, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_trigger_column_1, .number_type = SETTING_BYTE, .max = 9,
				.text = "Esqueleto: columna activa (1)",
				.explanation = "El esqueleto despierta si el príncipe está en una de estas columnas.\n(por defecto = 2,3)"},
		{.id = SETTING_SKELETON_TRIGGER_COLUMN_2, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_trigger_column_2, .number_type = SETTING_BYTE, .max = 9,
				.text = "Esqueleto: columna activa (2)",
				.explanation = "El esqueleto despierta si el príncipe está en una de estas columnas.\n(por defecto = 2,3)"},
		{.id = SETTING_SKELETON_COLUMN, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_column, .number_type = SETTING_BYTE, .max = 9,
				.text = "Columna de la baldosa del esqueleto",
				.explanation = "Posición (columna/fila) del esqueleto que despierta.\n(por defecto: columna = 5, fila = medio)"},
		{.id = SETTING_SKELETON_ROW, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_row, .number_type = SETTING_BYTE, .max = 2, .names_list = &row_setting_names_list,
				.text = "Fila de la baldosa del esqueleto",
				.explanation = "Posición (columna/fila) del esqueleto que despierta.\n(por defecto: columna = 5, fila = medio)"},
		{.id = SETTING_SKELETON_REQUIRE_OPEN_LEVEL_DOOR, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_require_open_level_door,
				.text = "Esqueleto requiere puerta",
				.explanation = "Si hay que abrir antes la puerta del nivel para que despierte.\n(por defecto = sí)"},
		{.id = SETTING_SKELETON_SKILL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_skill, .number_type = SETTING_BYTE, .max = 15,
				.text = "Habilidad del esqueleto",
				.explanation = "Habilidad del esqueleto despierto.\n(por defecto = 2)"},
		{.id = SETTING_SKELETON_REAPPEAR_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_reappear_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala de reaparición del esqueleto",
				.explanation = "Si el esqueleto cae a esta sala, reaparece allí.\n(por defecto = 3)"},
		{.id = SETTING_SKELETON_REAPPEAR_X, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_reappear_x, .number_type = SETTING_BYTE, .max = 255,
				.text = "X de reaparición del esqueleto",
				.explanation = "Coordenada horizontal donde reaparece.\n(por defecto = 133)\n"
						"(58 = borde izquierdo, 198 = borde derecho)"},
		{.id = SETTING_SKELETON_REAPPEAR_ROW, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_reappear_row, .number_type = SETTING_BYTE, .max = 2, .names_list = &row_setting_names_list,
				.text = "Fila de reaparición del esqueleto",
				.explanation = "Fila donde reaparece el esqueleto.\n(por defecto = medio)"},
		{.id = SETTING_SKELETON_REAPPEAR_DIR, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.skeleton_reappear_dir, .number_type = SETTING_SBYTE, .min = -1, .max = 0, .names_list = &direction_setting_names_list,
				.text = "Dirección del esqueleto al reaparecer",
				.explanation = "Hacia dónde mira al reaparecer.\n(por defecto = derecha)"},
		{.id = SETTING_MIRROR_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mirror_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel del espejo",
				.explanation = "Nivel y sala donde aparece el espejo.\n(por defecto: nivel = 4, sala = 4)"},
		{.id = SETTING_MIRROR_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mirror_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala del espejo",
				.explanation = "Nivel y sala donde aparece el espejo.\n(por defecto: nivel = 4, sala = 4)"},
		{.id = SETTING_MIRROR_COLUMN, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mirror_column, .number_type = SETTING_BYTE, .max = 9,
				.text = "Columna del espejo",
				.explanation = "Posición (columna/fila) donde aparece el espejo.\n(por defecto: columna = 4, fila = arriba)"},
		{.id = SETTING_MIRROR_ROW, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mirror_row, .number_type = SETTING_BYTE, .max = 2, .names_list = &row_setting_names_list,
				.text = "Fila del espejo",
				.explanation = "Posición (columna/fila) donde aparece el espejo.\n(por defecto: columna = 4, fila = arriba)"},
		{.id = SETTING_MIRROR_TILE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mirror_tile, .number_type = SETTING_BYTE, .max = 31, .names_list = &tile_type_setting_names_list,
				.text = "Baldosa del espejo",
				.explanation = "Tipo de baldosa que aparece como espejo.\n(por defecto = espejo)"},
		{.id = SETTING_SHOW_MIRROR_IMAGE, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.show_mirror_image, .number_type = SETTING_BYTE,
				.text = "Mostrar reflejo",
				.explanation = "Muestra el reflejo del príncipe en el espejo.\n(por defecto = sí)"},

		{.id = SETTING_SHADOW_STEAL_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shadow_steal_level, .number_type = SETTING_BYTE, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel del robo de la sombra",
				.explanation = "Nivel donde la sombra roba una poción.\n(por defecto = 5)"},
		{.id = SETTING_SHADOW_STEAL_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shadow_steal_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala del robo de la sombra",
				.explanation = "Sala donde la sombra roba una poción.\n(por defecto = 24)"},
		{.id = SETTING_SHADOW_STEP_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shadow_step_level, .number_type = SETTING_BYTE, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel del botón de la sombra",
				.explanation = "Nivel donde la sombra pisa un botón.\n(por defecto = 6)"},
		{.id = SETTING_SHADOW_STEP_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.shadow_step_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala del botón de la sombra",
				.explanation = "Sala donde la sombra pisa un botón.\n(por defecto = 1)"},

		{.id = SETTING_FALLING_EXIT_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.falling_exit_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel con salida por caída",
				.explanation = "Nivel donde se pasa al siguiente cayendo desde cierta sala.\n(por defecto = 6)"},
		{.id = SETTING_FALLING_EXIT_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.falling_exit_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala de salida por caída",
				.explanation = "Sala donde se pasa de nivel cayendo.\n(por defecto = 1)"},
		{.id = SETTING_FALLING_ENTRY_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.falling_entry_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel con entrada cayendo",
				.explanation = "Si el príncipe empieza en este nivel y sala, no se muestra la sala inicial,\n"
						"sino la de abajo, para entrar cayendo. (por defecto: nivel = 7, sala = 17)"},
		{.id = SETTING_FALLING_ENTRY_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.falling_entry_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala de entrada cayendo",
				.explanation = "Si el príncipe empieza en este nivel y sala, no se muestra la sala inicial,\n"
						"sino la de abajo, para entrar cayendo. (por defecto: nivel = 7, sala = 17)"},
		{.id = SETTING_MOUSE_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mouse_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel del ratón",
				.explanation = "Nivel donde aparece el ratón.\n(por defecto = 8)"},
		{.id = SETTING_MOUSE_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mouse_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala del ratón",
				.explanation = "Sala donde aparece el ratón.\n(por defecto = 16)"},
		{.id = SETTING_MOUSE_DELAY, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mouse_delay, .number_type = SETTING_WORD, .max = UINT16_MAX,
				.text = "Espera del ratón",
				.explanation = "Segundos antes de que aparezca el ratón.\n(por defecto = 12.5)"},
		{.id = SETTING_MOUSE_OBJECT, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mouse_object, .number_type = SETTING_BYTE, .max = 255,
				.text = "Objeto ratón",
				.explanation = "Tipo de objeto del ratón. (por defecto = 24)\n"
						"Cuidado: un valor distinto de 24 cambia el ratón por el príncipe."},
		{.id = SETTING_MOUSE_START_X, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.mouse_start_x, .number_type = SETTING_BYTE, .max = 255,
				.text = "X inicial del ratón",
				.explanation = "Coordenada horizontal inicial del ratón.\n(por defecto = 200)"},
		{.id = SETTING_LOOSE_TILES_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.loose_tiles_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel de baldosas que caen",
				.explanation = "Nivel donde caen las baldosas sueltas.\n(por defecto = 13)"},
		{.id = SETTING_LOOSE_TILES_ROOM_1, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.loose_tiles_room_1, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala de baldosas que caen (1)",
				.explanation = "Salas donde caen las baldosas sueltas visibles.\n(por defecto = 23, 16)"},
		{.id = SETTING_LOOSE_TILES_ROOM_2, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.loose_tiles_room_2, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala de baldosas que caen (2)",
				.explanation = "Salas donde caen las baldosas sueltas visibles.\n(por defecto = 23, 16)"},
		{.id = SETTING_LOOSE_TILES_FIRST_TILE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.loose_tiles_first_tile, .number_type = SETTING_BYTE, .max = 29,
				.text = "Primera baldosa que cae",
				.explanation = "Rango de posiciones de baldosas que se pisan.\n(por defecto = 22 a 27)"},
		{.id = SETTING_LOOSE_TILES_LAST_TILE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.loose_tiles_last_tile, .number_type = SETTING_BYTE, .max = 29,
				.text = "Última baldosa que cae",
				.explanation = "Rango de posiciones de baldosas que se pisan.\n(por defecto = 22 a 27)"},
		{.id = SETTING_JAFFAR_VICTORY_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.jaffar_victory_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel de la victoria sobre Jaffar",
				.explanation = "Vencer al guardia de este nivel hace destellar la pantalla y activa el evento 0 al salir.\n(por defecto = 13)"},
		{.id = SETTING_JAFFAR_VICTORY_FLASH_TIME, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.jaffar_victory_flash_time, .number_type = SETTING_BYTE, .max = UINT16_MAX,
				.text = "Destello tras vencer a Jaffar",
				.explanation = "Cuánto destella la pantalla tras vencer a Jaffar.\n(por defecto = 18)"},
		{.id = SETTING_HIDE_LEVEL_NUMBER_FIRST_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.hide_level_number_from_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Ocultar número desde el nivel",
				.explanation = "Primer nivel donde no se muestra el número.\n(por defecto = 14)"},
		{.id = SETTING_LEVEL_13_LEVEL_NUMBER, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.level_13_level_number, .number_type = SETTING_BYTE, .max = UINT16_MAX,
				.text = "Número mostrado en el nivel 13",
				.explanation = "Número de nivel que se muestra en el 13.\n(por defecto = 12)"},
		{.id = SETTING_VICTORY_STOPS_TIME_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.victory_stops_time_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel donde la victoria para el tiempo",
				.explanation = "Nivel donde la muerte de Jaffar detiene el tiempo.\n(por defecto = 13)"},
		{.id = SETTING_WIN_LEVEL, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.win_level, .number_type = SETTING_WORD, .max = 16, .names_list = &never_is_16_list,
				.text = "Nivel donde se gana",
				.explanation = "Nivel y sala donde se gana el juego.\n(por defecto: nivel = 14, sala = 5)"},
		{.id = SETTING_WIN_ROOM, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.win_room, .number_type = SETTING_BYTE, .min = 1, .max = 24,
				.text = "Sala donde se gana",
				.explanation = "Nivel y sala donde se gana el juego.\n(por defecto: nivel = 14, sala = 5)"},
		{.id = SETTING_LOOSE_FLOOR_DELAY, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.loose_floor_delay, .number_type = SETTING_BYTE, .min = 0, .max = 127,
				.text = "Espera de baldosa suelta",
				.explanation = "Segundos antes de que caiga una baldosa suelta.\n(por defecto = 0.92)"},
		{.id = SETTING_BASE_SPEED, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.base_speed, .number_type = SETTING_BYTE, .min = 1, .max = 127,
				.text = "Velocidad normal",
				.explanation = "Velocidad fuera de combate (espera entre cuadros en 1/60 s). Menos es más rápido.\n(por defecto = 5)"},
		{.id = SETTING_FIGHT_SPEED, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.fight_speed, .number_type = SETTING_BYTE, .min = 1, .max = 127,
				.text = "Velocidad en combate",
				.explanation = "Velocidad en combate (espera entre cuadros en 1/60 s). Menos es más rápido.\n(por defecto = 6)"},
		{.id = SETTING_CHOMPER_SPEED, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = &custom_saved.chomper_speed, .number_type = SETTING_BYTE, .min = 0, .max = 127,
				.text = "Velocidad de las cuchillas",
				.explanation = "Velocidad de las cuchillas (cuadros por ciclo). Menos es más rápido.\n(por defecto = 15)"},
		{.id = SETTING_NO_MOUSE_IN_ENDING, .style = SETTING_STYLE_TOGGLE, .required = &use_custom_options,
				.linked = &custom_saved.no_mouse_in_ending, .number_type = SETTING_BYTE,
				.text = "Sin ratón en el final",
				.explanation = "Omite el ratón en la escena final.\n(por defecto = no)"},
};

NAMES_LIST(level_type_setting_names, { "Mazmorra", "Palacio", });
KEY_VALUE_LIST(guard_type_setting_names, {{"Ninguno", -1}, {"Normal", 0}, {"Gordo", 1}, {"Esqueleto", 2}, {"Visir", 3}, {"Sombra", 4}});
NAMES_LIST(entry_pose_setting_names, {"Girando", "Cayendo", "Corriendo"});
KEY_VALUE_LIST(off_setting_name, {{"No", -1}}); // used for the seamless exit setting

setting_type level_settings[] = {
		{.id = SETTING_LEVEL_SETTINGS, .style = SETTING_STYLE_TEXT_ONLY, .required = &use_custom_options,
				.text = "Personalizar otro nivel...",
				.explanation = "Elige otro nivel para personalizar."},
		{.id = SETTING_LEVEL_TYPE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.names_list = &level_type_setting_names_list,
				.linked = NULL /* depends on which level */, .number_type = SETTING_BYTE, .max = 1,
				.text = "Tipo de nivel",
				.explanation = "Qué ambientación usa este nivel.\n"
						"(mazmorra o palacio)"},
		{.id = SETTING_LEVEL_COLOR, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = NULL, .number_type = SETTING_WORD, .max = 4,
				.text = "Paleta de colores del nivel",
				.explanation = "0: colores de VDUNGEON.DAT/VPALACE.DAT\n>0: colores de PRINCE.DAT.\n"
						"Requiere un PRINCE.DAT de PoP 1.3 o 1.4."},
		{.id = SETTING_GUARD_TYPE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.names_list = &guard_type_setting_names_list,
				.linked = NULL, .number_type = SETTING_SHORT, .min = -1, .max = 4,
				.text = "Tipo de guardia",
				.explanation = "Tipo de guardia de este nivel (normal, gordo, esqueleto, visir o sombra)."},
		{.id = SETTING_GUARD_HP, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = NULL, .number_type = SETTING_BYTE, .max = UINT8_MAX,
				.text = "Vida de los guardias",
				.explanation = "Puntos de vida de los guardias en este nivel."},
		{.id = SETTING_CUTSCENE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = NULL, .number_type = SETTING_BYTE, .max = 15,
				.text = "Escena antes del nivel",
				.explanation = "Escena que se ve entre el nivel anterior y este.\n"
						"0: ninguna, 2 o 6: de pie, 4: acostada, 8: el ratón se va,\n"
						"9: el ratón vuelve, 12: de pie o girándose"},
		{.id = SETTING_ENTRY_POSE, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = NULL, .number_type = SETTING_BYTE, .max = 2, .names_list = &entry_pose_setting_names_list,
				.text = "Pose de entrada",
				.explanation = "Pose del príncipe al empezar el nivel.\n"},
		{.id = SETTING_SEAMLESS_EXIT, .style = SETTING_STYLE_NUMBER, .required = &use_custom_options,
				.linked = NULL, .number_type = SETTING_SBYTE, .min = -1, .max = 24, .names_list = &off_setting_name_list,
				.text = "Salida continua",
				.explanation = "Entrar en esta sala lleva al siguiente nivel.\n"
						"Pon -1 para desactivarlo."},
};

setting_type controls_settings[] = {
		{.id = SETTING_KEY_LEFT, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_left, .number_type = SETTING_INT,
				.text = "Izquierda",
				.explanation = ""},
		{.id = SETTING_KEY_RIGHT, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_right, .number_type = SETTING_INT,
				.text = "Derecha",
				.explanation = ""},
		{.id = SETTING_KEY_UP, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_up, .number_type = SETTING_INT,
				.text = "Arriba",
				.explanation = ""},
		{.id = SETTING_KEY_DOWN, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_down, .number_type = SETTING_INT,
				.text = "Abajo",
				.explanation = ""},
		{.id = SETTING_KEY_JUMP_LEFT, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_jump_left, .number_type = SETTING_INT,
				.text = "Saltar a la izquierda",
				.explanation = ""},
		{.id = SETTING_KEY_JUMP_RIGHT, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_jump_right, .number_type = SETTING_INT,
				.text = "Saltar a la derecha",
				.explanation = ""},
		{.id = SETTING_KEY_ACTION, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_action, .number_type = SETTING_INT,
				.text = "Acción",
				.explanation = ""},
		{.id = SETTING_KEY_ENTER, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_enter, .number_type = SETTING_INT,
				.text = "Entrar en un menú",
				.explanation = ""},
		{.id = SETTING_KEY_ESC, .style = SETTING_STYLE_KEY, .required = NULL,
				.linked = &key_esc, .number_type = SETTING_INT,
				.text = "Salir del menú, pausa",
				.explanation = ""},
};

typedef struct settings_area_type {
	setting_type* settings;
	int setting_count;
} settings_area_type;

settings_area_type general_settings_area = { .settings = general_settings, .setting_count = COUNT(general_settings)};
settings_area_type gameplay_settings_area = { .settings = gameplay_settings, .setting_count = COUNT(gameplay_settings)};
settings_area_type visuals_settings_area = { .settings = visuals_settings, .setting_count = COUNT(visuals_settings)};
settings_area_type mods_settings_area = { .settings = mods_settings, .setting_count = COUNT(mods_settings)};
settings_area_type level_settings_area = { .settings = level_settings, .setting_count = COUNT(level_settings)};
settings_area_type controls_settings_area = { .settings = controls_settings, .setting_count = COUNT(controls_settings)};

settings_area_type* get_settings_area(int menu_item_id) {
	switch(menu_item_id) {
		default:
			return NULL;
		case SETTINGS_MENU_GENERAL:
			return &general_settings_area;
		case SETTINGS_MENU_GAMEPLAY:
			return &gameplay_settings_area;
		case SETTINGS_MENU_VISUALS:
			return &visuals_settings_area;
		case SETTINGS_MENU_MODS:
			return &mods_settings_area;
		case SETTINGS_MENU_LEVEL_CUSTOMIZATION:
			return &level_settings_area;
		case SETTINGS_MENU_CONTROLS:
			return &controls_settings_area;
	}
}

void init_pause_menu_items(pause_menu_item_type* first_item, int item_count) {
	if (item_count > 0) {
		for (int i = 0; i < item_count; ++i) {
			pause_menu_item_type* item = first_item + i;
			item->previous = (first_item + MAX(0, i-1));
			item->next = (first_item + MIN(item_count-1, i+1));
		}
		pause_menu_item_type* last_item = first_item + (item_count-1);
		first_item->previous = last_item;
		last_item->next = first_item;
	}
}

void init_settings_list(setting_type* first_setting, int setting_count) {
	if (setting_count > 0) {
		for (int i = 0; i < setting_count; ++i) {
			setting_type* item = first_setting + i;
			item->index = i;
			item->previous = (first_setting + MAX(0, i-1))->id;
			item->next = (first_setting + MIN(setting_count-1, i+1))->id;
		}
//		setting_type* last_item = first_setting + (setting_count-1);
//		first_setting->previous = last_item->id;
//		last_item->next = first_setting->id;
	}
}


void init_menu() {
	load_arrowhead_images();

	init_pause_menu_items(pause_menu_items, COUNT(pause_menu_items));
	init_pause_menu_items(settings_menu_items, COUNT(settings_menu_items));

	init_settings_list(general_settings, COUNT(general_settings));
	init_settings_list(visuals_settings, COUNT(visuals_settings));
	init_settings_list(gameplay_settings, COUNT(gameplay_settings));
	init_settings_list(mods_settings, COUNT(mods_settings));
	init_settings_list(level_settings, COUNT(level_settings));
	init_settings_list(controls_settings, COUNT(controls_settings));
}

bool is_mouse_over_rect(rect_type* rect) {
	return (mouse_x >= rect->left && mouse_x < rect->right && mouse_y >= rect->top && mouse_y < rect->bottom);
}

// Maps the cursor position into a coordinate between (0,0) and (320,200) and sets mouse_x, mouse_y and mouse_moved.
void read_mouse_state(void) {
	float scale_x, scale_y;
	SDL_RenderGetScale(renderer_, &scale_x, &scale_y);
	int logical_width, logical_height;
	SDL_RenderGetLogicalSize(renderer_, &logical_width, &logical_height);
	int logical_scale_x = logical_width / 320; // These may be higher than 1, if 4:3 aspect ratio scaling is enabled.
	int logical_scale_y = logical_height / 200;
	scale_x *= logical_scale_x;
	scale_y *= logical_scale_y;
	if (!(scale_x > 0 && scale_y > 0 && logical_scale_x > 0 && logical_scale_y > 0)) return;
	SDL_Rect viewport;
	SDL_RenderGetViewport(renderer_, &viewport); // Get the width/height of the 'black bars' around the rendering area.
	viewport.x /= logical_scale_x;
	viewport.y /= logical_scale_y;
	int last_mouse_x = mouse_x;
	int last_mouse_y = mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	mouse_x = (int) ((float)mouse_x/scale_x - viewport.x + 0.5f);
	mouse_y = (int) ((float)mouse_y/scale_y - viewport.y + 0.5f);
	mouse_moved = (last_mouse_x != mouse_x || last_mouse_y != mouse_y);
}

rect_type explanation_rect = {170, 20, 200, 300};
int highlighted_setting_id = SETTING_ENABLE_INFO_SCREEN;
int controlled_area = 0; // Whether the focus is on the left (0) or right (1) part of the screen in the Settings menu.
int next_setting_id = 0; // For navigating up/down.
int previous_setting_id = 0;
bool at_scroll_up_boundary; // When navigating up using keyboard/controller, whether we also need to scroll up
bool at_scroll_down_boundary; // When navigating down using keyboard/controller, whether we also need to scroll down

void play_menu_sound(int sound_id) {
	play_sound(sound_id);
	play_next_sound();
}

void enter_settings_subsection(int settings_menu_id) {
	settings_area_type* settings_area = get_settings_area(settings_menu_id);
	if (active_settings_subsection != settings_menu_id) {
		highlighted_setting_id = settings_area->settings[0].id;
	}
	active_settings_subsection = settings_menu_id;
	highlighted_settings_subsection = settings_menu_id;
	if (!mouse_clicked) hovering_pause_menu_item = 0;
	controlled_area = 1;
	scroll_position = 0;

	// Special case: for the level customization submenu, the linked variables should depend on menu_current_level.
	// So we need to initialize them now.
	if (settings_menu_id == SETTINGS_MENU_LEVEL_CUSTOMIZATION) {
		for (int i = 0; i < settings_area->setting_count; ++i) {
			setting_type* setting = &settings_area->settings[i];
			switch(setting->id) {
				default: break;
				case SETTING_LEVEL_TYPE:
					setting->linked = &custom_saved.tbl_level_type[menu_current_level];
					break;
				case SETTING_LEVEL_COLOR:
					setting->linked = &custom_saved.tbl_level_color[menu_current_level];
					break;
				case SETTING_GUARD_TYPE:
					setting->linked = &custom_saved.tbl_guard_type[menu_current_level];
					break;
				case SETTING_GUARD_HP:
					setting->linked = &custom_saved.tbl_guard_hp[menu_current_level];
					break;
				case SETTING_CUTSCENE:
					setting->linked = &custom_saved.tbl_cutscenes_by_index[menu_current_level];
					break;
				case SETTING_ENTRY_POSE:
					setting->linked = &custom_saved.tbl_entry_pose[menu_current_level];
					break;
				case SETTING_SEAMLESS_EXIT:
					setting->linked = &custom_saved.tbl_seamless_exit[menu_current_level];
					break;
			}
		}
	}
}

void leave_settings_subsection(void) {
	if (active_settings_subsection == SETTINGS_MENU_LEVEL_CUSTOMIZATION) {
		enter_settings_subsection(SETTINGS_MENU_MODS);
	} else {
		// Go back to the top level of the settings menu.
		controlled_area = 0;
		hovering_pause_menu_item = active_settings_subsection;
		active_settings_subsection = 0;
		highlighted_settings_subsection = 0;
	}
}

void reset_paused_menu(void) {
	drawn_menu = 0;
	controlled_area = 0;
	hovering_pause_menu_item = PAUSE_MENU_RESUME;
}

void pause_menu_clicked(pause_menu_item_type* item) {
	//printf("Clicked option %s\n", item->text);
	play_menu_sound(sound_22_loose_shake_3);
	switch(item->id) {
		default: break;
		case PAUSE_MENU_RESUME:
			need_close_menu = true;
			break;
		case PAUSE_MENU_SAVE_GAME:
			// TODO: Manual save games?
			if (Kid.alive < 0) need_quick_save = 1;
			need_close_menu = true;
			break;
		case PAUSE_MENU_LOAD_GAME:
			// TODO: Manual save games?
			need_quick_load = 1;
			need_close_menu = true;
			stop_sounds();
			break;
		case PAUSE_MENU_RESTART_LEVEL:
			last_key_scancode = SDL_SCANCODE_A | WITH_CTRL;
			break;
		case PAUSE_MENU_SETTINGS:
			drawn_menu = 1;
			hovering_pause_menu_item = SETTINGS_MENU_GENERAL;
			highlighted_settings_subsection = SETTINGS_MENU_GENERAL;
			active_settings_subsection = 0;
			controlled_area = 0;
			break;
		case PAUSE_MENU_RESTART_GAME:
			last_key_scancode = SDL_SCANCODE_R | WITH_CTRL;
			break;
		case PAUSE_MENU_QUIT_GAME:
			current_dialog_box = DIALOG_CONFIRM_QUIT;
			current_dialog_text = "¿Salir del juego?";
			break;
		case SETTINGS_MENU_GENERAL:
		case SETTINGS_MENU_GAMEPLAY:
		case SETTINGS_MENU_VISUALS:
		case SETTINGS_MENU_MODS:
		case SETTINGS_MENU_CONTROLS:
			enter_settings_subsection(item->id);
			break;
		case SETTINGS_MENU_BACK:
			reset_paused_menu();
			hovering_pause_menu_item = PAUSE_MENU_SETTINGS;
			break;
	}
	clear_menu_controls(); // prevent "click-through" because the screen changes
}

void draw_pause_menu_item(pause_menu_item_type* item, rect_type* parent, int* y_offset, int inactive_text_color) {
	if (item->required != NULL) {
		if (*(sbyte*)item->required == false) {
			return; // skip this item (disabled)
		}
	}

	rect_type text_rect = *parent;
	text_rect.top += *y_offset;
	int text_color = inactive_text_color;

	rect_type selection_box = text_rect;
	selection_box.bottom = selection_box.top + 8;
	selection_box.top -= 3;

	bool highlighted = (hovering_pause_menu_item == item->id);
	if (have_mouse_input && is_mouse_over_rect(&selection_box)) {
		hovering_pause_menu_item = item->id;
		highlighted = true;
	}

	if (highlighted) {
		previous_pause_menu_item = item->previous;
		next_pause_menu_item = item->next;
		// Skip over disabled items (such as the CHEATS menu in non-cheat mode)
		if (previous_pause_menu_item->required != NULL) {
			while (*(sbyte*)previous_pause_menu_item->required == false) {
				previous_pause_menu_item = previous_pause_menu_item->previous;
				if (previous_pause_menu_item->required == NULL) break;
			}
		}
		if (next_pause_menu_item->required != NULL) {
			while (*(sbyte*)next_pause_menu_item->required == false) {
				next_pause_menu_item = next_pause_menu_item->next;
				if (next_pause_menu_item->required == NULL) break;
			}
		}
		text_color = color_15_brightwhite;
		draw_rect_contours(&selection_box, color_7_lightgray);

		if (mouse_clicked) {
			if (is_mouse_over_rect(&selection_box)) {
				pause_menu_clicked(item);
			}
		} else if (pressed_enter && (drawn_menu == 0 || (drawn_menu == 1 && controlled_area == 0))) {
			pause_menu_clicked(item);
		}

	}
	show_text_with_color(&text_rect, halign_center, valign_top, item->text, text_color);
	*y_offset += 13;

}

void draw_pause_menu(void) {
	pause_menu_alpha = 120;
	draw_rect_with_alpha(&screen_rect, color_0_black, pause_menu_alpha);
	draw_rect_with_alpha(&rect_bottom_text, color_0_black, 0); // Transparent so that the text "JUEGO EN PAUSA" is visible.
	rect_type pause_rect_outer = {0, 110, 192, 210};
	rect_type pause_rect_inner;
	shrink2_rect(&pause_rect_inner, &pause_rect_outer, 5, 5);

	if (!have_mouse_input) {
		if (menu_control_y == 1) {
			play_menu_sound(sound_21_loose_shake_2);
			hovering_pause_menu_item = next_pause_menu_item->id;
		} else if (menu_control_y == -1) {
			play_menu_sound(sound_21_loose_shake_2);
			hovering_pause_menu_item = previous_pause_menu_item->id;
		}
	}

	int y_offset = 50;
	for (int i = 0; i < COUNT(pause_menu_items); ++i) {
		draw_pause_menu_item(&pause_menu_items[i], &pause_rect_inner, &y_offset, color_15_brightwhite);
	}
}

bool were_settings_changed;

void turn_setting_on_off(int setting_id, byte new_state, void* linked) {
	were_settings_changed = true;
	switch(setting_id) {
		default:
			if (linked != NULL) {
				*(byte*)(linked) = new_state;
			}
			break;
		case SETTING_FULLSCREEN:
			start_fullscreen = new_state;
			SDL_SetWindowFullscreen(window_, (new_state != 0) * SDL_WINDOW_FULLSCREEN_DESKTOP);
			break;
		case SETTING_USE_CORRECT_ASPECT_RATIO:
			use_correct_aspect_ratio = new_state;
			apply_aspect_ratio();
			break;
		case SETTING_USE_INTEGER_SCALING:
			use_integer_scaling = new_state;
			if (new_state) {
				window_resized();
			} else {
#if SDL_VERSION_ATLEAST(2,0,5) // SDL_RenderSetIntegerScale
				SDL_RenderSetIntegerScale(renderer_, SDL_FALSE);
#endif
			}
			break;
#ifdef USE_LIGHTING
		case SETTING_ENABLE_LIGHTING:
			enable_lighting = new_state;
			if (new_state && lighting_mask == NULL) {
				init_lighting();
			}
			need_full_redraw = 1;
			break;
#endif
		case SETTING_ENABLE_SOUND:
			turn_sound_on_off((new_state != 0) * 15);
			break;
		case SETTING_ENABLE_MUSIC:
			turn_music_on_off(new_state);
			break;
		case SETTING_USE_FIXES_AND_ENHANCEMENTS:
			turn_fixes_and_enhancements_on_off(new_state);
			break;
		case SETTING_USE_CUSTOM_OPTIONS:
			turn_custom_options_on_off(new_state);
			break;
	}
}

void turn_setting_on_off_with_sound(setting_type* setting, byte new_state) {
	play_menu_sound(sound_10_sword_vs_sword);
	turn_setting_on_off(setting->id, new_state, setting->linked);

}

int get_setting_value(setting_type* setting) {
	int value = 0;
	if (setting->linked != NULL) {
		switch(setting->number_type) {
			default:
			case SETTING_BYTE:
				value = *(byte*) setting->linked;
				break;
			case SETTING_SBYTE:
				value = *(sbyte*) setting->linked;
				break;
			case SETTING_WORD:
				value = *(word*) setting->linked;
				break;
			case SETTING_SHORT:
				value = *(short*) setting->linked;
				break;
			case SETTING_INT:
				value = *(int*) setting->linked;
				break;
		}
	}
	return value;
}

void set_setting_value(setting_type* setting, int value) {
	if (setting->linked != NULL) {
		switch(setting->number_type) {
			default:
			case SETTING_BYTE:
				*(byte*) setting->linked = (byte) value;
				break;
			case SETTING_SBYTE:
				*(sbyte*) setting->linked = (sbyte) value;
				break;
			case SETTING_WORD:
				*(word*) setting->linked = (word) value;
				break;
			case SETTING_SHORT:
				*(short*) setting->linked = (short) value;
				break;
			case SETTING_INT:
				*(int*) setting->linked = value;
				break;
		}
	}
}

void increase_setting(setting_type* setting, int old_value) {
	int new_value;
	if (setting->id == SETTING_JOYSTICK_THRESHOLD) {
		new_value = ((old_value / 1000) + 1) * 1000; // Nearest higher multiple of 1000.
	} else {
		new_value = old_value + 1;
	}
	if (setting->linked != NULL && new_value <= setting->max) {
		were_settings_changed = true;
		set_setting_value(setting, new_value);
	}
}

void decrease_setting(setting_type* setting, int old_value) {
	int new_value;
	if (setting->id == SETTING_JOYSTICK_THRESHOLD) {
		new_value = (((old_value+999) / 1000) - 1) * 1000; // Nearest lower multiple of 1000.
	} else {
		new_value = old_value - 1;
	}
	if (setting->linked != NULL && new_value >= setting->min) {
		were_settings_changed = true;
		set_setting_value(setting, new_value);
	}
}


void draw_setting_explanation(setting_type* setting) {
	show_text_with_color(&explanation_rect, halign_center, valign_top, setting->explanation, color_7_lightgray);
}

void draw_image_with_blending(image_type* image, int xpos, int ypos) {
	SDL_Rect src_rect = {0, 0, image->w, image->h};
	SDL_Rect dest_rect = {xpos, ypos, image->w, image->h};
	SDL_SetColorKey(image, SDL_TRUE, 0);
	if (SDL_BlitSurface(image, &src_rect, current_target_surface, &dest_rect) != 0) {
		sdlperror("SDL_BlitSurface");
		quit(1);
	}
}

#define print_setting_value(setting, value) print_setting_value_(setting, value, alloca(32), 32)
char* print_setting_value_(setting_type* setting, int value, char* buffer, size_t buffer_size) {
	bool has_name = false;
	names_list_type* list = setting->names_list;
	size_t max_len = MIN(MAX_OPTION_VALUE_NAME_LENGTH, buffer_size);
	if (list != NULL) {
		if (list->type == 0 && value >= 0 && value < list->names.count) {
			strncpy(buffer, (*(list->names.data))[value], max_len);
			has_name = true;
		} else if (list->type == 1) {
			for (int i = 0; i < list->kv_pairs.count; ++i) {
				key_value_type* kv_pair = list->kv_pairs.data + i;
				if (value == kv_pair->value) {
					strncpy(buffer, kv_pair->key, max_len);
					has_name = true;
					break;
				}
			}
		}
	}
	if (!has_name) {
		if (setting->id == SETTING_START_TICKS_LEFT ||
				setting->id == SETTING_SHIFT_L_REDUCED_TICKS ||
				setting->id == SETTING_MOUSE_DELAY ||
				setting->id == SETTING_LOOSE_FLOOR_DELAY
		) {
			float seconds = (float)value * (1.0f/12.0f);
			snprintf(buffer, buffer_size, "%.2f", seconds);
		} else {
			snprintf(buffer, buffer_size, "%d", value);
		}
	}
	return buffer;
}

void draw_setting(setting_type* setting, rect_type* parent, int* y_offset, int inactive_text_color) {
	rect_type text_rect = *parent;
	text_rect.top += *y_offset;
	int text_color = inactive_text_color;
	int selected_color = color_15_brightwhite;
	int unselected_color = color_7_lightgray;

	rect_type setting_box = text_rect;
	setting_box.top -= 5;
	setting_box.bottom = setting_box.top + 15;
	setting_box.left -= 10;
	setting_box.right += 10;

	if (mouse_clicked && is_mouse_over_rect(&setting_box)) {
		highlighted_setting_id = setting->id;
		controlled_area = 1;
	}

	if (highlighted_setting_id == setting->id) {
		next_setting_id = setting->next;
		previous_setting_id = setting->previous;
		at_scroll_up_boundary = (setting->index == scroll_position);
		at_scroll_down_boundary = (setting->index == scroll_position + 8);

		SDL_Rect dest_rect;
		rect_to_sdlrect(&setting_box, &dest_rect);
		uint32_t rgb_color = SDL_MapRGBA(overlay_surface->format, 55, 55, 55, 255);
		if (SDL_FillRect(overlay_surface, &dest_rect, rgb_color) != 0) {
			sdlperror("draw_setting: SDL_FillRect");
			quit(1);
		}
		rect_type left_side_of_setting_box = setting_box;
		left_side_of_setting_box.left = setting_box.left - 2;
		left_side_of_setting_box.right = setting_box.left;
		draw_rect(&left_side_of_setting_box, color_15_brightwhite);
		draw_setting_explanation(setting);
	}

	bool disabled = false;
	if (setting->required != NULL) {
		disabled = !(*(byte*)setting->required);
	}
	if (disabled) {
		text_color = color_7_lightgray;
	}

	show_text_with_color(&text_rect, halign_left, valign_top, setting->text, text_color);

	if (setting->style == SETTING_STYLE_TOGGLE && !disabled) {
		bool setting_enabled = true;
		if (setting->linked != NULL) {
			setting_enabled = *(byte*)setting->linked;
		}

		// Toggling the setting: either by clicking on "SÍ" or "NO", or by pressing left/right.
		if (highlighted_setting_id == setting->id) {
			if (mouse_clicked) {
				if (!setting_enabled) {
					rect_type ON_hitbox = setting_box;
					ON_hitbox.left = setting_box.right - 22;
					if (is_mouse_over_rect(&ON_hitbox)) {
						turn_setting_on_off_with_sound(setting, 1);
						setting_enabled = false;
					}
				} else {
					rect_type OFF_hitbox = setting_box;
					OFF_hitbox.left = setting_box.right - 49;
					OFF_hitbox.right = setting_box.right - 22;
					if (is_mouse_over_rect(&OFF_hitbox)) {
						turn_setting_on_off_with_sound(setting, 0);
						setting_enabled = true;
					}
				}
			} else if (setting_enabled && menu_control_x < 0) {
				turn_setting_on_off_with_sound(setting, 0);
				setting_enabled = false;
			} else if (!setting_enabled && menu_control_x > 0) {
				turn_setting_on_off_with_sound(setting, 1);
				setting_enabled = true;
			}
		}

		int OFF_color = (setting_enabled) ? unselected_color : selected_color;
		int ON_color = (setting_enabled) ? selected_color : unselected_color;
		show_text_with_color(&text_rect, halign_right, valign_top, "SÍ", ON_color);
		text_rect.right -= 15;
		show_text_with_color(&text_rect, halign_right, valign_top, "NO", OFF_color);

	} else if (setting->style == SETTING_STYLE_NUMBER && !disabled) {
		int value = get_setting_value(setting);
		if (highlighted_setting_id == setting->id) {
			if (mouse_clicked) {

				rect_type right_hitbox = {setting_box.top, text_rect.right - 5, setting_box.bottom, text_rect.right + 10};
				if (is_mouse_over_rect(&right_hitbox)) {
					increase_setting(setting, value);
				} else {
					char* value_text = print_setting_value(setting, value);
					int value_text_width = get_line_width(value_text, (int)strlen(value_text));
					rect_type left_hitbox = right_hitbox;
					left_hitbox.left -= (value_text_width + 10);
					left_hitbox.right -= (value_text_width + 5);
					if (is_mouse_over_rect(&left_hitbox)) {
						decrease_setting(setting, value);
					}
				}

			} else if (menu_control_x > 0) {
				increase_setting(setting, value);
			} else if (menu_control_x < 0) {
				decrease_setting(setting, value);
			}
		}

		value = get_setting_value(setting); // May have been updated.
		char* value_text = print_setting_value(setting, value);
		show_text_with_color(&text_rect, halign_right, valign_top, value_text, selected_color);

		if (highlighted_setting_id == setting->id) {
			int value_text_width = get_line_width(value_text, (int)strlen(value_text));
			draw_image_with_blending(arrowhead_right_image, text_rect.right + 2, text_rect.top);
			draw_image_with_blending(arrowhead_left_image, text_rect.right - value_text_width - 6, text_rect.top);
		}

	} else if (setting->style == SETTING_STYLE_KEY && !disabled) {
		int value = get_setting_value(setting);
		char value_text[256];
		snprintf(value_text, sizeof(value_text), "%s (%d)", SDL_GetScancodeName(value), value);
		show_text_with_color(&text_rect, 1, -1, value_text, selected_color);

	} else {
		// show text only
		if (highlighted_setting_id == setting->id && (setting->required == NULL || *(sbyte*)setting->required != 0)) {
			if (pressed_enter || (mouse_clicked && is_mouse_over_rect(&setting_box))) {
				if (setting->id == SETTING_RESET_ALL_SETTINGS) {
					play_menu_sound(sound_22_loose_shake_3);
					current_dialog_box = DIALOG_RESTORE_DEFAULT_SETTINGS;
					current_dialog_text = "¿Restaurar todas las opciones a sus valores originales?";
				} else if (setting->id == SETTING_LEVEL_SETTINGS) {
					play_menu_sound(sound_22_loose_shake_3);
					current_dialog_box = DIALOG_SELECT_LEVEL;
				}
			}

		}
	}

	*y_offset += 15;
}

// Handle the redefining of keyboard controls separately from drawing the menu.
// Otherwise the menu items under the current one will not be drawn until the dialog is closed.
void handle_setting(setting_type* setting, rect_type* parent, int* y_offset, int inactive_text_color) {
	rect_type text_rect = *parent;
	text_rect.top += *y_offset;

	rect_type setting_box = text_rect;
	setting_box.top -= 5;
	setting_box.bottom = setting_box.top + 15;
	setting_box.left -= 10;
	setting_box.right += 10;

	bool disabled = false;
	if (setting->required != NULL) {
		disabled = !(*(byte*)setting->required);
	}

	if (setting->style == SETTING_STYLE_KEY && !disabled) {
		if (highlighted_setting_id == setting->id) {
			if (pressed_enter || (mouse_clicked && is_mouse_over_rect(&setting_box))) {
				int value_before = get_setting_value(setting);
				redefine_key(setting->text, setting->linked);
				int value_after = get_setting_value(setting);
				// This should be in redefine_key()?
				if (value_before != value_after) were_settings_changed = true;
			}
		}
	}

	*y_offset += 15;
}

void menu_scroll(int y) {
	settings_area_type* current_settings_area = get_settings_area(active_settings_subsection);
	if (current_settings_area != NULL) {
		int max_scroll = MAX(0, current_settings_area->setting_count - 9);
		if (drawn_menu == 1 && controlled_area == 1) {
			if (y < 0 && scroll_position > 0) {
				--scroll_position;
			} else if (y > 0 && scroll_position < max_scroll) {
				++scroll_position;
			}
		}
	}
}

void draw_settings_area(settings_area_type* settings_area) {
	if (settings_area == NULL) return;
	rect_type settings_area_rect = {0, 80, 170, 320};
	shrink2_rect(&settings_area_rect, &settings_area_rect, 20, 20);

	int start_y_offset = 0;

	// The MODS subsection with level specific settings is a special case:
	// We want to display which level we are editing, and start drawing slightly lower.
	if (active_settings_subsection == SETTINGS_MENU_LEVEL_CUSTOMIZATION) {
		start_y_offset = 15;
		char level_text[16];
		snprintf(level_text, sizeof(level_text), "NIVEL %d", menu_current_level);
		show_text_with_color(&settings_area_rect, halign_center, valign_top, level_text, color_15_brightwhite);
	}

	int y_offset = start_y_offset;
	int num_drawn_settings = 0;
	for (int i = 0; (i < settings_area->setting_count) && (num_drawn_settings < 9); ++i) {
		if (i >= scroll_position) {
			++num_drawn_settings;
			draw_setting(&settings_area->settings[i], &settings_area_rect, &y_offset, color_15_brightwhite);
		}
	}

	y_offset = start_y_offset;
	num_drawn_settings = 0;
	for (int i = 0; (i < settings_area->setting_count) && (num_drawn_settings < 9); ++i) {
		if (i >= scroll_position) {
			++num_drawn_settings;
			handle_setting(&settings_area->settings[i], &settings_area_rect, &y_offset, color_15_brightwhite);
		}
	}

	if (scroll_position > 0) {
		draw_image_with_blending(arrowhead_up_image, 200, 10);
	}
	if (scroll_position + num_drawn_settings < settings_area->setting_count) {
		draw_image_with_blending(arrowhead_down_image, 200, 151);
	}

	// Draw a scroll bar if needed.
	// It's not clickable yet, it just shows where you are in the list.
	if (num_drawn_settings < settings_area->setting_count) {
		const int scrollbar_width = 2;
		rect_type scrollbar_rect = {
			.top = settings_area_rect.top - 5, .bottom = settings_area_rect.bottom,
			.left = settings_area_rect.right + 10 - scrollbar_width, .right = settings_area_rect.right + 10
		};
		method_5_rect(&scrollbar_rect, blitters_0_no_transp, color_8_darkgray);

		int scrollbar_height = scrollbar_rect.bottom - scrollbar_rect.top;
		rect_type scrollbar_slider_rect = {
			.top = scrollbar_rect.top + scroll_position * scrollbar_height / settings_area->setting_count,
			.bottom = scrollbar_rect.top + (scroll_position + num_drawn_settings) * scrollbar_height / settings_area->setting_count,
			.left = scrollbar_rect.left, .right = scrollbar_rect.right
		};
		method_5_rect(&scrollbar_slider_rect, blitters_0_no_transp, color_7_lightgray);
	}
}

void draw_settings_menu(void) {
	settings_area_type* settings_area = get_settings_area(active_settings_subsection);
	pause_menu_alpha = (settings_area == NULL) ? 220 : 255;
	draw_rect_with_alpha(&screen_rect, color_0_black, pause_menu_alpha);

	rect_type pause_rect_outer = {0, 10, 192, 80};
	rect_type pause_rect_inner;
	shrink2_rect(&pause_rect_inner, &pause_rect_outer, 5, 5);

	if (!have_mouse_input) {
		bool hovering_item_changed = false;
		if (controlled_area == 0) {
			int old_hovering_item_id = hovering_pause_menu_item;
			if (menu_control_y == 1) {
				hovering_pause_menu_item = next_pause_menu_item->id;
			} else if (menu_control_y == -1) {
				hovering_pause_menu_item = previous_pause_menu_item->id;
			}
			if (old_hovering_item_id != hovering_pause_menu_item) {
				hovering_item_changed = true;
			}
		} else if (controlled_area == 1) {
			// settings area
			int old_highlighted_setting_id = highlighted_setting_id;

			// Why does the global variable contain the ID instead of the index?...
			// Find the index from the ID.
			settings_area_type* current_settings_area = get_settings_area(active_settings_subsection);
			int highlighted_setting_index = -1;
			for (int i = 0; i < current_settings_area->setting_count; i++) {
				if (highlighted_setting_id == current_settings_area->settings[i].id) {
					highlighted_setting_index = i;
					break;
				}
			}

			int last = current_settings_area->setting_count - 1;
			int max_scroll = MAX(0, current_settings_area->setting_count - 9);

			if (menu_control_y > 0) {
				// DOWN
				highlighted_setting_index += menu_control_y;
				if (highlighted_setting_index > last) highlighted_setting_index = last;

				// With Page Down, try to leave the selection in the same row visually.
				if (menu_control_y > +1) scroll_position += menu_control_y;

			} else if (menu_control_y < 0) {
				// UP
				highlighted_setting_index += menu_control_y;
				if (highlighted_setting_index < 0) highlighted_setting_index = 0;

				// With Page Up, try to leave the selection in the same row visually.
				if (menu_control_y < -1) scroll_position += menu_control_y;

			}

			if (menu_control_y != 0) {
				// We check both directions in both cases, to scroll the highlighted row back into sight even if the user scrolled it out of sight (with the mouse wheel).
				if (highlighted_setting_index - 8 > scroll_position) scroll_position = highlighted_setting_index - 8;
				if (highlighted_setting_index < scroll_position) scroll_position = highlighted_setting_index;
				if (scroll_position > max_scroll) scroll_position = max_scroll;
				if (scroll_position < 0) scroll_position = 0;
			}

			// Find the ID from the index.
			highlighted_setting_id = current_settings_area->settings[highlighted_setting_index].id;

			if (old_highlighted_setting_id != highlighted_setting_id) {
				hovering_item_changed = true;
			}
		}
		if (hovering_item_changed) {
			play_menu_sound(sound_21_loose_shake_2);
		}
	}

	int y_offset = 50;
	for (int i = 0; i < COUNT(settings_menu_items); ++i) {
		pause_menu_item_type* item = &settings_menu_items[i];
		int text_color = (highlighted_settings_subsection == item->id) ? color_15_brightwhite : color_7_lightgray;
		draw_pause_menu_item(&settings_menu_items[i], &pause_rect_inner, &y_offset, text_color);
	}

	draw_settings_area(settings_area);
}

enum dialog_button_ids {
	DIALOG_BUTTON_CANCEL,
	DIALOG_BUTTON_OK,
};

void confirmation_dialog_result(int which_dialog, int button) {
	if (button == DIALOG_BUTTON_OK) {
		if (which_dialog == DIALOG_RESTORE_DEFAULT_SETTINGS) {
			play_menu_sound(sound_10_sword_vs_sword);
			were_settings_changed = true;
			set_options_to_default();
			turn_setting_on_off(SETTING_USE_INTEGER_SCALING, use_integer_scaling, NULL);
#ifdef USE_LIGHTING
			turn_setting_on_off(SETTING_ENABLE_LIGHTING, enable_lighting, NULL);
#endif
			apply_aspect_ratio();
			turn_sound_on_off((is_sound_on != 0) * 15);
			turn_music_on_off(enable_music);
		} else if (which_dialog == DIALOG_CONFIRM_QUIT) {
			last_key_scancode = SDL_SCANCODE_Q | WITH_CTRL;
			key_test_quit();
		}
	} else {
		play_menu_sound(sound_22_loose_shake_3);
	}
}

rect_type cancel_text_rect = {104, 162,  118,  212};
rect_type cancel_highlight_rect = {103, 162,  116,  212};
rect_type ok_text_rect = {104, 108,  118,  158};
rect_type ok_highlight_rect = {103, 108,  116,  158};

void draw_confirmation_dialog(int which_dialog, const char* text) {
	int highlighted_button = DIALOG_BUTTON_OK;
	int old_highlighted_button = -1;
	for (;;) {
		process_events();
		key_test_paused_menu(key_test_quit());
		process_additional_menu_input();

		if (menu_control_back == 1) {
			confirmation_dialog_result(which_dialog, DIALOG_BUTTON_CANCEL);
			break;
		}

		if (have_mouse_input) {
			if (is_mouse_over_rect(&ok_highlight_rect)) {
				highlighted_button = DIALOG_BUTTON_OK;
			} else if (is_mouse_over_rect(&cancel_highlight_rect)) {
				highlighted_button = DIALOG_BUTTON_CANCEL;
			}
		}

		if (menu_control_x < 0) {
			highlighted_button = DIALOG_BUTTON_OK;
		} else if (menu_control_x > 0) {
			highlighted_button = DIALOG_BUTTON_CANCEL;
		} else if (mouse_clicked || pressed_enter) {
			confirmation_dialog_result(which_dialog, highlighted_button);
			break;
		}

		if (highlighted_button != old_highlighted_button) {
			old_highlighted_button = highlighted_button;
			// Need to redraw the dialog box.
			uint32_t clear_color = SDL_MapRGBA(current_target_surface->format, 0, 0, 0, 255);
			SDL_FillRect(overlay_surface, NULL, clear_color);
			draw_rect(&copyprot_dialog->peel_rect, color_0_black);
			dialog_method_2_frame(copyprot_dialog);
			rect_type rect;
			shrink2_rect(&rect, &copyprot_dialog->text_rect, 2, 1);
			rect.bottom -= 14;
			show_text_with_color(&rect, halign_center, valign_middle, text, color_15_brightwhite);
			clear_kbd_buf();

			rect_type* highlight_rect;
			int ok_text_color, cancel_text_color;
			if (highlighted_button == DIALOG_BUTTON_OK) {
				highlight_rect = &ok_highlight_rect;
				ok_text_color = color_15_brightwhite;
				cancel_text_color = color_7_lightgray;
			} else {
				highlight_rect = &cancel_highlight_rect;
				ok_text_color = color_7_lightgray;
				cancel_text_color = color_15_brightwhite;
			}
			draw_rect(highlight_rect, color_8_darkgray);
			show_text_with_color(&ok_text_rect, halign_center, valign_middle, "Aceptar", ok_text_color);
			show_text_with_color(&cancel_text_rect, halign_center, valign_middle, "Cancelar", cancel_text_color);
			update_screen();
		}

		SDL_Delay(1); // Prevent 100% cpu usage.

	}
	current_dialog_box = 0;
	clear_menu_controls();
}

void draw_select_level_dialog(void) {
	clear_menu_controls();
	int old_edited_level_number = -1;
	for (;;) {
		process_events();
		key_test_paused_menu(key_test_quit());
		process_additional_menu_input();

		if (menu_control_back == 1) {
			menu_control_back = 0;
			play_menu_sound(sound_22_loose_shake_3);
			break;
		}

		if (menu_control_x < 0) {
			menu_current_level = MAX(0, menu_current_level - 1);
		} else if (menu_control_x > 0) {
			menu_current_level = MIN(15, menu_current_level + 1);
		} else if (mouse_clicked || pressed_enter) {
			enter_settings_subsection(SETTINGS_MENU_LEVEL_CUSTOMIZATION);
			highlighted_settings_subsection = SETTINGS_MENU_MODS;
			play_menu_sound(sound_22_loose_shake_3);
			break;
		}

		if (menu_current_level != old_edited_level_number) {
			font_type* saved_font = textstate.ptr_font;
			textstate.ptr_font = &hc_font;

			old_edited_level_number = menu_current_level;
			// Need to redraw the dialog box.
			uint32_t clear_color = SDL_MapRGBA(current_target_surface->format, 0, 0, 0, 255);
			SDL_FillRect(overlay_surface, NULL, clear_color);
			draw_rect(&copyprot_dialog->peel_rect, color_0_black);
			dialog_method_2_frame(copyprot_dialog);
			rect_type rect;
			shrink2_rect(&rect, &copyprot_dialog->text_rect, 2, 1);
			rect.bottom -= 14;
			show_text_with_color(&rect, halign_center, valign_middle, "Personalizar nivel...", color_15_brightwhite);
			clear_kbd_buf();
			rect_type input_rect = {104,   64,  118,  256};
			char level_text[8];
			snprintf(level_text, sizeof(level_text), "%d", menu_current_level);
			show_text_with_color(&input_rect, halign_center, valign_middle, level_text, color_15_brightwhite);
			draw_image_with_blending(arrowhead_right_image, 175, input_rect.top + 3);
			draw_image_with_blending(arrowhead_left_image, 145 - 3, input_rect.top + 3);

			update_screen();
			textstate.ptr_font = saved_font;
		}

		SDL_Delay(1); // Prevent 100% cpu usage.

	}
	clear_menu_controls();
}

int need_full_menu_redraw_count;

void draw_menu() {
	escape_key_suppressed = (key_states[SDL_SCANCODE_BACKSPACE] & KEYSTATE_HELD || key_states[SDL_SCANCODE_ESCAPE] & KEYSTATE_HELD);
	surface_type* saved_target_surface = current_target_surface;
	current_target_surface = overlay_surface;

	need_close_menu = false;
	while (!need_close_menu) {
		clear_menu_controls();
		process_events();
		if (process_key() != 0) {
			break; // Menu was forcefully closed, for example by pressing Ctrl+A.
		}
		process_additional_menu_input();

		if (current_dialog_box != DIALOG_NONE) {
			if (current_dialog_box == DIALOG_SELECT_LEVEL) {
				draw_select_level_dialog();
			} else {
				draw_confirmation_dialog(current_dialog_box, current_dialog_text);
			}
			current_dialog_box = DIALOG_NONE;
			clear_menu_controls();
		}

		if (is_menu_shown == 1) {
			is_menu_shown = -1; // reset the menu if the menu is drawn for the first time
			need_full_menu_redraw_count = 2;
			reset_paused_menu();
		}
		if (menu_control_back == 1) {
			play_menu_sound(sound_22_loose_shake_3);
			if (drawn_menu == 1) {
				if (controlled_area == 1) {
					leave_settings_subsection();
				} else {
					reset_paused_menu(); // Go back to the top level pause menu.
					hovering_pause_menu_item = PAUSE_MENU_SETTINGS;
				}
			} else {
				break; // Close the menu.
			}
		}

		if (menu_control_scroll_y != 0) {
			menu_scroll(menu_control_scroll_y);
		}

		if (have_mouse_input || have_keyboard_or_controller_input) {
			// The menu is updated+drawn within the same routine, so redrawing may be necessary after the first time.
			// TODO: Maybe in the future fully separate updating from drawing?
			need_full_menu_redraw_count = 2;
		} else {
			if (need_full_menu_redraw_count == 0) {
				SDL_Delay(1);
				continue; // Don't redraw if there is no input to process (save CPU cycles).
			}
		}

//		Uint64 begin = SDL_GetPerformanceCounter();
		font_type* saved_font = textstate.ptr_font;
		textstate.ptr_font = &hc_small_font;
		if (drawn_menu == 0) {
			draw_pause_menu();
		} else if (drawn_menu == 1) {
			draw_settings_menu();
		}
		textstate.ptr_font = saved_font;
		if (!need_close_menu) {
			update_screen();
		}
//		printf("Drawing the menu took %.2f ms.\n", (SDL_GetPerformanceCounter() - begin) * milliseconds_per_counter);

		--need_full_menu_redraw_count;
	}

	current_target_surface = saved_target_surface;
}

void clear_menu_controls() {
	pressed_enter = 0;
	mouse_moved = 0;
	mouse_clicked = 0;
	mouse_button_clicked_right = 0;
	have_mouse_input = 0;
	have_keyboard_or_controller_input = 0;
	menu_control_x = 0;
	menu_control_y = 0;
	menu_control_back = 0;
	menu_control_scroll_y = 0;
}

void process_additional_menu_input() {
	read_mouse_state();
	have_keyboard_or_controller_input = (menu_control_x || menu_control_y || menu_control_back || pressed_enter);
	have_mouse_input = (mouse_moved || mouse_clicked || mouse_button_clicked_right || menu_control_scroll_y);

	dword flags = SDL_GetWindowFlags(window_);
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		if (have_mouse_input) {
			SDL_ShowCursor(SDL_ENABLE);
		} else if (have_keyboard_or_controller_input) {
			SDL_ShowCursor(SDL_DISABLE);
		}
	} else {
		SDL_ShowCursor(SDL_ENABLE);
	}
}

bool joy_ABXY_buttons_released;
bool joy_xy_released;
Uint64 joy_xy_timeout_counter;

int key_test_paused_menu(int key) {
	menu_control_x = 0;
	menu_control_y = 0;
	menu_control_back = 0;

	if (mouse_button_clicked_right) {
		menu_control_back = 1; // Can use RMB to close menus.
	}

	if (is_joyst_mode) {
		int joy_x = 0;
		int joy_y = 0;
		if (joy_button_states[JOYINPUT_DPAD_LEFT] & KEYSTATE_HELD)
			joy_x = -1;
		else if (joy_button_states[JOYINPUT_DPAD_RIGHT] & KEYSTATE_HELD)
			joy_x = 1;
		if (joy_button_states[JOYINPUT_DPAD_UP] & KEYSTATE_HELD)
			joy_y = -1;
		else if (joy_button_states[JOYINPUT_DPAD_DOWN] & KEYSTATE_HELD)
			joy_y = 1;
		int y_threshold = 14000;
		int x_threshold = 26000; // Less sensitive, to prevent accidentally changing a setting.
		if (joy_axis[SDL_CONTROLLER_AXIS_LEFTY] < -y_threshold) {
			joy_y = -1;
		} else if (joy_axis[SDL_CONTROLLER_AXIS_LEFTY] > y_threshold) {
			joy_y = 1;
		} else if (joy_axis[SDL_CONTROLLER_AXIS_LEFTX] < -x_threshold) {
			joy_x = -1;
		} else if (joy_axis[SDL_CONTROLLER_AXIS_LEFTX] > x_threshold) {
			joy_x = 1;
		}

		float needed_timeout_s = 0.1f; // Delay for hold-down repeated input.
		if (joy_x == 0 && joy_y == 0) {
			joy_xy_released = true;
			joy_xy_timeout_counter = 0;
		} else {
			if (joy_xy_released) {
				needed_timeout_s = 0.3f; // The delay is longer for the first repetition.
				joy_xy_released = false;
			}
			Uint64 current_counter = SDL_GetPerformanceCounter();
			if (current_counter > joy_xy_timeout_counter) {
				menu_control_x = joy_x;
				menu_control_y = joy_y;
				joy_xy_timeout_counter = current_counter + (Uint64)((float)SDL_GetPerformanceFrequency() * needed_timeout_s);
				return 0; // cancel other input.
			}
		}

		if (!(joy_button_states[JOYINPUT_A] & KEYSTATE_HELD) && !(joy_button_states[JOYINPUT_Y] & KEYSTATE_HELD) && !(joy_button_states[JOYINPUT_B] & KEYSTATE_HELD)) {
			joy_ABXY_buttons_released = true;
		} else if (joy_ABXY_buttons_released) {
			joy_ABXY_buttons_released = false;
			if (joy_button_states[JOYINPUT_A] & KEYSTATE_HELD) {
				key = SDL_SCANCODE_RETURN;
				joy_button_states[JOYINPUT_A] = 0; // Prevent 'down' input being passed to the controls if the game is unpaused.
			} else if (joy_button_states[JOYINPUT_B] & KEYSTATE_HELD) {
				key = SDL_SCANCODE_ESCAPE;
			}
		}
	}

	// remap
	if (key == key_up) key = SDL_SCANCODE_UP; else
	if (key == key_down) key = SDL_SCANCODE_DOWN; else
	if (key == key_left) key = SDL_SCANCODE_LEFT; else
	if (key == key_right) key = SDL_SCANCODE_RIGHT; else
	if (key == key_enter) key = SDL_SCANCODE_RETURN; else
	if (key == key_esc) key = SDL_SCANCODE_ESCAPE; else
	/*nothing*/;

	switch(key) {
		default:
			if (key & WITH_CTRL) {
				need_close_menu = true;
				return key; // Allow Ctrl+R, etc.
			} else {
				break;
			}
		case SDL_SCANCODE_UP:
			menu_control_y = -1;
			break;
		case SDL_SCANCODE_DOWN:
			menu_control_y = 1;
			break;
		case SDL_SCANCODE_PAGEUP:
			menu_control_y = -9;
			break;
		case SDL_SCANCODE_PAGEDOWN:
			menu_control_y = +9;
			break;
		case SDL_SCANCODE_HOME:
			menu_control_y = -1000;
			break;
		case SDL_SCANCODE_END:
			menu_control_y = +1000;
			break;
		case SDL_SCANCODE_RIGHT:
			menu_control_x = 1;
			break;
		case SDL_SCANCODE_LEFT:
			menu_control_x = -1;
			break;
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_SPACE:
			pressed_enter = 1;
			break;
		case SDL_SCANCODE_ESCAPE:
		case SDL_SCANCODE_BACKSPACE:
			menu_control_back = 1;
			break;
		case SDL_SCANCODE_F6:
		case SDL_SCANCODE_F6 | WITH_SHIFT:
			if (Kid.alive < 0) need_quick_save = 1;
			need_close_menu = true;
			break;
		case SDL_SCANCODE_F9:
		case SDL_SCANCODE_F9 | WITH_SHIFT:
			need_quick_load = 1;
			need_close_menu = true;
			break;
	}
	return 0;
}

typedef int rw_process_func_type(SDL_RWops* rw, void* data, size_t data_size);

// For serializing/unserializing options in the in-game settings menu
#define process(x) if (!process_func(rw, &(x), sizeof(x))) return
void process_ingame_settings_user_managed(SDL_RWops* rw, rw_process_func_type process_func) {
	process(enable_pause_menu);
	process(enable_info_screen);
	process(is_sound_on);
	process(enable_music);
	process(enable_controller_rumble);
	process(joystick_threshold);
	process(joystick_only_horizontal);
	process(enable_replay);
	process(start_fullscreen);
	process(use_hardware_acceleration);
	process(use_correct_aspect_ratio);
	process(use_integer_scaling);
	process(scaling_type);
	process(enable_fade);
	process(enable_flash);
#ifdef USE_LIGHTING
	process(enable_lighting);
#endif
	// TODO: put these into a single variable?
	process(key_left      );
	process(key_right     );
	process(key_up        );
	process(key_down      );
	process(key_jump_left );
	process(key_jump_right);
	process(key_action    );
	process(key_enter     );
	process(key_esc       );
}

void process_ingame_settings_mod_managed(SDL_RWops* rw, rw_process_func_type process_func) {
	process(enable_copyprot);
	process(enable_quicksave);
	process(enable_quicksave_penalty);
	process(use_fixes_and_enhancements);
	process(fixes_saved);
	process(use_custom_options);
	process(custom_saved);
}
#undef process

// CRC-32 implementation adapted from:
// https://web.archive.org/web/20190108202303/http://www.hackersdelight.org/hdcodetxt/crc.c.txt
unsigned int crc32c(unsigned char *message, size_t size) {
	int i, j;
	unsigned int byte, crc, mask;
	static unsigned int table[256];

	/* Set up the table, if necessary. */
	if (table[1] == 0) {
		for (byte = 0; byte <= 255; byte++) {
			crc = byte;
			for (j = 7; j >= 0; j--) {    // Do eight times.
				mask = -(crc & 1);
				crc = (crc >> 1) ^ (0xEDB88320 & mask);
			}
			table[byte] = crc;
		}
	}

	/* Through with table setup, now calculate the CRC. */
	i = 0;
	crc = 0xFFFFFFFF;
	while (size--) {
		byte = message[i];
		crc = (crc >> 8) ^ table[(crc ^ byte) & 0xFF];
		i = i + 1;
	}
	return ~crc;
}

dword exe_crc = 0;

void calculate_exe_crc(void) {
	if (exe_crc == 0) {
		// Get the CRC32 fingerprint of the executable.
		FILE* exe_file = fopen(g_argv[0], "rb");
		if (exe_file != NULL) {
			fseek(exe_file, 0, SEEK_END);
			size_t size = ftell(exe_file);
			fseek(exe_file, 0, SEEK_SET);
			if (size > 0) {
				byte* buffer = malloc(size);
				size_t bytes = fread(buffer, 1, size, exe_file);
				if (bytes != size) {
					fprintf(stderr, "exec changed size during CRC32!?\n");
					size = bytes;
				}
				exe_crc = crc32c(buffer, size);
				free(buffer);
			}
			fclose(exe_file);
		}
	}
}

void save_ingame_settings(void) {
	SDL_RWops* rw = SDL_RWFromFile(locate_save_file("SDLPoP.cfg"), "wb");
	if (rw != NULL) {
		calculate_exe_crc();
		SDL_RWwrite(rw, &exe_crc, sizeof(exe_crc), 1);
		byte levelset_name_length = (byte)strnlen(levelset_name, UINT8_MAX);
		SDL_RWwrite(rw, &levelset_name_length, sizeof(levelset_name_length), 1);
		SDL_RWwrite(rw, levelset_name, levelset_name_length, 1);
		process_ingame_settings_user_managed(rw, process_rw_write);
		process_ingame_settings_mod_managed(rw, process_rw_write);
		SDL_RWclose(rw);
	}
}

void load_ingame_settings(void) {
	// We want the SDLPoP.cfg file (in-game menu settings) to override the SDLPoP.ini file,
	// but ONLY if the .ini file wasn't modified since the last time the .cfg file was saved!
	struct stat st_ini, st_cfg;
	const char* cfg_filename = locate_file("SDLPoP.cfg");
	const char* ini_filename = locate_file("SDLPoP.ini");
	if (stat( cfg_filename, &st_cfg ) == 0 && stat( ini_filename, &st_ini ) == 0) {
		if (st_ini.st_mtime > st_cfg.st_mtime ) {
			// SDLPoP.ini is newer than SDLPoP.cfg, so just go with the .ini configuration
			return;
		}
	}
	// If there is a SDLPoP.cfg file, let it override the settings
	SDL_RWops* rw = SDL_RWFromFile(cfg_filename, "rb");
	if (rw != NULL) {
		// SDLPoP.cfg should be invalidated if the prince executable changes.
		// This allows us not to worry about future and backward compatibility of this file.
		calculate_exe_crc();
		dword expected_crc = 0;
		SDL_RWread(rw, &expected_crc, sizeof(expected_crc), 1);
//		printf("CRC-32: exe = %x, expected = %x\n", exe_crc, expected_crc);
		if (exe_crc == expected_crc) {
			byte cfg_levelset_name_length;
			char cfg_levelset_name[256] = {0};
			SDL_RWread(rw, &cfg_levelset_name_length, sizeof(cfg_levelset_name_length), 1);
			SDL_RWread(rw, cfg_levelset_name, cfg_levelset_name_length, 1);
//			printf("%s, %s\n", cfg_levelset_name, levelset_name);
			process_ingame_settings_user_managed(rw, process_rw_read); // Load the settings.
			// For mod-managed settings: discard the CFG settings when switching to different mod.
			if (strncmp(levelset_name, cfg_levelset_name, 256) == 0) {
				process_ingame_settings_mod_managed(rw, process_rw_read);
			}
		}
		SDL_RWclose(rw);
	}
}

void menu_was_closed(void) {
	is_paused = 0;
	is_menu_shown = 0;
	escape_key_suppressed = (key_states[SDL_SCANCODE_BACKSPACE] & KEYSTATE_HELD || key_states[SDL_SCANCODE_ESCAPE] & KEYSTATE_HELD);
	if (were_settings_changed) {
		save_ingame_settings();
		were_settings_changed = false;
	}
	// In fullscreen mode, hide the mouse cursor (because it is only needed in the menu).
	dword flags = SDL_GetWindowFlags(window_);
	if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
		SDL_ShowCursor(SDL_DISABLE);
	} else {
		SDL_ShowCursor(SDL_ENABLE);
	}
}



// Small font (hardcoded).
// The alphanumeric characters were adapted from the freeware font '04b_03' by Yuji Oshimoto. See: http://www.04.jp.org/

#define BINARY_8(b7,b6,b5,b4,b3,b2,b1,b0) ((b0) | ((b1)<<1) | ((b2)<<2) | ((b3)<<3) | ((b4)<<4) | ((b5)<<5) | ((b6)<<6) | ((b7)<<7))
#define BINARY_4(b7,b6,b5,b4) (((b4)<<4) | ((b5)<<5) | ((b6)<<6) | ((b7)<<7))
#define _ 0
#define WORD(x) (byte)(x), (byte)((x)>>8)
#define IMAGE_DATA(height, width, flags) WORD(height), WORD(width), WORD(flags)

byte hc_small_font_data[] = {

		32, 255, WORD(5), WORD(2), WORD(1), WORD(1),

		// offsets (will be initialized at run-time)
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 41
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 51
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 61
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 71
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 81
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 91
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 101
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 111
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 121
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), // 126
		// offsets 127..255 (agregados para el español)
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 
		WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), WORD(0), 

		IMAGE_DATA(1, 3, 1), // space
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 1, 1), // !
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 3, 1), // "
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 5, 1), // #
		BINARY_8( _,1,_,1,_,_,_,_ ),
		BINARY_8( 1,1,1,1,1,_,_,_ ),
		BINARY_8( _,1,_,1,_,_,_,_ ),
		BINARY_8( 1,1,1,1,1,_,_,_ ),
		BINARY_8( _,1,_,1,_,_,_,_ ),

		IMAGE_DATA(6, 3, 1), // $
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 6, 1), // %
		BINARY_8( _,_,_,_,_,_,_,_ ),
		BINARY_8( 1,1,_,_,1,_,_,_ ),
		BINARY_8( 1,1,_,1,_,_,_,_ ),
		BINARY_8( _,_,1,_,1,1,_,_ ),
		BINARY_8( _,1,_,_,1,1,_,_ ),

		IMAGE_DATA(5, 5, 1), // &
		BINARY_8( _,1,1,_,_,_,_,_ ),
		BINARY_8( _,1,1,_,_,_,_,_ ),
		BINARY_8( 1,1,1,_,1,_,_,_ ),
		BINARY_8( 1,_,_,1,_,_,_,_ ),
		BINARY_8( _,1,1,_,1,_,_,_ ),

		IMAGE_DATA(2, 1, 1), // '
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 3, 1), // (
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 3, 1), // )
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(4, 5, 1),
		BINARY_8( _,_,_,_,_,_,_,_ ), // *
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( _,1,1,1,_,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),

		IMAGE_DATA(4, 3, 1), // +
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(6, 2, 1), // ,
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(3, 3, 1), // -
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 1, 1), // .
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // /
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 0
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 2, 1), // 1
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 4, 1), // 2
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // 3
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // 4
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( _,_,1,_ ),

		IMAGE_DATA(5, 4, 1), // 5
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // 6
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // 7
		BINARY_4( 1,1,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 4, 1), // 8
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // 9
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 1, 1), // :
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(6, 2, 1), // ;
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 3, 1), // <
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,1,_ ),

		IMAGE_DATA(4, 3, 1), // =
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // >
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // ?
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,1,_ ),

		IMAGE_DATA(6, 4, 1), // @
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // A
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // B
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // C
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // D
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // E
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // F
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // G
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // H
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 3, 1), // I
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // J
		BINARY_4( _,_,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // K
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // L
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),

		IMAGE_DATA(5, 5, 1), // M
		BINARY_8( 1,_,_,_,1,_,_,_ ),
		BINARY_8( 1,1,_,1,1,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( 1,_,_,_,1,_,_,_ ),
		BINARY_8( 1,_,_,_,1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // N
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,_,1 ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // O
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // P
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(6, 4, 1), // Q
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( _,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // R
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // S
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 3, 1), // T
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 4, 1), // U
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // V
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 5, 1),
		BINARY_8( 1,_,_,_,1,_,_,_ ), // W
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( _,1,_,1,_,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // X
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // Y
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 3, 1), // Z
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 2, 1), // [
		BINARY_4( 1,1,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,_,_ ),

		IMAGE_DATA(5, 4, 1), // '\'
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // ]
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,_,_ ),

		IMAGE_DATA(2, 3, 1), // ^
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,1,_ ),

		IMAGE_DATA(5, 3, 1), // _
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(2, 2, 1), // `
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 4, 1), // a
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // b
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 3, 1), // c
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // d
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // e
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(5, 3, 1), // f
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(7, 4, 1), // g
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // h
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 1, 1), // i
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(7, 2, 1), // j
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // k
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 1, 1), // l
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 5, 1), // m
		BINARY_8( _,_,_,_,_,_,_,_ ),
		BINARY_8( 1,1,1,1,_,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // n
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(5, 4, 1), // o
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(7, 4, 1), // p
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(7, 4, 1), // q
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,_,_,1 ),

		IMAGE_DATA(5, 3, 1), // r
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // s
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,_,1,1 ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(5, 3, 1), // t
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,1,_ ),

		IMAGE_DATA(5, 4, 1), // u
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // v
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( _,1,_,_ ),

		IMAGE_DATA(5, 5, 1), // w
		BINARY_8( _,_,_,_,_,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( 1,_,1,_,1,_,_,_ ),
		BINARY_8( _,1,_,1,_,_,_,_ ),
		BINARY_8( _,1,_,1,_,_,_,_ ),

		IMAGE_DATA(5, 3, 1), // x
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,1,_ ),

		IMAGE_DATA(7, 4, 1), // y
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( _,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(5, 4, 1), // z
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,1 ),

		IMAGE_DATA(5, 4, 1), // {
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,1,_ ),

		IMAGE_DATA(5, 1, 1), // |
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // }
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // ~
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,1 ),
		BINARY_4( 1,_,1,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 127
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 128
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 129
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 130
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 131
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 132
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 133
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 134
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 135
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 136
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 137
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 138
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 139
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 140
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 141
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 142
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 143
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 144
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 145
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 146
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 147
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 148
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 149
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 150
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 151
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 152
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 153
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 154
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 155
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 156
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 157
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 158
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 159
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 160
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 1, 1), // 161 (Windows-1252)
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 162
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 163
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 164
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 165
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 166
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 167
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 168
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 169
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 170
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 171
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 172
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 173
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 174
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 175
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 176
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 177
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 178
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 179
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 180
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 181
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 182
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 183
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 184
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 185
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 186
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 187
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 188
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 189
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 190
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 191 (Windows-1252)
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(1, 1, 1), // 192
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 193 (Windows-1252)
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(1, 1, 1), // 194
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 195
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 196
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 197
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 198
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 199
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 200
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 201 (Windows-1252)
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,1,1,1 ),

		IMAGE_DATA(1, 1, 1), // 202
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 203
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 204
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 3, 1), // 205 (Windows-1252)
		BINARY_4( 1,1,1,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( _,1,_,_ ),
		BINARY_4( 1,1,1,_ ),

		IMAGE_DATA(1, 1, 1), // 206
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 207
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 208
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 209 (Windows-1252)
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,1,_,1 ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(1, 1, 1), // 210
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 211 (Windows-1252)
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(1, 1, 1), // 212
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 213
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 214
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 215
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 216
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 217
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 218 (Windows-1252)
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(1, 1, 1), // 219
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 220 (Windows-1252)
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(1, 1, 1), // 221
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 222
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 223
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 224
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 225 (Windows-1252)
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,1,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(1, 1, 1), // 226
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 227
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 228
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 229
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 230
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 231
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 232
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 233 (Windows-1252)
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,1,1 ),
		BINARY_4( 1,1,_,_ ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(1, 1, 1), // 234
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 235
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 236
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 1, 1), // 237 (Windows-1252)
		BINARY_4( 1,_,_,_ ),
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),
		BINARY_4( 1,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 238
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 239
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 240
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 241 (Windows-1252)
		BINARY_4( 1,1,1,1 ),
		BINARY_4( 1,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),

		IMAGE_DATA(1, 1, 1), // 242
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 243 (Windows-1252)
		BINARY_4( _,_,1,_ ),
		BINARY_4( _,1,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,_ ),

		IMAGE_DATA(1, 1, 1), // 244
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 245
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 246
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 247
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 248
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 249
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 250 (Windows-1252)
		BINARY_4( _,_,1,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(1, 1, 1), // 251
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(5, 4, 1), // 252 (Windows-1252)
		BINARY_4( _,_,_,_ ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( 1,_,_,1 ),
		BINARY_4( _,1,1,1 ),

		IMAGE_DATA(1, 1, 1), // 253
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 254
		BINARY_4( _,_,_,_ ),

		IMAGE_DATA(1, 1, 1), // 255
		BINARY_4( _,_,_,_ ),

};

byte arrowhead_up_image_data[] = {
		IMAGE_DATA(4, 7, 1),
		BINARY_8( _,_,_,1,_,_,_,_ ),
		BINARY_8( _,_,1,1,1,_,_,_ ),
		BINARY_8( _,1,1,1,1,1,_,_ ),
		BINARY_8( 1,1,1,1,1,1,1,_ ),
};

byte arrowhead_down_image_data[] = {
		IMAGE_DATA(4, 7, 1),
		BINARY_8( 1,1,1,1,1,1,1,_ ),
		BINARY_8( _,1,1,1,1,1,_,_ ),
		BINARY_8( _,_,1,1,1,_,_,_ ),
		BINARY_8( _,_,_,1,_,_,_,_ ),
};

byte arrowhead_left_image_data[] = {
		IMAGE_DATA(5, 3, 1),
		BINARY_8( _,_,1,_,_,_,_,_ ),
		BINARY_8( _,1,1,_,_,_,_,_ ),
		BINARY_8( 1,1,1,_,_,_,_,_ ),
		BINARY_8( _,1,1,_,_,_,_,_ ),
		BINARY_8( _,_,1,_,_,_,_,_ ),
};

byte arrowhead_right_image_data[] = {
		IMAGE_DATA(5, 3, 1),
		BINARY_8( 1,_,_,_,_,_,_,_ ),
		BINARY_8( 1,1,_,_,_,_,_,_ ),
		BINARY_8( 1,1,1,_,_,_,_,_ ),
		BINARY_8( 1,1,_,_,_,_,_,_ ),
		BINARY_8( 1,_,_,_,_,_,_,_ ),
};

#endif //USE_MENU
