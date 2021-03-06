package de.thofoer.tetriscontrol;

import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.net.LocalSocket;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.util.UUID;

public class BluetoothService {

    // Debugging
    private static final String TAG = "BluetoothService";

    // Name for the SDP record when creating server socket
    private static final String NAME_INSECURE = "TetrisControlBluetooth";

    // Unique UUID for this application
    private static final UUID MY_UUID_INSECURE = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    private final BluetoothAdapter adapter;
    private BluetoothDevice device;
    private UUID deviceUuid;
    private final Context context;

    private ProgressDialog progressDialog;

    private AcceptThread acceptThread;
    private ConnectThread connectThread;
    private ConnectedThread connectedThread;


    public BluetoothService(Context context) {
        this.context = context;
        adapter = BluetoothAdapter.getDefaultAdapter();
    }


    private class AcceptThread extends Thread {
        private final BluetoothServerSocket serverSocket;

        private AcceptThread() {
            BluetoothServerSocket newSocket = null;
            try {
                newSocket = adapter.listenUsingInsecureRfcommWithServiceRecord(NAME_INSECURE, MY_UUID_INSECURE);
                Log.d(TAG, "AcceptThread: Setting up server using: " + MY_UUID_INSECURE);
            } catch (IOException ioex) {
                Log.e(TAG, "Cannot set up server", ioex);
            }
            serverSocket = newSocket;
        }

        public void run() {
            Log.d(TAG, "AcceptThread.run()");
            try {
                BluetoothSocket socket = serverSocket.accept();
                Log.d(TAG, "Connection accepted");
                if (socket != null) {
                    connected(socket, device);
                }
            } catch (IOException ioex) {
                Log.e(TAG, "Cannot accept connection", ioex);
            }
            Log.i(TAG, "END AcceptThread.run()");
        }

        public void cancel() {
            try {
                Log.i(TAG, "AcceptThread.cancel()");
                serverSocket.close();
            } catch (IOException ioex) {
                Log.e(TAG, "Cannot close serverSocket", ioex);
            }

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
                newSocket = device.createInsecureRfcommSocketToServiceRecord (deviceUuid);
            } catch (IOException ioex) {
                Log.e(TAG, "Cannot create socket", ioex);
            }
            socket = newSocket;
            Log.d(TAG, "socket created "+socket);
            BluetoothService.this.adapter.cancelDiscovery();

            try {
                socket.connect();

                Log.d(TAG, "run: ConnectThread connected.");
            }
            catch (IOException e) {
                try {
                    Log.e(TAG, "run: ConnectThread exception.", e);
                    socket.close();
                    Log.d(TAG, "run: Closed Socket.");
                } catch (IOException e1) {
                    Log.e(TAG, "ConnectThread: run: Unable to close connection in socket", e1);
                }
                Log.d(TAG, "run: ConnectThread: Could not connect to UUID: " + MY_UUID_INSECURE );
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

    public synchronized void start() {
        Log.d(TAG, "start");

        // Cancel any thread attempting to make a connection
        if (connectThread != null) {
            connectThread.cancel();
            connectThread = null;
        }

        if (acceptThread == null) {
            acceptThread = new AcceptThread();
            acceptThread.start();
        }

    }

    public void startConnection(BluetoothDevice device){
        Log.d(TAG, "startClient: Started.");

        //initprogress dialog
        progressDialog = ProgressDialog.show(context,"Connecting Bluetooth"
                ,"Please Wait...",true);

        connectThread = new ConnectThread(device, MY_UUID_INSECURE);
        connectThread.start();
    }

    /**
     Finally the ConnectedThread which is responsible for maintaining the BTConnection, Sending the data, and
     receiving incoming data through input/output streams respectively.
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
            try{
                progressDialog.dismiss();
            }catch (NullPointerException e){
                e.printStackTrace();
            }


            try {
                Log.d(TAG, "get instream ");
                tmpIn = socket.getInputStream();
                Log.d(TAG, "get outstream ");
                tmpOut = socket.getOutputStream();
                Log.d(TAG, "outstream "+tmpOut);
                Log.d(TAG, "instream "+tmpIn.available());
            } catch (IOException e) {
                Log.d(TAG, "creating sockets: ", e);
            }

            inStream = tmpIn;
            outStream = tmpOut;
        }

        public void run(){
            byte[] buffer = new byte[1024];  // buffer store for the stream

            int bytes; // bytes returned from read()

            // Keep listening to the InputStream until an exception occurs
            while (true) {
                // Read from the InputStream
                try {
                    bytes = inStream.read(buffer);
                    String incomingMessage = new String(buffer, 0, bytes);
                    Log.d(TAG, "InputStream: " + incomingMessage);
                } catch (IOException e) {
                    Log.e(TAG, "write: Error reading Input Stream. ", e) ;
                    break;
                }
            }
        }

        //Call this from the main activity to send data to the remote device
        public void write(byte[] bytes) {
            String text = new String(bytes, Charset.defaultCharset());
            Log.d(TAG, "write: Writing to outputstream: " + text);
            try {
                outStream.write(bytes);
            } catch (IOException e) {
                Log.e(TAG, "write: Error writing to output stream. " , e );
            }
        }

        /* Call this from the main activity to shutdown the connection */
        public void cancel() {
            try {
                socket.close();
            } catch (IOException e) { }
        }
    }

    private void connected(BluetoothSocket socket, BluetoothDevice device) {
        Log.d(TAG, "connected: Starting.");

        // Start the thread to manage the connection and perform transmissions
        connectedThread = new ConnectedThread(socket);
        connectedThread.start();
    }

    /**
     * Write to the ConnectedThread in an unsynchronized manner
     *
     * @param out The bytes to write
     * @see ConnectedThread#write(byte[])
     */
    public void write(byte[] out) {
        Log.d(TAG, "write: Write Called.");
        //perform the write
        connectedThread.write(out);
    }
}
