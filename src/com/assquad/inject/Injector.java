package com.assquad.inject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;

/**
 * Welcome to use Injector tool coded by assquad, hope you can enjoy the
 * injection journey. First of all, you must make sure your phone is rooted
 * 
 * @author assquad
 * 
 */
public class Injector {
    final static boolean DEBUG = true;
    final static String TAG = "assquad";
    private static final String ROOT = "root.jar";
    private Context mContext = null;

    /**
     * Constructor of Injector, you may pass any jar path which can be accessed
     * by target process. Here is a tip that system_server can not access
     * sdcard, while the 3rd app could
     * 
     * @param context
     *            Prefer the application context
     * @param hookjarPath
     *            A jar file path that the target process can access
     */
    public Injector(Context context) {
        mContext = context.getApplicationContext();
    }

    /**
     * This is the beginning of injection, we only support system_server process
     * and application process here. You should pass the main class name in your
     * hook jar and the method which is invoked first.
     * 
     * @param targetProcess
     *            The process name of the target, usually the package name
     */
    public void startInjection(String targetProcess) {
        transferFiles(ROOT);
        startRoot(targetProcess);
    }

    private void transferFiles(String filename) {
        AssetManager assetManager = mContext.getAssets();
        try {
            String path = mContext.getCacheDir() + "/";
            File file = new File(path + filename);
            if (file.exists()) {
                // return;
            }
            FileOutputStream fos = new FileOutputStream(path + filename);
            InputStream inputStream = assetManager.open(filename);
            byte[] buffer = new byte[8192];
            int count = 0;
            while ((count = inputStream.read(buffer)) != -1) {
                fos.write(buffer, 0, count);
            }
            fos.flush();
            fos.close();
            inputStream.close();
        } catch (IOException e) {
            Log.e(TAG, "transfer files failed", e);
        } catch (Exception e) {
            Log.e(TAG, "transfer files failed", e);
        }
    }

    private void startRoot(String targetProcess) {
        String libpath = null;
        String filepath = mContext.getCacheDir().getPath() + "/";
        try {
            libpath = mContext.getApplicationInfo().nativeLibraryDir + "/";
        } catch (Exception e) {
            Log.e(TAG, "android version should be up to 2.3, exit");
            return;
        }

        String jarpath = mContext.getPackageCodePath();

        String export = "export CLASSPATH=" + filepath + ROOT + "\n";
        String cmd = "app_process /system/bin com.assquad.inject.Root "
                + libpath + " " + jarpath + " " + targetProcess + " \n";
        String chmod = "chmod 777 " + filepath + ROOT + "\n";
        LOGD("export: " + export);
        LOGD("cmd: " + cmd);
        try {
            Process rootprocess = Runtime.getRuntime().exec("su");
            OutputStream out = rootprocess.getOutputStream();
            out.write(chmod.getBytes());
            out.flush();
            out.write(export.getBytes());
            out.flush();
            out.write(cmd.getBytes());
            out.flush();
            out.write("exit\n".getBytes());
            out.flush();
        } catch (IOException e) {
            Log.e(TAG, "root failed", e);
        }
    }

    static void LOGD(String msg) {
        if (DEBUG) {
            Log.d(TAG, msg);
        }
    }
}
