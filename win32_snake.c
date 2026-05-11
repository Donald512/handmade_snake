#include <windows.h>
#include <stdint.h>

#define errorToReturn 1
#define true 1
#define false 0

#define boardWidth 640
#define boardHeight 480
#define bytesPerPixel 4


#define black 0x00000000
#define white 0x00FFFFFF    // little endian, in memory stored ABGR


typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t i64;
typedef uint32_t bool32;

u32 imageSize = boardWidth * boardHeight * bytesPerPixel;    // in bytes


typedef struct{
    u16 x;
    u16 y;
    u8 size;
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

BITMAPINFO bmpInfo = {0};
void* imagePtr = NULL;

bool32 RUNNING = true;
square testSqr = {600, 400, 10};
square lastSqr = {600, 400, 10};
direction dir = {0, 0};
int CALLBACK WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){ // entry point for Windows GUI
    LARGE_INTEGER ticksPerSecResult;
    QueryPerformanceFrequency(&ticksPerSecResult);
    i64 ticksPerSec = ticksPerSecResult.QuadPart;
    

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
    addSquare(testSqr);
    HDC deviceContext = GetDC(windowHandle);
    while (RUNNING){
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE /*Remove from queue after checking*/)){
            if (msg.message == WM_QUIT){
                RUNNING = false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (lastSqr.x != testSqr.x || lastSqr.y != testSqr.y){
            removeSquare(lastSqr);
            addSquare(testSqr);
            char buffer[256];
            wsprintf(buffer, "x, y: %d, %d\n", testSqr.x, testSqr.y);
            OutputDebugString(buffer);
            lastSqr.x = testSqr.x;
            lastSqr.y = testSqr.y;
        }

        updateScreen(deviceContext);
    }

    return 0;
}

LRESULT Win32MainWindowCallback(HWND hwnd, UINT event, WPARAM info1, LPARAM info2){
    LRESULT result = 0;
    switch(event){
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
                        if (testSqr.y >= testSqr.size){
                            testSqr.y -= testSqr.size;
                        }
                    }   break;
                    case VK_DOWN:{
                        if (testSqr.y < boardHeight - 5*testSqr.size){
                            testSqr.y += testSqr.size;
                        }
                    }   break;
                    case VK_LEFT:{
                        if (testSqr.x >= testSqr.size){
                            testSqr.x -= testSqr.size;
                        }
                    }   break;
                    case VK_RIGHT:{
                        if (testSqr.x <= boardWidth - 4*testSqr.size){
                            testSqr.x += testSqr.size;
                        }
                    }   break;
                }
            }
        }
        default:{
            result = DefWindowProc(hwnd, event, info1, info2);
        }
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
    u32* pixels = (u32*) imagePtr;
    
    for (u32 i = sqr.y; i < sqr.y + sqr.size; i++){
        for (u32 j = sqr.x; j < sqr.x + sqr.size; j++ ){
            *(pixels + (i * boardWidth + j)) = white;
        }
    }
}

void removeSquare(square sqr){
    u32* pixels = (u32*) imagePtr;

    for (u32 i = sqr.y; i < sqr.y + sqr.size; i++){
        for (u32 j = sqr.x; j < sqr.x + sqr.size; j++ ){
            *(pixels + (i * boardWidth + j)) = black;
        }
    }
}

void updateScreen(HDC deviceContext){
    StretchDIBits(deviceContext,0, 0, boardWidth, boardHeight, 0, 0, boardWidth, boardHeight, imagePtr, &bmpInfo, DIB_RGB_COLORS, SRCCOPY);
}
