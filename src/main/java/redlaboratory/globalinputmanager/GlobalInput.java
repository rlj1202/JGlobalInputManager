package redlaboratory.globalinputmanager;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class GlobalInput {
	
	private static boolean libraryLoaded = false;
	
	static {
		libraryInitialize();
	}
	
	public static native void typeString(String str);	
	
	private static void libraryInitialize() {
		if (libraryLoaded) return;
		
		libraryLoaded = true;
		
		if (System.getProperty("os.name").toLowerCase().contains("windows")) {
			if (System.getProperty("os.arch").contains("64")) {
				loadLibrary("/redlaboratory/globalinputmanager/lib/windows/libGlobalInput64.dll");
			} else {
				loadLibrary("/redlaboratory/globalinputmanager/lib/windows/libGlobalInput32.dll");
			}
		}
	}
	
	private static String loadLibrary(String name) {
		try {
			String fileName = name.substring(name.lastIndexOf('/') + 1);
			String tmpFolderPath = System.getProperty("java.io.tmpdir");
			if (tmpFolderPath.charAt(tmpFolderPath.length() - 1) == File.separatorChar) tmpFolderPath = tmpFolderPath.substring(0, tmpFolderPath.lastIndexOf(File.separatorChar));
			String tmpFilePath = tmpFolderPath + File.separatorChar + fileName;
			
			File tmpFile = new File(tmpFilePath);
			
			InputStream is = GlobalInput.class.getResourceAsStream(name);
			FileOutputStream fos = new FileOutputStream(tmpFile);
			
			byte buffer[] = new byte[4096];
			int n;
			while ((n = is.read(buffer, 0, buffer.length)) > 0) {
				fos.write(buffer, 0, n);
			}
			
			fos.close();
			is.close();
			
			System.load(tmpFilePath);
			
			return tmpFilePath;
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		return null;
	}
	
}
