package de.thofoer.tetriscontrol;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

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

public class MainActivity extends AppCompatActivity {

    private ImageButton buttonLeft;
    private ImageButton buttonRight;
    private ImageButton buttonTurn;
    private Button buttonStart;
    private TextView textView;
    private ScrollView scrollView;

    private BluetoothAdapter bluetoothAdapter;
    private BluetoothService bluetoothService;
    private BluetoothDevice device;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        setContentView(R.layout.activity_main);
        buttonLeft = findViewById(R.id.buttonLeft);
        buttonRight = findViewById(R.id.buttonRight);
        buttonTurn = findViewById(R.id.buttonTurn);
        buttonStart = findViewById(R.id.buttonStart);
        textView = findViewById(R.id.textView);
        scrollView = findViewById(R.id.scrollView);

        textView.setMovementMethod(new ScrollingMovementMethod());

        buttonStart.setOnClickListener(event->start());
        buttonRight.setOnClickListener(event->right());
        buttonLeft.setOnClickListener(event->left());
        buttonTurn.setOnClickListener(event->turn());

    }

    private void left() {
        textView.append("left\n");
        bluetoothService.write(new byte[]{97, 98, 99, 13});
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void right() {
        bluetoothService.startConnection(device);

        textView.append("right\n");
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void turn() {
        textView.setText("");
        scrollView.fullScroll(View.FOCUS_DOWN);
    }

    private void start() {
        if (!bluetoothAdapter.isEnabled()) {
            log("Bluetooth ist nicht aktiviert.");
            startActivity(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE));

            IntentFilter intentFilter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
            registerReceiver(turnOnReceiver, intentFilter);
        }
        else {
            discover();
        }
    }

    private void log(String message) {
        Log.d("Main", message);
        textView.append(message);
        textView.append("\n");
        scrollView.fullScroll(View.FOCUS_DOWN);
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(discoverReceiver);
    }


    private final BroadcastReceiver discoverReceiver = new BroadcastReceiver() {
        @RequiresApi(api = Build.VERSION_CODES.R)
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                log("discovered device: "+device.getName()+":"+device.getAlias()+"-"+device.getAddress());
                if ("Tetris".equals(device.getName())) {
                    unregisterReceiver(this);
                    bluetoothAdapter.cancelDiscovery();
                    foundTetrisDevice(device);
                }
            }
        }
    };

    private void foundTetrisDevice(BluetoothDevice device) {
        log("found tetris device"+device.getAddress());
        IntentFilter intentFilter = new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED);
        registerReceiver(bondingReceiver, intentFilter);
        int state = device.getBondState();
        switch(state) {
            case BluetoothDevice.BOND_BONDED:
                log("foundTetrisDevice: BOND_BONDED. "+device);
                startBluetoothService(device);
                break;
            case BluetoothDevice.BOND_BONDING:
                log("foundTetrisDevice: BOND_BONDING. "+device);
                break;
            case BluetoothDevice.BOND_NONE:
                log("foundTetrisDevice: BOND_NONE. "+device);
                log("Creating bond ");
                boolean bondingStarted = device.createBond();
                if (!bondingStarted) {
                    log("Error creating bond.");
                    unregisterReceiver(bondingReceiver);
                }
                break;
        }
    }

    private final BroadcastReceiver turnOnReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothAdapter.ACTION_STATE_CHANGED.equals(action)) {
                int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
                switch(state) {
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
                        break;
                    case BluetoothAdapter.STATE_TURNING_ON:
                        log("BT is turning on.");
                        break;
                    default:
                        log("BT error "+state);
                }
            }
        }
    };

    private final BroadcastReceiver bondingReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            if(action.equals(BluetoothDevice.ACTION_BOND_STATE_CHANGED)){
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                switch(device.getBondState()) {
                    case BluetoothDevice.BOND_BONDED:
                        log("BroadcastReceiver: BOND_BONDED. "+device);
                        startBluetoothService(device);
                        break;
                    case BluetoothDevice.BOND_BONDING:
                        log("BroadcastReceiver: BOND_BONDING. "+device);
                        break;
                    case BluetoothDevice.BOND_NONE:
                        log("BroadcastReceiver: BOND_NONE. "+device);
                        break;
                }
            }
        }
    };

    private void startBluetoothService(BluetoothDevice device) {
        bluetoothService = new BluetoothService(this);
        bluetoothService.start( );

        this.device = device;
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
}