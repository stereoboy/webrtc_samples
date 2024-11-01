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

import android.Manifest;
import android.content.pm.PackageManager;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.content.Context;
/*
 * Simple Java UI to trigger jni function. It is exactly same as Java code
 * in hello-jni.
 */
public class MainActivity extends AppCompatActivity implements ActivityCompat.OnRequestPermissionsResultCallback {

    private static final int REQUEST_RECORD = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i("PeerAudio", "onCreate()");
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText( stringFromJNI() );

        //
        // old version permission request
        // references
        //  - https://developer.android.com/reference/androidx/core/app/ActivityCompat#requestPermissions(android.app.Activity,java.lang.String[],int)
        //  - https://developer.android.com/reference/androidx/core/app/ActivityCompat.OnRequestPermissionsResultCallback
        //  - https://www.digitalocean.com/community/tutorials/android-runtime-permissions-example
        //  - https://stackoverflow.com/questions/51304399/unable-to-request-record-audio-permission-in-android
        //
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.RECORD_AUDIO}, REQUEST_RECORD);
        }

        initNative(getApplicationContext());
        setContentView(tv);
    }
    @Override
    protected void onDestroy() {
        deinitNative();
        super.onDestroy();
        Log.i("PeerAudio", "onDestroy()");
    }



    public native String  stringFromJNI();

    //
    // old version permission request
    // references
    //  - https://developer.android.com/reference/androidx/core/app/ActivityCompat#requestPermissions(android.app.Activity,java.lang.String[],int)
    //  - https://developer.android.com/reference/androidx/core/app/ActivityCompat.OnRequestPermissionsResultCallback
    //  - https://www.digitalocean.com/community/tutorials/android-runtime-permissions-example
    //
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == REQUEST_RECORD) {
            if (grantResults.length != 1 || grantResults[0] != PackageManager.PERMISSION_GRANTED) {
                Log.e("PeerAudio", "Failed to get Permission for REQUEST_RECORD");
            } else {
                Log.i("PeerAudio", "Requist Permissions granted");
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    public native void  initNative(Context applicationContext);
    public native void  deinitNative();

    static {
        System.loadLibrary("hello-libs");
    }

}
