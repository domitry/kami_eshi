#include <map>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <math.h>

using namespace cv;
typedef unsigned int uint;

unsigned int colorByte(Vec3b color){
	unsigned int ret = 0;
	ret += color[0] << 16;
	ret += color[1] << 8;
	ret += color[2];
	return ret;
}

Vec3b revertColor(uint code){
	auto ret = Vec3b();
	ret[0] = code >> 16;
	ret[1] = (code << 16) >> 24;
	ret[2] = (code << 24) >> 24;
	return ret;
}

Vec3b mode(std::vector<Vec3b> vec){
	auto m = std::map<uint, int>();
	Vec3b ret; int num=0;
	for(auto i=vec.begin(); i!=vec.end(); i++){
		auto code = colorByte(*i);
		if(m.find(code) == m.end())m[code] = 0;
		else m[code]++;
	}
	for(auto i=m.begin(); i!=m.end(); i++)
		if(i->second > num)ret = revertColor(i->first);

	return ret;
}

Vec3b average(std::vector<Vec3b> vec){
	typedef unsigned long long ull;
	ull r=0, g=0, b=0;
	auto n = vec.size();

	for(auto i=vec.begin(); i!=vec.end(); i++){
		r += (*i)[0];
		g += (*i)[1];
		b += (*i)[2];
	}

	return Vec3b((double)r/n, (double)g/n, (double)b/n);
}

class Region{
public:
	Vec3b color;
	std::vector<Point> border;
	std::vector<Point> inside;
	std::vector<std::vector<Point>> sub_regions;
	bool isFilled;
	unsigned int id_color;
	int size();
	void setColor(Mat&);
	void setSubRegions();
	Region();
	Region(unsigned int);
};

Region::Region(unsigned int id_color_){
	id_color = id_color_;
	isFilled=false;
}

Region::Region(){
	id_color = 0;
	isFilled=false;
}

int Region::size(){
	return border.size() + inside.size();
}

void Region::setColor(Mat &original_img){
	auto colors = std::vector<Vec3b>();
	for(auto i=inside.begin(); i!=inside.end(); i++){
		colors.push_back(original_img.at<Vec3b>(i->y, i->x));
	}
	for(auto i=border.begin(); i!=border.end(); i++){
		colors.push_back(original_img.at<Vec3b>(i->y, i->x));
	}
	
	//color = mode(colors);
	color = average(colors);
}
