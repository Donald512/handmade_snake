#include <windows.h>
#include <stdint.h>

#define errorToReturn 1
#define true 1
#define false 0

#define boardWidth 640
#define boardHeight 480

#define bytesPerPixel 4
#define size 10

#define minimumGameSpeed 50

#define rightWall (boardWidth - 3*size)
#define bottomWall (boardHeight - 5*size)

#define maxNumPartsSnake (rightWall/size * bottomWall/size)


#define black 0x00000000
#define white 0x00FFFFFF    // why is it ARGB instead of ABGR or BGRA on mon portable
#define red   0x00FF0000   

#define largePrimeNumber 1103515245
#define smallConstant 12345 // make sure seed doesnt become 0
#define cleaner ~(1 << 31) // this is 01111111 11111111 11111111 11111111 to make sure the seed is positive


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

u32 snakeSize = maxNumPartsSnake * sizeof(square);

LRESULT Win32MainWindowCallback(HWND hwnd, UINT event, WPARAM info1, LPARAM info2);
void createBoard();
void drawSquare(square sqr);
void eraseSquare(square sqr);
void updateScreen(HDC deviceContext);
void clearBoard();
void spawnSnake(square snake[], u16 length);
bool32 addStructs(square *head, direction dir);
void drawSnake(square snake[], u16 length);
void createSnake();
void moveSnake();
u32 getRandom();
i32 random(i32 min, i32 max);
void generateApple();
void drawApple();
bool32 headOnApple();

BITMAPINFO bmpInfo = {0};
void* imagePtr = NULL;
void* snakePtr = NULL;

bool32 RUNNING = true;
square testSqr = {600, 400};
square lastSqr = {600, 400};
direction headDir = {0, 0};

u16 currentLevel = 1;
square apple;
bool32 FAILED = false;

u16 gameSpeedms = 200;    // 500 ms
u64 numTicksPerGameSpeedms;

u32 seed;

int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){ // entry point for Windows GUI
    LARGE_INTEGER ticksPerSecResult;
    QueryPerformanceFrequency(&ticksPerSecResult);
    i64 ticksPerSec = ticksPerSecResult.QuadPart;
    u64 ticksPerMs = ticksPerSec/1000;
    numTicksPerGameSpeedms = gameSpeedms * ticksPerMs;
    
    LARGE_INTEGER lastNumTicks, currentNumTicks;
    QueryPerformanceCounter(&lastNumTicks);
    seed = lastNumTicks.LowPart;     // happens once



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
    createSnake();
    generateApple();
    HDC deviceContext = GetDC(windowHandle);


    while (RUNNING){
        numTicksPerGameSpeedms = gameSpeedms * ticksPerMs;
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE /*Remove from queue after checking*/)){
            if (msg.message == WM_QUIT){
                RUNNING = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        QueryPerformanceCounter(&currentNumTicks);
        if ((u64)currentNumTicks.QuadPart - (u64)lastNumTicks.QuadPart >= numTicksPerGameSpeedms){
            if (headOnApple()){
                currentLevel++;
                eraseSquare(apple);
                generateApple();
                if (currentLevel && !(currentLevel % 10) && gameSpeedms < minimumGameSpeed){
                    gameSpeedms -= 10;
                }
            }
            eraseSquare(((square*)snakePtr)[currentLevel - 1]); 
            moveSnake();
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
            // OutputDebugString("Test\t");
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
                        }
                        // OutputDebugString("Up\t");
                    }   break;
                    case VK_DOWN:{
                        if (headDir.down >= 0){ 
                            headDir.right = 0;
                            headDir.down = size;
                        }
                        // OutputDebugString("Down\t");
                    }   break;
                    case VK_LEFT:{
                        if (headDir.right <= 0){ 
                            headDir.right = -size;
                            headDir.down = 0;
                        }
                        // OutputDebugString("Left\t");
                    }   break;
                    case VK_RIGHT:{
                        if (headDir.right >= 0){ 
                            headDir.right = size;
                            headDir.down = 0;
                        }
                        // OutputDebugString("Right\t");
                    }   break;
                    // case 'W':{
                    //     currentLevel++;
                    // }
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



void drawSquare(square sqr){
    i32* pixels = (i32*) imagePtr;
    
    for (i32 i = sqr.y; i < sqr.y + size; i++){
        for (i32 j = sqr.x; j < sqr.x + size; j++ ){
            *(pixels + (i * boardWidth + j)) = white;
        }
    }
}

void eraseSquare(square sqr){
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

void drawSnake(square snake[], u16 length){
    for (u16 i = 0; i < length; i++){
        drawSquare(snake[i]);
    }
}

bool32 addStructs(square *head, direction dir){
    i16 resultX = head->x + dir.right;
    i16 resultY = head->y + dir.down;
    if (dir.right){
        if (resultX >= 0 && resultX <= rightWall){
            head->x = resultX;
        }
        else {
            return false;
        }
    }
    else if (dir.down){
        if (resultY >= 0 && resultY <= bottomWall){
            head->y = resultY;
        }
        else {
            return false;
        }
    }
    return true;
}

void createSnake(){
    // pointer returned to start of memory will be head
    snakePtr = VirtualAlloc(0, snakeSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    square* head = snakePtr;

    head[0].x = random(0, rightWall/size) * size;  // makes sure its a multiple of size
    head[0].y = random(0, bottomWall/size) * size; 
    drawSquare(head[0]);
}

void moveSnake(){
    square* head = snakePtr;
    square headCopy = head[0];
    if (!addStructs(&headCopy, headDir)){
        RUNNING = false;
        return;
    }
    u16 i = currentLevel;
    for ( ; i > 1; i--){
        head[i - 1] = head[i - 2];
    }
    head[0] = headCopy;
    // ! must do body collision check inside this function, instead of addStructs because it takes a copy
    for (u16 i = 1; i < currentLevel; i++){
        if (head[0].x == head[i].x && head[0].y == head[i].y){
            RUNNING = false;
            return;
        }
    }
    
    drawSnake(head, currentLevel);
}


u32 getRandom(){
    seed = (seed * largePrimeNumber + smallConstant) & cleaner;   
    return seed;
}

i32 random(i32 min, i32 max){
    return getRandom() % (max - min + 1) + min;
}

void generateApple(){
    square* head = snakePtr;
    while (true){
        u16 x = random(0, rightWall/size) * size;  // makes sure its a multiple of size
        u16 y = random(0, bottomWall/size) * size;
        u16 i = 0; 
        for ( ; i < currentLevel; i++){
            if (head[i].x == x && head[i].y == y ){
                break;
            }
        }
        if (i == currentLevel){ // that means the loop finished and no overlap was found
            apple.x = x;
            apple.y = y;
            drawApple();
            break;
        }
    }
}

void drawApple(){
    i32* pixels = (i32*) imagePtr;
    
    for (i32 i = apple.y; i < apple.y + size; i++){
        for (i32 j = apple.x; j < apple.x + size; j++ ){
            *(pixels + (i * boardWidth + j)) = red;
        }
    }
}

bool32 headOnApple(){
    square* head = snakePtr;
    if (head[0].x == apple.x && head[0].y == apple.y){
        return true;
    }
    return false;
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

*/