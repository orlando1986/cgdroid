package com.android.inject;

import android.app.Activity;
import android.os.Bundle;

import com.assquad.inject.Injector;

public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Injector inj = new Injector(this);
        inj.startInjection("com.marvell.mars");
    }

}
