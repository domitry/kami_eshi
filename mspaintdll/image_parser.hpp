#include <cmath>
#include <map>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include "region.hpp"
#include "labeling.hpp"
using namespace cv;

typedef short LABELED_TYPE;
typedef unsigned int uint;

std::vector<uint> neighbors8(Point p, Mat &img){
	auto ret = std::vector<uint>();
	for(int dx=-1;dx<=1;dx++){
		for(int dy=-1;dy<=1;dy++){
			if(dx==0 && dy==0)continue;
			int x = p.x + dx, y= p.y + dy;
			if(x<0 || x>= img.cols || y<0 || y>=img.rows)continue;
			auto c = colorByte(img.ptr<Vec3b>(y)[x]);
			ret.push_back(c);
		}
	}
	return ret;
}

bool isNeighbor(Point &a, Point &b){
	auto dx = a.x - b.x;
	auto dy = a.y - b.y;
	return (dx==1 || dx==-1 || dx==0) && (dy==1 || dy==-1 || dy==0);
}

std::vector<Region> parseImage(Mat &segment_img, Mat &original_img){
	std::map<uint, Region> regions;
	auto buff = Mat(segment_img.size(), CV_32SC1); // segmented image with border filled in the color "-1"
	
	// divide one image into some regions
	for(int h=0; h<segment_img.rows; h++){
		Vec3b *ptr = segment_img.ptr<Vec3b>(h);
		for(int w=0; w<segment_img.cols; w++){
			auto c = colorByte(ptr[w]);
			auto p = Point(w, h);
			auto ns = neighbors8(p, segment_img);
			
			if(regions.find(c) == regions.end())regions[c] = Region(c);
			if(std::all_of(ns.begin(), ns.end(), [&c](uint v){return v == c;})){
				regions[c].inside.push_back(p);
				buff.at<int>(h, w) = c;
			}else{
				regions[c].border.push_back(p);
				buff.at<int>(h, w) = -1;
			}
		}
	}

	// labeling
	auto labeled = labelImage(buff);
	auto labeled_hash = std::map<uint, std::vector<std::vector<Point>>>();
	for(auto i : labeled){
		auto col = i.first;
		auto points = i.second;
		if(labeled_hash.find(col) == labeled_hash.end())
			labeled_hash[col] = std::vector<std::vector<Point>>();
		labeled_hash[col].push_back(points);
	}
	
	for(auto i=regions.begin(); i!=regions.end(); i++){
		auto r = &(i->second);
		r->sub_regions = labeled_hash[r->id_color];

		if(r->sub_regions.size()==0 && r->inside.size() != 0){
			printf("error!! : the number of sub-regions is zero even the size of inside pixels is not zero.\n");
			break;
		}

		// set the representative color
		r->setColor(original_img);
		
		// sort "border" pixels
		auto border_path = std::vector<Point>();

		border_path.push_back(*r->border.rbegin());
		r->border.pop_back();

		while(r->border.size() > 0){
			auto prev_size = r->border.size();
			std::vector<Point>::iterator erase;

			for(auto j = r->border.begin(); j != r->border.end(); j++){
				if(isNeighbor(*j, *border_path.rbegin())){
					border_path.push_back(*j);
					r->border.erase(j);
					break;
				}
			}
			if(prev_size == r->border.size()){
				auto p = *r->border.rbegin();
				border_path.push_back(p);
				r->border.pop_back();
			}
		}

		r->border = border_path;
	}

	auto ret = std::vector<Region>();
	for(auto i=regions.begin(); i!=regions.end(); i++){
		ret.push_back(i->second);
	}
	return ret;
}
