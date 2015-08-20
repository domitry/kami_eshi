#define _USE_MATH_DEFINES
#include <windows.h>
#include <process.h>
#include <cmath>
#include <cstdio>
#include <random>
#include <cstdlib>
#include <Commctrl.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include "controller.hpp"
#include "image_parser.hpp"
#include "segment/image.h"
#include "segment/misc.h"
#include "segment/pnmfile.h"
#include "segment/segment-image.h"

using namespace cv;
static uintptr_t thread;

void open_console(){
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONIN$", "r", stdin);
}

// Draw red circle for debugging.
void drawCircle(Controller &c){
	c.setCanvasSize(500, 500);
	c.changeColor(0, 0, 0);

	for(int i=0; i<5000; i++){
		double con = 2*M_PI/5000;
		c.markPoint(250+250*cos(con*i), 250+250*sin(con*i));
		Sleep(1);
	}

	Sleep(10);
	c.changeColor(30, 100, 80);
	c.fill(250, 250);
}

void segment(float sigma, float k, int min_size, const char* fname, const char* output){
	image<rgb> *input = loadPPM(fname);
	int num_ccs;
	image<rgb> *seg = segment_image(input, sigma, k, min_size, &num_ccs); 
	savePPM(seg, output);
}

unsigned int WINAPI ThreadFunc(void* arg){
	const int BUFF_SIZE = 200;

	char input[BUFF_SIZE];
	char output[BUFF_SIZE];
	char cdir[BUFF_SIZE];
	char tmp_i[BUFF_SIZE];
	char tmp_s[BUFF_SIZE];
	float sigma, k; int min_size, sleep_time, frag_debug;

	puts("--------------------------------------");
	puts("|--- KAMI ESHI v0.1.0 by @domitry ---|");
	puts("--------------------------------------");
	printf("\n");
	puts("This software is distributed under the GPL version 2.0.");
	puts("Read LICENSE.txt for more details.");
	printf("\n");
	puts("Copyright (C) 2015 Naoki Nishida");
	puts("Copyright (C) 2006 Pedro Felzenszwalb");
	printf("\n\n--\n");

	printf("Input file path:\n");
	scanf("%100s", input);

	printf("Output file name:\n");
	scanf("%100s", output);

	printf("Specify parameters: sigma, k, min_size (typically 0.5 2000 20)\n");
	scanf("%f %f %d", &sigma, &k, &min_size);

	printf("Specify sleeping time (mili sec, typically 4):\n");
	scanf("%d", &sleep_time);

	printf("Create debug picture? (0:false, the others: true): \n");
	scanf("%d", &frag_debug);

	GetCurrentDirectoryA(BUFF_SIZE, cdir);
	strncpy(tmp_i, cdir, BUFF_SIZE);
	strncpy(tmp_s, cdir, BUFF_SIZE);
	strncat(tmp_i, "\\tmp_i.ppm", BUFF_SIZE);
	strncat(tmp_s, "\\tmp_s.ppm", BUFF_SIZE);

	auto original = imread(input);
	imwrite(tmp_i, original);

	printf("segmenting image... \n");
	segment(sigma, k, min_size, tmp_i, tmp_s);
	auto seg_img = imread(tmp_s);

	if(frag_debug){
		char debug[BUFF_SIZE];
		strncpy(debug, cdir, BUFF_SIZE);
		strncat(debug, "\\debug.png", BUFF_SIZE);
		imwrite(debug, seg_img);
	}

	printf("parsing...\n");
	auto regions = parseImage(seg_img, original);
	auto s = seg_img.size();

	printf("sorting...\n");
	std::sort(regions.begin(), regions.end(), [](Region a, Region b){
		return a.size() > b.size();
	});

	printf("done! \n");

	auto controller = Controller();
	controller.setCanvasSize(s.width, s.height);

	std::random_device r;
	auto mt = std::mt19937(r());

	for(auto r=regions.begin(); r!=regions.end(); r++){
		printf("begin rendering...%d\n", r-regions.begin());
		controller.changeColor(0, 0, 0);
		for(auto p=r->border.begin(); p!=r->border.end(); p++){
			controller.markPoint(p->x, p->y);
			
			if(GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState('C')){
				printf("intrrupted.\n");
				return -1;
			};
			
			Sleep(sleep_time);
		}
		if(r->inside.size() > 0){
			auto color = r->color;
			printf("change color to %08x\n", colorByte(color));
			controller.changeColor(color[2], color[1], color[0]);

			for(auto sr : r->sub_regions){
				auto rnd = std::uniform_int_distribution<int>(0, sr.size() - 1);
				auto p = sr[rnd(mt)];
				controller.fill(p.x, p.y);
			}

		}else{
			printf("No pixel to fill!\n");
		}
		Sleep(1000);
	}

	controller.save(output);
	DeleteFileA(tmp_i);
	DeleteFileA(tmp_s);

	return 0;
}
 
INT APIENTRY DllMain(HMODULE hDLL, DWORD Reason, LPVOID Reserved) {
	DisableThreadLibraryCalls(hDLL);
	switch(Reason) {
	case DLL_PROCESS_ATTACH:
		open_console();
		printf("attached to the process.:) \n\n");
		thread = _beginthreadex(NULL, 0, ThreadFunc, NULL, NULL, NULL);
		break;
	case DLL_PROCESS_DETACH:
		printf("DLL ditatched.");
		if(thread)_endthreadex(thread);
		break;
	case DLL_THREAD_ATTACH:
		printf("attached to THREAD.\n");
		break;
	case DLL_THREAD_DETACH:
		printf("detached fron THREAD");
		break;
	}
	return TRUE;
}
