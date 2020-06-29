#include <jni.h>
#include <string>
#include <jpeglib.h>
#include <android/bitmap.h>
#include <malloc.h>
#include <android/log.h>
#include <bitset>
#include <iosfwd>

// 声明函数
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
    }// 截止到此处, 已经读取出 JPEG 图片所需的数据, 在 data 指针中

    // 将 data 指针中的数据压缩到 JPEG 格式图片中
    compressJpegFile(temp, width, height, quality, filePath);

    // 解锁
    AndroidBitmap_unlockPixels(env,jbitmap);
    // 注意要释放 temp 指针 , 不要释放成 data 指针, 否则会出错
    free(temp);
    // 释放局部引用, 不释放, GC 也会回收, 但是有延迟
    env->ReleaseStringUTFChars(path, filePath);
}



/**
 * 压缩 Jpeg 图片
 *
 * 完整的带详细注释的代码示例参考源码 libjpeg-turbo-2.0.5/example.txt 示例文件
 * 里面有详细的定义图片压缩的过程
 *
 * @param data
 * @param width
 * @param height
 * @param quality
 * @param filePath
 */
void compressJpegFile(uint8_t *data, int width, int height, jint quality, const char *filePath) {


    // 1. 为 JPEG 图片压缩对象, 分配内存空间
    struct jpeg_compress_struct cinfo;

    /* 为了防止 JPEG 压缩对象初始化时出错, 这里首先设置错误处理
     * 在内存不足时, 创建 jpeg_compress_struct 可能会失败
     *
     */
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    //创建压缩对象
    jpeg_create_compress(&cinfo);

    // 2. 打开文件, 准备向文件写出二进制数据
    // w 代表写出数据, b 代表二进制数据
    FILE *f = fopen(filePath, "wb");
    // 设置文件输出
    jpeg_stdio_dest(&cinfo, f);

    // 3. 设置压缩参数, 宽, 高
    cinfo.image_width = width;
    cinfo.image_height = height;


    // 设置输入组件, BGR 3个
    cinfo.input_components = 3;
    // 颜色空间
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);

    // 打开哈夫曼编码
    cinfo.optimize_coding = TRUE;
    // 设置压缩指令
    jpeg_set_quality(&cinfo, quality, 1);

    // 4. 开始压缩 JPEG 格式图片
    jpeg_start_compress(&cinfo, 1);

    // 5. 循环写入数据
    // 每一个行的数据个数
    int row_stride = width * 3;

    //next_scanline 一行数据开头的位置
    JSAMPROW row[1];
    while (cinfo.next_scanline < cinfo.image_height) {
        //拿一行数据
        uint8_t *pixels = data + cinfo.next_scanline * row_stride;
        row[0] = pixels;
        jpeg_write_scanlines(&cinfo, row, 1);
    }

    // 6. 完成图片压缩
    jpeg_finish_compress(&cinfo);

    // 7. 释放相关资源
    fclose(f);
    jpeg_destroy_compress(&cinfo);
}