# JGlobalInputManager
This is a project to manage native keyboard and mouse in Java.
Only supported with Windows, currently.

Dll file is programmed in C with JNI to hook keyboard.

## GlobalHook

This class is used to hook native keyboard and mouse input messages.
You can register `GlobalKeyboardEventHandler` using `GlobalHook.registerKeyboardEventHandler`.

```Java
public interface GlobalKeyboardEventHandler {

	void keyPressed(GlobalKeyboardEvent event);

	void keyReleased(GlobalKeyboardEvent event);

}
```

In keyPressed method, you can cancel key input by calling `event.setCanceled(true);`.

If program delayed in keyPressed method, your whole computer system will blocked because KeyboardProc has to wait to receive information about whether key input is canceled or not.

## GlobalInput

This class is used to type a unicode character like Korean, Chinese, Japanese characters etc.
There is only one method called `typeString(String)`.
The string will be inserted on current text cursor.
