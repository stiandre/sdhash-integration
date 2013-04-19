package com.pcbje.sdhashjni;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.nio.ByteBuffer;
import java.util.logging.Level;
import java.util.logging.Logger;

public class SDHash_JNI {

    private static final String[] linux_x64_boostlib = new String[]{
        "libboost_thread.so.1.49.0",
        "libboost_system.so.1.49.0",
        "libboost_program_options.so.1.49.0",
        "libboost_filesystem.so.1.49.0"
    };
    private static final String[] osx_boostlib = new String[]{
        "libboost_thread.dylib",
        "libboost_system.dylib",
        "libboost_program_options.dylib",
        "libboost_filesystem.dylib"
    };
    private static final String os = System.getProperty("os.name").toLowerCase();
    private static final SDHash_JNI jni = new SDHash_JNI();

    private native String getSDBF(String filename, ByteBuffer content, int length);

    private native String compare(ByteBuffer sdbfs, int threshold);

    static {
        try {
            String libsdhash;
            String boostdir;

            String[] boostlib;

            if (os.contains("linux")) {
                libsdhash = "libsdhash_jni-linux-x64.so";
                boostdir = "boost/linux-x64";
                boostlib = linux_x64_boostlib;
            } else {
                libsdhash = "libsdhash_jni.so";
                boostdir = "boost/osx";
                boostlib = osx_boostlib;
            }

            for (String lib : boostlib) {
                loadLib(boostdir, lib);
            }

            loadLib(null, libsdhash);
        } catch (IOException ex) {
            Logger.getLogger(SDHash_JNI.class.getName()).log(Level.SEVERE, null, ex);
            System.exit(1);
        }
    }

    private static void loadLib(String root, String libname) throws IOException {
        String path = "";

        if (root != null) {
            path += root + "/";
        }

        path += libname;

        Logger.getLogger(SDHash_JNI.class.getName()).log(Level.INFO, "Loading {0}", path);

        InputStream in = SDHash_JNI.class.getClassLoader().getResourceAsStream(path);

        File temp = File.createTempFile(libname, "");
        
        temp.deleteOnExit();

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
        run(args, System.out);
    }

    public static String compare(String sdbf) {
        byte[] content = sdbf.getBytes();

        ByteBuffer buffer = ByteBuffer.allocateDirect(content.length);

        buffer.put(content);

        return jni.compare(buffer, 1);
    }

    public static String digestBytes(String filename, byte[] content) {
        if (content.length >= 512 && content.length <= 1024*1024*100) {
            ByteBuffer buffer = ByteBuffer.allocateDirect(content.length);

            buffer.put(content);

            return jni.getSDBF(filename, buffer, content.length);
        }

        return null;
    }

    public static String digestFile(String filename) throws Exception {
        File f = new File(filename);

        InputStream reader = new FileInputStream(f);

        byte[] in = new byte[(int) f.length()];

        reader.read(in);

        reader.close();

        ByteBuffer buffer = ByteBuffer.allocateDirect(in.length);

        buffer.put(in);
        
        return jni.getSDBF(f.getName(), buffer, (int) f.length());
    }
    
    static void run(String[] args, PrintStream out) throws Exception {
        StringBuilder digests = new StringBuilder();
        
        if (new File(args[0]).isDirectory()) {
            int counter = 0;
            
            int limit = args.length == 2 ? Integer.parseInt(args[1]) : 0;
            
            for (File c : new File(args[0]).listFiles()) {                
                try {
                    if (!c.isDirectory() && c.length() >= 512 && c.length() <= 1024*1024) {                     
                        digests.append(digestFile(c.getAbsolutePath()));
                        counter++;
                    }
                }
                catch (Exception e) {}
                
                if (limit > 0 && counter == limit) {
                    break;
                }
            }
        }
        else if (args.length == 1) {
            out.println(digestFile(args[0]));
            System.exit(0);
        }
        else {
            for (int i = 0; i < args.length; i++) {
                digests.append(digestFile(args[i]));
            }
        }

        out.println(compare(digests.toString()));
    }
}
