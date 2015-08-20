#define FILL_TOOL 0x0026F
#define PENCIL_TOOL 0x0027C

#define CToolBoxWnd 0x005088FC
#define SelectTool  0x0102C98F

_declspec(naked) void changeTool(short tool_type){
	__asm{
		pushad;
		push tool_type;
		mov ecx, CToolBoxWnd;
		call SelectTool;
		popad;
	}
}