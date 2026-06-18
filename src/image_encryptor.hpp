#ifndef IMAGE_ENCRYPTOR_HPP
#define IMAGE_ENCRYPTOR_HPP

#include <string>

// 图像加密器类
class ImageEncryptor {
private:
    // 常量定义
    static const unsigned MOD = 2147483647;  // 2^31 - 1，用于模运算的质数
    static const unsigned PIXEL_RANGE = 256;  // uint8的取值范围，对应一个字节
    static const unsigned LCG_MULTIPLIER = 16807;  // Park-Miller线性同余生成器的乘数

    // 从密钥生成种子值
    // 使用简单的哈希函数将字符串转换为数值
    static unsigned derive_seed(const std::string &secret_key) {
        unsigned seed = 0;
        for (char c: secret_key) {
            seed = (static_cast<unsigned long long>(seed) * 31 + static_cast<unsigned char>(c)) % MOD;
        }
        return seed == 0 ? 1 : seed;  // 确保种子不为0
    }

    // 生成加密查找表
    // 使用Fisher-Yates洗牌算法生成随机置换
    void generate_encrypt_table(const std::string &secret_key) {
        unsigned state = derive_seed(secret_key);

        // 初始化查找表为顺序值
        for (int i = 0; i < PIXEL_RANGE; ++i) {
            encrypt_lut[i] = static_cast<char>(i);
        }

        // 使用Fisher-Yates洗牌算法随机打乱
        for (int i = PIXEL_RANGE - 1; i > 0; --i) {
            state = static_cast<unsigned>((static_cast<unsigned long long>(state) * LCG_MULTIPLIER) % MOD);
            unsigned j = state % (i + 1);
            // 交换元素
            char temp = encrypt_lut[i];
            encrypt_lut[i] = encrypt_lut[j];
            encrypt_lut[j] = temp;
        }
    }

    // 生成解密查找表
    // 通过加密表的反向映射构建解密表
    void generate_decrypt_table() {
        for (int i = 0; i < PIXEL_RANGE; ++i) {
            decrypt_lut[static_cast<unsigned char>(encrypt_lut[i])] = static_cast<char>(i);
        }
    }

public:
    char encrypt_lut[256]{};  // 加密查找表，固定大小为256
    char decrypt_lut[256]{};  // 解密查找表，固定大小为256

    // 构造函数
    // 根据提供的密钥初始化加密和解密表
    explicit ImageEncryptor(const std::string &secret_key) {
        generate_encrypt_table(secret_key);
        generate_decrypt_table();
    }

    // 原地加密数据
    // 使用加密查找表对数据进行替换加密
    void encrypt_data(char *data, size_t length) const {
        for (size_t i = 0; i < length; ++i) {
            data[i] = encrypt_lut[static_cast<unsigned char>(data[i])];
        }
    }

    // 原地解密数据
    // 使用解密查找表对数据进行替换解密
    void decrypt_data(char *data, size_t length) const {
        for (size_t i = 0; i < length; ++i) {
            data[i] = decrypt_lut[static_cast<unsigned char>(data[i])];
        }
    }
};

#endif // IMAGE_ENCRYPTOR_HPP