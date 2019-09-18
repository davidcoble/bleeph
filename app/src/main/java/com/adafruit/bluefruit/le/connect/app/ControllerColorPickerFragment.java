package com.adafruit.bluefruit.le.connect.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.ActionBar;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.util.Log;

import com.adafruit.bluefruit.le.connect.BluefruitApplication;
import com.adafruit.bluefruit.le.connect.R;
import com.adafruit.bluefruit.le.connect.app.colorPicker.ColorPicker;
import com.adafruit.bluefruit.le.connect.app.colorPicker.SaturationBar;
import com.adafruit.bluefruit.le.connect.app.colorPicker.ValueBar;
import com.adafruit.bluefruit.le.connect.app.neopixel.NeopixelColorPickerFragment;

import java.util.Timer;
import java.util.TimerTask;

public class ControllerColorPickerFragment extends Fragment implements ColorPicker.OnColorChangedListener,
        ValueBar.OnValueChangedListener
{
    // Log
    @SuppressWarnings("unused")
    private final static String TAG = ControllerColorPickerFragment.class.getSimpleName();

    // Constants
    private final static boolean kPersistValues = true;
    private final static String kPreferences = "ColorPickerActivity_prefs";
    private final static String kPreferences_color = "color";

    private final static int kFirstTimeColor = 0x0000ff;
    private final static byte kModeCodeSwirl = 1;
    private final static byte kModeCodeReverse = 2;
    private final static byte kModeCodeTwist = 3;
    private final static byte kModeCodeRainbow = 4;
    private final static byte kModeCodeMarquee = 5;

    // UI
    private ColorPicker mColorPicker;
    private View mRgbColorView;
    private TextView mRgbTextView;

    // Data
    private int mSelectedColor;
    private ControllerColorPickerFragmentListener mListener;
    private TextView mSelectedColorAlert;
    private Button[] colorButton = new Button[10];
    private int[] colors = new int[10];
    private int[] buttonColor = new int[10];
    private int[] buttonColorSV = new int[10];
    private float mColorValue = 10;
    private SharedPreferences mSharedPreferences = BluefruitApplication.getAppContext().getSharedPreferences("BleephSeetings", 0);
    private SharedPreferences.Editor editor = mSharedPreferences.edit();

    // region Lifecycle
    @SuppressWarnings("UnnecessaryLocalVariable")
    public static ControllerColorPickerFragment newInstance() {
        ControllerColorPickerFragment fragment = new ControllerColorPickerFragment();
        return fragment;
    }

    public ControllerColorPickerFragment() {
        // Required empty public constructor
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        AppCompatActivity activity = (AppCompatActivity) getActivity();
        if (activity != null) {
            ActionBar actionBar = activity.getSupportActionBar();
            if (actionBar != null) {
                actionBar.setTitle(R.string.colorpicker_title);
                actionBar.setDisplayHomeAsUpEnabled(true);
            }
        }
    }

    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_controller_colorpicker, container, false);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        // UI
        mRgbColorView = view.findViewById(R.id.rgbColorView);
        mRgbTextView = view.findViewById(R.id.rgbTextView);
        mSelectedColorAlert = view.findViewById(R.id.color_selected);

        mColorValue = mSharedPreferences.getFloat("mColorValue", 0.1f);
        LinearLayout row = view.findViewById(R.id.color_button_row_01);
        for(int i = 0 ; i < row.getChildCount(); i++) {
            colorButton[i] = (Button) row.getChildAt(i);
        }
        row = view.findViewById(R.id.color_button_row_02);
        for(int i = 0 ; i < row.getChildCount(); i++) {
            colorButton[i+5] = (Button) row.getChildAt(i);
        }
        for(int i = 0 ; i < colorButton.length; i++ ) {
            colorButton[i].setTextColor(0xFF000000);
            int sel = mSharedPreferences.getInt("colorButtonSelected" + i, 0);
            if(sel == 1) {
                colorButton[i].setText("Yes");
                colorButton[i].setSelected(true);
            } else {
                colorButton[i].setText("No");
                colorButton[i].setSelected(false);
            }
            int col = mSharedPreferences.getInt("colorButtonBG" + i, 0);
            colorButton[i].setBackgroundColor(col);
            col = mSharedPreferences.getInt("color" + i, 0);
            buttonColorSV[i] = col;

            colorButton[i].setTag(i);
            colorButton[i].setOnClickListener(view1 -> {
                Button button = (Button) view1;
                Integer x = (Integer)button.getTag();
                button.setBackgroundColor(mSelectedColor);
                buttonColor[x] = mSelectedColor;
                buttonColorSV[x] = mSelectedColor;
                editor.putInt("colorButtonBG" + x, mSelectedColor);
                editor.putInt("color" + x, mSelectedColor);
                editor.apply();
                mColorPicker.setOldCenterColor(mSelectedColor);
            });
            colorButton[i].setOnLongClickListener(view1 -> {
                Button button = (Button) view1;
                Integer x = (Integer)button.getTag();
                if(button.isSelected()) {
                    mSelectedColorAlert.setText(R.string.color_unselected);
                    mSelectedColorAlert.setVisibility(View.VISIBLE);
                    button.setText("No");
                    editor.putInt("colorButtonSelected" + x, 0);
                    button.setTextColor(0xFF000000);
                    button.setSelected(false);
                    TimerTask timerTask = new TimerTask() {
                        @Override
                        public void run() {
                            mSelectedColorAlert.setVisibility(View.INVISIBLE);
                        }
                    };
                    Timer timer = new Timer();
                    timer.schedule(timerTask, 1000);
                } else {
                    mSelectedColorAlert.setText(R.string.color_selected);
                    mSelectedColorAlert.setVisibility(View.VISIBLE);
                    button.setText("Yes");
                    editor.putInt("colorButtonSelected" + x, 1);
                    button.setTextColor(0xFF000000);
                    button.setSelected(true);
                    TimerTask timerTask = new TimerTask() {
                        @Override
                        public void run() {
                            mSelectedColorAlert.setVisibility(View.INVISIBLE);
                        }
                    };
                    Timer timer = new Timer();
                    timer.schedule(timerTask, 1000);
                }
                editor.apply();
                return true;
            });
        }

        class SendColorTimer extends TimerTask {
            byte colorIndex;
            public SendColorTimer(byte colorIndex) {
                this.colorIndex = colorIndex;
            };
            public void run() {
                mListener.onSendColor(colors[colorIndex], colorIndex);
            }
        };
        class SendNumColorsTimer extends TimerTask {
            byte numColors;
            public SendNumColorsTimer(byte numColors) {
                this.numColors = numColors;
            }
            @Override
            public void run() {
                mListener.onSendNumColors(numColors);
            }
        }
        class SendModeTimer extends TimerTask {
            byte mode;
            public SendModeTimer(byte mode) {
                this.mode = mode;
            }
            @Override
            public void run() {
                mListener.onSendMode(mode);
            }
        }

        Button buttonSwirl = view.findViewById(R.id.modeButtonSwirl);
        buttonSwirl.setOnClickListener(view1 -> {
            Log.d(TAG, "Swirl button clicked.");
            Timer timer = new Timer();
            timer.schedule(new SendModeTimer(kModeCodeSwirl), 3000);
            byte j = 0;
            for(int i = 0; i < 10; i++) {
                if(colorButton[i].isSelected()) {
                    colors[j] = buttonColor[i];
                    j++;
                }
            }
            timer.schedule(new SendNumColorsTimer(j), 100);
            while (j > 0) {
                timer.schedule(new SendColorTimer((byte)(j-1)), (j + 1) * 100);
                j--;
            }
        });
        view.findViewById(R.id.modeButtonMarquee).setOnClickListener(view1 -> {
            int j = 0;
            for(int i = 0; i < 10; i++) {
                if(colorButton[i].isSelected()) {
                    colors[j] = buttonColor[i];
                    j++;
                }
            }
//            mListener.onSendMode(kModeCodeMarquee, j, colors);
        });
        view.findViewById(R.id.modeButtonRainbow).setOnClickListener(view1 -> {
            mListener.onSendMode(kModeCodeRainbow);
        });
        view.findViewById(R.id.modeButtonReverse).setOnClickListener(view1 -> {
//            mListener.onSendMode(kModeCodeReverse, (int)(mColorValue * 255));
        });
        view.findViewById(R.id.modeButtonTwist).setOnClickListener(view1 -> {
            Log.d(TAG, "Twist button clicked.");
            mListener.onSendMode(kModeCodeTwist);
            int j = 0;
            for(int i = 0; i < 10; i++) {
                if(colorButton[i].isSelected()) {
                    colors[j] = buttonColor[i];
                    j++;
                }
            }
        });
//        Button sendButton = view.findViewById(R.id.sendButton);
//        sendButton.setOnClickListener(view1 -> {
//            // Set the old color
//            mColorPicker.setOldCenterColor(mSelectedColor);
//            mListener.onSendColorComponents(mSelectedColor);
//        });

        SaturationBar mSaturationBar = view.findViewById(R.id.brightnessbar);
        ValueBar mValueBar = view.findViewById(R.id.valuebar);
        mValueBar.setOnValueChangedListener(this);
        float mColorValue = mSharedPreferences.getFloat("mColorValue", 0.5f);
        System.err.println("mColorValue = " + mColorValue);
        mValueBar.setColor(Color.HSVToColor(new float[]{1.0f, 1.0f, mColorValue}));
        mColorPicker = view.findViewById(R.id.colorPicker);
        if (mColorPicker != null) {
            mColorPicker.addSaturationBar(mSaturationBar);
            mColorPicker.addValueBar(mValueBar);
            mColorPicker.setOnColorChangedListener(this);
        }
        mValueBar.setValue(mColorValue);

        final Context context = getContext();
        if (context != null && kPersistValues) {
            SharedPreferences preferences = context.getSharedPreferences(kPreferences, Context.MODE_PRIVATE);
            mSelectedColor = preferences.getInt(kPreferences_color, kFirstTimeColor);
        } else {
            mSelectedColor = kFirstTimeColor;
        }

        mColorPicker.setOldCenterColor(mSelectedColor);
        mColorPicker.setColor(mSelectedColor);
        onColorChanged(mSelectedColor);
    }


    @Override
    public void onStop() {

        final Context context = getContext();
        // Preserve values
        if (context != null && kPersistValues) {
            SharedPreferences settings = context.getSharedPreferences(kPreferences, Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = settings.edit();
            editor.putInt(kPreferences_color, mSelectedColor);
            editor.apply();
        }

        super.onStop();
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        if (context instanceof NeopixelColorPickerFragment.NeopixelColorPickerFragmentListener) {
            System.err.println(("######################## 11111"));
            mListener = (ControllerColorPickerFragmentListener) context;
        } else if (getTargetFragment() instanceof ControllerColorPickerFragmentListener) {
            System.err.println(("######################## 22222"));
            mListener = (ControllerColorPickerFragmentListener) getTargetFragment();
        }  else {
            throw new RuntimeException(context.toString() + " must implement NeopixelColorPickerFragmentListener");
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mListener = null;
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        inflater.inflate(R.menu.menu_help, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        FragmentActivity activity = getActivity();

        switch (item.getItemId()) {
            case R.id.action_help:
                if (activity != null) {
                    FragmentManager fragmentManager = activity.getSupportFragmentManager();
                    if (fragmentManager != null) {
                        CommonHelpFragment helpFragment =
                                CommonHelpFragment.newInstance(getString(R.string.colorpicker_help_title),
                                        getString(R.string.colorpicker_help_text));
                        FragmentTransaction fragmentTransaction = fragmentManager.beginTransaction()
                                .replace(R.id.contentLayout, helpFragment, "Help");
                        fragmentTransaction.addToBackStack(null);
                        fragmentTransaction.commit();
                    }
                }
                return true;

            default:
                return super.onOptionsItemSelected(item);
        }
    }

    // endregion

    // region OnColorChangedListener

    @SuppressWarnings("PointlessBitwiseExpression")

    private int calcColor(int color) {
        float[] hSV = new float[3];
        Color.colorToHSV(color, hSV);
        hSV[2] = mColorValue;
        return Color.HSVToColor(hSV);
    }

    @Override
    public void onColorChanged(int color) {
        // Save selected color
        mSelectedColor = color;

        // Update UI
        mRgbColorView.setBackgroundColor(color);

        final int r = (color >> 16) & 0xFF;
        final int g = (color >> 8) & 0xFF;
        final int b = (color >> 0) & 0xFF;
        final String text = String.format(getString(R.string.colorpicker_rgb_format), r, g, b);
        mRgbTextView.setText(text);
        //mListener.onSendColorComponents(color);
    }

    @Override
    public void onValueChanged(int color) {
        float[] hSV = new float[3];
        Color.colorToHSV(color, hSV);
        mColorValue = hSV[2];
        editor.putFloat("mColorValue", mColorValue);
        for(int i = 0 ; i < colorButton.length; i++ ) {
            Color.colorToHSV(buttonColorSV[i], hSV);
            hSV[2] = mColorValue;
            buttonColor[i] = Color.HSVToColor(hSV);
        }
        Log.d("---", "mColorValue = " + mColorValue);
        mListener.onSendValue((byte)(255f * mColorValue));
    }


    // endregion

    // region
    interface ControllerColorPickerFragmentListener {
        void onSendColorComponents(int color);
        void onSendMode(byte mode);
        // void onSendMode(int mode, int numColors, int[] colors);
        void onSendNumColors(byte numColors);
        void onSendColor(int color, byte num);
        void onSendValue(byte value);
    }

    // endregion
}
