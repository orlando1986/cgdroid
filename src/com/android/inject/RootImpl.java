package com.android.inject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.content.Context;
import android.content.res.AssetManager;
import android.util.Log;
import dalvik.system.DexClassLoader;

public class RootImpl {
    final static String TAG = "inject";
    private static Context sContext = null;
    private static final String INJ = "inj";
    private static final String HOOKLIB = "libhook.so";
    private static final String HOOKDEX = "hook.jar";

    public static void startInject(Context context) {
        sContext = context;
        // transfer inject components to assets
        transferFiles(HOOKDEX);
        transferFiles(HOOKLIB);
        transferFiles(INJ);

        dexOpt();
        startRoot();
    }

    private static void transferFiles(String filename) {
        AssetManager assetManager = sContext.getAssets();
        try {
            String path = sContext.getFilesDir() + "/";
            File file = new File(path + filename);
            if (file.exists()) {
                //return;
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

    private static void dexOpt() {
        String filepath = sContext.getFilesDir().getPath() + "/";
        new DexClassLoader(filepath + HOOKDEX, filepath, null, ClassLoader.getSystemClassLoader());
    }

    private static void startRoot() {
        String filepath = sContext.getFilesDir().getPath() + "/";
        final String cmd = filepath + INJ + " " + filepath + "\n";
        final String mkdir = "mkdir /data/system/inject/ \n";
        final String chmod = "chmod -R 777 /data/system/inject/ \n";
        final String chmod_inj = "chmod 777 " + filepath + INJ + " \n";
        Log.d(TAG, "cmd=" + cmd);
        try {
            Process rootprocess = Runtime.getRuntime().exec("su");
            OutputStream out = rootprocess.getOutputStream();
            out.write(mkdir.getBytes());
            out.flush();
            out.write(chmod.getBytes());
            out.flush();
            out.write(chmod_inj.getBytes());
            out.flush();
            out.write(cmd.getBytes());
            out.flush();
            out.write("exit\n".getBytes());
            out.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
