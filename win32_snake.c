#include <windows.h>
#include <stdint.h>

#define errorToReturn 1
#define true 1
#define false 0

#define boardWidth 640
#define boardHeight 480

#define bytesPerPixel 4
#define size 10

#define rightWall (boardWidth - 3*size)
#define bottomWall (boardHeight - 5*size)


#define black 0x00000000
#define white 0x00FFFFFF    // little endian, in memory stored ABGR


typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t bool32;

u32 imageSize = boardWidth * boardHeight * bytesPerPixel;    // in bytes


typedef struct{
    u16 x;
    u16 y;
}   square;

typedef struct{
    i8 right;
    i8 down;
}   direction;


LRESULT Win32MainWindowCallback(HWND hwnd, UINT event, WPARAM info1, LPARAM info2);
void createBoard();
void addSquare(square sqr);
void removeSquare(square sqr);
void updateScreen(HDC deviceContext);
void clearBoard();
void spawnSnake(square snake[], u16 length);
void addStructs(square *head, direction dir);
void moveSnake(square snake[], u16 length);
void DEBUGprintSnake(square snake[], u16 length);
void DEBUGprintDir(direction dir);
void DEBUGprintStruct(square part);
void addSnake(square snake[], u16 length);

BITMAPINFO bmpInfo = {0};
void* imagePtr = NULL;

bool32 RUNNING = true;
square testSqr = {600, 400};
square lastSqr = {600, 400};
direction headDir = {0, 0};

square globalSnake[12] = {{6, 1}, {7, 1}, {7, 2}, {7, 3}, {7, 4}, {8, 4}, {9, 4}, {10, 4}, {10, 3}, {10, 2}, {10, 1}, {10, 0}};
square lastHead;
bool32 move = false;

u16 gameSpeedms = 200;    // 500 ms
u64 numTicksPerGameSpeedms;

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){ // entry point for Windows GUI
    LARGE_INTEGER ticksPerSecResult;
    QueryPerformanceFrequency(&ticksPerSecResult);
    i64 ticksPerSec = ticksPerSecResult.QuadPart;
    u64 ticksPerMs = ticksPerSec/1000;
    numTicksPerGameSpeedms = gameSpeedms * ticksPerMs;
    

    WNDCLASSEXA windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_OWNDC| CS_HREDRAW| CS_VREDRAW ;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = "Snake";
    // todo look into WNDCLASSEXA doc for how to create a HCURSOR 

    if (!RegisterClassExA(&windowClass)){ 
        OutputDebugString("Registering windowClass failed");
        return errorToReturn;
    }

    HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName, "Snake Game",WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, 0, 0, hInstance, 0);

    if (!windowHandle){
        OutputDebugString("Error creating windowHandle (NULL)");
        return errorToReturn;
    }

    RUNNING = true;

    createBoard();
    // addSquare(testSqr);
    spawnSnake(globalSnake, 12);
    lastHead = globalSnake[0];
    HDC deviceContext = GetDC(windowHandle);

    LARGE_INTEGER lastNumTicks, currentNumTicks;
    QueryPerformanceCounter(&lastNumTicks);


    while (RUNNING){
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE /*Remove from queue after checking*/)){
            if (msg.message == WM_QUIT){
                RUNNING = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        // if (move){
        //     moveSnake(globalSnake, 12);
        //     removeSquare(globalSnake[11]);
        //     DEBUGprintSnake(globalSnake, 12);
        //     OutputDebugString("\n");
            
        // }

        QueryPerformanceCounter(&currentNumTicks);
        if ((u64)currentNumTicks.QuadPart - (u64)lastNumTicks.QuadPart >= numTicksPerGameSpeedms){
            removeSquare(globalSnake[11]);
            moveSnake(globalSnake, 12);
            DEBUGprintStruct(globalSnake[0]);
            lastNumTicks = currentNumTicks;
        }


        updateScreen(deviceContext);
    }

    return 0;
}

LRESULT Win32MainWindowCallback(HWND hwnd, UINT event, WPARAM info1, LPARAM info2){
    LRESULT result = 0;
    switch(event){
        case WM_ACTIVATE:{
            OutputDebugString("Test\t");
        }   break;
        case WM_CLOSE:{
            RUNNING = false;
            result = DefWindowProc(hwnd, event, info1, info2);
        }   break;
        case WM_KEYUP:
        case WM_KEYDOWN:{
            WPARAM VKCode = info1;    // which key was pressed 
            bool32 wasDown = (info2 & (1 << 30)) != 0;
            bool32 isDown = (info2 & (1 << 31)) == 0;
            if (isDown != wasDown && isDown){
                switch (VKCode){
                    case VK_UP:{
                        if (headDir.down <= 0){  // cant move in opposite direction blocker
                            headDir.right = 0;
                            headDir.down = -size;
                            move = true;
                        }
                        OutputDebugString("Up\t");
                    }   break;
                    case VK_DOWN:{
                        if (headDir.down >= 0){ 
                            headDir.right = 0;
                            headDir.down = size;
                            move = true;
                        }
                        OutputDebugString("Down\t");
                    }   break;
                    case VK_LEFT:{
                        if (headDir.right <= 0){ 
                            headDir.right = -size;
                            headDir.down = 0;
                            move = true;
                        }
                        OutputDebugString("Left\t");
                    }   break;
                    case VK_RIGHT:{
                        if (headDir.right >= 0){ 
                            headDir.right = size;
                            headDir.down = 0;
                            move = true;
                        }
                        OutputDebugString("Right\t");
                    }   break;
                }
            }
        }   break;
        default:{
            result = DefWindowProc(hwnd, event, info1, info2);
        }   break;
    }
    return result;
}


void createBoard(){
    if (imagePtr){
        return; // dont want to cause memory leak
    }

    BITMAPINFOHEADER bmpInfoHdr = {0};
    bmpInfoHdr.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfoHdr.biWidth = boardWidth;
    bmpInfoHdr.biHeight = -boardHeight;
    bmpInfoHdr.biPlanes = 1;
    bmpInfoHdr.biBitCount = 32;
    bmpInfoHdr.biCompression = BI_RGB;

    bmpInfo.bmiHeader = bmpInfoHdr;


    imagePtr = VirtualAlloc(0, imageSize,MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

}

void clearBoard(){
    u32* pixels = (u32*) imagePtr;
    u32 totalPixels = boardWidth * boardHeight;
    for (u32 i = 0; i < totalPixels; i++){
        *(pixels + i) = black;
    }
}



void addSquare(square sqr){
    i32* pixels = (i32*) imagePtr;
    
    for (i32 i = sqr.y; i < sqr.y + size; i++){
        for (i32 j = sqr.x; j < sqr.x + size; j++ ){
            *(pixels + (i * boardWidth + j)) = white;
        }
    }
}

void removeSquare(square sqr){
    u32* pixels = (u32*) imagePtr;

    for (u32 i = sqr.y; i < sqr.y + size; i++){
        for (u32 j = sqr.x; j < sqr.x + size; j++ ){
            *(pixels + (i * boardWidth + j)) = black;
        }
    }
}

void updateScreen(HDC deviceContext){
    StretchDIBits(deviceContext,0, 0, boardWidth, boardHeight, 0, 0, boardWidth, boardHeight, imagePtr, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
}

void spawnSnake(square snake[], u16 length){
    for (u16 i = 0; i < length; i++){
        snake[i].x *= size;
        snake[i].y *= size;
        // snake[i].x += 50; // ! TEST
        // snake[i].y += 100; 
        addSquare(snake[i]);
    }
}
void addSnake(square snake[], u16 length){
    for (u16 i = 0; i < length; i++){
        addSquare(snake[i]);
    }
}

void addStructs(square *head, direction dir){
    if (dir.right){
        i16 resultX = head->x + dir.right;
        if (resultX >= 0 && resultX <= rightWall){
            head->x = resultX;
        }
    }
    else if (dir.down){
        i16 resultY = head->y + dir.down;
        if (resultY >= 0 && resultY <= bottomWall){
            head->y = resultY;
        }
    }
    // DEBUGprintStruct(*head);
    // DEBUGprintDir(dir);


}

void moveSnake(square snake[], u16 length){
    square headCopy = snake[0];
    addStructs(&headCopy, headDir);
    u16 i = length;
    for ( ; i > 1; i--){
        snake[i - 1] = snake[i - 2];
    }
    snake[0] = headCopy;
    move = false;
    addSnake(snake, length);
}

void DEBUGprintSnake(square snake[], u16 length){
    char buffer[1024];
    char *at = buffer;

    for (u16 i = 0; i < length; i++){
        u8 charsWritten = wsprintf(at, "(%d, %d), ", snake[i].x, snake[i].y);
        at += charsWritten;
    }
    OutputDebugString(buffer);
}

void DEBUGprintStruct(square part){
    char buffer[256];
    wsprintf(buffer, "(%d, %d)\n", part.x, part.y);
    OutputDebugString(buffer);
}
void DEBUGprintDir(direction dir){
    char buffer[256];
    wsprintf(buffer, "(%d, %d)\n", dir.right, dir.down);
    OutputDebugString(buffer);
}

/* Pseudocode
Imagine a snake:
[[(6, 1), (7, 1), (7, 2), (7, 3), (7, 4), (8, 4), (9, 4), (10, 4), (10, 3), (10, 2), (10, 1), (10, 0)]
[(5, 1), (6, 1), (7, 1), (7, 2), (7, 3), (7, 4), (8, 4), (9, 4), (10, 4), (10, 3), (10, 2), (10, 1)]
[(4, 1), (5, 1), (6, 1), (7, 1), (7, 2), (7, 3), (7, 4), (8, 4), (9, 4), (10, 4), (10, 3), (10, 2)]
[(3, 1), (4, 1), (5, 1), (6, 1), (7, 1), (7, 2), (7, 3), (7, 4), (8, 4), (9, 4), (10, 4), (10, 3)]
[(2, 1), (3, 1), (4, 1), (5, 1), (6, 1), (7, 1), (7, 2), (7, 3), (7, 4), (8, 4), (9, 4), (10, 4)]
[(1, 1), (2, 1), (3, 1), (4, 1), (5, 1), (6, 1), (7, 1), (7, 2), (7, 3), (7, 4), (8, 4), (9, 4)]
]
wtf is the pattern
direction rn is (0, -1) (Left)
looks like i might have to start recreating the snake everytime, and find an efficient way later

*/