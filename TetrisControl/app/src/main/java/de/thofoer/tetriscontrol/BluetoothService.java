package de.thofoer.tetriscontrol;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;
import java.util.function.Consumer;

public class BluetoothService {

    // Debugging
    private static final String TAG = "BluetoothService";

    // Name for the SDP record when creating server socket
    private static final String NAME_INSECURE = "TetrisControlBluetooth";

    // Unique UUID for this application
    private static final UUID MY_UUID_INSECURE = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private final BluetoothAdapter adapter;
    private final Context context;
    private BluetoothDevice device;
    private UUID deviceUuid;
    private ProgressDialog progressDialog;

    private ConnectThread connectThread;
    private ConnectedThread connectedThread;

    private Consumer<byte[]>  receiver;
    private Consumer<Exception> exceptionHandler;

    public BluetoothService(Context context, Consumer<byte[]> receiver, Consumer<Exception> exceptionHandler) {
        this.context = context;
        this.receiver = receiver;
        this.exceptionHandler = exceptionHandler;
        adapter = BluetoothAdapter.getDefaultAdapter();
    }

    public void startConnection(BluetoothDevice device) {
        Log.d(TAG, "startClient: Started.");

        //initprogress dialog
        progressDialog = ProgressDialog.show(context, "Connecting Bluetooth"
                , "Please Wait...", true);

        connectThread = new ConnectThread(device, MY_UUID_INSECURE);
        connectThread.start();
    }

    private void connected(BluetoothSocket socket, BluetoothDevice device) {
        Log.d(TAG, "connected: Starting.");
        connectedThread = new ConnectedThread(socket);
        connectedThread.start();
    }

    public void write(byte[] out) {
        Log.d(TAG, "write: Write Called.");
        connectedThread.write(out);
    }

    public void write(char out) {
        write(new byte[]{(byte) out});
    }

    public void destroy() {
        if (connectedThread != null) {
            connectedThread.cancel();
            connectedThread = null;
        }
        if (connectThread != null) {
            connectThread.cancel();
            connectThread = null;
        }
    }

    private class ConnectThread extends Thread {
        private BluetoothSocket socket;

        ConnectThread(BluetoothDevice device, UUID uuid) {
            Log.d(TAG, "ConnectThread started");
            BluetoothService.this.device = device;
            BluetoothService.this.deviceUuid = uuid;
        }

        public void run() {
            BluetoothSocket newSocket = null;
            Log.d(TAG, "ConnectThread.run()");
            try {
                newSocket = device.createInsecureRfcommSocketToServiceRecord(deviceUuid);
            } catch (IOException ioex) {
                Log.e(TAG, "Cannot create socket", ioex);
            }
            socket = newSocket;
            Log.d(TAG, "socket created " + socket);
            BluetoothService.this.adapter.cancelDiscovery();

            try {
                socket.connect();

                Log.d(TAG, "run: ConnectThread connected.");
            } catch (IOException e) {
                try {
                    Log.e(TAG, "run: ConnectThread exception.", e);
                    socket.close();
                    Log.d(TAG, "run: Closed Socket.");
                } catch (IOException e1) {
                    Log.e(TAG, "ConnectThread: run: Unable to close connection in socket", e1);
                }
                Log.d(TAG, "run: ConnectThread: Could not connect to UUID: " + MY_UUID_INSECURE);
            }

            connected(socket, device);
        }

        public void cancel() {
            try {
                Log.d(TAG, "cancel: Closing Client Socket.");
                socket.close();
            } catch (IOException e) {
                Log.e(TAG, "cancel: close() of socket in Connectthread failed. ", e);
            }
        }
    }

    /**
     * Finally the ConnectedThread which is responsible for maintaining the BTConnection, Sending the data, and
     * receiving incoming data through input/output streams respectively.
     **/
    private class ConnectedThread extends Thread {
        private final BluetoothSocket socket;
        private final InputStream inStream;
        private final OutputStream outStream;

        public ConnectedThread(BluetoothSocket socket) {
            Log.d(TAG, "ConnectedThread: Starting.");

            this.socket = socket;
            InputStream tmpIn = null;
            OutputStream tmpOut = null;

            //dismiss the progressdialog when connection is established
            try {
                progressDialog.dismiss();
            } catch (NullPointerException e) {
                e.printStackTrace();
            }

            try {
                tmpIn = socket.getInputStream();
                tmpOut = socket.getOutputStream();
            }
            catch (IOException e) {
                Log.d(TAG, "creating sockets: ", e);
                exceptionHandler.accept(e);
            }

            inStream = tmpIn;
            outStream = tmpOut;
        }

        public void run() {
            byte[] buffer = new byte[1024];  // buffer store for the stream

            int bytes; // bytes returned from read()

            // Keep listening to the InputStream until an exception occurs
            while (true) {
                // Read from the InputStream
                try {
                    bytes = inStream.read(buffer);
                    receiver.accept(buffer);
                    String incomingMessage = new String(buffer, 0, bytes);
                    Log.d(TAG, "InputStream: " + incomingMessage+ " - "+bytes+" bytes");
                }
                catch (IOException e) {
                    Log.e(TAG, "write: Error reading Input Stream. ", e);
                    cancel();
                    exceptionHandler.accept(e);
                    break;
                }
            }
        }

        public void write(byte[] bytes) {
            try {
                outStream.write(bytes);
            } catch (IOException e) {
                Log.e(TAG, "write: Error writing to output stream. ", e);
                cancel();
                exceptionHandler.accept(e);
            }
        }

        public void cancel() {
            try {
                Log.d(TAG, "Cancel ConnectedThread");
                socket.close();
            } catch (IOException e) {
            }
        }
    }
}
