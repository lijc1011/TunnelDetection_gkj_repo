#pragma once
#include "Globals.h"

// 单台相机的自动亮度控制器（每台一个实例，状态互不干扰）。
// 策略：曝光优先、增益兜底，两者都在 config 限值内。
// 亮度测量抽样、死区、单步限速，避免带管线延迟时振荡。
class AutoExposureController
{
private:
	double target_brightness_;  // 目标平均亮度(0-255)
	double kp_;                 // 比例系数
	double ki_;                 // 积分系数
	double integral_error_;     // 积分累计(带抗饱和)
	double deadband_;           // 亮度死区：|误差|小于它不动作
	double max_step_ratio_;     // 单次最大调整比例(如0.15=±15%)
	long long sample_stride_;   // 测亮度抽样步长(每隔N个像素取1个)

	double clamp(double value, double min_val, double max_val);

public:
 	AutoExposureController(double target = 128.0, double kp = 0.3, double ki = 0.02);
	~AutoExposureController();

	// 清积分：手动↔自动切换、重连时调用，避免残留积分导致跳变
	void reset();

	void adjust(const unsigned char* image,
		int image_height, int image_width,
		float& currentExposure, float& currentGain, // 使用引用来修改值
		float minExposure, float maxExposure,
		float minGain, float maxGain);
};
