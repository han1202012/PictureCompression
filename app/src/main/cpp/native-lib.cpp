#include <jni.h>
#include <string>
#include <jpeglib.h>
#include <android/bitmap.h>
#include <malloc.h>
#include <android/log.h>
#include <bitset>
#include <iosfwd>

// 声明函数
void compressJpegFile(uint8_t *data, int imageWidth, int imageHeight,
                      jint compressQuality, const char *filename);

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
Java_kim_hsl_pc_MainActivity_native_1pictureCompress(JNIEnv *env, jobject thiz,
                                                     jobject jbitmap,
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
 * @param data      要压缩的图片数据, 像素格式是 BGR
 * @param imageWidth     输出的 JPEG 图片宽度
 * @param imageHeight    输出的 JPEG 图片高度
 * @param compressQuality   输出的 JPEG 图片质量
 * @param filename  输出文件路径
 */
void compressJpegFile(uint8_t *data, int imageWidth, int imageHeight,
                      jint compressQuality, const char *filename) {

    // 1. 为 JPEG 图片压缩对象, 分配内存空间

    /* 该对象中存储了 JPEG 压缩参数, 还包含了指向工作空间的指针, JPEG 库会在需要时分配该指针;
     * 该结构体可能会存在多个, 每个结构体对象都表示了一个压缩或解压缩的工作;
     * JPEG 对象 : jpeg_compress_struct 结构体和与其关联的工作数据
     */
    struct jpeg_compress_struct cinfo;

    /* 错误处理程序 : jpeg_error_mgr 结构体表示错误处理程序,
     * 将其单独定义成一个结构体, 是因为应用经常需要提供一个专门的错误处理程序;
     * 处理处理机制 : 在这里我们采用最简单的方法, 使用标准的错误处理程序,
     * 如果压缩失败, 在 stderr 上打印失败信息, 并调用 exit() 退出程序 ;
     * 结构体声明周期 : 该结构体的生命周期必须与 jpeg_compress_struct 结构体的生命周期保持一致,
     * 以免产生野指针问题 ;
     */
    struct jpeg_error_mgr jerr;

    /* 为了防止 JPEG 压缩对象初始化时出错, 这里首先设置错误处理
     * 在内存不足时, 创建 jpeg_compress_struct 可能会失败
     */
    cinfo.err = jpeg_std_error(&jerr);

    // 初始化 JPEG 压缩对象
    jpeg_create_compress(&cinfo);


    // 2. 打开文件, 准备向文件写出二进制数据

    // w 代表写出数据, b 代表二进制数据
    FILE *outfile;
    if ((outfile = fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        exit(1);
    }
    // 设置文件输出
    jpeg_stdio_dest(&cinfo, outfile);


    // 3. 设置压缩参数

    // 下面的四个参数是必须设置的参数
    // 设置图片的宽度
    cinfo.image_width = imageWidth;
    // 设置图片的高度
    cinfo.image_height = imageHeight;
    // 设置每个像素的颜色组件, BGR 3个
    cinfo.input_components = 3;
    // 输入图像数据的颜色空间
    cinfo.in_color_space = JCS_RGB;

    // 设置默认的压缩参数, 该操作是函数库的常规步骤
    // 设置该参数前需要设置 cinfo.in_color_space 输入数据的颜色空间
    jpeg_set_defaults(&cinfo);

    // 打开哈夫曼编码
    cinfo.optimize_coding = TRUE;

    // 设置非默认参数, 该方法设置质量
    jpeg_set_quality(&cinfo, compressQuality, 1);


    // 4. 开始压缩 JPEG 格式图片, 设置 TRUE 参数, 表示将完整的图片进行压缩
    // 一般情况下都是设置 TRUE, 如果进行定制压缩, 可以设置 FALSE
    jpeg_start_compress(&cinfo, TRUE);


    // 5. 循环写入数据

    /* 循环原理 : 使用函数库的状态变量, cinfo.next_scanline 作为循环控制变量
     * 这样就可以不同自己实现循环控制
     * 为了保持简单, 每次传递一行图像数据
     */


    // 每一个行的数据个数
    int row_stride = imageWidth * 3;

    // 指向图像数据中的某一行数据
    JSAMPROW row[1];
    while (cinfo.next_scanline < cinfo.image_height) {
        /* 获取一行图像数据
         * data 是图像的起始位置
         * row_stride 是每一行的字节数
         * cinfo.next_scanline 是当前的行数
         * 计算出来的 pixels 指针, 指向要写出行的首地址
         */
        uint8_t *pixels = data + cinfo.next_scanline * row_stride;
        row[0] = pixels;
        // 调用 jpeg_write_scanlines 方法后, cinfo.next_scanline 自动加 1
        jpeg_write_scanlines(&cinfo, row, 1);
    }

    // 6. 完成图片压缩
    jpeg_finish_compress(&cinfo);

    // 7. 释放相关资源
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}