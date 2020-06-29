#include <jni.h>
#include <string>
#include <jpeglib.h>
#include <android/bitmap.h>
#include <malloc.h>
#include <android/log.h>
#include <bitset>
#include <iosfwd>

void compressJpegFile(uint8_t *data, int width, int height, jint quality, const char *filePath);

extern "C" JNIEXPORT jstring JNICALL
Java_kim_hsl_pc_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";

    // 测试 libturbojpeg.a 函数库
    jpeg_compress_struct jcs;
    __android_log_print(ANDROID_LOG_INFO, "JPEG", "jpeg_compress_struct jcs = %d", jcs.image_width);
    hello = hello + " , jpeg_compress_struct jcs = " + std::to_string(jcs.image_width);

    return env->NewStringUTF(hello.c_str());
}

/**
 * 图片压缩方法
 */
extern "C"
JNIEXPORT void JNICALL
Java_kim_hsl_pc_MainActivity_native_1pictureCompress(JNIEnv *env, jobject thiz, jobject jbitmap,
                                                     jint quality, jstring path) {

    // 将 Java 字符串转为 C 字符串, 注意这是局部引用
    const char *filePath = env->GetStringUTFChars(path, 0);

    // 声明 位图信息, 该变量作为返回值使用
    // 引用自 bitmap.h
    AndroidBitmapInfo info;
    // 从 bitmap 中获得信息位图信息 AndroidBitmapInfo
    AndroidBitmap_getInfo(env, jbitmap, &info);

    // 该类型最终类型是 unsigned char, 相当于 Java 中的 byte
    // 这是个 byte 指针, 指向一个数组
    // 此处作为返回值使用
    uint8_t *addrPtr;
    // 注意该获取的信息中包含透明度信息, 像素格式是 ARGB
    AndroidBitmap_lockPixels(env, jbitmap, (void **) &addrPtr);

    // JPEG 格式的图片, 没有透明度信息, 像素格式是 RGB
    // 这里需要去掉透明度信息
    // 获取图片的像素宽度
    int width = info.width;
    // 获取图片的像素高度
    int height = info.height;

    // 单个像素点的颜色值, ARGB 每个都是 1 byte, 总共 4 字节, 刚好可以存储在 int 中
    int color;

    //rgb
    uint8_t* data = (uint8_t *) malloc(width * height * 3);

    // data 指针在后续发生了移动, 这里保存下初始的指针, 方便在之后释放该指针
    uint8_t* temp = data;

    // JPEG 像素中的 RGB 三原色, 红绿蓝
    uint8_t red, green, blue;

    // 遍历从 Bitmap 内存 addrPtr 中读取 BGRA 数据, 然后向 data 内存存储 BGR 数据中
    for(int i = 0; i < height; i++){
        for (int j = 0; j < width; ++j) {
            // 在 Bitmap 中内存存储序列是 BGRA
            blue = *( addrPtr );
            green = *( addrPtr + 1 );
            red = *( addrPtr + 2 );

            // libturbojpeg 中 JPEG 图像内存排列格式是 BGR
            *data = blue;
            *( data + 1 ) = green;
            *( data + 2 ) = red;

            // 移动 data 指针
            data += 3;

            //移动 addrPtr 指针, 为下一次读取数据做准备
            addrPtr +=4;
        }
    }

    compressJpegFile(temp, width, height, quality, filePath);

    AndroidBitmap_unlockPixels(env,jbitmap);

    // 注意要释放 temp 指针 , 不要释放成 data 指针
    free(temp);

    // 释放局部引用
    env->ReleaseStringUTFChars(path, filePath);
}



/**
 * 压缩 Jpeg 图片
 * @param data
 * @param width
 * @param height
 * @param quality
 * @param filePath
 */
void compressJpegFile(uint8_t *data, int width, int height, jint quality, const char *filePath) {

//    3.1、创建jpeg压缩对象
    jpeg_compress_struct jcs;
    //错误回调
    jpeg_error_mgr error;
    jcs.err = jpeg_std_error(&error);

    //创建压缩对象
    jpeg_create_compress(&jcs);

//    3.2、指定存储文件
    // w = 写,b = 二进制
    FILE *f = fopen(filePath, "wb");
    jpeg_stdio_dest(&jcs,f);

//    3.3、设置压缩参数
    jcs.image_width = width;
    jcs.image_height = height;
    //bgr
    jcs.input_components = 3;
    jcs.in_color_space =  JCS_RGB;
    jpeg_set_defaults(&jcs);
    //开启哈夫曼 1=true 0=false
    jcs.optimize_coding = 1;
    jpeg_set_quality(&jcs, quality, 1);

//    3.4、开始压缩
    jpeg_start_compress(&jcs,1);

//    3.5、循环写入每一行数据
    int row_stride = width * 3;
    //next_scanline 一行数据开头的位置
    JSAMPROW row[1];
    while (jcs.next_scanline < jcs.image_height) {
        //拿一行数据
        uint8_t *pixels = data + jcs.next_scanline * row_stride;
        row[0] = pixels;
        jpeg_write_scanlines(&jcs, row, 1);
    }

//    3.6、压缩完成
    jpeg_finish_compress(&jcs);
//    3.7、释放jpeg对象

    fclose(f);
    jpeg_destroy_compress(&jcs);
}