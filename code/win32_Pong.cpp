#include "Pong.h"
#include "win32_Pong.h"

#if !defined(PONG_DEV)
#include "Pong.cpp"
#endif
global_variable bool32 GlobalRunning;

internal void 
Win32SizeGraphicsBuffer(game_graphics_buffer *Buffer, BITMAPINFO *BitmapInfo, HWND Window)
{
	//Free memory if already allocated
	if(Buffer->BitmapMemory)
	{
		VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
	}
	
	// Resize/reallocate bitmap buffer
	BitmapInfo->bmiHeader.biSize = sizeof(BitmapInfo->bmiHeader);
	BitmapInfo->bmiHeader.biWidth = Buffer->Width;
	BitmapInfo->bmiHeader.biHeight = -Buffer->Height;
	BitmapInfo->bmiHeader.biPlanes = 1;
	BitmapInfo->bmiHeader.biBitCount = 32;
	BitmapInfo->bmiHeader.biCompression = BI_RGB;
	int BytesPerPixel = BitmapInfo->bmiHeader.biBitCount / 8;
	int BitmapMemorySize = BytesPerPixel*(Buffer->Width*Buffer->Height);
	Buffer->BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
}

internal void
Win32RenderGraphics(game_graphics_buffer *Buffer, BITMAPINFO *BitmapInfo,
					HDC DeviceContext, HWND Window)
{
	StretchDIBits(DeviceContext,
				  0, 0, Buffer->Width, Buffer->Height,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->BitmapMemory, BitmapInfo,
				  DIB_RGB_COLORS, SRCCOPY);
	UpdateWindow(Window);
}

internal bool32
Win32InitializeSound(IAudioClient **MainAudioClient, IAudioRenderClient **MainRenderClient, HWND Window)
{
	HRESULT hr;
	bool32 Result = 1;
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
			
	IMMDeviceEnumerator *DeviceEnumerator;
	CoCreateInstance(__uuidof(MMDeviceEnumerator), 
					 NULL, CLSCTX_ALL,
					 __uuidof(IMMDeviceEnumerator),
					 (void **)&DeviceEnumerator);

	IMMDevice *DefaultDevice;
	DeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &DefaultDevice);
	
	IAudioClient *AudioClient;
	DefaultDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **)&AudioClient);
	*MainAudioClient = AudioClient;
	
	WAVEFORMATEX WaveFormat = {};
	WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	WaveFormat.nChannels = 2;
	WaveFormat.nSamplesPerSec = 44100;
	WaveFormat.wBitsPerSample = 16;
	WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
	WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
	
	int64 BufferLengthMS = 1000;
	WAVEFORMATEX *ClosestMatchFormat = (WAVEFORMATEX *)0xFF;
	hr = AudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &WaveFormat, &ClosestMatchFormat);
	if(hr == S_OK)
	{
		AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
			(BufferLengthMS*10000), 0,
			&WaveFormat, NULL);
	}
	else
	{
		hr = AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
			(BufferLengthMS*10000), 0,
			ClosestMatchFormat, NULL);
		MessageBox(Window, "Default audio format unsupported.", "Startup", MB_OK);
		Result = 0;
		if(hr != S_OK)
		{
			MessageBox(Window, "Audio client failed to initialize.", "Startup", MB_OK);
		}
	}

	IAudioRenderClient *RenderClient;
	AudioClient->GetService(__uuidof(IAudioRenderClient), (void **)&RenderClient);
	*MainRenderClient = RenderClient;
	
	if(AudioClient == NULL)
	{
		MessageBox(Window, "Failed to open audio client.", "Startup", MB_OK);
		GlobalRunning = false;
	}
	if(RenderClient == NULL)
	{
		MessageBox(Window, "Failed to open render client.", "Startup", MB_OK);
		GlobalRunning = false;
	}
	
	DefaultDevice->Release();
	DeviceEnumerator->Release();
	CoTaskMemFree(ClosestMatchFormat);
	
	return Result;
}
							
DEBUG_PLATFORM_READ_FILE(DEBUGPlatformReadFile)
{
	// ReadFile takes DWORD for nNumberOfBytesToRead - meaning this can't read more than ~4GB
	
	HANDLE FileHandle = CreateFile(FileName, GENERIC_READ, 0, NULL,
								   OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
	
	debug_file_info FileInfo = {};
	LARGE_INTEGER FileSize;
	GetFileSizeEx(FileHandle, &FileSize);
	FileInfo.FileSize = (uint32)FileSize.QuadPart;
	FileInfo.FileMemory = VirtualAlloc(0, FileInfo.FileSize,
									MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	DWORD BytesRead;
	ReadFile(FileHandle, FileInfo.FileMemory,
			 FileInfo.FileSize, &BytesRead, 0);

	CloseHandle(FileHandle);
	
	return FileInfo;
}

DEBUG_PLATFORM_WRITE_FILE(DEBUGPlatformWriteFile)
{
	// ReadFile takes DWORD for nNumberOfBytesToRead - meaning this can't read more than ~4GB
	
	HANDLE FileHandle = CreateFile(FileName, GENERIC_WRITE, 0, NULL,
								   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD BytesWritten;
	WriteFile(FileHandle, FileInfo->FileMemory,
			 FileInfo->FileSize, &BytesWritten, 0);

	CloseHandle(FileHandle);
	
	return (BytesWritten == FileInfo->FileSize);
}

global_variable LARGE_INTEGER GlobalCountsPerSecond;

inline float
Win32GetPeriod(LARGE_INTEGER BeginTime, LARGE_INTEGER EndTime)
{
	return (1000.0f * ((float)(EndTime.QuadPart - BeginTime.QuadPart) /
					   GlobalCountsPerSecond.QuadPart));
}

#ifdef PONG_DEV
internal void
Win32LoadGameCode(win32_game_code *GameCode)
{
	if(CopyFile("c:\\GameDev\\build\\Pong.dll", "c:\\GameDev\\build\\Pong_running.dll", FALSE))
	{
		OutputDebugString("Temp DLL created.\n");
		GameCode->LibraryHandle = LoadLibrary("c:\\GameDev\\build\\Pong_running.dll");
		if(GameCode->LibraryHandle)
		{
			GameCode->GameUpdateAndRender = 
				(game_update_and_render *)GetProcAddress(GameCode->LibraryHandle, "GameUpdateAndRender");
			
			GameCode->IsValid = (GameCode->GameUpdateAndRender != 0);
		}
	}
	
	if(!GameCode->IsValid)
	{
		GameCode->GameUpdateAndRender = GameUpdateAndRenderStub;
		OutputDebugString("WARNING: Stub was loaded.\n");
	}
}

internal void
Win32FreeGameCode(win32_game_code *GameCode)
{
	if(GameCode->LibraryHandle)
	{
		FreeLibrary(GameCode->LibraryHandle);
		GameCode->LibraryHandle = 0;
	}
	
	GameCode->IsValid = false;
	GameCode->GameUpdateAndRender = GameUpdateAndRenderStub;
	OutputDebugString("Unloaded library.\n");
}
#endif

internal void
Win32ProcessMessages(game_input *Input)
{
	uint8 WKey = 0x57;
	uint8 AKey = 0x41;
	uint8 SKey = 0x53;
	uint8 DKey = 0x44;
	uint8 BKey = 0x42;
	
	MSG Message;
	while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_KEYUP:
			case WM_SYSKEYUP:
			{	
				if(Message.wParam == WKey)
				{
					Input->WKey.IsDown = false;
				}
				if(Message.wParam == SKey)
				{
					Input->SKey.IsDown = false;
				}
				if(Message.wParam == DKey)
				{
					Input->DKey.IsDown = false;
				}
				if(Message.wParam == AKey)
				{
					Input->AKey.IsDown = false;
				}
				if(Message.wParam == BKey)
				{
					Input->BKey.IsDown = false;
				}
				if(Message.wParam == VK_SPACE)
				{
					Input->Spacebar.IsDown = false;
				}
				if(Message.wParam == VK_UP)
				{
					Input->UpKey.IsDown = false;
				}
				if(Message.wParam == VK_DOWN)
				{
					Input->DownKey.IsDown = false;
				}
			} break;
			
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
			{
				if(Message.wParam == VK_ESCAPE)
				{
					GlobalRunning = false;
				}
				if(Message.wParam == WKey)
				{
					Input->WKey.IsDown = true;
				}
				if(Message.wParam == SKey)
				{
					Input->SKey.IsDown = true;
				}
				if(Message.wParam == DKey)
				{
					Input->DKey.IsDown = true;
				}
				if(Message.wParam == AKey)
				{
					Input->AKey.IsDown = true;
				}
				if(Message.wParam == BKey)
				{
					Input->BKey.IsDown = true;
				}
				if(Message.wParam == VK_SPACE)
				{
					Input->Spacebar.IsDown = true;
				}
				if(Message.wParam == VK_UP)
				{
					Input->UpKey.IsDown = true;
				}
				if(Message.wParam == VK_DOWN)
				{
					Input->DownKey.IsDown = true;
				}
			} break;
			
			default:
			{
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			} break;
		}
	}
}

LRESULT CALLBACK
Win32WindowCallback(HWND Window,
				    UINT Message,
				    WPARAM wParam,
				    LPARAM lParam)
{
	LRESULT Result = 0;
	
	switch(Message)
	{
		case WM_CLOSE:
		{
			GlobalRunning = false;
		} break;
		
		case WM_PAINT:
		{
			HDC PaintContext;
			PAINTSTRUCT PaintStruct = {};
			RECT ClientRect;

			GetClientRect(Window, &ClientRect);
			PaintContext = BeginPaint(Window, &PaintStruct);
			PaintStruct.hdc = PaintContext;
			PaintStruct.fErase = true;
			PaintStruct.rcPaint = ClientRect;
			
			PatBlt(PaintContext,
				   ClientRect.left, ClientRect.top, 
				  (ClientRect.right - ClientRect.left), (ClientRect.bottom - ClientRect.top),
				   WHITENESS);
				   
			EndPaint(Window, &PaintStruct);
		} break;
		
		default:
		{
			Result = DefWindowProc(Window, Message, wParam, lParam);
		} break;
		
	}
	return Result;
}

int CALLBACK
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int WindowOptions)
{
	WNDCLASSEX WindowClass = {};
	WindowClass.cbSize = sizeof(WindowClass);
	WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
	WindowClass.lpfnWndProc = Win32WindowCallback;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon = Icon;
	//WindowClass.hCursor;
	WindowClass.lpszClassName = "GamekooWindowClass";
	//WindowClass.hIconSm;
	
	if(RegisterClassExA(&WindowClass))
	{
		uint16 GameWidth = 1280;
		uint16 GameHeight = 720;
		
		HWND Window = CreateWindowExA
		  (WS_EX_OVERLAPPEDWINDOW,
		   WindowClass.lpszClassName,
		   "Gamekoo",
		   WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
		   CW_USEDEFAULT,
		   CW_USEDEFAULT,
		   1600,
		   900,
		   0,
		   0,
		   Instance,
		   0);
		
		if(Window)
		{
			GlobalRunning = true;
			//Sizing
			RECT ClientRect = {};
			RECT WindowRect = {};
			GetClientRect(Window, &ClientRect);
			GetWindowRect(Window, &WindowRect);
			uint16 NonClientWidth = (uint16)((WindowRect.right - WindowRect.left) - ClientRect.right);
			uint16 NonClientHeight = (uint16)((WindowRect.bottom - WindowRect.top) - ClientRect.bottom);
			SetWindowPos(Window, 0, 
				0, 0, 
				GameWidth + NonClientWidth, GameHeight + NonClientHeight,
				SWP_NOMOVE);

			//Sound initialization
			IAudioClient *AudioClient = NULL;
			IAudioRenderClient *RenderClient = NULL;
			game_sound_buffer GameSoundBuffer = {};
			if(Win32InitializeSound(&AudioClient, &RenderClient, Window))
			{
				GameSoundBuffer.IsFormatSupported = true;
			}
			
			uint32 BufferFrames;
			AudioClient->GetBufferSize(&BufferFrames);
			GameSoundBuffer.ToneHz = 260;
			GameSoundBuffer.FrameLatency = 1;

			GameSoundBuffer.FramesPerHalfCycle = (BufferFrames / GameSoundBuffer.ToneHz) / 2;

			
			// Graphics initialization
			//TODO: Why does it break the program to put GetDC above CoCreateInstance?
			HDC DeviceContext =  NULL;
			DeviceContext = GetDC(Window);
			if(DeviceContext == NULL)
			{
				MessageBox(Window, "Failed to get device context.", "Startup", MB_OK);
				GlobalRunning = false;
			}
			
			BITMAPINFO BitmapInfo = {};
			game_graphics_buffer GameGraphicsBuffer = {};
			GameGraphicsBuffer.Width = GameWidth;
			GameGraphicsBuffer.Height = GameHeight;
			Win32SizeGraphicsBuffer(&GameGraphicsBuffer, &BitmapInfo, Window);

			// Input initialization
			game_input Input = {};

			// Memory initialization
			game_memory GameMemory = {};
			GameMemory.DEBUGPlatformReadFile = DEBUGPlatformReadFile;
			GameMemory.DEBUGPlatformWriteFile = DEBUGPlatformWriteFile;
			GameMemory.StorageSize = Megabytes(512);
			GameMemory.Storage = VirtualAlloc(0, GameMemory.StorageSize,
											  MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
			if(GameMemory.Storage == NULL)
			{
				MessageBox(Window, "Failed to initialize memory.", "Startup", MB_OK);
				GlobalRunning = false;
			}
			
			// Timing initialization
			uint32 MonitorRefreshHz = 60;
			uint32 TargetFrameRate = MonitorRefreshHz;
			float TargetMSPerFrame = 1000.0f / TargetFrameRate;
			QueryPerformanceFrequency(&GlobalCountsPerSecond);
			timeBeginPeriod(1);
			
			LARGE_INTEGER LastRender;
			QueryPerformanceCounter(&LastRender);

#ifdef PONG_DEV			
			//initialize game code
			win32_game_code GameCode = {};
			FILETIME DLLLoadTime = {};
			FILETIME DLLWriteTime = {};
			WIN32_FIND_DATA FileData = {};
#endif
			
			// Main loop
			while(GlobalRunning)
			{
				LARGE_INTEGER BeganWork = LastRender;
				
#ifdef PONG_DEV				
				HANDLE FileHandle = CreateFile("c:\\GameDev\\build\\Pong.dll", GENERIC_READ | GENERIC_WRITE,
												0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if(FileHandle != INVALID_HANDLE_VALUE)
				{
					GetFileTime(FileHandle, NULL, NULL, &DLLWriteTime);
					CloseHandle(FileHandle);
				}
				
				if(CompareFileTime(&DLLLoadTime, &DLLWriteTime) != 0)
				{
					Win32FreeGameCode(&GameCode);
					Win32LoadGameCode(&GameCode);
					DLLLoadTime = DLLWriteTime;
				}
#endif				
				Win32ProcessMessages(&Input);

				uint32 PaddingFrames;
				AudioClient->GetCurrentPadding(&PaddingFrames);
				if((BufferFrames - PaddingFrames) < GameSoundBuffer.FramesToWrite)
				{	
					GameSoundBuffer.FramesToWrite = 0;
				}
				else
				{
					GameSoundBuffer.FramesToWrite = GameSoundBuffer.FrameLatency*(BufferFrames / TargetFrameRate);
				}

				RenderClient->GetBuffer(GameSoundBuffer.FramesToWrite, (uint8 **)&GameSoundBuffer.SampleToWrite);
#ifdef PONG_DEV				
				GameCode.GameUpdateAndRender(&GameMemory, &GameGraphicsBuffer, 
											 &GameSoundBuffer, &Input);
#else											 
				GameUpdateAndRender(&GameMemory, &GameGraphicsBuffer, 
					&GameSoundBuffer, &Input);
#endif					
				RenderClient->ReleaseBuffer(GameSoundBuffer.FramesToWrite, 0);
				
				LARGE_INTEGER WorkComplete;
				QueryPerformanceCounter(&WorkComplete);
				float TimeElapsedMS = Win32GetPeriod(LastRender, WorkComplete);
									   
				if(TimeElapsedMS <= TargetMSPerFrame)
				{
					DWORD TimeToSleep = (DWORD)(TargetMSPerFrame - TimeElapsedMS);
					Sleep(TimeToSleep);
					
					while(TimeElapsedMS < TargetMSPerFrame)
					{
						LARGE_INTEGER TimeAfterSleep;
						QueryPerformanceCounter(&TimeAfterSleep);
						TimeElapsedMS = Win32GetPeriod(LastRender, TimeAfterSleep);
					}
				}
				else
				{
					//TODO: Logging. Missed a frame
					//TODO: Some frames are running 1-3ms too long without triggering this warning. 
					//Might be causing visual skips.
					OutputDebugString("WARNING: Missed a frame.\n");
				}

				if(!GameSoundBuffer.IsAudioPlaying && GameSoundBuffer.IsFormatSupported)
				{	
					GameSoundBuffer.IsAudioPlaying = true;
					AudioClient->Start();
				}
				QueryPerformanceCounter(&LastRender);
				Win32RenderGraphics(&GameGraphicsBuffer, &BitmapInfo,
									DeviceContext, Window);
				
				//Output info for last frame
				float TotalFrameTime = Win32GetPeriod(BeganWork, LastRender);
				char PrintBuffer[200];
				float FPS = 1000.0f / TotalFrameTime;
				sprintf_s(PrintBuffer, 200, 
					"Time spent this frame: %.2fms | FPS: %.2f\n, Sound frames to write: %d | Sound frames written: %d | Padding frames: %d\n",
					TotalFrameTime, FPS, GameSoundBuffer.FramesToWrite, GameSoundBuffer.FramesWritten, PaddingFrames);
				OutputDebugString(PrintBuffer);
			}
			
			if(GameSoundBuffer.IsAudioPlaying)
			{
				AudioClient->Stop();
			}
			
			ReleaseDC(Window, DeviceContext);
			RenderClient->Release();
			AudioClient->Release();
		}
		else
		{
			//TODO: CreateWindow failure logging
		}
	}
	
	else
	{
		//TODO: RegisterClass failure logging
	}
	CoUninitialize();
	return 0;
}