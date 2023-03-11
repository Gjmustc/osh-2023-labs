## Task1

![沧浪亭一角](src/姑苏园林.jpg "Green")

## Task2

```C
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265359f

float sx, sy;

typedef float Mat[4][4];
typedef float Vec[4];

void scale(Mat* m, float s)
{
    Mat temp = { {s,0,0,0}, {0,s,0,0 }, { 0,0,s,0 }, { 0,0,0,1 } };
    memcpy(m, &temp, sizeof(Mat));
}

void rotateY(Mat* m, float t)
{
    float c = cosf(t), s = sinf(t);
    Mat temp = { {c,0,s,0}, {0,1,0,0}, {-s,0,c,0}, {0,0,0,1} };
    memcpy(m, &temp, sizeof(Mat));
}

void rotateZ(Mat* m, float t)
{
    float c = cosf(t), s = sinf(t);
    Mat temp = { {c,-s,0,0}, {s,c,0,0}, {0,0,1,0}, {0,0,0,1} };
    memcpy(m, &temp, sizeof(Mat));
}

void translate(Mat* m, float x, float y, float z)
{
    Mat temp = { {1,0,0,x}, {0,1,0,y}, {0,0,1,z}, {0,0,0,1} };
    memcpy(m, &temp, sizeof(Mat));
}

void mul(Mat* m, Mat a, Mat b)
{
    Mat temp;
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++) {
            temp[j][i] = 0.0f;
            for (int k = 0; k < 4; k++)
                temp[j][i] += a[j][k] * b[k][i];
        }
    memcpy(m, &temp, sizeof(Mat));
}

void transformPosition(Vec* r, Mat m, Vec v)
{
    Vec temp = { 0, 0, 0, 0 };
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++)
            temp[j] += m[j][i] * v[i];
    memcpy(r, &temp, sizeof(Vec));
}

float transformLength(Mat m, float r)
{
    return sqrtf(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]) * r;
}

float sphere(Vec c, float r)
{
    float dx = c[0] - sx, dy = c[1] - sy;
    float a = dx * dx + dy * dy;
    return a < r * r ? sqrtf(r * r - a) + c[2] : -1.0f;
}

float opUnion(float z1, float z2)
{
    return z1 > z2 ? z1 : z2;
}

float f(Mat m, int n)
{
    // Culling
    {
        Vec v = { 0.0f, 0.5f, 0.0f, 1.0f };
        transformPosition(&v, m, v);
        if (sphere(v, transformLength(m, 0.55f)) == -1.0f)
            return -1.0f;
    }

    float z = -1.0f;

    if (n == 0) { // Leaf
        Vec v = { 0.0f, 0.5f, 0.0f, 1.0f };
        transformPosition(&v, m, v);
        z = sphere(v, transformLength(m, 0.3f));
    }
    else { // Branch
        for (float r = 0.0f; r < 0.8f; r += 0.02f) {
            Vec v = { 0.0f, r, 0.0f, 1.0f };
            transformPosition(&v, m, v);
            z = opUnion(z, sphere(v, transformLength(m, 0.05f * (0.95f - r))));
        }
    }

    if (n > 0) {
        Mat ry, rz, s, t, m2, m3;
        rotateZ(&rz, 1.8f);

        for (int p = 0; p < 6; p++) {
            rotateY(&ry, p * (2 * PI / 6));
            mul(&m2, ry, rz);
            float ss = 0.45f;
            for (float r = 0.2f; r < 0.8f; r += 0.1f) {
                scale(&s, ss);
                translate(&t, 0.0f, r, 0.0f);
                mul(&m3, s, m2);
                mul(&m3, t, m3);
                mul(&m3, m, m3);
                z = opUnion(z, f(m3, n - 1));
                ss *= 0.8f;
            }
        }
    }

    return z;
}

float f0(float x, float y, int n)
{
    sx = x;
    sy = y;
    Mat m;
    scale(&m, 1.0f);
    return f(m, n);
}

int main(int argc, char* argv[])
{
    int n = argc > 1 ? atoi(argv[1]) : 3;
    float zoom = argc > 2 ? atof(argv[2]) : 1.0f;

    puts("\e[1;32m");	// 亮绿色输出
    for (float y = 0.8f; y > -0.0f; y -= 0.02f / zoom, putchar('\n'))
        for (float x = -0.35f; x < 0.35f; x += 0.01f / zoom) {
            float z = f0(x, y, n);
            if (z > -1.0f) {
                float nz = 0.001f;
                float nx = f0(x + nz, y, n) - z;
                float ny = f0(x, y + nz, n) - z;
                float nd = sqrtf(nx * nx + ny * ny + nz * nz);
                float d = (nx - ny + nz) / sqrtf(3) / nd;
                d = d > 0.0f ? d : 0.0f;
                // d = d < 1.0f ? d : 1.0f;
                putchar(".-:=+*#%@@"[(int)(d * 9.0f)]);
            }
            else
                putchar(' ');
        }
    puts("\e[0m");
    return 0;
}
```

```C++
/*
 * @Author       : Chivier Humber
 * @Date         : 2021-08-30 14:29:14
 * @LastEditors  : liuly
 * @LastEditTime : 2022-11-15 21:32:19
 * @Description  : A small assembler for LC-3
 */

#include "assembler.h"

bool gIsErrorLogMode = false;   //设置纠错调试模式
bool gIsHexMode = false;        //设置16进制输出模式
// A simple arguments parser
std::pair<bool, std::string> getCmdOption(char **begin, char **end,
                                          const std::string &option) {  //获取命令行指令输入
    char **itr = std::find(begin, end, option);
    if (itr != end && ++itr != end) {
        return std::make_pair(true, *itr);
    }
    return std::make_pair(false, "");
}

bool cmdOptionExists(char **begin, char **end, const std::string &option) {  //是否输入附加命令选项-e/-s/-h
    return std::find(begin, end, option) != end;
}

int main(int argc, char **argv) {
    // Print out Basic information about the assembler
    if (cmdOptionExists(argv, argv + argc, "-h")) {
        std::cout << "This is a simple assembler for LC-3." << std::endl
                  << std::endl;
        std::cout << "\e[1mUsage\e[0m" << std::endl;
        std::cout << "./assembler \e[1m[OPTION]\e[0m ... \e[1m[FILE]\e[0m ..."
                  << std::endl
                  << std::endl;
        std::cout << "\e[1mOptions\e[0m" << std::endl;
        std::cout << "-h : print out help information" << std::endl;  //显示帮助信息
        std::cout << "-f : the path for the input file" << std::endl; //待编译文件输入路径
        std::cout << "-e : print out error information" << std::endl; //以纠错调试模式运行
        std::cout << "-o : the path for the output file" << std::endl; //编译完成文件输出路径
        std::cout << "-s : hex mode" << std::endl; //以十六进制模式转换输出
        return 0;
    }

    auto input_info = getCmdOption(argv, argv + argc, "-f");
    std::string input_filename;
    auto output_info = getCmdOption(argv, argv + argc, "-o");
    std::string output_filename;

    // Check the input file name
    if (input_info.first) {
        input_filename = input_info.second;
    } else {
        input_filename = "input.txt";
    }

    if (output_info.first) {
        output_filename = output_info.second;
    } else {
        output_filename = "";
    }

    // Check output file name
    if (output_filename.empty()) {
        output_filename = input_filename;   //若未输入output文件路径的处理方式,在input文件名后加上my后缀
        if (output_filename.find('.') == std::string::npos) {
            output_filename = output_filename + "my.bin";
        } else {
            output_filename =
                output_filename.substr(0, output_filename.rfind('.'));
            output_filename = output_filename + "my.bin";
        }
    }

    if (cmdOptionExists(argv, argv + argc, "-e")) {
        // * Error Log Mode :
        // * With error log mode, we can show error type
        SetErrorLogMode(true);
    }
    if (cmdOptionExists(argv, argv + argc, "-s")) {
        // * Hex Mode:
        // * With hex mode, the result file is shown in hex
        SetHexMode(true);
    }

    auto ass = assembler();
    auto status = ass.assemble(input_filename, output_filename); //汇编器主功能函数

    if (gIsErrorLogMode) {
        std::cout << std::dec << status << std::endl;
    }
    return 0;
}

```

```Python
import cv2 as cv

width = 200
height = 150
img = cv.imread(r'D:\Microsoft Visual Studio\.py\background3.jpg')
#例如cv.imread("test/1.jpg")

img = cv.resize(img,(width,height))
# 默认使用双线性插值法

cv.imshow("img",img)
cv.imwrite('BG3.jpg', img)
cv.waitKey(0)
cv.destroyAllWindows()
```

## Task3

Fomula **within** the line: $E=mc^2$

Fomula **_between_** the lines:

\[ e^{iπ} + 1 = 0 \]
