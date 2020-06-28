package kim.hsl.pc;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.widget.TextView;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());

        // 初始化权限
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            initPermissions();
        }

        // 将图片压缩成 JPEG 格式, 不缩放
        compressBitmap(R.drawable.blog, Bitmap.CompressFormat.JPEG, 0,
                Environment.getExternalStorageDirectory() + "/blog_jpeg.jpeg",
                0, 0);

        // 将图片压缩成 WEBP  格式
        compressBitmap(R.drawable.blog, Bitmap.CompressFormat.WEBP, 0,
                Environment.getExternalStorageDirectory() + "/blog_webp.webp",
                0, 0);

        // 将图片压缩成 PNG  格式
        compressBitmap(R.drawable.blog, Bitmap.CompressFormat.PNG, 0,
                Environment.getExternalStorageDirectory() + "/blog_png.png",
                0, 0);

        // 将图片宽高各压缩一半
        compressBitmap(R.drawable.blog, Bitmap.CompressFormat.PNG, 0,
                Environment.getExternalStorageDirectory() + "/blog_png_half.png",
                995, 510);
    }

    /**
     * 压缩图片, 并将压缩结果保存到指定文件
     * @param resId 图片资源
     * @param compressFormat 图片压缩格式
     * @param quality 压缩质量
     * @param path 文件保存路径
     */
    public void compressBitmap(int resId, Bitmap.CompressFormat compressFormat,
                               int quality, String path, int width, int height){
        // 从资源文件中加载一张图片
        Bitmap bitmap = BitmapFactory.decodeResource(getResources(), resId);

        // 如果传入的尺寸参数大于 0, 那么压缩尺寸
        if(width > 0 && height > 0){
            bitmap = Bitmap.createScaledBitmap(bitmap, width, height, true);
        }

        // 用于写出压缩后的图片到文件中
        FileOutputStream fos = null;
        try {
            // 打开文件输出流
            fos = new FileOutputStream(path);

            // 图片压缩操作
            // 如果图片格式是 PNG 格式, 会忽略 质量 参数
            bitmap.compress(compressFormat, quality, fos);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            Log.i("TAG", "文件输出流打开失败");
        }finally {
            if(fos != null){
                try {
                    fos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                    Log.i("TAG", "文件输出流关闭失败");
                }
            }
        }
    }

    public native String stringFromJNI();

    public native void native_pictureCompress(Bitmap bitmap, int quality, String path);


    /**
     * 需要获取的权限列表
     */
    private String[] permissions = new String[]{
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    /**
     * 动态申请权限的请求码
     */
    private static final int PERMISSION_REQUEST_CODE = 888;

    /**
     * 动态申请权限
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    private void initPermissions() {
        if (isLacksPermission()) {
            //动态申请权限 , 第二参数是请求吗
            requestPermissions(permissions, PERMISSION_REQUEST_CODE);
        }
    }

    /**
     * 判断是否有 permissions 中的权限
     * @return
     */
    @RequiresApi(api = Build.VERSION_CODES.M)
    public boolean isLacksPermission() {
        for (String permission : permissions) {
            if(checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED){
                return true;
            }
        }
        return false;
    }
}
