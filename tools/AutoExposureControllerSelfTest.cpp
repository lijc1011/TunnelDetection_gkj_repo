#include "AutoExposureController.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{
bool near(double actual, double expected, double tolerance)
{
    return std::abs(actual - expected) <= tolerance;
}

int require(bool condition, const char* message)
{
    if (condition) return 0;
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
    int failures = 0;
    constexpr int width = 200;
    constexpr int height = 200;

    AutoExposureController controller;
    controller.configure(92.0, 108.0, 0.08, 5.0);

    std::vector<unsigned char> image(width * height, 50);
    auto result = controller.adjust(image.data(), height, width, 1,
                                    10.0, 1.0, 5.0, 30.0, 1.0, 3.0);
    failures += require(result.valid && result.changed, "dark image must request an adjustment");
    failures += require(near(result.brightness, 50.0, 0.01), "uniform brightness measurement");
    failures += require(result.exposure > 10.0 && near(result.gain, 1.0, 0.001),
                        "dark image must increase exposure before gain");

    controller.reset();
    std::fill(image.begin(), image.end(), 200);
    result = controller.adjust(image.data(), height, width, 1,
                               20.0, 3.0, 5.0, 30.0, 1.0, 3.0);
    failures += require(result.gain < 3.0 && near(result.exposure, 20.0, 0.001),
                        "bright image must remove gain before reducing exposure");

    controller.reset();
    result = controller.adjust(image.data(), height, width, 1,
                               20.0, 1.0, 5.0, 30.0, 1.0, 3.0);
    failures += require(near(result.gain, 1.0, 0.001) && result.exposure < 20.0,
                        "bright image at minimum gain must reduce exposure");

    controller.reset();
    std::fill(image.begin(), image.end(), 100);
    // 采样栅格上的5%纯黑和5%纯白应被截尾统计剔除。
    for (int y = 0; y < height; y += 4)
    {
        for (int x = 0; x < width; x += 4)
        {
            const int sampleNumber = (y / 4) * (width / 4) + x / 4;
            if (sampleNumber < 125) image[y * width + x] = 0;
            else if (sampleNumber < 250) image[y * width + x] = 255;
        }
    }
    result = controller.adjust(image.data(), height, width, 1,
                               15.0, 1.0, 5.0, 30.0, 1.0, 3.0);
    failures += require(near(result.brightness, 100.0, 0.01) && !result.changed,
                        "trimmed mean must ignore outliers and hold inside the allowed range");

    AutoExposureController defaultRange;
    for (unsigned char accepted : {static_cast<unsigned char>(78), static_cast<unsigned char>(91),
                                   static_cast<unsigned char>(123), static_cast<unsigned char>(169)})
    {
        std::fill(image.begin(), image.end(), accepted);
        result = defaultRange.adjust(image.data(), height, width, 1,
                                     15.0, 1.0, 5.0, 30.0, 1.0, 3.0);
        failures += require(!result.changed, "all four reference brightness values must be accepted");
    }

    defaultRange.reset();
    std::fill(image.begin(), image.end(), 74);
    result = defaultRange.adjust(image.data(), height, width, 1,
                                 15.0, 1.0, 5.0, 30.0, 1.0, 3.0);
    failures += require(result.changed && result.exposure > 15.0,
                        "brightness below the default range must increase exposure");

    defaultRange.reset();
    std::fill(image.begin(), image.end(), 176);
    result = defaultRange.adjust(image.data(), height, width, 1,
                                 15.0, 1.0, 5.0, 30.0, 1.0, 3.0);
    failures += require(result.changed && result.exposure < 15.0,
                        "brightness above the default range must reduce exposure at minimum gain");

    if (failures == 0)
        std::cout << "AutoExposureControllerSelfTest: PASS\n";
    return failures == 0 ? 0 : 1;
}
