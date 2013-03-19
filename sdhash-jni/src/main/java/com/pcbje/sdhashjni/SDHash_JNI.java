package com.pcbje.sdhashjni;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SDHash_JNI {

    private static final String os = System.getProperty("os.name").toLowerCase();
    private static final SDHash_JNI jni = new SDHash_JNI();

    private native String getSDBF(String filename, ByteBuffer content, int length);

    private native String compare(String sdbfs, int threshold);

    static {
        try {
            String libsdhash = "libsdhash_jni.so";

            loadLib(libsdhash);

            Logger.getLogger(SDHash_JNI.class.getName()).log(Level.INFO, "Loaded {0}", libsdhash);
        } catch (IOException ex) {
            Logger.getLogger(SDHash_JNI.class.getName()).log(Level.SEVERE, null, ex);
            System.exit(1);
        }
    }

    private static void loadLib(String libsdhash) throws IOException {
        InputStream in = SDHash_JNI.class.getClassLoader().getResourceAsStream(libsdhash);

        File temp = File.createTempFile(libsdhash, "");

        OutputStream out = new FileOutputStream(temp);

        byte[] buffer = new byte[1024];

        int len;

        while ((len = in.read(buffer)) != -1) {
            out.write(buffer, 0, len);
        }

        out.close();

        System.load(temp.getAbsolutePath());
    }

    public static void main(String[] args) throws Exception {
        StringBuilder digests = new StringBuilder();

        for (int i = 0; i < args.length; i++) {
            digests.append(digest(jni, args[i]));
        }

        File tmpfile = File.createTempFile("sdhash-compare", "txt");

        FileOutputStream fos = new FileOutputStream(tmpfile);

        fos.write(digests.toString().getBytes());

        fos.close();

        System.out.println(jni.compare(digests.toString(), 16));
    }

    public static String compare(String sdbf) {
        return jni.compare(sdbf, 16);
    }

    public static String digestBytes(String filename, byte[] content) {
        if (content.length >= 512) {
            ByteBuffer buffer = ByteBuffer.allocateDirect(content.length);

            buffer.put(content);

            return jni.getSDBF(filename, buffer, content.length);
        }

        return null;
    }

    private static String digest(SDHash_JNI jni, String filename) throws Exception {
        File f = new File(filename);

        InputStream reader = new FileInputStream(f);

        byte[] in = new byte[(int) f.length()];

        reader.read(in);

        reader.close();

        ByteBuffer buffer = ByteBuffer.allocateDirect(in.length);

        buffer.put(in);

        return jni.getSDBF(f.getName(), buffer, (int) f.length());
    }
}
