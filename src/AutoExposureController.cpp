#include "AutoExposureController.h"

#include <algorithm>
#include <array>
#include <cmath>

AutoExposureController::AutoExposureController(double minBrightness, double maxBrightness,
	double kp, double ki)
	: min_brightness_(minBrightness)
	, max_brightness_(maxBrightness)
	, kp_(kp)
	, ki_(ki)
	, integral_error_(0.0)
	, max_step_ratio_(0.08)
	, trim_fraction_(0.05)
	, sample_stride_(4)
{
}

void AutoExposureController::configure(double minBrightness, double maxBrightness,
	double maxStepRatio, double trimPercent)
{
	min_brightness_ = clamp(minBrightness, 1.0, 253.0);
	max_brightness_ = clamp(maxBrightness, min_brightness_ + 1.0, 254.0);
	max_step_ratio_ = clamp(maxStepRatio, 0.01, 0.25);
	trim_fraction_ = clamp(trimPercent / 100.0, 0.0, 0.20);
	reset();
}

void AutoExposureController::reset()
{
	integral_error_ = 0.0;
}

double AutoExposureController::clamp(double value, double min_val, double max_val)
{
	return std::max(min_val, std::min(value, max_val));
}

double AutoExposureController::measureBrightness(const unsigned char* image,
	int image_height, int image_width, int channels) const
{
	if (image == nullptr || image_height <= 0 || image_width <= 0)
		return -1.0;
	if (channels != 1 && channels != 3)
		return -1.0;

	std::array<unsigned long long, 256> histogram{};
	unsigned long long sampleCount = 0;
	const int stride = std::max(1, sample_stride_);

	for (int y = 0; y < image_height; y += stride)
	{
		for (int x = 0; x < image_width; x += stride)
		{
			const long long pixelIndex = static_cast<long long>(y) * image_width + x;
			int gray = 0;
			if (channels == 1)
			{
				gray = image[pixelIndex];
			}
			else
			{
				const unsigned char* rgb = image + pixelIndex * 3;
				gray = (77 * rgb[0] + 150 * rgb[1] + 29 * rgb[2] + 128) >> 8;
			}
			++histogram[gray];
			++sampleCount;
		}
	}

	if (sampleCount == 0)
		return -1.0;

	// 去掉两端少量像素，避免电缆、绝缘子和局部反光主导整幅图的亮度。
	unsigned long long trimEachSide =
		static_cast<unsigned long long>(sampleCount * trim_fraction_);
	if (trimEachSide * 2 >= sampleCount)
		trimEachSide = 0;

	auto trimmed = histogram;
	unsigned long long remaining = trimEachSide;
	for (int value = 0; value < 256 && remaining > 0; ++value)
	{
		const unsigned long long remove = std::min(remaining, trimmed[value]);
		trimmed[value] -= remove;
		remaining -= remove;
	}
	remaining = trimEachSide;
	for (int value = 255; value >= 0 && remaining > 0; --value)
	{
		const unsigned long long remove = std::min(remaining, trimmed[value]);
		trimmed[value] -= remove;
		remaining -= remove;
	}

	double sum = 0.0;
	unsigned long long kept = 0;
	for (int value = 0; value < 256; ++value)
	{
		sum += static_cast<double>(value) * trimmed[value];
		kept += trimmed[value];
	}
	return kept > 0 ? sum / kept : -1.0;
}

AutoExposureAdjustment AutoExposureController::adjust(const unsigned char* image,
	int image_height, int image_width, int channels,
	double currentExposure, double currentGain,
	double minExposure, double maxExposure,
	double minGain, double maxGain)
{
	AutoExposureAdjustment result;
	result.exposure = currentExposure;
	result.gain = currentGain;

	if (minExposure <= 0.0 || maxExposure < minExposure || maxGain < minGain ||
		currentExposure <= 0.0 || !std::isfinite(currentExposure) || !std::isfinite(currentGain))
	{
		return result;
	}

	result.brightness = measureBrightness(image, image_height, image_width, channels);
	if (result.brightness < 0.0)
		return result;
	result.valid = true;

	if (result.brightness >= min_brightness_ && result.brightness <= max_brightness_)
	{
		// 落入本通道的可接受范围后保持相机参数，并逐步释放积分。
		integral_error_ *= 0.5;
		return result;
	}

	const double boundary = result.brightness < min_brightness_
		? min_brightness_ : max_brightness_;
	const double error = boundary - result.brightness;
	const double rangeCenter = (min_brightness_ + max_brightness_) * 0.5;
	const double relativeError = error / std::max(1.0, rangeCenter);
	if (integral_error_ * relativeError < 0.0)
		integral_error_ = 0.0;
	integral_error_ = clamp(integral_error_ + relativeError, -1.0, 1.0);
	double factor = 1.0 + kp_ * relativeError + ki_ * integral_error_;
	factor = clamp(factor, 1.0 - max_step_ratio_, 1.0 + max_step_ratio_);

	double newExposure = currentExposure;
	double newGain = currentGain;
	if (factor > 1.0)
	{
		// 需要增亮：先增加曝光。只有曝光达到上限，才用增益补足剩余量。
		newExposure = clamp(currentExposure * factor, minExposure, maxExposure);
		const double exposureFactor = newExposure / currentExposure;
		const double residual = factor / exposureFactor;
		if (residual > 1.000001)
			newGain = clamp(currentGain + 20.0 * std::log10(residual), minGain, maxGain);
	}
	else
	{
		// 需要变暗：优先撤掉已有增益，避免在较短曝光下保留不必要的噪声。
		newGain = clamp(currentGain + 20.0 * std::log10(factor), minGain, maxGain);
		const double gainFactor = std::pow(10.0, (newGain - currentGain) / 20.0);
		const double residual = factor / gainFactor;
		if (residual < 0.999999)
			newExposure = clamp(currentExposure * residual, minExposure, maxExposure);
	}

	result.exposure = newExposure;
	result.gain = newGain;
	result.changed = std::abs(newExposure - currentExposure) > 0.0001 ||
		std::abs(newGain - currentGain) > 0.0001;
	return result;
}
