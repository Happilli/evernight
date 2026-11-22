package com.safalski.evernight;

import org.qtproject.qt.android.bindings.QtActivity;

public class EvernightActivity extends QtActivity {
    @Override
    protected void onDestroy() {
        if (isFinishing())
            PlaybackService.shutdown(this);
        super.onDestroy();
    }
}
