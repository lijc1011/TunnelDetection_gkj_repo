//#include "stdafx.h"
#include "AutoExposureController.h"

AutoExposureController::AutoExposureController(double target, double kp, double ki)
{
	target_brightness_ = target;
	kp_ = kp;
	ki_ = ki;
	integral_error_ = 0.0;
	deadband_ = 4.0;        // 亮度误差在 ±4 内不调整
	max_step_ratio_ = 0.15; // 单次最多改 ±15%
	sample_stride_ = 8;     // 每隔 8 个像素取 1 个测亮度(A3 抽样降 CPU)
}
AutoExposureController::~AutoExposureController()
{

}
void AutoExposureController::reset()
{
	integral_error_ = 0.0;
}
double AutoExposureController::clamp(double value, double min_val, double max_val)
{
	return std::max(min_val, std::min(value, max_val));
}
void AutoExposureController::adjust(const unsigned char *image,
									int image_height, int image_width,
									float &currentExposure, float &currentGain, // 使用引用来修改值
									float minExposure, float maxExposure,
									float minGain, float maxGain)
{
	if (image == nullptr) return;
	long long pixel_count = static_cast<long long>(image_height) * image_width;
	if (pixel_count <= 0) return;

	// 1. 测量：抽样求平均亮度(A3)
	long long step = sample_stride_ > 0 ? sample_stride_ : 1;
	double sum = 0.0;
	long long n = 0;
	for (long long i = 0; i < pixel_count; i += step)
	{
		sum += image[i];
		++n;
	}
	if (n == 0) return;
	double current_brightness = sum / n;

	// 2. 误差 + 死区(A2)：亮度已够接近目标就不动，避免抖动
	double error = target_brightness_ - current_brightness;
	if (std::abs(error) < deadband_) return;

	// 3. PI 控制器
	double p_term = kp_ * error;
	integral_error_ += error;
	integral_error_ = clamp(integral_error_, -100.0, 100.0); // 抗积分饱和
	double i_term = ki_ * integral_error_;

	// 亮度校正系数：误差归一化到亮度中值(128)，再限速到 ±max_step_ratio_
	double factor = 1.0 + (p_term + i_term) / 128.0;
	factor = clamp(factor, 1.0 - max_step_ratio_, 1.0 + max_step_ratio_);

	// 4. 曝光优先(A2)：先用曝光吃掉校正
	double desired = static_cast<double>(currentExposure) * factor;
	double new_exposure = clamp(desired, minExposure, maxExposure);

	// 5. 增益兜底：仅当曝光顶到限值仍不够时，用残余系数调增益。
	//    残余系数 = 期望/实际曝光；曝光未到轨则≈1，增益不动。
	//    增益按 dB 在线性域换算(亮度∝10^(dB/20))，不做除以增益的运算，杜绝除零。
	double new_gain = currentGain;
	double residual = (new_exposure > 0.0) ? (desired / new_exposure) : 1.0;
	if (residual > 1.000001 || residual < 0.999999)
	{
		double lin = std::pow(10.0, static_cast<double>(currentGain) / 20.0);
		lin *= residual;
		double lin_min = std::pow(10.0, static_cast<double>(minGain) / 20.0);
		double lin_max = std::pow(10.0, static_cast<double>(maxGain) / 20.0);
		lin = clamp(lin, lin_min, lin_max);
		new_gain = 20.0 * std::log10(lin);
		new_gain = clamp(new_gain, minGain, maxGain);
	}

	currentExposure = static_cast<float>(new_exposure);
	currentGain = static_cast<float>(new_gain);
}
