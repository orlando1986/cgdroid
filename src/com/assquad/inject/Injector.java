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
 * injection journey. First of all, you must make sure your device is rooted;
 * Second you should implement a class whose class name is
 * "com.assquad.inject.HookerRunner" and with a method called "onHooked()". This
 * method will be called in target process, you can design everything beginning
 * from this method. Be warning: we do not support native library in target
 * process, which means you can not call your own native method in target
 * process unless you explicitly load a native library with System.load().
 * 
 * Here is a sample for HookedRunner:
 * 
 * package com.assquad.inject;
 * 
 * import android.util.Log;
 * 
 * public class HookerRunner {
 *     public void onHooked() {
 *          Log.e("assquad", "onHooked");
 *      }
 * }
 *
 * @author assquad
 * 
 */
public class Injector {
    final static boolean DEBUG = true;
    final static String TAG = "assquad";
    private final static String EXECUTABLE = "inj";
    private Context mContext = null;

    /**
     * Constructor of Injector, pass a context here.
     * 
     * @param context
     *            Prefer the application context
     */
    public Injector(Context context) {
        mContext = context.getApplicationContext();
    }

    /**
     * This is the beginning of injection, we only support system_server process
     * and application process here.
     * 
     * @param targetProcess
     *            The process name of the target, usually the package name
     */
    public void startInjection(String targetProcess) {
        transferFiles(EXECUTABLE);
        startRoot(targetProcess);
    }

    private void transferFiles(String filename) {
        AssetManager assetManager = mContext.getAssets();
        try {
            String path = mContext.getFilesDir() + "/";
            File file = new File(path + filename);
            File data = new File(mContext.getPackageCodePath());
            if (file.exists() && (file.lastModified() > data.lastModified())) {
                return;
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
        String filepath = mContext.getFilesDir().getPath() + "/";
        String jarpath = mContext.getPackageCodePath();
        String libpath = null;
        try {
            libpath = mContext.getApplicationInfo().nativeLibraryDir + "/";
        } catch (Exception e) {
            Log.e(TAG, "android version should be up to 2.3, exit");
            return;
        }

        String chmod = "chmod 777 " + filepath + EXECUTABLE + "\n";
        String cmd = filepath + EXECUTABLE + " " + targetProcess + " " + libpath + " " + jarpath + " \n";
        LOGD("cmd: " + cmd);
        try {
            Process rootprocess = Runtime.getRuntime().exec("su");
            OutputStream out = rootprocess.getOutputStream();
            out.write(chmod.getBytes());
            out.flush();
            out.write(cmd.getBytes());
            out.flush();
            out.write("exit\n".getBytes());
            out.flush();
        } catch (IOException e) {
            Log.e(TAG, "root failed", e);
        }
    }

    private static void LOGD(String msg) {
        if (DEBUG) {
            Log.d(TAG, msg);
        }
    }
}
