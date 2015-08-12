#if !defined(WIN32_PONG_H)

struct win32_game_code
{
	HMODULE LibraryHandle;
	game_update_and_render *GameUpdateAndRender;
	bool32 IsValid;
};

#define WIN32_PONG_H
#endif