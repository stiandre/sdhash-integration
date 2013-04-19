package com.pcbje.sdhashjni;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.PrintStream;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 * @author pcbje
 */
public class TestSDHash_JNI {

    private static final String LOREM_IPSUM_SIGNATURE = "sdbf:03:10:loremipsum:1673:sha1:256:5:7ff:160:1:31:BAAAgAAAAAiAAABQCYCgEAAEBAAAgABACAAMQBAIAAAAAAEACGEIAgCAEAQAAAAAgEAAAAAAAAAAAAAAGACAaQEAGgCQBAAACACQAAACAAAAAAAAAAACAAACAAAAAAAAAIAgAQUAEACCAAAAAYCAAgAAAhARQAAQAAQAEAQABCGAAACkIAAAAAgihAgSCAgISAAAFAEMAAAAAAAAAAAABAAAAAAEACAAAAAAABAEIAAAgAAACAEAAAAAAAABhCAAAECAABABIAAAEAAAIlgAAAAAAIAAAARCkAABAAAAAAAAAIQABAAEAQAAAoAAiAAAAAAgQQUECAAAAAAIAIAhAA==\n";
    private static final String FILENAME = "loremipsum";
    private static final String FILENAME_COMPARE = "ipsumlorem";

    @Test
    public void testDigestBytes() throws Exception {
        File file = new File(getClass().getClassLoader().getResource("folder/" + FILENAME).getPath());

        FileInputStream reader = new FileInputStream(file);

        byte[] content = new byte[(int) file.length()];

        reader.read(content);

        String signature = SDHash_JNI.digestBytes(FILENAME, content);

        assertEquals(LOREM_IPSUM_SIGNATURE, signature);
    }

    @Test
    public void testDigestFile() throws Exception {
        String signature = SDHash_JNI.digestFile(getClass().getClassLoader().getResource("folder/" + FILENAME).getPath());

        assertEquals(LOREM_IPSUM_SIGNATURE, signature);
    }

    @Test
    public void testCompare() throws Exception {
        StringBuilder sdbf = new StringBuilder();

        sdbf.append(SDHash_JNI.digestFile(getClass().getClassLoader().getResource("folder/" + FILENAME).getPath()));
        sdbf.append(SDHash_JNI.digestFile(getClass().getClassLoader().getResource("folder/" + FILENAME_COMPARE).getPath()));

        String result = SDHash_JNI.compare(sdbf.toString());

        assertNotNull(result);

        assertEquals(FILENAME + "|" + FILENAME_COMPARE + "|087\n", result);
    }

    @Test
    public void testRunOnFile() throws Exception {
        String[] input = new String[]{getClass().getClassLoader().getResource("folder/" + FILENAME).getPath()};

        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        PrintStream stream = new PrintStream(baos);

        SDHash_JNI.run(input, stream);

        assertEquals(LOREM_IPSUM_SIGNATURE, baos.toString().trim());
    }

    @Test
    public void testRunOnFiles() throws Exception {
        String[] input = new String[]{getClass().getClassLoader().getResource("folder/" + FILENAME).getPath(),
            getClass().getClassLoader().getResource("folder/" + FILENAME_COMPARE).getPath()};

        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        PrintStream stream = new PrintStream(baos);

        SDHash_JNI.run(input, stream);

        assertEquals(FILENAME + "|" + FILENAME_COMPARE + "|087", baos.toString().trim());
    }

    @Test
    public void testRunOnFolder() throws Exception {
        String[] input = new String[]{getClass().getClassLoader().getResource("folder/").getPath()};

        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        PrintStream stream = new PrintStream(baos);

        SDHash_JNI.run(input, stream);

        assertEquals(FILENAME_COMPARE + "|" + FILENAME + "|087", baos.toString().trim());
    }
}
