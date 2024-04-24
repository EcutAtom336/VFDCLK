package com.vfdclocksetting.util;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;
import androidx.core.app.ActivityCompat;

public class PermissionsUtil implements ActivityCompat.OnRequestPermissionsResultCallback {
    public void requestPermission(String[] requestPermissions, Context context, Activity activity) {
        for (int i = 0; i < requestPermissions.length; i++) {
            if (ActivityCompat.checkSelfPermission(context, requestPermissions[i]) == PackageManager.PERMISSION_DENIED) {
                ActivityCompat.requestPermissions(activity, new String[]{requestPermissions[i]}, 1);
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == 1) {

        }
    }
}
