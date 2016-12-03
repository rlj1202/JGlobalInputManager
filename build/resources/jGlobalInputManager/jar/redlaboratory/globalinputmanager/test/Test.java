package redlaboratory.globalinputmanager.test;

import java.io.File;
import java.util.Scanner;

import redlaboratory.globalinputmanager.GlobalHook;
import redlaboratory.globalinputmanager.hookEvent.GlobalEventAdapter;
import redlaboratory.globalinputmanager.hookEvent.GlobalKeyboardEvent;
import redlaboratory.globalinputmanager.hookEvent.GlobalMouseEvent;

public class Test {

	public static void main(String[] args) {
		File file = new File("D:/output.txt");
		file.delete();

		GlobalHook.create();
		GlobalHook.registerEventAdapter(new GlobalEventAdapter() {

			@Override
			public void keyPressed(GlobalKeyboardEvent event) {
				System.out.println("	message pressed: " + event.getVirtureKeyCode() + ", " + event.isAltDowned()
						+ ", " + event.isCtrlDowned());

				// switch (event.getVirtureKeyCode()) {
				// case GlobalKeyboardEvent.VK_E: case GlobalKeyboardEvent.VK_X:
				// case GlobalKeyboardEvent.VK_I: case GlobalKeyboardEvent.VK_T:
				// case GlobalKeyboardEvent.VK_ENTER: case
				// GlobalKeyboardEvent.VK_BACK:
				// break;
				// default:
				// event.setCanceled(true);
				// break;
				// }
			}

			@Override
			public void keyReleased(GlobalKeyboardEvent event) {
				System.out.println("	message released.");

				if (event.getVirtureKeyCode() == GlobalKeyboardEvent.VK_BACKSPACE) {
					event.setCanceled(true);
					System.out.println("	canceling message.");
				}
			}

			@Override
			public void mouseClick(GlobalMouseEvent event) {
				System.out.println("	mouse click: " + event.getX() + ", " + event.getY());
			}

			@Override
			public void mouseRelease(GlobalMouseEvent event) {
				System.out.println("	mouse release: " + event.getX() + ", " + event.getY());
			}

			@Override
			public void mouseDoubleClick(GlobalMouseEvent event) {
				System.out.println("	mouse double click: " + event.getX() + ", " + event.getY());
			}

			@Override
			public void mouseMove(GlobalMouseEvent event) {
				System.out.println("	mouse move: " + event.getX() + ", " + event.getY());
			}

			@Override
			public void mouseWheelScroll(GlobalMouseEvent event) {
				System.out.println(
						"	mouse wheel scroll: " + event.getX() + ", " + event.getY() + ", " + event.getDelta());
			}

		});

		Scanner scan = new Scanner(System.in);

		while (true) {
			System.out.print("> ");
			String str = scan.next();

			if (str.equals("exit"))
				break;
		}

		GlobalHook.destroy();
		scan.close();
	}

}
