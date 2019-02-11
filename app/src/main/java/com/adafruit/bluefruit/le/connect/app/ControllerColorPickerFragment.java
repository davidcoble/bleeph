package com.adafruit.bluefruit.le.connect.app;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.RippleDrawable;
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

import com.adafruit.bluefruit.le.connect.BluefruitApplication;
import com.adafruit.bluefruit.le.connect.BuildConfig;
import com.adafruit.bluefruit.le.connect.R;
import com.adafruit.bluefruit.le.connect.app.neopixel.NeopixelColorPickerFragment;
import com.larswerkman.holocolorpicker.ColorPicker;
import com.larswerkman.holocolorpicker.SaturationBar;
import com.larswerkman.holocolorpicker.ValueBar;
import com.squareup.leakcanary.RefWatcher;

import java.lang.reflect.Field;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

public class ControllerColorPickerFragment extends Fragment implements ColorPicker.OnColorChangedListener {
    // Log
    @SuppressWarnings("unused")
    private final static String TAG = ControllerColorPickerFragment.class.getSimpleName();

    // Constants
    private final static boolean kPersistValues = true;
    private final static String kPreferences = "ColorPickerActivity_prefs";
    private final static String kPreferences_color = "color";

    private final static int kFirstTimeColor = 0x0000ff;
    private final static int kModeCodeSwirl = 1;
    private final static int kModeCodeReverse = 2;
    private final static int kModeCodeTwist = 3;
    private final static int kModeCodeRainbow = 4;

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
    private SharedPreferences mSharedPreferences;
    private SharedPreferences.Editor editor;

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

        mSharedPreferences = BluefruitApplication.getAppContext().getSharedPreferences("BleephSeetings", 0);
        editor = mSharedPreferences.edit();


        LinearLayout row = view.findViewById(R.id.color_button_row_01);
        for(int i = 0 ; i < row.getChildCount(); i++) {
            colorButton[i] = (Button) row.getChildAt(i);
        }
        row = view.findViewById(R.id.color_button_row_02);
        for(int i = 0 ; i < row.getChildCount(); i++) {
            colorButton[i+5] = (Button) row.getChildAt(i);
        }
        for(int i = 0 ; i < colorButton.length; i++ ) {
            colorButton[i].setTextColor(0xffffffff);
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
            buttonColor[i] = col;
            colorButton[i].setTag(i);
            colorButton[i].setOnClickListener(view1 -> {
                Button button = (Button) view1;
                Integer x = (Integer)button.getTag();
                int bgColor = lightenColor(mSelectedColor);
                button.setBackgroundColor(bgColor);
                buttonColor[x] = mSelectedColor;
                editor.putInt("colorButtonBG" + x, bgColor);
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
                    button.setTextColor(0xffffffff);
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
                    button.setTextColor(0xffffffff);
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

        Button buttonSwirl = view.findViewById(R.id.modeButtonSwirl);
        buttonSwirl.setOnClickListener(view1 -> {
            int j = 0;
            for(int i = 0; i < 10; i++) {
                if(colorButton[i].isSelected()) {
                    colors[j] = buttonColor[i];
                    j++;
                }
            }
            mListener.onSendMode(kModeCodeSwirl, j, colors);
        });
        view.findViewById(R.id.modeButtonRainbow).setOnClickListener(view1 -> {
            mListener.onSendMode(kModeCodeRainbow);
        });
        view.findViewById(R.id.modeButtonReverse).setOnClickListener(view1 -> {
            mListener.onSendMode(kModeCodeReverse);
        });
        view.findViewById(R.id.modeButtonTwist).setOnClickListener(view1 -> {
            int j = 0;
            for(int i = 0; i < 10; i++) {
                if(colorButton[i].isSelected()) {
                    colors[j] = buttonColor[i];
                    j++;
                }
            }
            mListener.onSendMode(kModeCodeTwist, j, colors);
        });
//        Button sendButton = view.findViewById(R.id.sendButton);
//        sendButton.setOnClickListener(view1 -> {
//            // Set the old color
//            mColorPicker.setOldCenterColor(mSelectedColor);
//            mListener.onSendColorComponents(mSelectedColor);
//        });

        SaturationBar mSaturationBar = view.findViewById(R.id.brightnessbar);
        ValueBar mValueBar = view.findViewById(R.id.valuebar);
        mColorPicker = view.findViewById(R.id.colorPicker);
        if (mColorPicker != null) {
            mColorPicker.addSaturationBar(mSaturationBar);
            mColorPicker.addValueBar(mValueBar);
            mColorPicker.setOnColorChangedListener(this);
        }

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

    private int lightenColor(int color) {
        int r = (color >> 16) & 0xFF;
        int g = (color >> 8) & 0xFF;
        int b = (color >> 0) & 0xFF;

        int first, second, third, newFirst, newSecond, newThird;
        int order = 6;
        // rgb = 0, rbg = 1, grb = 2, gbr = 3, brg = 4, bgr = 5
        if(r > b && r > g) {
            if(b > g) {
                order = 0;
                first = r;
                second = g;
                third = b;
            } else {
                order = 1;
                first = r;
                second = b;
                third = g;
            }
        } else if (g > r && g > b) {
            if (r > b) {
                order = 2;
                first = g;
                second = r;
                third = b;
            } else {
                order = 3;
                first = g;
                second = b;
                third = r;
            }
        } else {
            if(r > g) {
                order = 4;
                first = b;
                second = r;
                third = g;
            } else {
                order = 5;
                first = b;
                second = g;
                third = r;
            }
        }
        if(first == 0) {
            first = 1;
            second = 1;
            third = 1;
        }
        newFirst = (first + 255) / 2;
        newSecond = (newFirst * second) / first;
        newThird = (newFirst * third) / first;
        switch (order) {
            case 0:
                r = newFirst;
                g = newSecond;
                b = newThird;
                break;
            case 1:
                r = newFirst;
                b = newSecond;
                g = newThird;
                break;
            case 2:
                g = newFirst;
                r = newSecond;
                b = newThird;
                break;
            case 3:
                g = newFirst;
                b = newSecond;
                r = newThird;
                break;
            case 4:
                b = newFirst;
                r = newSecond;
                g = newThird;
                break;
            case 5:
                b = newFirst;
                g = newSecond;
                r = newThird;
                break;
            default:
                break;

        }

        int newColor = 0xFF000000 + (r * 0x10000) + (g * 0x100) + b;

        return newColor;


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
            mListener = (ControllerColorPickerFragmentListener) context;
        } else if (getTargetFragment() instanceof ControllerColorPickerFragmentListener) {
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
                        CommonHelpFragment helpFragment = CommonHelpFragment.newInstance(getString(R.string.colorpicker_help_title), getString(R.string.colorpicker_help_text));
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


    // endregion

    // region
    interface ControllerColorPickerFragmentListener {
        void onSendColorComponents(int color);
        void onSendMode(int mode);
        void onSendMode(int mode, int numColors, int[] colors);
    }

    // endregion
}
