package com.vfdclocksetting;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.IntRange;

import com.google.gson.annotations.SerializedName;

public class VfdClkConfig implements Parcelable {
    @SerializedName("myVfd")
    public myVfd myVfd;
    @SerializedName("myClock")
    public MyClock myClock;
    @SerializedName("myWifi")
    public MyWifi myWifi;

    public VfdClkConfig() {
    }

    protected VfdClkConfig(Parcel in) {
        if (in.readBoolean()) {
            myVfd = new myVfd();
            if (in.readBoolean()) {
                myVfd.brightness = in.readInt();
            }
        }

        if (in.readBoolean()) {
            myClock = new MyClock();
            if (in.readBoolean()) {
                myClock.onSCRNDmin = in.readInt();
                myClock.offSCRNDmin = in.readInt();
            }

            if (in.readBoolean()) {
                myClock.hourPrompt = in.readBoolean();
            }
        }

        if (in.readBoolean()) {
            myWifi = new MyWifi();
            if (in.readBoolean()) {
                myWifi.ssid = in.readString();
                myWifi.password = in.readString();
                myWifi.needAuth = in.readBoolean();
            }
        }
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeBoolean(myVfd != null);
        if (myVfd != null) {
            dest.writeBoolean(myVfd.brightness != null);
            if (myVfd != null) {
                dest.writeInt(myVfd.brightness);
            }
        }

        dest.writeBoolean(myClock != null);
        if (myClock != null) {
            dest.writeBoolean(myClock.onSCRNDmin != null && myClock.offSCRNDmin != null);
            if (myClock.onSCRNDmin != null && myClock.offSCRNDmin != null) {
                dest.writeInt(myClock.onSCRNDmin);
                dest.writeInt(myClock.offSCRNDmin);
            }

            dest.writeBoolean(myClock.hourPrompt != null);
            if (myClock.hourPrompt != null) {
                dest.writeBoolean(myClock.hourPrompt);
            }
        }

        dest.writeBoolean(myWifi != null);
        if (myWifi != null) {
            dest.writeBoolean(myWifi.ssid != null && myWifi.password != null && myWifi.needAuth != null);
            if (myWifi.ssid != null && myWifi.password != null && myWifi.needAuth != null) {
                dest.writeString(myWifi.ssid);
                dest.writeString(myWifi.password);
                dest.writeBoolean(myWifi.needAuth);
            }
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Creator<VfdClkConfig> CREATOR = new Creator<VfdClkConfig>() {
        @Override
        public VfdClkConfig createFromParcel(Parcel in) {
            return new VfdClkConfig(in);
        }

        @Override
        public VfdClkConfig[] newArray(int size) {
            return new VfdClkConfig[size];
        }
    };

    public class myVfd {
        private Integer brightness;

        public Integer getBrightness() {
            return brightness;
        }
    }


    public void setBrightness(@IntRange(from = 0, to = 240) int i) {
        myVfd.brightness = i;
    }

    public class MyWifi {
        private Boolean needAuth;
        private String ssid;
        private String password;

        public Boolean getNeedAuth() {
            return needAuth;
        }

        public String getSsid() {
            return ssid;
        }

        public String getPassword() {
            return password;
        }
    }

    public void setWifiInfo(String ssid, String password, boolean needAuth) {
        if (myWifi == null) myWifi = new MyWifi();

        myWifi.ssid = ssid;
        myWifi.password = password;
        myWifi.needAuth = needAuth;
    }

    public class MyClock {
        private Integer onSCRNDmin;
        private Integer offSCRNDmin;
        private Boolean hourPrompt;

        public boolean getTimingSwitchState() {
            return !(onSCRNDmin.intValue() == offSCRNDmin.intValue());
        }

        public Integer getOnSCRNDmin() {
            return onSCRNDmin;
        }

        public Integer getOffSCRNDmin() {
            return offSCRNDmin;
        }

        public Boolean getHourPrompt() {
            return hourPrompt;
        }
    }

    public void setTimingSwitchScreen(boolean state, @IntRange(from = 0, to = 1440) int on,
                                      @IntRange(from = 0, to = 1440) int off) {
        if (myClock == null) myClock = new MyClock();

        if (state) {
            myClock.onSCRNDmin = on;
            myClock.offSCRNDmin = off;
        } else {
            myClock.onSCRNDmin = 0;
            myClock.offSCRNDmin = 0;
        }
    }

    public void setHourPrompt(boolean b) {
        if (myClock == null) myClock = new MyClock();

        myClock.hourPrompt = b;
    }
}