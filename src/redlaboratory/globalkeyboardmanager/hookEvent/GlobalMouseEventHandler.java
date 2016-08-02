package redlaboratory.globalkeyboardmanager.hookEvent;

public interface GlobalMouseEventHandler {
	
	void mouseClick(GlobalMouseEvent event);
	
	void mouseRelease(GlobalMouseEvent event);
	
	void mouseDoubleClick(GlobalMouseEvent event);
	
	void mouseMove(GlobalMouseEvent event);
	
	void mouseWheelScroll(GlobalMouseEvent event);
	
}
