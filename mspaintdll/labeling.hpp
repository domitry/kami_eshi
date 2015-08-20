#include <map>
#include <opencv2/core.hpp>
using namespace cv;

typedef std::pair<unsigned int, std::vector<Point>> Segment;

// An implementetion of the following algorism:
// https://en.wikipedia.org/wiki/Connected-component_labeling
//
// buff: cv::Mat, CV_32SC1
// pixels whose color == -1 (border) will be ignored.
//
std::vector<Segment> labelImage(cv::Mat &buff){
	auto hash = std::map<int, int>();
	auto colors = std::map<int, int>();
	auto regions = std::map<int, std::vector<Point>>();
	colors[-1] = -1; // border

	auto register_point = [&buff, &regions](int h, int w, int id){
		buff.at<int>(h, w) = id;
		if(regions.find(id) == regions.end())regions[id] = std::vector<Point>();
		regions[id].push_back(Point(w, h));
	};

	auto up_and_left_ids = [buff](int h, int w){
		auto ret = std::vector<int>();
		if(h>=1)ret.push_back(buff.at<int>(h-1, w));
		if(w>=1)ret.push_back(buff.at<int>(h, w-1));
		return ret;
	};

	int current = -1; // current region id
	for(int h=0; h<buff.rows; h++){
		for(int w=0; w<buff.cols; w++){
			auto c = buff.at<int>(h, w);
			if(c == -1)continue; // border

			auto neighor_ids = up_and_left_ids(h, w);

			auto if_pixel_has_same_color_as_me = [c, &colors](int id){
				return c == colors[id];
			};

			if(std::any_of(neighor_ids.begin(), neighor_ids.end(), if_pixel_has_same_color_as_me) && neighor_ids.size()!=0){
				if(neighor_ids.size() == 1){
					register_point(h, w, neighor_ids[0]);
				}else if(!std::all_of(neighor_ids.begin(), neighor_ids.end(), if_pixel_has_same_color_as_me)){
					int new_id = (colors[neighor_ids[0]] == c ? neighor_ids[0] : neighor_ids[1]);
					register_point(h, w, new_id);
				}else if(neighor_ids[0] == neighor_ids[1]){
					auto new_id = neighor_ids[0];
					register_point(h, w, new_id);
				}else{
					//merge
					auto replaced = max(neighor_ids[0], neighor_ids[1]);
					auto new_id = min(neighor_ids[0], neighor_ids[1]);
					hash[replaced] = new_id;
					register_point(h, w, new_id);
				}
			}else{
				register_point(h, w, --current);
				colors[current] = c;
			}
		}
	}

	printf("num segments: %d\n", -current);

	auto last_id = current;
	auto ret = std::vector<Segment>();
	auto ids = std::vector<int>();

	for(; current <= -2; current++){
		if(hash.find(current) == hash.end()){
			ids.push_back(current);
			continue;
		}

		auto seek = hash[current];
		while(hash.find(seek) != hash.end())seek = hash[seek];
		hash[current] = seek;

		auto dist = &regions[seek];
		auto src = &regions[current];
		dist->insert(dist->end(), src->begin(), src->end());
	}

	printf("num unique segments: %d\n", ids.size());

	for(auto id=ids.begin(); id!=ids.end(); id++){
		auto s = new Segment(colors[*id], regions[*id]);
		ret.push_back(*s);
	}

	return ret;
}