#include "Pong.h"

inline uint32
RoundFloatToUint32(float Float)
{
	uint32 Result = (uint32)(Float + 0.5);
	return Result;
}

inline int32
RoundFloatToInt32(float Float)
{
	uint32 Result;
	
	if(Float > 0)
	{
		Result = (int32)(Float + 0.5);
	}
	else
	{
		Result = (int32)(Float - 0.5);
	}
	return Result;
}

internal void
DrawRectangle(game_graphics_buffer *Buffer,
	state *State,
	float FloatX, float FloatY,
	float FloatWidth, float FloatHeight,
	uint8 Color)
{
	// NOTE: Does two conversions. One, from bottom-left coordinate system to top-left.
	// Two, from center object origin to top-left object origin.
	int32 HalfWidth = RoundFloatToInt32(FloatWidth / 2); 
	int32 HalfHeight = RoundFloatToInt32(FloatHeight / 2); 
	int32 X = RoundFloatToInt32(FloatX - HalfWidth);
	int32 Y = RoundFloatToInt32((State->ArenaHeight - FloatY) - HalfHeight);
	int32 Width = RoundFloatToInt32(FloatWidth);
	int32 Height = RoundFloatToInt32(FloatHeight);
	
	if(X < 0)
	{
		Width -= -X;
		X = 0;
	}
	if(Y < 0)
	{
		Height -= -Y;
		Y = 0;
	}
	if((X + Width) > Buffer->Width)
	{
		Width = Buffer->Width - X;
	}
	if((Y + Height) > Buffer->Height)
	{
		Height = Buffer->Height - Y;
	}
	
	int32 RowIncrement = Y * Buffer->Width;
	int32 *PixelIndex = ((int32 *)Buffer->BitmapMemory) + RowIncrement + X;
	for(int32 Row = 0; 
		Row < Height;
		Row++)
	{
		for(int32 Column = 0;
			Column < Width;
			Column++)
		{
			*PixelIndex++ = Color|(Color << 8)|(Color << 16);
		}
		RowIncrement += Buffer->Width;
		PixelIndex = ((int32 *)Buffer->BitmapMemory) + RowIncrement + X;
	}
}

internal void
GameFillSoundBuffer(game_sound_buffer *Buffer, uint16 SoundValue)
{
	if(Buffer->IsFormatSupported)
	{
		for(uint32 FrameIndex = 0; FrameIndex < Buffer->FramesToWrite; FrameIndex++)
		{
			Buffer->HalfCycleIndex = Buffer->RunningFrameIndex / Buffer->FramesPerHalfCycle;

			if(Buffer->HalfCycleIndex % 2)
			{
				*Buffer->SampleToWrite = -SoundValue;
				Buffer->SampleToWrite++;
				*Buffer->SampleToWrite = -SoundValue;
				Buffer->SampleToWrite++;
			}
			else
			{
				*Buffer->SampleToWrite = SoundValue;
				Buffer->SampleToWrite++;
				*Buffer->SampleToWrite = SoundValue;
				Buffer->SampleToWrite++;	
			}
			Buffer->RunningFrameIndex++;
			Buffer->FramesWritten = FrameIndex + 1;
		}
	}
}

internal void
ClearScreen(game_graphics_buffer *Buffer)
{
	int32 *PixelIndex = (int32*)Buffer->BitmapMemory;
	for(int i = 0; i < (Buffer->Width * Buffer->Height); i++)
	{
		*PixelIndex = 0;
		PixelIndex++;
	}
}

inline float
IsInbounds(int P, float TestPos, state *State)
{
	if(TestPos - (State->Player[P].Height/2) < 0)
	{
		return State->Player[P].Height/2;
	}
	else if(TestPos + (State->Player[P].Height/2) > State->ArenaHeight)
	{
		return State->ArenaHeight - (State->Player[P].Height/2);
	}
	else
	{
		return TestPos;
	}
}

internal void
GameProcessInput(state *State, game_input *Input)
{
	float TestPos = 0;
	
	if(Input->WKey.IsDown)
	{
		TestPos = State->Player[0].CenterY + State->PlayerMoveSpeed;
		State->Player[0].CenterY = IsInbounds(0, TestPos, State);
	}
	 
	if(Input->SKey.IsDown)
	{
		TestPos = State->Player[0].CenterY - State->PlayerMoveSpeed;
		State->Player[0].CenterY = IsInbounds(0, TestPos, State);
	}

	if(Input->UpKey.IsDown)
	{
		TestPos = State->Player[1].CenterY + State->PlayerMoveSpeed;
		State->Player[1].CenterY = IsInbounds(1, TestPos, State);
	}
	 
	if(Input->DownKey.IsDown)
	{
		TestPos = State->Player[1].CenterY - State->PlayerMoveSpeed;
		State->Player[1].CenterY = IsInbounds(1, TestPos, State);
	}
	
	if(Input->Spacebar.IsDown && !State->Ball.IsMoving)
	{
		State->Ball.IsMoving = true;
	}
}

inline void
GetBounds(rect *Rect)
{
	Rect->X0 = Rect->CenterX - (Rect->Width/2);
	Rect->X1 = Rect->CenterX + (Rect->Width/2);
	Rect->Y0 = Rect->CenterY - (Rect->Height/2);
	Rect->Y1 = Rect->CenterY + (Rect->Height/2);
}

inline float
RandomAngle()
{
	float Result;
	int64 Rand = __rdtsc();
	Result = fmod((float)Rand, Pi32);
	return Result;
}

inline void
RandomAdjustBallAngle(float *Angle)
{
	float Adjustment = RandomAngle() / 20;
	if((__rdtsc() % 2))
	{
		*Angle -= Adjustment;
	}
	else
	{
		*Angle +=Adjustment;
	}
}

inline void
ResetBall(state *State)
{
	State->Ball.Rect.CenterX = State->ArenaWidth/2;
	State->Ball.Rect.CenterY = State->ArenaHeight/2;
	State->Ball.Velocity = State->BallInitialVelocity;
	State->Ball.IsMoving = false;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	state *State = (state*)Memory->Storage;

	float PlayerXOffset = 50;
	float PlayerYOffset = 200;
	float PlayerWidth = 30;
	float PlayerHeight = 100;
	uint8 MaxScore = 3;
	State->PlayerMoveSpeed = 13;
	State->BallInitialVelocity = 10;
	
	bool32 SoundThisFrame = false;
	
	if(!State->IsInitialized)
	{
		State->ArenaWidth = GraphicsBuffer->Width;
		State->ArenaHeight = GraphicsBuffer->Height;
		
		State->Player[0].CenterX = PlayerXOffset;
		State->Player[0].CenterY = PlayerYOffset;
		State->Player[0].Width = PlayerWidth;
		State->Player[0].Height = PlayerHeight;
		
		State->Player[1].CenterX = State->ArenaWidth - PlayerXOffset;
		State->Player[1].CenterY = State->ArenaHeight - PlayerYOffset;
		State->Player[1].Width = PlayerWidth;
		State->Player[1].Height = PlayerHeight;

		State->Ball.Rect.CenterX = State->ArenaWidth/2;
		State->Ball.Rect.CenterY = State->ArenaHeight/2;
		State->Ball.Rect.Width = 20;
		State->Ball.Rect.Height = 20;
		State->Ball.Velocity = State->BallInitialVelocity;
		State->Ball.IsMoving = false;
		State->Ball.MovePositiveX = true;
		State->Ball.MovePositiveY = true;
		
		State->IsInitialized = true;
	}

	ClearScreen(GraphicsBuffer);
		
	GameProcessInput(State, Input);
	GetBounds(&State->Player[0]);
	GetBounds(&State->Player[1]);
	
	if(!State->Ball.IsMoving)
	{
		do{State->Ball.Angle = RandomAngle();}
		while((State->Ball.Angle > (Pi32 / 3) && State->Ball.Angle < (2*Pi32 / 3)) ||
			(State->Ball.Angle > (4*Pi32 / 3) && State->Ball.Angle < (5*Pi32 / 3)) ||
			State->Ball.Angle < (Pi32 / 12) || State->Ball.Angle > (23*Pi32 / 12) ||
			(State->Ball.Angle > (11*Pi32 / 12) && State->Ball.Angle < (13*Pi32 / 12)));
	}
	
	if(State->Ball.IsMoving)
	{
		if(State->Player1Score == MaxScore || State->Player2Score == MaxScore)
		{
			State->Player1Score = 0;
			State->Player2Score = 0;
		}
		
		//horrible code to just invert one axis of movement on specific collisions
		float PreviousX0 = State->Ball.Rect.X0;
		float PreviousX1 = State->Ball.Rect.X1;
		//Movement
		if(State->Ball.MovePositiveX)
		{
			State->Ball.Rect.CenterX += State->Ball.Velocity * cos(State->Ball.Angle);
		}
		else
		{
			State->Ball.Rect.CenterX -= State->Ball.Velocity * cos(State->Ball.Angle);
		}
		
		if(State->Ball.MovePositiveY)
		{
			State->Ball.Rect.CenterY += State->Ball.Velocity * sin(State->Ball.Angle);
		}
		else
		{
			State->Ball.Rect.CenterY -= State->Ball.Velocity * sin(State->Ball.Angle);
		}
		GetBounds(&State->Ball.Rect);
		
		//Scoring
		if(State->Ball.Rect.CenterX >= State->ArenaWidth)
		{
			State->Player1Score++;
			ResetBall(State);
		}
		
		if(State->Ball.Rect.CenterX <= 0)
		{
			State->Player2Score++;
			ResetBall(State);
		}
		
		if(!State->Ball.CollisionLastFrame)
		{
			//Paddle collision
			for(int P = 0; P < 2; P++)
			{
				bool32 XCollision = false;
				bool32 YCollision = false;
				
				if((State->Ball.Rect.X0 >= State->Player[P].X0 &&
					State->Ball.Rect.X0 <= State->Player[P].X1) ||
					(State->Ball.Rect.X1 >= State->Player[P].X0 &&
					State->Ball.Rect.X1 <= State->Player[P].X1))
				{
					XCollision = true;
				}	

				if((State->Ball.Rect.Y0 >= State->Player[P].Y0 &&
					State->Ball.Rect.Y0 <= State->Player[P].Y1) ||
					(State->Ball.Rect.Y1 >= State->Player[P].Y0 &&
					State->Ball.Rect.Y1 <= State->Player[P].Y1))
				{
					YCollision = true;
				}			
				
				if(XCollision && YCollision)
				{
					//handles collision with top/bottom of paddle
					if((PreviousX0 >= State->Player[P].X0 && PreviousX0 <= State->Player[P].X1) ||
						(PreviousX1 >= State->Player[P].X0 && PreviousX1 <= State->Player[P].X1))
					{
						if(State->Ball.MovePositiveY)
							{
								State->Ball.MovePositiveY = false;
							}
							else
							{
								State->Ball.MovePositiveY = true;
							}
					}
					else
					{
						if(State->Ball.MovePositiveX)
						{
							State->Ball.MovePositiveX = false;
						}
						else
						{
							State->Ball.MovePositiveX = true;
						}
					}
					
					if(State->Ball.Velocity < PlayerWidth)
					{
						State->Ball.Velocity += 1;
					}
					RandomAdjustBallAngle(&State->Ball.Angle);
					GameFillSoundBuffer(SoundBuffer, 500);
					SoundThisFrame = true;
					State->Ball.CollisionLastFrame = true;
					break;
				}
			}
			
			//Arena collision
			if(State->Ball.Rect.CenterY >= State->ArenaHeight || 
					State->Ball.Rect.CenterY <= 0)
			{
				if(State->Ball.MovePositiveY)
				{
					State->Ball.MovePositiveY = false;
				}
				else
				{
					State->Ball.MovePositiveY = true;
				}
				RandomAdjustBallAngle(&State->Ball.Angle);
				GameFillSoundBuffer(SoundBuffer, 500);
				SoundThisFrame = true;
				State->Ball.CollisionLastFrame = true;
			}
		}
		else
		{
			State->Ball.CollisionLastFrame = false;
		}
	}

	DrawRectangle(GraphicsBuffer, State,
		State->Player[0].CenterX, State->Player[0].CenterY,
		State->Player[0].Width, State->Player[0].Height,
		255);
	DrawRectangle(GraphicsBuffer, State,
		State->Player[1].CenterX, State->Player[1].CenterY,
		State->Player[1].Width, State->Player[1].Height,
		255);
	
	for(int i = 0; i < State->Player1Score; i++)
	{
		DrawRectangle(GraphicsBuffer, State,
		(100.0f + (i * 20)), (State->ArenaHeight - 20),
		10, 10,
		150);
	}
	
	for(int i = 0; i < State->Player2Score; i++)
	{
		DrawRectangle(GraphicsBuffer, State,
		(State->ArenaWidth - (100.0f + (i * 20))), (State->ArenaHeight - 20),
		10, 10,
		150);
	}
	
	DrawRectangle(GraphicsBuffer, State,
		State->Ball.Rect.CenterX, State->Ball.Rect.CenterY,
		State->Ball.Rect.Width, State->Ball.Rect.Height,
		255);	
	
	if(!SoundThisFrame)
	{
		GameFillSoundBuffer(SoundBuffer, 0);
	}
}