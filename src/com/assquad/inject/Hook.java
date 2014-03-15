package com.assquad.inject;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.util.Log;

public class Hook {
    public final static String TAG = "assquad";

    public static void main(String[] args) {
        Log.e(TAG, "hook starts");

        Bitmap b = Bitmap.createBitmap(100, 100, Config.ARGB_8888);
        Bitmap fb = (Bitmap) FakeProxy.newInstance(b);
        fb.describeContents();

//        ContextWrapper obj = new ContextWrapper(new Application());
//        ContextWrapper fake = (ContextWrapper) FakeProxy.newInstance(obj);
//        fake.deleteFile(null);
    }

    public static class FakeProxy implements java.lang.reflect.InvocationHandler {
        private Object obj;

        public static Object newInstance(Object obj) {
            return Proxy.newProxyInstance(obj.getClass().getClassLoader(), obj.getClass(), new FakeProxy(obj));
        }

        public FakeProxy(Object obj) {
            this.obj = obj;
        }

        public Object invoke(Object proxy, Method m, Object[] args) throws Throwable {
            Object result;
            try {
                Log.i(TAG, "hook done. called: " + m.getName());
                result = m.invoke(obj, args);
            } catch (InvocationTargetException e) {
                throw e.getTargetException();
            } catch (Exception e) {
                throw new RuntimeException("unexpected invocation exception: " + e.getMessage());
            } finally {
            }
            return result;
        }
    }

}