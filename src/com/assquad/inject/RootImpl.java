package com.assquad.inject;

public class RootImpl {
    private final static String TAG = Root.TAG;
    static final String INJLIB = "libinj.so";
    static {
        if (Root.sLibPath != null && Root.sLibPath.length() != 0) {
            System.load(Root.sLibPath + INJLIB);
        }
    }

    void execute(String[] args) {
        inject(args[0], args[2], args[1]);
    }

    private native void inject(String nativelib, String targetProcess, String hookjar);
}
