package com.vfdclocksetting;

import android.Manifest;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.NumberPicker;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.materialswitch.MaterialSwitch;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.timepicker.MaterialTimePicker;
import com.google.android.material.timepicker.TimeFormat;
import com.google.gson.Gson;

import java.nio.charset.StandardCharsets;
import java.util.Objects;

public final class VfdClkSettingPage extends AppCompatActivity implements View.OnClickListener, Handler.Callback {
    final private static String LOG_TAG = "VfdClkSettingPage";

    private String[] permissions;

    private TextInputEditText etWifiSsid;
    private TextInputEditText etWifiPassword;
    private MaterialSwitch swTimingSwitchScreen;
    private TextInputEditText etOnScreenDmin;
    private TextInputEditText etOffScreenDmin;
    private MaterialSwitch swAutoBrightness;
    private TextInputEditText etScreenBrightness;
    private MaterialSwitch swHourPrompt;
    private MaterialButton btnSave;

    private BluetoothGatt gatt;
    private MaterialTimePicker.Builder materialTimePickerBuilder;
    private BleUtClient bleUtClient;

    private Handler handler;

    private VfdClkConfig vfdClkConfig;

    private VfdClkConfig newConfig;

    @SuppressLint("MissingPermission")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_vfd_clk_setting_page);

        etWifiSsid = findViewById(R.id.et_wifi_ssid);
        etWifiPassword = findViewById(R.id.et_wifi_password);
        swTimingSwitchScreen = findViewById(R.id.sw_timing_switch_screen);
        etOnScreenDmin = findViewById(R.id.et_on_screen_dmin);
        etOffScreenDmin = findViewById(R.id.et_off_screen_dmin);
        swAutoBrightness = findViewById(R.id.sw_auto_brightness);
        etScreenBrightness = findViewById(R.id.et_screen_brightness);
        swHourPrompt = findViewById(R.id.sw_hour_prompt);
        btnSave = findViewById(R.id.btn_save);

        etOnScreenDmin.setOnClickListener(this);
        etOffScreenDmin.setOnClickListener(this);
        etScreenBrightness.setOnClickListener(this);
        btnSave.setOnClickListener(this);

        swAutoBrightness.setOnCheckedChangeListener((buttonView, isChecked) -> {
            etScreenBrightness.setEnabled(!isChecked);
        });

        materialTimePickerBuilder = new MaterialTimePicker.Builder();

        handler = new Handler(Looper.getMainLooper(), this);

        ((MaterialSwitch) (findViewById(R.id.sw_timing_switch_screen))).setOnCheckedChangeListener((buttonView, isChecked) -> {
            switchTimingSwitchScreenSetting(isChecked);
        });

        AlertDialog alertDialog = new MaterialAlertDialogBuilder(this).create();
        alertDialog.setCancelable(false);
        alertDialog.setTitle("连接中");
        alertDialog.show();

        Intent intent = getIntent();
        Bundle bundle = intent.getExtras();
        BluetoothDevice bluetoothDevice = null;
        if (bundle != null) {
            bluetoothDevice = bundle.getParcelable("leDevice");
        }

        if (bluetoothDevice != null) {
            bleUtClient = new BleUtClient() {
                @Override
                void onReceiveToClientData(byte[] data) {
                    Log.v(LOG_TAG, "onReceiveToClientData");
                    Gson gson = new Gson();
                    Log.v(LOG_TAG, "recv data: " + new String(data, StandardCharsets.US_ASCII));
                    vfdClkConfig = gson.fromJson(new String(data, StandardCharsets.US_ASCII), VfdClkConfig.class);
                    Log.v(LOG_TAG, gson.toJson(vfdClkConfig));

                    Message message = Message.obtain();
                    message.what = 0;
                    Bundle bundle1 = new Bundle();
                    bundle1.putParcelable("config", vfdClkConfig);
                    message.setData(bundle1);
                    handler.sendMessage(message);
                    alertDialog.dismiss();
                }
            };
            gatt = bluetoothDevice.connectGatt(this, false, bleUtClient);
            if (gatt == null) {
                Log.e(LOG_TAG, "connect fail");
                Toast.makeText(this, "连接失败", Toast.LENGTH_SHORT).show();
                intent = new Intent();
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
                intent.setClass(getApplicationContext(), MainActivity.class);
                startActivity(intent);
                return;
            }
            alertDialog.setTitle("连接成功");
        } else {
            Log.e(LOG_TAG, "null bluetooth device");
            Toast.makeText(this, "无效设备", Toast.LENGTH_SHORT).show();
            intent = new Intent();
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
            intent.setClass(getApplicationContext(), MainActivity.class);
            startActivity(intent);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions = new String[]{Manifest.permission.BLUETOOTH_CONNECT};
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        for (String permission : permissions) {
            if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this, new String[]{permission}, 0);
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.v(LOG_TAG, "onDestroy");
        bleUtClient.disconnect();
    }

    @Override
    public void onClick(View v) {
        int vId = v.getId();
        if (vId == R.id.et_on_screen_dmin) {
            MaterialTimePicker materialTimePicker = materialTimePickerBuilder.setInputMode(MaterialTimePicker.INPUT_MODE_CLOCK).setTitleText("设置屏幕开启时间").setTimeFormat(TimeFormat.CLOCK_24H).build();
            materialTimePicker.addOnPositiveButtonClickListener(v1 -> {
                etOnScreenDmin.setText(String.format(getString(R.string.hour_minute),
                        materialTimePicker.getHour(),
                        materialTimePicker.getMinute()));
            });
            materialTimePicker.showNow(getSupportFragmentManager(), "6");
        } else if (vId == R.id.et_off_screen_dmin) {
            MaterialTimePicker materialTimePicker =
                    materialTimePickerBuilder.setInputMode(MaterialTimePicker.INPUT_MODE_CLOCK).setTitleText("设置屏幕关闭时间").setTimeFormat(TimeFormat.CLOCK_24H).build();
            materialTimePicker.addOnPositiveButtonClickListener(v1 -> {
                etOffScreenDmin.setText(String.format(String.valueOf(R.string.hour_minute),
                        materialTimePicker.getHour(),
                        materialTimePicker.getMinute()));
            });
            materialTimePicker.showNow(getSupportFragmentManager(), "6");
        } else if (vId == R.id.et_screen_brightness) {
            NumberPicker numberPicker = new NumberPicker(getApplicationContext());
            numberPicker.setMinValue(0);
            numberPicker.setMaxValue(240);
            numberPicker.setValue(Integer.parseInt("0" + Objects.requireNonNull(etScreenBrightness.getText()).toString()));
            numberPicker.setOnValueChangedListener((picker, oldVal, newVal) -> {
                etScreenBrightness.setText(String.valueOf(newVal));
            });
            MaterialAlertDialogBuilder alertDialogBuilder = new MaterialAlertDialogBuilder(this).setView(numberPicker);
            alertDialogBuilder.show();
        } else if (vId == R.id.btn_save) {
            //wifi info
            if (etWifiSsid.length() == 0) {
                etWifiSsid.setError("wifi ssid 未设置");
                return;
            }
            vfdClkConfig.setWifiInfo(Objects.requireNonNull(etWifiSsid.getText()).toString(), Objects.requireNonNull(etWifiPassword.getText()).toString(), false);
            //timing switch screen
            if (swTimingSwitchScreen.isChecked()) {
                if (etOnScreenDmin.length() == 0) {
                    etOnScreenDmin.setError("屏幕开启时间 未设置");
                    return;
                }
                if (etOffScreenDmin.length() == 0) {
                    etOnScreenDmin.setError("屏幕关闭时间 未设置");
                    return;
                }
                vfdClkConfig.setTimingSwitchScreen(true,
                        Integer.parseInt(Objects.requireNonNull(etOnScreenDmin.getText()).subSequence(0, 2).toString()) * 60 + Integer.parseInt(etOnScreenDmin.getText().subSequence(3, 5).toString()),
                        Integer.parseInt(Objects.requireNonNull(etOffScreenDmin.getText()).subSequence(0, 2).toString()) * 60 + Integer.parseInt(etOffScreenDmin.getText().subSequence(3, 5).toString()));
                Log.v(LOG_TAG,
                        "on screen: " + String.valueOf(Integer.parseInt(etOnScreenDmin.getText().subSequence(0, 2).toString()) * 60 + Integer.parseInt(etOnScreenDmin.getText().subSequence(3, 5).toString())));
                Log.v(LOG_TAG,
                        "off screen: " + String.valueOf(Integer.parseInt(etOffScreenDmin.getText().subSequence(0, 2).toString()) * 60 + Integer.parseInt(etOffScreenDmin.getText().subSequence(3, 5).toString())));
            } else {
                vfdClkConfig.setTimingSwitchScreen(false, 0, 0);
            }
            //screen brightness
            if (swAutoBrightness.isChecked()) {
                vfdClkConfig.setBrightness(0);
            } else {
                if (etScreenBrightness.length() == 0) {
                    etScreenBrightness.setError("屏幕亮度 未设置");
                    return;
                }
                vfdClkConfig.setBrightness(Integer.parseInt(Objects.requireNonNull(etScreenBrightness.getText()).toString()));
            }

            vfdClkConfig.setHourPrompt(swHourPrompt.isChecked());

            Gson gson = new Gson();
            Log.v(LOG_TAG, " new config json: " + gson.toJson(vfdClkConfig));
            bleUtClient.sendToServer(gson.toJson(vfdClkConfig).getBytes(StandardCharsets.US_ASCII));

            btnSave.setEnabled(false);
            btnSave.setText(R.string.num_three);
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    btnSave.setText(R.string.num_two);
                }
            }, 1000);
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    btnSave.setText(R.string.num_one);
                }
            }, 2000);
            new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
                @Override
                public void run() {
                    btnSave.setEnabled(true);
                    btnSave.setText(getString(R.string.save));
                }
            }, 3000);
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }

    private void switchTimingSwitchScreenSetting(boolean newState) {
        etOnScreenDmin.setEnabled(newState);
        etOffScreenDmin.setEnabled(newState);
    }

    private void renewUi(@NonNull VfdClkConfig config) {
        if (config.myVfd.getBrightness() == 0) {
            swAutoBrightness.setChecked(true);
            etScreenBrightness.setEnabled(false);
        } else {
            swAutoBrightness.setChecked(false);
            etScreenBrightness.setText(config.myVfd.getBrightness().toString());
        }

        etWifiSsid.setText(config.myWifi.getSsid());
        etWifiPassword.setText(config.myWifi.getPassword());

        if (config.myClock.getTimingSwitchState()) {
            swTimingSwitchScreen.setChecked(true);
            etOnScreenDmin.setText(String.format(getString(R.string.hour_minute),
                    config.myClock.getOnSCRNDmin() / 60, config.myClock.getOnSCRNDmin() % 60));
            etOffScreenDmin.setText(String.format(getString(R.string.hour_minute),
                    config.myClock.getOffSCRNDmin() / 60, config.myClock.getOffSCRNDmin() % 60));
        } else {
            swTimingSwitchScreen.setChecked(false);
            etOnScreenDmin.setEnabled(false);
            etOffScreenDmin.setEnabled(false);
        }
        swHourPrompt.setChecked(config.myClock.getHourPrompt());
    }

    @Override
    public boolean handleMessage(@NonNull Message msg) {
        if (msg.what == 0) {
            Bundle bundle = msg.getData();
            VfdClkConfig vfdClkConfig = bundle.getParcelable("config");
            renewUi(Objects.requireNonNull(vfdClkConfig));
        }
        return true;
    }
}