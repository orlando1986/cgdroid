package com.assquad.inject;

import android.util.Log;

public class Root {
    final static boolean DEBUG = true;
    final static String TAG = "assquad";
    static String sLibPath = null;

    public static void main(String[] args) {
        StringBuffer sb = new StringBuffer();
        sb.append("native lib path: " + args[0] + "\n");
        sb.append("hook jar path: " + args[1] + "\n");
        sb.append("target process: " + args[2] + "\n");
        LOGD(sb.toString());

        sLibPath = args[0];
        new RootImpl().execute(args);
    }

    static void LOGD(String msg) {
        if (DEBUG) {
            Log.d(TAG, msg);
        }
    }
}
