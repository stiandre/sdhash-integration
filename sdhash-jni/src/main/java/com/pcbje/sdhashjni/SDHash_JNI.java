package com.pcbje.sdhashjni;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SDHash_JNI {

    private native String getSDBF(String filename, byte[] content, int length);

    private native String compare(byte[] sdbfs, int threshold);

    static {
        try {
            String sdhash = "libsdhash.so";

            InputStream in = SDHash_JNI.class.getClassLoader().getResourceAsStream(sdhash);

            File temp = File.createTempFile(sdhash, "");

            OutputStream out = new FileOutputStream(temp);

            byte[] buffer = new byte[1024];

            int len;

            while ((len = in.read(buffer)) != -1) {
                out.write(buffer, 0, len);
            }

            out.close();

            System.load(temp.getAbsolutePath());

        } catch (IOException ex) {
            Logger.getLogger(SDHash_JNI.class.getName()).log(Level.SEVERE, null, ex);
            System.exit(1);
        }
    }

    public static void main(String[] args) throws Exception {        
        SDHash_JNI jni = new SDHash_JNI();
        
        StringBuilder digests = new StringBuilder();
        
        for (String filename : args) {
            digests.append(digest(jni, filename));
        }
        
        System.out.println(jni.compare(digests.toString().getBytes(), 16));
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