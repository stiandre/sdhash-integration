package sdhashjni;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.security.NoSuchAlgorithmException;

public class SDHash_JNI {

    private native String getSDBF(String filename, byte[] content, int length);

    static {
        System.load("libsdbf.so.0.0.0");
    }

    public static void main(String[] args) throws FileNotFoundException, IOException, NoSuchAlgorithmException {
        SDHash_JNI jni = new SDHash_JNI();
        
        File f = new File(args[0]);
        
        InputStream reader = new FileInputStream(f);
        
        byte[] in = new byte[(int) f.length()];
        
        reader.read(in);
        
        reader.close();
        
        String digest = jni.getSDBF(f.getName(), in, (int) f.length());
        
        System.out.println(digest);
    }
}
