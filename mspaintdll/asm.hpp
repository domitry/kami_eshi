#define CToolBoxWnd 0x002C89E4
#define SelectTool   0x0102C98F
#define SetDrawColor 0x0100C255

void change_color(unsigned int color){
    __asm{
        pushad;
        push color;
        mov eax, SetDrawColor;
        call eax;
        popad;
    }
    return;
}
