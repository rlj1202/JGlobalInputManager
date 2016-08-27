package redlaboratory.globalinputmanager.hookEvent;

public class GlobalMouseEvent {
	
	public static final short MOUSE_DELTA = 120;
	
	public static final int MOUSE_LEFT = 0;
	public static final int MOUSE_RIGHT = 1;
	public static final int MOUSE_MIDDLE = 2;
	public static final int MOUSE_XBUTTON1 = 3;
	public static final int MOUSE_XBUTTON2 = 4;
	
	public static final int MOUSE_DOWN = 0;
	public static final int MOUSE_UP = 1;
	public static final int MOUSE_DB_CLICK = 2;
	public static final int MOUSE_MOVE = 3;
	public static final int MOUSE_WHEEL_SCROLL = 4;
	
	private int mouseButton;
	private int x;
	private int y;
	private short delta;
	
	public GlobalMouseEvent(int mouseButton, int x, int y, short delta) {
		this.mouseButton = mouseButton;
		this.x = x;
		this.y = y;
		this.delta = delta;
	}
	
	public boolean isLeftButton() {
		return mouseButton == MOUSE_LEFT;
	}
	
	public boolean isRightButton() {
		return mouseButton == MOUSE_RIGHT;
	}
	
	public boolean isMiddleButton() {
		return mouseButton == MOUSE_MIDDLE;
	}
	
	public boolean isXButton1() {
		return mouseButton == MOUSE_XBUTTON1;
	}
	
	public boolean isXButton2() {
		return mouseButton == MOUSE_XBUTTON2;
	}
	
	public int getMouseButton() {
		return mouseButton;
	}
	
	public int getX() {
		return x;
	}
	
	public int getY() {
		return y;
	}
	
	public short getDelta() {
		return (short) (delta / MOUSE_DELTA);
	}
	
}
