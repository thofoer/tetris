package de.thofoer.tetriscontrol;

import android.Manifest;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ScrollView;
import android.widget.TextView;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import java.util.function.Consumer;

public class MainActivity extends AppCompatActivity {

    static final char CMD_RIGHT = 'r';
    static final char CMD_LEFT = 'l';
    static final char CMD_TURN = 't';
    static final char CMD_DOWN = 'd';
    static final char CMD_START = 's';
    static final char CMD_RESET = 'x';

    static final char MSG_LEVEL = 'L';
    static final char MSG_SCORE = 'S';
    static final char MSG_NEXT_TILE = 'N';

    private ImageButton buttonLeft;
    private ImageButton buttonRight;
    private ImageButton buttonTurn;
    private ImageButton buttonDown;
    private Button buttonStart;
    private TextView textView;
    private TextView textViewScoreValue;
    private TextView textViewLevelValue;

    private ScrollView scrollView;

    private int level = 1;
    private int score = 0;


    private BluetoothAdapter bluetoothAdapter;
    private BluetoothService bluetoothService;
    private BluetoothDevice device;
    private BroadcastReceiver turnOnReceiver;
    private BroadcastReceiver bondingReceiver;
    private BroadcastReceiver discoverReceiver;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        setContentView(R.layout.activity_main);
        buttonLeft = findViewById(R.id.buttonLeft);
        buttonRight = findViewById(R.id.buttonRight);
        buttonTurn = findViewById(R.id.buttonTurn);
        buttonDown = findViewById(R.id.buttonDown);
        buttonStart = findViewById(R.id.buttonStart);
        textView = findViewById(R.id.textView);
        textViewScoreValue = findViewById(R.id.textViewScoreValue);
        textViewLevelValue = findViewById(R.id.textViewLevelValue);
        scrollView = findViewById(R.id.scrollView);
        textViewScoreValue.setText(String.valueOf(score));
        textViewLevelValue.setText(String.valueOf(level));
        textView.setMovementMethod(new ScrollingMovementMethod());

        buttonStart.setOnClickListener(view -> start());
        buttonRight.setOnClickListener(view -> right());
        buttonLeft.setOnClickListener(view -> left());
        buttonTurn.setOnClickListener(view -> turn());
        buttonDown.setOnClickListener(view -> down());
    }

    private void left() {
        bluetoothService.write(CMD_LEFT);
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void right() {
        bluetoothService.write(CMD_RIGHT);
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void turn() {
        bluetoothService.write(CMD_TURN);
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void down() {
        bluetoothService.write(CMD_DOWN);
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void start() {
        if (!bluetoothAdapter.isEnabled()) {
            log("Bluetooth ist nicht aktiviert.");
            startActivity(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE));

            IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            turnOnReceiver = createTurnOnReceiver();
            registerReceiver(turnOnReceiver, intentFilter);
        } else if (device == null) {
            discover();
        }
    }

    private void log(String message) {
        Log.d("Main", message);
        runOnUiThread(() -> {
            textView.append(message);
            textView.append("\n");
            scrollView.fullScroll(View.FOCUS_DOWN);
        });

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (turnOnReceiver != null) {
            unregisterReceiver(turnOnReceiver);
        }
        if (discoverReceiver != null) {
            unregisterReceiver(discoverReceiver);
        }
        if (bondingReceiver != null) {
            unregisterReceiver(bondingReceiver);
        }
        if (bluetoothService != null) {
            bluetoothService.destroy();
        }
    }

    private void foundTetrisDevice(BluetoothDevice device) {
        log("found tetris device" + device.getAddress());
        IntentFilter intentFilter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        bondingReceiver = createBondingReceiver();
        registerReceiver(bondingReceiver, intentFilter);
        int state = device.getBondState();
        switch (state) {
            case BluetoothDevice.BOND_BONDED:
                log("foundTetrisDevice: BOND_BONDED. " + device);
                startBluetoothService(device);
                break;
            case BluetoothDevice.BOND_BONDING:
                log("foundTetrisDevice: BOND_BONDING. " + device);
                break;
            case BluetoothDevice.BOND_NONE:
                log("foundTetrisDevice: BOND_NONE. " + device);
                log("Creating bond ");
                boolean bondingStarted = device.createBond();
                if (!bondingStarted) {
                    log("Error creating bond.");
                    bondingReceiver = null;
                    unregisterReceiver(bondingReceiver);
                }
                break;
        }
    }

    private void startBluetoothService(BluetoothDevice device) {
        bluetoothService = new BluetoothService(this, createReceiver());
        this.device = device;
        bluetoothService.startConnection(device);
    }

    private Consumer<byte[]> createReceiver() {
        return (bytes) -> {
            switch (bytes[0]) {
                case MSG_LEVEL:
                    level(bytes[1]);
                    break;
                case MSG_SCORE:
                    score(bytes[1]);
                    break;
            }
        };
    }

    private void level(byte level) {
        log("Level: " + level);
        this.level = level;
        runOnUiThread(() -> textViewLevelValue.setText(String.valueOf(level)));
    }

    private void score(byte points) {
        log("score: " + score);
        this.score += points;
        runOnUiThread(() -> textViewScoreValue.setText(String.valueOf(this.score)));

    }

    private void discover() {
        if (bluetoothAdapter.isDiscovering()) {
            log("cancelDiscovery()");
            bluetoothAdapter.cancelDiscovery();
        }
        log("discover()");
        checkBTPermissions();
        bluetoothAdapter.startDiscovery();
        IntentFilter intentFilter = new IntentFilter(BluetoothDevice.ACTION_FOUND);
        discoverReceiver = createDiscoverReceiver();
        registerReceiver(discoverReceiver, intentFilter);
    }


    private void checkBTPermissions() {
        int permissionCheck = this.checkSelfPermission("Manifest.permission.ACCESS_FINE_LOCATION");
        permissionCheck += this.checkSelfPermission("Manifest.permission.ACCESS_COARSE_LOCATION");
        if (permissionCheck != 0) {
            requestPermissions(new String[]{
                            Manifest.permission.ACCESS_FINE_LOCATION,
                            Manifest.permission.ACCESS_COARSE_LOCATION},
                    1001); //Any number
        }
    }

    private BroadcastReceiver createTurnOnReceiver() {

        return new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                    int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                    switch (state) {
                        case BluetoothAdapter.STATE_OFF:
                            log("BT is off.");
                            break;
                        case BluetoothAdapter.STATE_TURNING_OFF:
                            log("BT is turning off.");
                            break;
                        case BluetoothAdapter.STATE_ON:
                            log("BT is on.");
                            discover();
                            unregisterReceiver(this);
                            turnOnReceiver = null;
                            break;
                        case BluetoothAdapter.STATE_TURNING_ON:
                            log("BT is turning on.");
                            break;
                        default:
                            log("BT error " + state);
                    }
                }
            }
        };
    }

    private BroadcastReceiver createBondingReceiver() {
        return new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final String action = intent.getAction();

                if (action.equals(BluetoothDevice.ACTION_BOND_STATE_CHANGED)) {
                    BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    switch (device.getBondState()) {
                        case BluetoothDevice.BOND_BONDED:
                            log("BroadcastReceiver: BOND_BONDED. " + device);
                            unregisterReceiver(this);
                            bondingReceiver = null;
                            startBluetoothService(device);
                            break;
                        case BluetoothDevice.BOND_BONDING:
                            log("BroadcastReceiver: BOND_BONDING. " + device);
                            break;
                        case BluetoothDevice.BOND_NONE:
                            log("BroadcastReceiver: BOND_NONE. " + device);
                            break;
                    }
                }
            }
        };
    }


    private BroadcastReceiver createDiscoverReceiver() {
        return new BroadcastReceiver() {
            @RequiresApi(api = Build.VERSION_CODES.R)
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();

                if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                    BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                    log("discovered device: " + device.getName() + ":" + device.getAlias() + "-" + device.getAddress());
                    if ("Tetris".equals(device.getName())) {
                        unregisterReceiver(this);
                        discoverReceiver = null;
                        bluetoothAdapter.cancelDiscovery();
                        foundTetrisDevice(device);
                    }
                }
            }
        };
    }

}