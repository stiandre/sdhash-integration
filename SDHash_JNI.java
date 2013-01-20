

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;

public class SDHash_JNI {

    private native String getSDBF(String filename, byte[] content, int length);
    private native String compare(byte[] sdbfs, int threshold);

    static {
        System.load("libsdbf.so.0.0.0");
    }

    public static void main(String[] args)throws Exception {
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
