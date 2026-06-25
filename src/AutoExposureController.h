#pragma once
#include "Globals.h"

class AutoExposureController 
{
private:
	double target_brightness_;
	double kp_;
	double ki_;
	double integral_error_;

	double clamp(double value, double min_val, double max_val);
	 

public:
 	AutoExposureController(double target = 128.0, double kp = 0.5, double ki = 0.1);
	~AutoExposureController();

	void adjust(const unsigned char* image,
		int image_height, int image_width,
		float& currentExposure, float& currentGain, // 使用引用来修改值
		float minExposure, float maxExposure,
		float minGain, float maxGain);
};