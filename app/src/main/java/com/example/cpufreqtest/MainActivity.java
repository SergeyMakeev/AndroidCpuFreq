package com.example.cpufreqtest;

import androidx.appcompat.app.AppCompatActivity;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Bundle;
import android.widget.TextView;

import com.example.cpufreqtest.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'cpufreqtest' library on application startup.
    static {
        System.loadLibrary("cpufreqtest");
    }

    private ActivityMainBinding binding;
    private Timer timer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Initialize Timer
        timer = new Timer();

        // Schedule the TimerTask to run every 1000 milliseconds (1 second)
        timer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                // This runs on a background thread, use runOnUiThread if you need to update the UI
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateGui();
                    }
                });
            }
        }, 1000, 1000);
    }

    private void updateGui() {
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Cancel the timer to prevent memory leaks
        if (timer != null) {
            timer.cancel();
        }
    }

    /**
     * A native method that is implemented by the 'cpufreqtest' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}