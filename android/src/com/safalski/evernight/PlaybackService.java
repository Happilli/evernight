package com.safalski.evernight;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ServiceInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.Icon;
import android.media.AudioAttributes;
import android.media.AudioFocusRequest;
import android.media.AudioManager;
import android.media.MediaMetadata;
import android.media.session.MediaSession;
import android.media.session.PlaybackState;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.util.Log;

public class PlaybackService extends Service {
    private static final String TAG = "Evernight";
    private static final int NOTIFICATION_ID = 1;
    private static final String CHANNEL_ID = "play";
    private static final String ACTION_PLAY = "com.safalski.evernight.PLAY";
    private static final String ACTION_PAUSE = "com.safalski.evernight.PAUSE";
    private static final String ACTION_NEXT = "com.safalski.evernight.NEXT";
    private static final String ACTION_PREV = "com.safalski.evernight.PREV";
    private static final int CMD_PLAY = 0;
    private static final int CMD_PAUSE = 1;
    private static final int CMD_NEXT = 2;
    private static final int CMD_PREV = 3;
    private static final int CMD_SEEK = 4;
    private static final long SESSION_ACTIONS = PlaybackState.ACTION_PLAY
            | PlaybackState.ACTION_PAUSE
            | PlaybackState.ACTION_PLAY_PAUSE
            | PlaybackState.ACTION_SEEK_TO
            | PlaybackState.ACTION_SKIP_TO_NEXT
            | PlaybackState.ACTION_SKIP_TO_PREVIOUS;

    private static final Handler MAIN = new Handler(Looper.getMainLooper());
    private static PlaybackService live;
    private static String title = "Evernight";
    private static String artist = "";
    private static String thumb = "";
    private static long duration;
    private static long position;
    private static boolean playing;
    private static boolean quitting;

    private static native void nativeCommand(int cmd, long arg);

    private static void send(int cmd, long arg) {
        try {
            nativeCommand(cmd, arg);
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    private static Runnable deferred;

    public static void update(Context context, String t, String a, String th,
                              long d, long p, boolean pl) {
        final Context app = context.getApplicationContext();
        final String newTitle = t == null ? "" : t;
        final String newArtist = a == null ? "" : a;
        final String newThumb = th == null ? "" : th;
        final Runnable apply = () -> {
            deferred = null;
            title = newTitle;
            artist = newArtist;
            thumb = newThumb;
            duration = d;
            position = p;
            playing = pl;
            if (live != null) {
                live.render();
            } else if (pl) {
                try {
                    app.startForegroundService(new Intent(app, PlaybackService.class));
                } catch (RuntimeException e) {
                    Log.w(TAG, "startForegroundService failed", e);
                }
            }
        };
        MAIN.post(() -> {
            if (deferred != null) {
                MAIN.removeCallbacks(deferred);
                deferred = null;
            }
            if (!pl && playing) {
                deferred = apply;
                MAIN.postDelayed(apply, 1500);
            } else {
                apply.run();
            }
        });
    }

    public static void stop(Context context) {
        final Context app = context.getApplicationContext();
        MAIN.post(() -> app.stopService(new Intent(app, PlaybackService.class)));
    }

    public static void shutdown(Context context) {
        if (live == null) {
            android.os.Process.killProcess(android.os.Process.myPid());
            return;
        }
        quitting = true;
        final Context app = context.getApplicationContext();
        app.stopService(new Intent(app, PlaybackService.class));
        new Thread(() -> {
            try {
                Thread.sleep(1500);
            } catch (InterruptedException ignored) {
            }
            android.os.Process.killProcess(android.os.Process.myPid());
        }).start();
    }

    private final BroadcastReceiver noisyReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (playing && AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(intent.getAction()))
                send(CMD_PAUSE, 0);
        }
    };

    private boolean hasFocus;
    private boolean resumeOnGain;
    private final AudioManager.OnAudioFocusChangeListener focusListener = change -> {
        switch (change) {
            case AudioManager.AUDIOFOCUS_GAIN:
                hasFocus = true;
                if (resumeOnGain) {
                    resumeOnGain = false;
                    send(CMD_PLAY, 0);
                }
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                hasFocus = false;
                resumeOnGain = playing;
                send(CMD_PAUSE, 0);
                break;
            case AudioManager.AUDIOFOCUS_LOSS:
                hasFocus = false;
                resumeOnGain = false;
                send(CMD_PAUSE, 0);
                break;
            default:
                break;
        }
    };

    private NotificationManager nm;
    private MediaSession session;
    private PendingIntent openIntent;
    private Notification.Action prevAction;
    private Notification.Action playAction;
    private Notification.Action pauseAction;
    private Notification.Action nextAction;
    private Bitmap art;
    private String artPath = "";
    private String shownKey = "";
    private long shownDuration = Long.MIN_VALUE;
    private boolean shownPlaying;
    private boolean foreground;
    private boolean promoted;
    private PowerManager.WakeLock wakeLock;
    private AudioManager audioManager;
    private AudioFocusRequest focusRequest;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        live = this;
        quitting = false;
        nm = getSystemService(NotificationManager.class);
        audioManager = getSystemService(AudioManager.class);

        NotificationChannel channel = new NotificationChannel(
                CHANNEL_ID, "Playback", NotificationManager.IMPORTANCE_LOW);
        channel.setShowBadge(false);
        nm.createNotificationChannel(channel);

        Intent launch = getPackageManager().getLaunchIntentForPackage(getPackageName());
        if (launch != null)
            openIntent = PendingIntent.getActivity(this, 0, launch,
                    PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);

        prevAction = buildAction(android.R.drawable.ic_media_previous, "Previous", ACTION_PREV, 10);
        playAction = buildAction(android.R.drawable.ic_media_play, "Play", ACTION_PLAY, 11);
        pauseAction = buildAction(android.R.drawable.ic_media_pause, "Pause", ACTION_PAUSE, 12);
        nextAction = buildAction(android.R.drawable.ic_media_next, "Next", ACTION_NEXT, 13);

        session = new MediaSession(this, "Evernight");
        session.setCallback(new MediaSession.Callback() {
            @Override public void onPlay() { send(CMD_PLAY, 0); }
            @Override public void onPause() { send(CMD_PAUSE, 0); }
            @Override public void onSkipToNext() { send(CMD_NEXT, 0); }
            @Override public void onSkipToPrevious() { send(CMD_PREV, 0); }
            @Override public void onSeekTo(long pos) { send(CMD_SEEK, pos); }
        }, MAIN);
        if (openIntent != null)
            session.setSessionActivity(openIntent);
        session.setActive(true);

        IntentFilter filter = new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
        if (Build.VERSION.SDK_INT >= 33)
            registerReceiver(noisyReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        else
            registerReceiver(noisyReceiver, filter);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String name = intent == null ? null : intent.getAction();
        if (name == null) {
            render();
            return START_NOT_STICKY;
        }
        switch (name) {
            case ACTION_PLAY: send(CMD_PLAY, 0); break;
            case ACTION_PAUSE: send(CMD_PAUSE, 0); break;
            case ACTION_NEXT: send(CMD_NEXT, 0); break;
            case ACTION_PREV: send(CMD_PREV, 0); break;
            default: break;
        }
        if (!promoted)
            stopSelf(startId);
        return START_NOT_STICKY;
    }

    @Override
    public void onTaskRemoved(Intent rootIntent) {
        shutdown(this);
    }

    @Override
    public void onDestroy() {
        live = null;
        try {
            unregisterReceiver(noisyReceiver);
        } catch (IllegalArgumentException ignored) {
        }
        if (wakeLock != null && wakeLock.isHeld())
            wakeLock.release();
        if (focusRequest != null && audioManager != null)
            audioManager.abandonAudioFocusRequest(focusRequest);
        hasFocus = false;
        resumeOnGain = false;
        stopForeground(STOP_FOREGROUND_REMOVE);
        nm.cancel(NOTIFICATION_ID);
        session.setActive(false);
        session.release();
        super.onDestroy();
        if (quitting)
            MAIN.postDelayed(() -> android.os.Process.killProcess(android.os.Process.myPid()), 200);
    }

    private Notification.Action buildAction(int icon, String label, String name, int code) {
        PendingIntent pi = PendingIntent.getService(this, code,
                new Intent(this, PlaybackService.class).setAction(name),
                PendingIntent.FLAG_IMMUTABLE | PendingIntent.FLAG_UPDATE_CURRENT);
        return new Notification.Action.Builder(Icon.createWithResource(this, icon), label, pi).build();
    }

    private static Bitmap decode(String path) {
        if (path.isEmpty())
            return null;
        try {
            return BitmapFactory.decodeFile(path);
        } catch (RuntimeException | OutOfMemoryError e) {
            return null;
        }
    }

    private void render() {
        if (!thumb.equals(artPath)) {
            artPath = thumb;
            art = decode(thumb);
        }
        final String key = title + '\u0000' + artist + '\u0000' + artPath;
        final boolean metaChanged = !key.equals(shownKey) || duration != shownDuration;
        if (metaChanged) {
            MediaMetadata.Builder md = new MediaMetadata.Builder()
                    .putString(MediaMetadata.METADATA_KEY_TITLE, title)
                    .putString(MediaMetadata.METADATA_KEY_ARTIST, artist)
                    .putLong(MediaMetadata.METADATA_KEY_DURATION, duration > 0 ? duration : -1L);
            if (art != null)
                md.putBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART, art);
            session.setMetadata(md.build());
            shownKey = key;
            shownDuration = duration;
        }
        session.setPlaybackState(new PlaybackState.Builder()
                .setActions(SESSION_ACTIONS)
                .setState(playing ? PlaybackState.STATE_PLAYING : PlaybackState.STATE_PAUSED,
                        Math.max(0L, position), playing ? 1f : 0f)
                .build());
        if (metaChanged || playing != shownPlaying || !promoted)
            publish();
    }

    private Notification buildNotification() {
        Notification.Builder nb = new Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.ic_media_play)
                .setContentTitle(title)
                .setContentText(artist)
                .setContentIntent(openIntent)
                .setOnlyAlertOnce(true)
                .setShowWhen(false)
                .setOngoing(playing)
                .setVisibility(Notification.VISIBILITY_PUBLIC)
                .addAction(prevAction)
                .addAction(playing ? pauseAction : playAction)
                .addAction(nextAction)
                .setStyle(new Notification.MediaStyle()
                        .setMediaSession(session.getSessionToken())
                        .setShowActionsInCompactView(0, 1, 2));
        if (art != null)
            nb.setLargeIcon(art);
        return nb.build();
    }

    private void publish() {
        final Notification n = buildNotification();
        if (playing) {
            if (foreground)
                nm.notify(NOTIFICATION_ID, n);
            else
                promote(n);
        } else if (foreground) {
            nm.notify(NOTIFICATION_ID, n);
            demote();
        } else if (!promoted) {
            promote(n);
            demote();
        } else {
            nm.notify(NOTIFICATION_ID, n);
        }
        shownPlaying = playing;
        syncWakeLock();
        syncFocus();
    }

    private void promote(Notification n) {
        try {
            startForeground(NOTIFICATION_ID, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK);
            foreground = true;
            promoted = true;
        } catch (RuntimeException e) {
            Log.w(TAG, "startForeground failed", e);
            nm.notify(NOTIFICATION_ID, n);
        }
    }

    private void demote() {
        if (!foreground)
            return;
        stopForeground(STOP_FOREGROUND_DETACH);
        foreground = false;
    }

    private void syncWakeLock() {
        if (playing) {
            if (wakeLock == null) {
                PowerManager pm = getSystemService(PowerManager.class);
                wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "evernight:playback");
                wakeLock.setReferenceCounted(false);
            }
            if (!wakeLock.isHeld())
                wakeLock.acquire();
        } else if (wakeLock != null && wakeLock.isHeld()) {
            wakeLock.release();
        }
    }

    private void syncFocus() {
        if (playing && !hasFocus) {
            if (focusRequest == null) {
                focusRequest = new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                        .setAudioAttributes(new AudioAttributes.Builder()
                                .setUsage(AudioAttributes.USAGE_MEDIA)
                                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                                .build())
                        .setOnAudioFocusChangeListener(focusListener, MAIN)
                        .build();
            }
            hasFocus = audioManager.requestAudioFocus(focusRequest)
                    == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
            Log.i(TAG, "audio focus granted=" + hasFocus);
        } else if (!playing && hasFocus && focusRequest != null) {
            audioManager.abandonAudioFocusRequest(focusRequest);
            hasFocus = false;
        }
    }
}
