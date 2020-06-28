#include <jni.h>
#include <string>
#include <jpeglib.h>
#include <android/bitmap.h>
#include <malloc.h>
#include <android/log.h>
#include <bitset>
#include <iosfwd>

extern "C" JNIEXPORT jstring JNICALL
Java_kim_hsl_pc_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
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
    int color;
    //rgb
    uint8_t* data = (uint8_t *) malloc(width * height * 3);
    uint8_t* temp = data;

    // JPEG 像素中的 RGB 三原色, 红绿蓝
    uint8_t red, green, blue;
    for(int i = 0; i < height; i++){
        for (int j = 0; j < width; ++j) {
            //argb = 4字节  int
            // * =>解引用=》解释引用
            color = *(int*)addrPtr; //0-3字节
            //argb
            red = (color >> 16) & 0xFF;
            green = (color >> 8) & 0xFF;
            blue = color & 0xFF;
            /**
             * 以前主流bgr
             * libjpeg  bgr
             */
            *data = blue;
            *(data+1) = green;
            *(data+2) = red;
            data += 3;
            //指针 跳过4个字节
            addrPtr +=4; //4-7字节
        }
    }

    //write_JPEG_file(temp,w,h,q,path);

    jpeg_compress_struct jcs;


    AndroidBitmap_unlockPixels(env,jbitmap);
    free(data);


    // 释放局部引用
    env->ReleaseStringUTFChars(path, filePath);
}