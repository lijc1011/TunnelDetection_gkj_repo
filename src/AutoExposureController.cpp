//#include "stdafx.h"
#include "AutoExposureController.h"

AutoExposureController::AutoExposureController(double target, double kp, double ki)
{
	target_brightness_ = target;
	kp_ = kp;
	ki_ = ki;
	integral_error_ = 0.0;
}
AutoExposureController::~AutoExposureController()
{

}
double AutoExposureController::clamp(double value, double min_val, double max_val)
{
	return std::max(min_val, std::min(value, max_val));
}
void AutoExposureController::adjust(const unsigned char* image,
	int image_height, int image_width,
	float& currentExposure, float& currentGain, // ʹ���������޸�ֵ
	float minExposure, float maxExposure,
	float minGain, float maxGain)
{
	long long pixel_count = static_cast<long long>(image_height) * image_width;
	if (pixel_count == 0) return;

	// 1. ����
	double sum = 0.0;
	for (long long i = 0; i < pixel_count; ++i) {
		sum += image[i];
	}
	double current_brightness = sum / pixel_count;

	// 2. �Ƚ�
	double error = target_brightness_ - current_brightness;

	// 3. ������
	double p_term = kp_ * error;
	integral_error_ += error;
	integral_error_ = clamp(integral_error_, -100.0, 100.0);
	double i_term = ki_ * integral_error_;

	double adjustment_factor = 1.0 + (p_term + i_term) / 100.0;

	// 4. ִ���� & 5. ����
	double current_brightness_scale = currentExposure * currentGain;
	double new_brightness_scale = current_brightness_scale * adjustment_factor;

	double new_exposure = new_brightness_scale / currentGain;
	new_exposure = clamp(new_exposure, minExposure, maxExposure);

	double new_gain = currentGain;
	if (new_exposure >= maxExposure) {
		new_gain = new_brightness_scale / maxExposure;
		new_gain = clamp(new_gain, minGain, maxGain);
	}
	if (new_exposure <= minExposure) {
		new_gain = new_brightness_scale / minExposure;
		new_gain = clamp(new_gain, minGain, maxGain);
	}

	currentExposure = clamp(new_brightness_scale / new_gain, minExposure, maxExposure);
	currentGain = new_gain;
}