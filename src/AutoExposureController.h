#pragma once

// 一次自动亮度计算的结果。控制器只计算建议值，不直接访问相机，便于在
// QShowThd 中统一完成下发、错误检查和实际值回读。
struct AutoExposureAdjustment
{
	double brightness = 0.0;
	double exposure = 0.0;
	double gain = 0.0;
	bool valid = false;
	bool changed = false;
};

// 单台相机的自动亮度控制器（每台一个实例，状态互不干扰）。
// 变暗时曝光优先、增益兜底；变亮时先撤掉增益再降低曝光，以减少噪声。
class AutoExposureController
{
private:
	double min_brightness_;
	double max_brightness_;
	double kp_;
	double ki_;
	double integral_error_;
	double max_step_ratio_;
	double trim_fraction_;
	int sample_stride_;

	static double clamp(double value, double min_val, double max_val);
	double measureBrightness(const unsigned char* image,
		int image_height, int image_width, int channels) const;

public:
	AutoExposureController(double minBrightness = 75.0, double maxBrightness = 175.0,
		double kp = 0.35, double ki = 0.02);

	void configure(double minBrightness, double maxBrightness,
		double maxStepRatio, double trimPercent);
	void reset();

	AutoExposureAdjustment adjust(const unsigned char* image,
		int image_height, int image_width, int channels,
		double currentExposure, double currentGain,
		double minExposure, double maxExposure,
		double minGain, double maxGain);
};
