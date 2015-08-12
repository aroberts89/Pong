#if !defined(PONG_H)

#include <stdint.h>
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;

#define Kilobytes(Value) (Value * 1024)
#define Megabytes(Value) ((uint64)(Kilobytes(Value)*1024))
#define Gigabytes(Value) ((uint64)(Megabytes(Value)*1024))

#define Pi32 (float)3.141592

// Game only

struct rect
{
	float CenterX;
	float CenterY;
	float Width;
	float Height;
	float X0;
	float Y0;
	float X1;
	float Y1;
};

struct ball
{
	rect Rect;
	float Angle;
	float Velocity;
	bool32 IsMoving;
	bool32 MovePositiveX;
	bool32 MovePositiveY;
	bool32 CollisionLastFrame;
};

struct state
{
	float ArenaWidth;
	float ArenaHeight;
	
	rect Player[2];
	float PlayerMoveSpeed;
	uint8 Player1Score;
	uint8 Player2Score;
	
	ball Ball;
	float BallInitialVelocity;
	
	bool32 IsInitialized;
};

// Services platform provides to game

struct debug_file_info
{
	uint32 FileSize;
	void *FileMemory;
};

#define DEBUG_PLATFORM_READ_FILE(name) debug_file_info name(char *FileName)
typedef DEBUG_PLATFORM_READ_FILE(debug_platform_read_file);

#define DEBUG_PLATFORM_WRITE_FILE(name) bool32 name(char *FileName, debug_file_info *FileInfo)
typedef DEBUG_PLATFORM_WRITE_FILE(debug_platform_write_file);

// Services the game provides to the platform layer

struct game_graphics_buffer
{
	void *BitmapMemory;
	uint16 Height;
	uint16 Width;
};

struct game_sound_buffer
{
	int16 *SampleToWrite;
	uint32 FramesToWrite;
	uint32 FramesPerHalfCycle;
	uint32 RunningFrameIndex;
	uint32 HalfCycleIndex;
	uint32 ToneHz;
	uint32 FramesWritten;
	uint32 FrameLatency;
	bool32 IsAudioPlaying;
	bool32 IsFormatSupported;
};

struct game_key_state
{
	bool32 IsDown;
};

struct game_input
{
	game_key_state WKey;
	game_key_state AKey;
	game_key_state SKey;
	game_key_state DKey;
	game_key_state BKey;
	game_key_state Spacebar;
	game_key_state UpKey;
	game_key_state DownKey;
};

struct game_memory
{
	// This should point to zero-initialized memory
	void *Storage;
	uint64 StorageSize;
	
	debug_platform_read_file *DEBUGPlatformReadFile;
	debug_platform_write_file *DEBUGPlatformWriteFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_graphics_buffer *GraphicsBuffer, game_sound_buffer *SoundBuffer, game_input *Input)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

#define PONG_H
#endif