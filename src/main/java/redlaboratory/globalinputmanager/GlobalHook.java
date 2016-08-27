package redlaboratory.globalinputmanager;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import redlaboratory.globalinputmanager.hookEvent.GlobalEventAdapter;
import redlaboratory.globalinputmanager.hookEvent.GlobalKeyboardEvent;
import redlaboratory.globalinputmanager.hookEvent.GlobalKeyboardEventHandler;
import redlaboratory.globalinputmanager.hookEvent.GlobalMouseEvent;
import redlaboratory.globalinputmanager.hookEvent.GlobalMouseEventHandler;

public class GlobalHook {
	
	private static List<GlobalKeyboardEventHandler> keyboardHandlers;
	private static List<GlobalMouseEventHandler> mouseHandlers;
	
	private static boolean libraryLoaded = false;
	
	private static void libraryInitialize() {
		if (libraryLoaded) return;
		
		libraryLoaded = true;
		
		if (System.getProperty("os.name").toLowerCase().contains("windows")) {
			if (System.getProperty("os.arch").contains("64")) {
				loadLibrary("/redlaboratory/globalinputmanager/lib/windows/libGlobalHook64.dll");
			} else {
				loadLibrary("/redlaboratory/globalinputmanager/lib/windows/libGlobalHook32.dll");
			}
		}
	}
	
	private static native void enable();
	
	private static native void disable();
	
	public static void create() {
		keyboardHandlers = new ArrayList<GlobalKeyboardEventHandler>();
		mouseHandlers = new ArrayList<GlobalMouseEventHandler>();
		
		libraryInitialize();
		
		enable();
	}
	
	public static void destroy() {
		disable();
	}
	
	public static void registerEventAdapter(GlobalEventAdapter adapter) {
		keyboardHandlers.add(adapter);
		mouseHandlers.add(adapter);
	}
	
	public static void registerKeyboardEventHandler(GlobalKeyboardEventHandler handler) {
		keyboardHandlers.add(handler);
	}
	
	public static void registerMouseEventHandler(GlobalMouseEventHandler handler) {
		mouseHandlers.add(handler);
	}
	
	public static void unregisterKeyboardEventHandler(GlobalKeyboardEventHandler handler) {
		keyboardHandlers.remove(handler);
	}
	
	public static void unregisterMouseEventHandler(GlobalMouseEventHandler handler) {
		mouseHandlers.remove(handler);
	}
	
	private static String loadLibrary(String name) {
		try {
			String fileName = name.substring(name.lastIndexOf('/') + 1);
			String tmpFolderPath = System.getProperty("java.io.tmpdir");
			if (tmpFolderPath.charAt(tmpFolderPath.length() - 1) == File.separatorChar) tmpFolderPath = tmpFolderPath.substring(0, tmpFolderPath.lastIndexOf(File.separatorChar));
			String tmpFilePath = tmpFolderPath + File.separatorChar + fileName;
			
			File tmpFile = new File(tmpFilePath);
			
			InputStream is = GlobalHook.class.getResourceAsStream(name);
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
	
	private static boolean nativeKeyboardEvent(int rawKeyCode, boolean extendedKey, boolean released, boolean altDowned, boolean ctrlDowned, boolean shiftDowned) {
		GlobalKeyboardEvent event = new GlobalKeyboardEvent(rawKeyCode, extendedKey, altDowned, ctrlDowned, shiftDowned);
		
		if (released) {
			for (GlobalKeyboardEventHandler handler : keyboardHandlers) handler.keyReleased(event);
		} else {
			for (GlobalKeyboardEventHandler handler : keyboardHandlers) handler.keyPressed(event);
		}
		
		return event.isCanceled();
	}
	
	private static void nativeMouseEvent(int mouseEventType, int mouseButton, int x, int y, short delta) {
		GlobalMouseEvent event = new GlobalMouseEvent(mouseButton, x, y, delta);
		
		if (mouseEventType == GlobalMouseEvent.MOUSE_DOWN) {
			for (GlobalMouseEventHandler handler : mouseHandlers) handler.mouseClick(event);
		} else if (mouseEventType == GlobalMouseEvent.MOUSE_UP) {
			for (GlobalMouseEventHandler handler : mouseHandlers) handler.mouseRelease(event);
		} else if (mouseEventType == GlobalMouseEvent.MOUSE_DB_CLICK) {
			for (GlobalMouseEventHandler handler : mouseHandlers) handler.mouseDoubleClick(event);
		} else if (mouseEventType == GlobalMouseEvent.MOUSE_MOVE) {
			for (GlobalMouseEventHandler handler : mouseHandlers) handler.mouseMove(event);
		} else if (mouseEventType == GlobalMouseEvent.MOUSE_WHEEL_SCROLL) {
			for (GlobalMouseEventHandler handler : mouseHandlers) handler.mouseWheelScroll(event);
		}
	}
	
}
