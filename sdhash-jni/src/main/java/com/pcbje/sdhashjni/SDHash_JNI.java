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

    private native String getSDBF(String filename, byte[] content, int length);

    private native String compare(byte[] sdbfs, int threshold);

    static {
        try {
            InputStream in = SDHash_JNI.class.getClassLoader().getResourceAsStream("libsdhash.so");
                        
            File temp = File.createTempFile("tempfile", ".so");

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
        String digest1 = digest(args[0]);
        String digest2 = digest(args[1]);

        String compare = digest1 + digest2;

        SDHash_JNI jni = new SDHash_JNI();

        System.out.println(jni.compare(compare.getBytes(), 16));
    }

    private static String digest(String filename) throws Exception {
        SDHash_JNI jni = new SDHash_JNI();

        File f = new File(filename);

        InputStream reader = new FileInputStream(f);

        byte[] in = new byte[(int) f.length()];

        reader.read(in);

        reader.close();

        return jni.getSDBF(f.getName(), in, (int) f.length());
    }
}
