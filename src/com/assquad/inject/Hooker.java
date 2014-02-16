package com.assquad.inject;

import java.lang.reflect.Method;

import android.util.Log;
import dalvik.system.PathClassLoader;

public class Hooker extends Thread {
    public final static String TAG = "assquad";
    private String mJar = null;

    public static void main(String[] args) {
        Log.e(TAG, "hook starts: " + args[0]);
        new Hooker(args[0]).start();
    }

    public Hooker(String jarpath) {
        mJar = jarpath;
    }

    @Override
    public void run() {
        try {
            PathClassLoader path = new PathClassLoader(mJar, ClassLoader.getSystemClassLoader());
            Class c = path.loadClass("com.assquad.inject.HookerRunner");
            Object h = c.newInstance();

            Method m = c.getDeclaredMethod("onHooked", (Class[]) null);
            m.invoke(h);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage(), e);
        }
    }
}
