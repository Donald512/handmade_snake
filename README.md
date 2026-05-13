<b>Win32 Snake: Using windows.h and stdint.h only</b>
A C implementation of the classic Snake game, built using the Win32 API. No game engines, just software rendering with the CPU.
Made this with windows.h to run on a modern laptop first with infinite memory, so I can build it on an Arduino using an OLED screen, so the difficuly is less difficult.

Uses VirtualAlloc to reserve and commit a specific memory for snake's body and backbuffer.
No DirectX or OpenGL
Custom built LCG (Linear Congruential Generator) similar to stdlib's rand() 

gameSpeedms can be changed to desired value
#define size 10 can also be changed to anything, but it would be best to change  the 3 and 5 below:
#define rightWall (boardWidth - 3*size)
#define bottomWall (boardHeight - 5*size)
E.g: A size of 20 would need 2 and 3 respectively
