#include <windows.h>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cstdio>
#include <Commctrl.h>
#include "asm.hpp"

#define FLOOD_TOOL 1
#define PENCIL_TOOL 2

int pow(int a, int b){
    int ret=1;
    for(;b>0;b--)ret*=a;
    return ret;
}

void pressNumber(int num){
    int digit;
    for(digit=0; num/pow(10, digit); digit++);
    for(--digit;digit>=0;digit--){
        keybd_event(48+num/pow(10, digit), 0, 0, 0);
        Sleep(10);
        num %= pow(10, digit) ;
    }
}

class Controller{
public:
    Controller();
    void changeColor(unsigned char, unsigned char, unsigned char);
    void markPoint(int, int);
    void fill(int, int);
    void setCanvasSize(int, int);
    void save(char *);
    void changeTool(int tool);

private:
    POINT LocalToScreen(int x, int y);
    HWND findMainWindow();
    HWND findDrawingWindow();
    HWND findToolWindow();
    void Controller::setToolPositions();

    RECT context_rect;
    int context_offset;
    HWND main_window, drawing_window, tool_window;
    POINT pencil_tool, flood_tool;
};

Controller::Controller(){
    context_offset = 3;
    main_window = findMainWindow();
    ShowWindow(main_window, SW_MAXIMIZE);
    SetForegroundWindow(main_window);

    drawing_window = findDrawingWindow();
    tool_window = findToolWindow();
    setToolPositions();

    GetWindowRect(drawing_window, &context_rect);
}

BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM IParam){
    DWORD pid;
    GetWindowThreadProcessId(hWnd, &pid);
    if(pid == GetCurrentProcessId() && IsWindowVisible(hWnd)){
        *(HWND *)(IParam) = hWnd;
    }
    return true;
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM IParam){
    RECT rect;
    GetWindowRect(hwnd, &rect);
    ((std::vector<std::pair<HWND, RECT>> *)IParam)->push_back(std::pair<HWND, RECT>(hwnd, rect));
    return true;
}

BOOL CALLBACK EnumChildProc2(HWND hwnd, LPARAM IParam){
    ((std::vector<HWND> *)IParam)->push_back(hwnd);
    return true;
}

HWND Controller::findMainWindow(){
    HWND window = NULL;
    EnumWindows(EnumWndProc, (LPARAM)(&window));
    return window;
}

// Find drawing region of mspaint
HWND Controller::findDrawingWindow(){
    HWND ret = NULL;
    typedef std::pair<HWND, RECT> pair;

    auto vec = std::vector<pair>();
    EnumChildWindows(main_window, EnumChildProc, (LPARAM)&vec);

    std::sort(vec.begin(), vec.end(), [](pair &a, pair &b) -> bool{
        return (int)a.second.left > (int)b.second.left;
    });

    return vec[1].first;
}

HWND Controller::findToolWindow(){
    typedef std::pair<HWND, RECT> pair;
    auto vec = std::vector<pair>();
    EnumChildWindows(main_window, EnumChildProc, (LPARAM)&vec);
    auto filtered = std::vector<pair>();

    // x coordinate of tool window is 0
    std::copy_if(vec.begin(), vec.end(), back_inserter(filtered), [](pair &a){
        return (a.second.left == 0);
    });

    std::sort(filtered.begin(), filtered.end(), [](pair &a, pair &b) -> bool{
        return (int)a.second.top < (int)b.second.top;
    });
    // y coordinate of tool window is the second among all child windows
    for(auto i=0; i<=1; i++){
        auto p = filtered[i];
        auto v = std::vector<HWND>();
        EnumChildWindows(p.first, EnumChildProc2, (LPARAM)&v);
        // tool window has one parent window
        if(v.size()==1)return v[0];
    }
    return NULL;
}

void Controller::setToolPositions(){
    RECT rect;
    GetWindowRect(tool_window, &rect);
    auto tool_width = (rect.right - rect.left)/2;
    pencil_tool.x = rect.left + tool_width/2;
    pencil_tool.y = rect.top + (tool_width*6)/2;//? constant should be 7, not 6
    flood_tool.x = rect.left + (tool_width*3)/2;
    flood_tool.y = rect.top + (tool_width*3)/2;
}

void Controller::setCanvasSize(int width, int height){
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('E', 0, 0, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    pressNumber(width);
    keybd_event(VK_TAB, 0, 0, 0);
    pressNumber(height);
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
}

// filename should consist of ascii code
void Controller::save(char *filename){
    keybd_event(VK_CONTROL, 0, 0, 0);
    keybd_event('S', 0, 0, 0);
    keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    Sleep(1000);
    for(char* c=filename; *c!='\0'; c++){
        if(*c=='.'){
            keybd_event(110, 0, 0, 0);
        }else if(*c > 'a' && *c < 'z'){
            keybd_event((*c - 'a')+65, 0, 0, 0);
        }
        Sleep(100);
    }
    keybd_event(VK_RETURN, 0, 0, 0);
    keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
}

void Controller::changeColor(unsigned char red, unsigned char green, unsigned char blue){
    unsigned int col = 0;
    col += ((unsigned int)blue) << 16;
    col += ((unsigned int)green) << 8;
    col += red;
    change_color(col);
}

void Controller::changeTool(int tool){
    switch(tool){
    case FLOOD_TOOL:
        SetCursorPos(flood_tool.x, flood_tool.y);
        break;
    case PENCIL_TOOL:
        SetCursorPos(pencil_tool.x, pencil_tool.y);
        break;
    }
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void Controller::markPoint(int x, int y){
    auto p = LocalToScreen(x, y);
    SetCursorPos(p.x, p.y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void Controller::fill(int x, int y){
    changeTool(FLOOD_TOOL);
    markPoint(x, y);
    changeTool(PENCIL_TOOL);
}

POINT Controller::LocalToScreen(int x, int y){
    POINT p = {context_rect.left+x+context_offset, context_rect.top+y+context_offset};
    return p;
}