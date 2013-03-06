package com.pcbje.sdhashjni;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SDHash_JNI {

    private static final String os = System.getProperty("os.name").toLowerCase();
    private static final SDHash_JNI jni = new SDHash_JNI();

    private native String getSDBF(String filename, byte[] content, int length);

    private native String compare(String sdbfs, int threshold);

    static {
        try {
            if (os.indexOf("linux") < 0) {
                throw new Error("OS " + os + " is not supported");
            }

            String libsdhash = "libsdhash_jni.so";            

            loadLib(libsdhash);
            

        } catch (IOException ex) {
            Logger.getLogger(SDHash_JNI.class.getName()).log(Level.SEVERE, null, ex);
            System.exit(1);
        }
    }

    private static void loadLib(String libsdhash) throws IOException {
        Logger.getLogger(SDHash_JNI.class.getName()).log(Level.INFO, "Loading {0}...", libsdhash);

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
        return jni.getSDBF(filename, content, content.length);
    }

    private static String digest(SDHash_JNI jni, String filename) throws Exception {
        File f = new File(filename);

        InputStream reader = new FileInputStream(f);

        byte[] in = new byte[(int) f.length()];

        reader.read(in);

        reader.close();

        return jni.getSDBF(f.getName(), in, (int) f.length());
    }
}
