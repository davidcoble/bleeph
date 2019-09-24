package com.adafruit.bluefruit.le.connect.app;

import android.content.Context;
import android.graphics.Canvas;
import android.support.annotation.Nullable;
import android.util.AttributeSet;
import android.util.Log;

import java.util.Locale;

public class BatteryTextView extends android.support.v7.widget.AppCompatTextView {
    private final static String TAG = ControllerFragment.class.getSimpleName();

    private int batteryVal = 0;

    public BatteryTextView(Context context) {
        super(context);
    }

    public BatteryTextView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public BatteryTextView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected int[] onCreateDrawableState(int extraSpace) {
        return super.onCreateDrawableState(extraSpace);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        Log.d(TAG, "HHHHHHHHHHHHHHHHHH  onDraw");
        setText(String.format(Locale.US, "new bat lev:\n%d quatloos", batteryVal));
        super.onDraw(canvas);
    }

    @Override
    public boolean onPreDraw() {
        Log.d(TAG, "HHHHHHHHHHHHHHHHHH  onPreDraw");
        setText(String.format(Locale.US, "new bat lev:\n%d quatloos", batteryVal));
        return super.onPreDraw();
    }

    public int getBatteryVal() {
        return batteryVal;
    }

    public void setBatteryVal(int batteryVal) {
        this.batteryVal = batteryVal;
    }
}
