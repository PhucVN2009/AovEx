package com.star.android.utils;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.io.OutputStream;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class SocketClient {
    private static final String SOCKET_NAME = "StarcoolPRO_socket";
    private static final ExecutorService executor = Executors.newSingleThreadExecutor();

    private static void send(final String message) {
        executor.execute(() -> {
            try (LocalSocket socket = new LocalSocket()) {
                socket.connect(new LocalSocketAddress(SOCKET_NAME, LocalSocketAddress.Namespace.ABSTRACT));
                OutputStream out = socket.getOutputStream();
                out.write(message.getBytes());
                out.flush();
            } catch (Exception ignored) {
            }
        });
    }

    public static void setMapHack(boolean active) {
        send("MAP_HACK:" + (active ? "1" : "0"));
    }

    public static void setCamXa(float value) {
        send("CAMXA_VAL:" + value);
    }

    public static void setEspLine(boolean active) {
        send("ESP_LINE:" + (active ? "1" : "0"));
    }
}
