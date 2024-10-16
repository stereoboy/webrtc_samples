/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.stereoboy.peer_audio_client;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
/*
 * Simple Java UI to trigger jni function. It is exactly same as Java code
 * in hello-jni.
 */
public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i("PeerDataChannel", "onCreate()");
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText( stringFromJNI() );
        initNative();
        setContentView(tv);
    }
    @Override
    protected void onDestroy() {
        deinitNative();
        super.onDestroy();
        Log.i("PeerDataChannel", "onDestroy()");
    }

    public native String  stringFromJNI();
    public native void  initNative();
    public native void  deinitNative();

    static {
        System.loadLibrary("hello-libs");
    }

}
