#include <stdio.h>
#include <stdlib.h>
#include "imglib.h" // 包含图像库的头文件

// 创建一个简单的测试图像
struct image* create_test_image(int width, int height) {
    struct image* img = malloc(sizeof(struct image));
    img->width = width;
    img->height = height;
    img->data = malloc(width * height * 3); // 假设每个像素 3 字节（RGB）

    // 填充测试图像（例如，创建一个红色的图像）
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            img->data[(y * width + x) * 3 + 0] = 255; // R
            img->data[(y * width + x) * 3 + 1] = 0;   // G
            img->data[(y * width + x) * 3 + 2] = 0;   // B
        }
    }

    return img;
}

int main() {
    const char *filename = "test_output.bmp"; // 输出文件名
    struct image *test_image = create_test_image(100, 100); // 创建 100x100 的测试图像

    // 保存测试图像
    if (saveBMP(filename, test_image)) {
        printf("Image saved successfully as %s.\n", filename);
    } else {
        printf("Failed to save image.\n");
    }

    // 清理资源
    free(test_image->data);
    free(test_image);
    
    return 0;
}

