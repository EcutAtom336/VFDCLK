package com.vfdclocksetting;

import static android.bluetooth.le.ScanSettings.MATCH_NUM_ONE_ADVERTISEMENT;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textview.MaterialTextView;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, ActivityCompat.OnRequestPermissionsResultCallback {
    final static String LOG_TAG = "MainActivity";
    final static int scanTimeoutMs = 2500;
    private String[] permissions;
    private static BluetoothAdapter btAdapter;
    private MaterialButton btnNumZero;
    private MaterialButton btnNumOne;
    private MaterialButton btnNumTwo;
    private MaterialButton btnNumThree;
    private MaterialButton btnNumFour;
    private MaterialButton btnNumFive;
    private MaterialButton btnNumSix;
    private MaterialButton btnNumSeven;
    private MaterialButton btnNumEight;
    private MaterialButton btnNumNine;
    private MaterialButton btnBackspace;
    private MaterialTextView tvDeviceCode;
    private AlertDialog alertDialogConnect;
    private BluetoothLeScanner leScanner;
    private ActivityResultLauncher launcher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.v(LOG_TAG, "onStart");

        btAdapter = BluetoothAdapter.getDefaultAdapter();

        tvDeviceCode = findViewById(R.id.tv_pair_code);
        btnNumZero = findViewById(R.id.btn_num_zero);
        btnNumOne = findViewById(R.id.btn_num_one);
        btnNumTwo = findViewById(R.id.btn_num_two);
        btnNumThree = findViewById(R.id.btn_num_three);
        btnNumFour = findViewById(R.id.btn_num_four);
        btnNumFive = findViewById(R.id.btn_num_five);
        btnNumSix = findViewById(R.id.btn_num_six);
        btnNumSeven = findViewById(R.id.btn_num_seven);
        btnNumEight = findViewById(R.id.btn_num_eight);
        btnNumNine = findViewById(R.id.btn_num_nine);
        btnBackspace = findViewById(R.id.btn_backspace);

        alertDialogConnect = new MaterialAlertDialogBuilder(this).create();
        alertDialogConnect.setCancelable(false);
        alertDialogConnect.setTitle("查找设备");

        btnNumZero.setOnClickListener(this);
        btnNumOne.setOnClickListener(this);
        btnNumTwo.setOnClickListener(this);
        btnNumThree.setOnClickListener(this);
        btnNumFour.setOnClickListener(this);
        btnNumFive.setOnClickListener(this);
        btnNumSix.setOnClickListener(this);
        btnNumSeven.setOnClickListener(this);
        btnNumEight.setOnClickListener(this);
        btnNumNine.setOnClickListener(this);
        btnBackspace.setOnClickListener(this);

        launcher = registerForActivityResult(new ActivityResultContracts.StartActivityForResult(), o -> {
            if (o.getResultCode() == RESULT_OK) {
                leScanner = btAdapter.getBluetoothLeScanner();
            } else {
                Toast.makeText(this, "蓝牙未开启", Toast.LENGTH_SHORT).show();
                finish();
            }
        });

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
            permissions = new String[]{Manifest.permission.BLUETOOTH,
                    Manifest.permission.BLUETOOTH_ADMIN,
                    Manifest.permission.BLUETOOTH_CONNECT,
                    Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.ACCESS_FINE_LOCATION};
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.v(LOG_TAG, "onStart");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.v(LOG_TAG, "onStop");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.v(LOG_TAG, "onPause");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.v(LOG_TAG, "onResume");
        for (String permission : permissions)
            if (ActivityCompat.checkSelfPermission(MainActivity.this, permission) != PackageManager.PERMISSION_GRANTED)
                ActivityCompat.requestPermissions(MainActivity.this, new String[]{permission}, 0);
        if (!btAdapter.isEnabled()) {
            Intent intent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            launcher.launch(intent);
        } else {
            leScanner = btAdapter.getBluetoothLeScanner();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.v(LOG_TAG, "onDestroy");
    }

    private class BleScanCallback extends android.bluetooth.le.ScanCallback {
        public BleScanCallback() {
            super();
        }

        @SuppressLint("MissingPermission")
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);
            Log.v(LOG_TAG, "进入onScanResult方法");
            if (callbackType == ScanCallback.SCAN_FAILED_ALREADY_STARTED && result != null) {
                if (alertDialogConnect.isShowing()) {
                    alertDialogConnect.dismiss();
                    switchSoftKeyboardState(true);
                    leScanner.stopScan(new BleScanCallback());
                    //后期在生产商自定义数据中加入设备型号识别码，根据设备型号启动不同的activity
                    Intent intent = new Intent();
                    intent.setClass(getApplicationContext(), VfdClkSettingPage.class);
                    intent.putExtra("leDevice", result.getDevice());
                    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                    startActivity(intent);
                }
            }
        }

        @Override
        public void onBatchScanResults(List<ScanResult> results) {
            super.onBatchScanResults(results);
            Log.v(LOG_TAG, "进入onBatchScanResults方法");
        }

        @Override
        public void onScanFailed(int errorCode) {
            super.onScanFailed(errorCode);
            Log.v(LOG_TAG, "进入onScanFailed方法，错误代码：" + errorCode);
            if (errorCode == SCAN_FAILED_ALREADY_STARTED) {
                Toast.makeText(MainActivity.this, "正在扫描...", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @SuppressLint({"SetTextI18n", "MissingPermission"})
    @Override
    public void onClick(View v) {
        int vId = v.getId();
        if (vId == R.id.btn_num_zero || vId == R.id.btn_num_one || vId == R.id.btn_num_two || vId == R.id.btn_num_three || vId == R.id.btn_num_four || vId == R.id.btn_num_five || vId == R.id.btn_num_six || vId == R.id.btn_num_seven || vId == R.id.btn_num_eight || vId == R.id.btn_num_nine) {
            if (tvDeviceCode.getText().length() < 6)
                tvDeviceCode.setText(tvDeviceCode.getText().toString() + ((Button) v).getText().toString());
            if (tvDeviceCode.getText().length() == 6) {
                switchSoftKeyboardState(false);

                ScanFilter.Builder scanFilterBuilder = new ScanFilter.Builder();
                String deviceCodeString = tvDeviceCode.getText().toString();
                ArrayList<ScanFilter> scanFilters = new ArrayList<>();
                byte[] deviceId = new byte[]{0, 0, 0, 0, 0, 0};
                for (int i = 0; i < 6; i++)
                    deviceId[i] = Byte.parseByte(deviceCodeString.substring(i, i + 1));
                Log.v(LOG_TAG, "device id: " + Integer.toString(deviceId[0]) + Integer.toString(deviceId[1]) + Integer.toString(deviceId[2]) + Integer.toString(deviceId[3]) + Integer.toString(deviceId[4]) + Integer.toString(deviceId[5]));
//                scanFilterBuilder.setManufacturerData(0xFFFF, deviceId);
                scanFilterBuilder.setManufacturerData(0xFFFF, deviceId,
                        new byte[]{0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
                ScanFilter scanFilter = scanFilterBuilder.build();
                scanFilters.add(scanFilter);
                ScanSettings.Builder scanSettingBuilder = new ScanSettings.Builder();
                scanSettingBuilder.setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).setNumOfMatches(MATCH_NUM_ONE_ADVERTISEMENT);

                alertDialogConnect.show();
                leScanner.startScan(scanFilters, scanSettingBuilder.build(),
                        new BleScanCallback());
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (alertDialogConnect.isShowing()) {
                            alertDialogConnect.dismiss();
                            Toast.makeText(MainActivity.this, "找不到设备", Toast.LENGTH_SHORT).show();
                            leScanner.stopScan(new BleScanCallback());
                            switchSoftKeyboardState(true);
                            tvDeviceCode.setText("");
                        }
                    }
                }, scanTimeoutMs);
            }
        } else if (vId == R.id.btn_backspace) {
            if (tvDeviceCode.getText().length() > 0) {
                Log.v(LOG_TAG, "backspace");
                tvDeviceCode.setText(tvDeviceCode.getText().toString().substring(0, tvDeviceCode.getText().length() - 1));
            }
        }
    }

    private void switchSoftKeyboardState(boolean newState) {
        btnNumZero.setClickable(newState);
        btnNumOne.setClickable(newState);
        btnNumTwo.setClickable(newState);
        btnNumThree.setClickable(newState);
        btnNumFour.setClickable(newState);
        btnNumFive.setClickable(newState);
        btnNumSix.setClickable(newState);
        btnNumSeven.setClickable(newState);
        btnNumEight.setClickable(newState);
        btnNumNine.setClickable(newState);
        btnBackspace.setClickable(newState);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        for (int res : grantResults) {
            if (res == PackageManager.PERMISSION_DENIED) {
                Toast.makeText(this, "权限不完整", Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }
}
