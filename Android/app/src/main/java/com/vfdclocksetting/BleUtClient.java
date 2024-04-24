package com.vfdclocksetting;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import androidx.annotation.NonNull;

import java.util.Objects;
import java.util.UUID;
import java.util.concurrent.Semaphore;

public abstract class BleUtClient extends BluetoothGattCallback {
    final static private String LOG_TAG = "BleUtClient";
    final static private String BLE_UT_SERVICE_UUID_STRING = "5c111a6e-53b0-46f4-aff5-8505f943411b";
    final static private String BLE_UT_CHARACTERISTIC_TO_SERVER_UUID_STRING = "f69cc8c9-358e-6d98-604b-51217440a5b5";
    final static private String BLE_UT_CHARACTERISTIC_TO_CLIENT_UUID_STRING = "1b4143f9-0585-f5af-f446-b0536e1a115c";
    final static private String BLE_UT_CHARACTERISTIC_TO_CLIENT_DESC_UUID_STRING = "00002902-0000-1000-8000-00805F9B34FB";
    final static private String BUNDLE_KEY_TO_SERVER_DATA = "toServerTightData";
    final static private String BUNDLE_KEY_TO_CLIENT_DATA = "toClientData";
    final static private int TARGET_MTU = 512;
    private BluetoothGatt gatt;
    private BluetoothGattCharacteristic bleUtToServerCharacteristic;
    private BluetoothGattCharacteristic bleUtToClientCharacteristic;

    private Handler utToServerHandler;
    private final Thread utToServerThread;
    private Looper utToServerThreadLooper;
    private Semaphore utToServerSemaphore;
    private int toServerAllSize;
    private int toServerRemainSize;

    private Handler utToClientHandler;
    private final Thread utToClientThread;
    private Looper utToClientThreadLooper;
    private int toClientAllSize;
    private int toClientRemainSize;
    private byte[] toClientData;

    private int mtu;

    @SuppressLint("MissingPermission")
    public BleUtClient() {
        super();
        this.mtu = 23;

        utToServerThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                utToServerThreadLooper = Looper.myLooper();
                utToServerSemaphore = new Semaphore(1, true);
                utToServerHandler = new Handler(Objects.requireNonNull(Looper.myLooper()), new Handler.Callback() {
                    private byte[] toServerData;

                    @SuppressLint("MissingPermission")
                    @Override
                    public boolean handleMessage(@NonNull Message msg) {
                        toServerData = msg.getData().getByteArray(BUNDLE_KEY_TO_SERVER_DATA);
                        if (toServerData == null) return false;
                        toServerAllSize = toServerData.length;
                        toServerRemainSize = toServerAllSize;
                        try {
                            utToServerSemaphore.acquire();
                        } catch (InterruptedException e) {
                            throw new RuntimeException(e);
                        }
                        byte[] b = new byte[5];
                        b[0] = 0x01;
                        b[1] = (byte) (toServerAllSize % 256);
                        b[2] = (byte) (toServerAllSize % 65536 / 256);
                        b[3] = (byte) (toServerAllSize % 16777215 / 65536);
                        b[4] = (byte) (toServerAllSize / 16777215);
                        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                            gatt.writeCharacteristic(bleUtToServerCharacteristic, b, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                        } else {
                            bleUtToServerCharacteristic.setValue(b);
                        }
                        while (toServerRemainSize != 0) {
                            int currentSize =
                                    toServerRemainSize > mtu - 3 - 4 ? mtu - 3 - 4
                                            : toServerRemainSize;
                            b = new byte[currentSize];
                            System.arraycopy(toServerData, toServerAllSize - toServerRemainSize, b, 0, currentSize);
                            try {
                                utToServerSemaphore.acquire();
                            } catch (InterruptedException e) {
                                throw new RuntimeException(e);
                            }
                            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                                gatt.writeCharacteristic(bleUtToServerCharacteristic, b, BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE);
                            } else {
                                bleUtToServerCharacteristic.setValue(b);
                            }
                            toServerRemainSize -= currentSize;
                        }
                        toServerData = null;
                        return true;
                    }
                });
                Looper.loop();
            }
        });

        utToClientThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                utToClientThreadLooper = Looper.myLooper();
                utToClientHandler = new Handler(Objects.requireNonNull(Looper.myLooper()), new Handler.Callback() {
                    @Override
                    public boolean handleMessage(@NonNull Message msg) {
                        byte[] toClientData = msg.getData().getByteArray(BUNDLE_KEY_TO_CLIENT_DATA);
                        if (toClientData != null) {
                            onReceiveToClientData(toClientData);
                            toClientData = null;
                        }
                        return true;
                    }
                });
                Looper.loop();
            }
        });
    }

    @SuppressLint("MissingPermission")
    @Override
    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
        super.onConnectionStateChange(gatt, status, newState);
        Log.v(LOG_TAG, "onConnectionStateChange");
        if (newState == BluetoothGatt.STATE_CONNECTED) {
            this.gatt = gatt;
            Log.v(LOG_TAG, "request mtu " + (gatt.requestMtu(TARGET_MTU) ?
                    "success" : "fail"));
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
            Log.v(LOG_TAG, "start discover service: " + (gatt.discoverServices() ? "success" : "'fail"));

//            new Thread(new Runnable() {
//                @Override
//                public void run() {
//                    for (int i = 0; i < 3; i++) {
//                        try {
//                            Thread.sleep(500);
//                        } catch (InterruptedException e) {
//                            throw new RuntimeException(e);
//                        }
//                        if (gatt.discoverServices()) break;
//                    }
//
//                }
//            }).start();
        } else if (newState == BluetoothGatt.STATE_DISCONNECTED) {
            utToServerThreadLooper.quit();
            utToClientThreadLooper.quit();
            utToServerHandler.removeCallbacksAndMessages(null);
            utToClientHandler.removeCallbacksAndMessages(null);
        }
    }

    @Override
    public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
        super.onMtuChanged(gatt, mtu, status);
        Log.v(LOG_TAG, "onMtuChanged");
        if (status == BluetoothGatt.GATT_SUCCESS) {
            Log.v(LOG_TAG, "new mtu: " + String.valueOf(mtu));
            this.mtu = mtu;
        }
    }

    @SuppressLint("MissingPermission")
    @Override
    public void onServicesDiscovered(BluetoothGatt gatt, int status) {
        super.onServicesDiscovered(gatt, status);
        Log.v(LOG_TAG, "onServicesDiscovered");
        if (status == BluetoothGatt.GATT_SUCCESS) {
            Log.v(LOG_TAG, "discover service success");
            BluetoothGattService bleUtService = gatt.getService(UUID.fromString(BLE_UT_SERVICE_UUID_STRING));
            if (bleUtService == null) return;
            bleUtToServerCharacteristic = bleUtService.getCharacteristic(UUID.fromString(BLE_UT_CHARACTERISTIC_TO_SERVER_UUID_STRING));
            bleUtToClientCharacteristic = bleUtService.getCharacteristic(UUID.fromString(BLE_UT_CHARACTERISTIC_TO_CLIENT_UUID_STRING));
            if (bleUtToClientCharacteristic == null) return;
            BluetoothGattDescriptor bleUtToClientDescriptor = bleUtToClientCharacteristic.getDescriptor(UUID.fromString(BLE_UT_CHARACTERISTIC_TO_CLIENT_DESC_UUID_STRING));
            utToServerThread.start();
            utToClientThread.start();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                gatt.setCharacteristicNotification(bleUtToClientCharacteristic, true);
                gatt.writeDescriptor(bleUtToClientDescriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            }
        } else Log.v(LOG_TAG, "discover service fail");
    }

    @Override
    public void onCharacteristicRead(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value, int status) {
        super.onCharacteristicRead(gatt, characteristic, value, status);
    }

    @Override
    public void onCharacteristicChanged(@NonNull BluetoothGatt gatt, @NonNull BluetoothGattCharacteristic characteristic, @NonNull byte[] value) {
        super.onCharacteristicChanged(gatt, characteristic, value);
        if (characteristic == bleUtToClientCharacteristic) {
            if (value.length == 5 && value[0] == 1 && toClientRemainSize == 0) {
                toClientAllSize = Byte.toUnsignedInt(value[1]) + Byte.toUnsignedInt(value[2]) * 256 + Byte.toUnsignedInt(value[3]) * 65536 + Byte.toUnsignedInt(value[4]) * 16777215;
                toClientRemainSize = toClientAllSize;
                toClientData = new byte[toClientAllSize];
                Log.v(LOG_TAG, "recv pkg head: All size: " + String.valueOf(toClientAllSize));
            } else {
                System.arraycopy(value, 0, toClientData, toClientAllSize - toClientRemainSize, value.length);
                toClientRemainSize -= value.length;
                Log.v(LOG_TAG, "remain size: " + String.valueOf(toClientRemainSize));
                if (toClientRemainSize == 0) {
                    //测试完成后，再优化性能
                    Bundle bundle = new Bundle();
                    bundle.putByteArray(BUNDLE_KEY_TO_CLIENT_DATA, toClientData);
                    Message msg = new Message();
                    msg.setData(bundle);
                    utToClientHandler.sendMessage(msg);
                    toClientData = null;
                }
            }
        }
    }

    @Override
    public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic, int status) {
        super.onCharacteristicWrite(gatt, characteristic, status);
        if (status == BluetoothGatt.GATT_SUCCESS) {
            if (characteristic == bleUtToServerCharacteristic) {
                utToServerSemaphore.release();
            }
        }
    }

    @Override
    public void onDescriptorWrite(BluetoothGatt gatt, BluetoothGattDescriptor descriptor, int status) {
        super.onDescriptorWrite(gatt, descriptor, status);
    }

    final public void sendToServer(byte[] data) {
        if (utToServerThread.isAlive()) {
            Message msg = new Message();
            Bundle bundle = new Bundle();
            bundle.putByteArray(BUNDLE_KEY_TO_SERVER_DATA, data);
            msg.setData(bundle);
            utToServerHandler.sendMessage(msg);
        }
    }

    abstract void onReceiveToClientData(byte[] data);

    @SuppressLint("MissingPermission")
    public void disconnect() {
        gatt.disconnect();
    }
}
