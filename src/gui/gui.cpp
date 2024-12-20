#include <Arduino.h>
#include "gui.h"
#include "mavlink/mav.h"
#include "tracking/tracking.h"

#define DISPLAY_CS 5
#define BLACK 0
#define WHITE 1
#define FREQ_2MHZ 2000000
#define FPS 30
#define BUTTON_1_PIN 26
#define BUTTON_2_PIN 25
#define BUTTON_3_PIN 35
#define BUTTON_4_PIN 34

// Useful links:
// https://javl.github.io/image2cpp/
// https://www.streamlinehq.com/icons/pixel

bool button_flag = false;
bool settings_flag = false;

eButton button = NONE;
unsigned long lastButtonPress;
unsigned long buttonDebounce = 200;

void (*loopFunction)() = mainScreen;
void (*lastLoopFunction)() = NULL;

SPIClass vspi = SPIClass(VSPI);
Adafruit_SharpMem display(&vspi, DISPLAY_CS, 400, 240, FREQ_2MHZ);

int width = display.width();
int height = display.height();

GFXcanvas1 a_h(150, 150);					   // Artificial Horizon
GFXcanvas1 text(width / 2, width / 2);		   // text part of the screen
GFXcanvas1 navigationBar(width, 30);		   // navigation bar
GFXcanvas1 popupWindow(width / 2, height / 2); // popup window

// MENU CREATION
// Courtesy of @sterretje
// https://forum.arduino.cc/t/menu-and-sub-menu-scrolling-with-keypad/461328
// ----------------------------------------------------------------
#define NUM_ELEMENTS(X) (sizeof(X) / sizeof(X[0]))

// Class prototypes
// ===============================
extern Menu settingsMenu;
extern Menu languageMenu;
extern Menu bridgeModeMenu;
// ===============================

// Function prototypes
// ===============================
void polishPopup(void *parameters);
void englishPopup(void *parameters);
void servoTest(void *parameters);
void bluetoothBridge(void *parameters);
void udpBridge(void *parameters);
void usbBridge(void *parameters);
// ===============================

// Menu state variables
Menu *currentMenu = &settingsMenu;
void (*currentFunction)(void *parameters) = NULL;
void *currentParameters = NULL;

// name, function, parameters, function type, toggle, submenu
MenuElement settingsElements[] = {
	{"System status", NULL, NULL, SINGLE, false, NULL},
	{"Language", NULL, NULL, SINGLE, false, &languageMenu},
	{"Bridge mode", NULL, NULL, SINGLE, false, &bridgeModeMenu},
	{"Servo demo", &servoTest, NULL, LOOP, false, NULL}};

MenuElement languageElements[] = {
	{"English", NULL, NULL, SINGLE, false, NULL},
	{"Polish", polishPopup, NULL, SINGLE, false, NULL},
};

MenuElement bridgeModeElements[] = {
	{"UDP", udpBridge, NULL, SINGLE, false, NULL},
	{"BT", bluetoothBridge, NULL, SINGLE, false, NULL},
	{"USB", usbBridge, NULL, SINGLE, false, NULL}};

// name, elements, element count, selected element, parent
Menu settingsMenu = {"Settings", settingsElements, NUM_ELEMENTS(settingsElements), 0, NULL};
Menu systemStatusMenu = {"System status", NULL, 0, 0, &settingsMenu};
Menu languageMenu = {"Language", languageElements, NUM_ELEMENTS(languageElements), 0, &settingsMenu};
Menu bridgeModeMenu = {"Bridge mode", bridgeModeElements, NUM_ELEMENTS(bridgeModeElements), 0, &settingsMenu};

void centerPerfectly(const String &textValue, int cursorXPos, int cursorYPos, GFXcanvas1 &display)
{
	int16_t x1, y1;
	uint16_t textWidth, textHeight;

	display.getTextBounds(textValue, 0, 0, &x1, &y1, &textWidth, &textHeight);
	display.setCursor((display.width() - textWidth) / 2, (display.height() - textHeight) / 2);

	display.println(textValue);
}

void centerPerfectly(const String &textValue, int cursorXPos, int cursorYPos, Adafruit_SharpMem &display)
{
	int16_t x1, y1;
	uint16_t textWidth, textHeight;

	display.getTextBounds(textValue, cursorXPos, cursorYPos, &x1, &y1, &textWidth, &textHeight);
	display.setCursor((display.width() - textWidth) / 2, (display.height() - textHeight) / 2);

	display.print(textValue);
}

void polishPopup(void *parameters)
{
	display.clearDisplay();
	popupWindow.fillScreen(WHITE);
	popupWindow.drawRect(0, 0, popupWindow.width(), popupWindow.height(), BLACK);
	popupWindow.setTextSize(1);
	popupWindow.setTextColor(BLACK);
	popupWindow.setCursor(5, 10);
	centerPerfectly("Język zmieniony na polski", 0, 0, popupWindow);
	centerPerfectly("Kliknij dowolny przycisk, aby kontynuować", 0, 10, popupWindow);
	display.drawBitmap(width / 4, height / 4, popupWindow.getBuffer(), popupWindow.width(), popupWindow.height(), WHITE, BLACK);
}

void servoTest(void *parameters)
{
	display.clearDisplay();
	popupWindow.fillScreen(WHITE);
	popupWindow.drawRect(0, 0, popupWindow.width(), popupWindow.height(), BLACK);
	popupWindow.setTextSize(1);
	popupWindow.setTextColor(BLACK);
	popupWindow.setCursor(5, 10);
	centerPerfectly("Serwo demo", 0, 0, popupWindow);
	display.drawBitmap(width / 4, height / 4, popupWindow.getBuffer(), popupWindow.width(), popupWindow.height(), WHITE, BLACK);
	display.refresh();
	servoDemo();
	Serial.println("Servo test");
}

void udpBridge(void *parameters)
{
	setBridgeType(UDP);
	display.clearDisplay();
	popupWindow.fillScreen(WHITE);
	popupWindow.drawRect(0, 0, popupWindow.width(), popupWindow.height(), BLACK);
	popupWindow.setTextSize(1);
	popupWindow.setTextColor(BLACK);
	popupWindow.setCursor(5, 10);
	centerPerfectly("Bridge mode changed to UDP", 0, 0, popupWindow);
	display.drawBitmap(width / 4, height / 4, popupWindow.getBuffer(), popupWindow.width(), popupWindow.height(), WHITE, BLACK);
}

void bluetoothBridge(void *parameters)
{
	setBridgeType(BLUETOOTH);
	display.clearDisplay();
	popupWindow.fillScreen(WHITE);
	popupWindow.drawRect(0, 0, popupWindow.width(), popupWindow.height(), BLACK);
	popupWindow.setTextSize(1);
	popupWindow.setTextColor(BLACK);
	popupWindow.setCursor(5, 10);
	centerPerfectly("Bridge mode changed to Bluetooth", 0, 0, popupWindow);
	display.drawBitmap(width / 4, height / 4, popupWindow.getBuffer(), popupWindow.width(), popupWindow.height(), WHITE, BLACK);
}

void usbBridge(void *parameters)
{
	setBridgeType(USB);
	display.clearDisplay();
	popupWindow.fillScreen(WHITE);
	popupWindow.drawRect(0, 0, popupWindow.width(), popupWindow.height(), BLACK);
	popupWindow.setTextSize(1);
	popupWindow.setTextColor(BLACK);
	popupWindow.setCursor(5, 10);
	centerPerfectly("Bridge mode changed to USB", 0, 0, popupWindow);
	display.drawBitmap(width / 4, height / 4, popupWindow.getBuffer(), popupWindow.width(), popupWindow.height(), WHITE, BLACK);
}

// ----------------------------------------------------------------

void IRAM_ATTR button_1ISR()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastButtonPress > buttonDebounce)
	{
		lastButtonPress = currentMillis;
		button = BUTTON_1;
	}
}

void IRAM_ATTR button_2ISR()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastButtonPress > buttonDebounce)
	{
		lastButtonPress = currentMillis;
		button = BUTTON_2;
	}
}

void IRAM_ATTR button_3ISR()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastButtonPress > buttonDebounce)
	{
		lastButtonPress = currentMillis;
		button = BUTTON_3;
	}
}

void IRAM_ATTR button_4ISR()
{
	unsigned long currentMillis = millis();
	if (currentMillis - lastButtonPress > buttonDebounce)
	{
		lastButtonPress = currentMillis;
		button = BUTTON_4;
	}
}

/**
 * @brief Initializes buttons and display
 */
void guiInitialize()
{
	pinMode(BUTTON_1_PIN, INPUT);
	pinMode(BUTTON_2_PIN, INPUT);
	pinMode(BUTTON_3_PIN, INPUT);
	pinMode(BUTTON_4_PIN, INPUT);
	attachInterrupt(digitalPinToInterrupt(BUTTON_1_PIN), button_1ISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(BUTTON_2_PIN), button_2ISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(BUTTON_3_PIN), button_3ISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(BUTTON_4_PIN), button_4ISR, FALLING);
	display.begin();
	display.clearDisplay();
}

/**
 * @brief Draws navigation bar at the bottom of the screen
 */
void drawNaviBar()
{
	navigationBar.fillScreen(WHITE);
	navigationBar.fillRect(0, 0, width, navigationBar.height(), BLACK);
	display.drawBitmap(0, height - navigationBar.height(), navigationBar.getBuffer(), navigationBar.width(), navigationBar.height(), WHITE, BLACK);
}

/**
 * @brief Draws menu structure
 */
void displayMenu()
{
	display.setTextSize(1);
	display.setTextColor(BLACK);
	display.setCursor(5, 10);
	display.println(currentMenu->name);
	for (int i = 0; i < currentMenu->elementCount; i++)
	{
		display.setCursor(5, 20 + i * 10);
		if (i == currentMenu->selectedElement)
		{
			display.setTextColor(WHITE);
			display.fillRect(0, 20 + i * 10, 400, 10, BLACK);
		}
		else
		{
			display.setTextColor(BLACK);
		}
		display.println(currentMenu->elements[i].name);
	}
}

void mainScreen()
{
	text.fillScreen(WHITE);
	text.setTextSize(1);
	text.setTextColor(BLACK);
	text.setCursor(5, 10);
	// text.println("Mag heading: " + String(getCompassDegree()));
	if (getConnectionStatus())
	{
		text.println("UAV connected");
	}
	else
	{
		text.println("UAV disconnected");
	}
	text.setCursor(5, 20);
	text.println("GPS: " + String(getUavGPS()->global_lat) + " " + String(getUavGPS()->global_lon) + " " + String(getUavGPS()->global_alt));
	text.setCursor(5, 40);
	text.println("Tracker");
	text.setCursor(5, 50);
	text.println("GPS: " + String(getTrackerGPS().latitude) + " " + String(getTrackerGPS().longitude) + " " + String(getTrackerGPS().altitude));
	text.setCursor(5, 60);
	text.println("Fix: " + String(getTrackerGPS().fixType) + " Sat: " + String(getTrackerGPS().satCount));
	text.setCursor(5, 80);
	text.println("SYS TEXT");
	text.setCursor(5, 90);
	text.println(String(getUavSysText()->text));
	text.setCursor(5, 110);
	text.println("Distance: " + String(getTrackerGPS().angles.distance));
	text.setCursor(5, 120);
	text.println("Azimuth: " + String(getTrackerGPS().angles.azimuth));
	text.setCursor(5, 130);
	text.println("Elevation: " + String(getTrackerGPS().angles.elevation));
	display.drawBitmap(0, 0, text.getBuffer(), text.width(), text.height(), WHITE, BLACK);
}

void screenTest()
{
	String motive = "light";
	static const unsigned char PROGMEM image_paint_0_bits[] = {0x07, 0xff, 0xc0, 0x0f, 0xff, 0xe0, 0x18, 0x00, 0x30, 0x33, 0xff, 0x98, 0x67, 0xff, 0xcc, 0xcc, 0x00, 0x66, 0x99, 0xff, 0x32, 0xb3, 0xff, 0x9a, 0xa6, 0x00, 0xca, 0xac, 0xfe, 0x6a, 0xa9, 0xff, 0x2a, 0x2b, 0x01, 0xa8, 0x0a, 0x7c, 0xa0, 0x02, 0xfe, 0x80, 0x00, 0xfe, 0x00, 0x00, 0xfe, 0x00};
	int background = WHITE;
	int foreground = BLACK;

	if (motive == "light")
	{
		background = WHITE;
		foreground = BLACK;
	}
	else if (motive == "dark")
	{
		background = BLACK;
		foreground = WHITE;
	}

	display.fillScreen(background);
	display.drawLine(1, 209, 399, 209, foreground);
	display.drawRect(10, 216, 70, 20, foreground);
	display.setTextColor(foreground);
	display.setTextSize(1);
	display.setTextWrap(false);
	display.setCursor(36, 222);
	display.drawLine(11, 215, 80, 215, foreground);
	display.drawRect(110, 215, 70, 20, foreground);
	display.drawLine(80, 234, 80, 216, foreground);
	display.setCursor(126, 222);
	display.drawLine(111, 214, 180, 214, foreground);
	display.drawLine(180, 233, 180, 215, foreground);
	display.drawRect(210, 215, 70, 20, foreground);
	display.setCursor(219, 222);
	display.drawLine(211, 214, 280, 214, foreground);
	display.drawLine(280, 233, 280, 215, foreground);
	display.drawRect(309, 215, 70, 20, foreground);
	display.setCursor(333, 221);
	display.drawLine(310, 214, 379, 214, foreground);
	display.drawLine(379, 233, 379, 215, foreground);
	display.drawLine(1, 207, 399, 207, foreground);
	display.drawLine(0, 23, 399, 23, foreground);
	display.drawBitmap(5, 4, image_paint_0_bits, 23, 16, foreground);
}

void artificialHorizon()
{
	// Rysowanie sztucznego horyzontu
	a_h.fillScreen(WHITE);
	int srodekX = a_h.width() / 2;
	int srodekY = a_h.width() / 2;
	int szer = 3;
	int y1 = (srodekY + a_h.width() / 2 * (tan((getUavAttitude()->roll))));
	int y2 = (srodekY - a_h.width() / 2 * (tan((getUavAttitude()->roll))));
	// canvas.drawLine(display.width()/2, y1, display.width(), y2, BLACK);
	a_h.drawLine(0, 0, 0, a_h.height(), BLACK);
	a_h.drawLine(0, a_h.height() - 1, a_h.width(), a_h.height() - 1, BLACK);
	a_h.drawLine(0, y1, a_h.width(), y2, BLACK);

	// Lewa strona
	a_h.fillRect(srodekX - srodekX * 0.7, srodekY, (srodekX - srodekX * 0.25) - (srodekX - srodekX * 0.7) + 1, (srodekY + szer) - srodekY, WHITE);
	a_h.fillRect(srodekX - srodekX * 0.25 - szer, srodekY, (srodekX - srodekX * 0.25) - (srodekX - srodekX * 0.25 - szer), (srodekY + 10) - srodekY, WHITE);
	a_h.drawLine(srodekX - srodekX * 0.7, srodekY, srodekX - srodekX * 0.25, srodekY, BLACK);							 // górna linia pozioma
	a_h.drawLine(srodekX - srodekX * 0.7, srodekY + szer, srodekX - srodekX * 0.25 - szer, srodekY + szer, BLACK);		 // dolna linia pozioma
	a_h.drawLine(srodekX - srodekX * 0.7, srodekY, srodekX - srodekX * 0.7, srodekY + szer, BLACK);						 // lewa linia pionowa
	a_h.drawLine(srodekX - srodekX * 0.25 - szer, srodekY + szer, srodekX - srodekX * 0.25 - szer, srodekY + 10, BLACK); // lewa linia pionowa mniejsza
	a_h.drawLine(srodekX - srodekX * 0.25, srodekY, srodekX - srodekX * 0.25, srodekY + 10, BLACK);						 // prawa linia pionowa mniejsza
	a_h.drawLine(srodekX - srodekX * 0.25 - szer, srodekY + 10, srodekX - srodekX * 0.25, srodekY + 10, BLACK);			 // linia pozioma mniejsza

	// Prawa strona
	a_h.fillRect(srodekX + srodekX * 0.25, srodekY, (srodekX + srodekX * 0.7) - (srodekX + srodekX * 0.25) + 1, (srodekY + szer) - srodekY, WHITE);
	a_h.fillRect(srodekX + srodekX * 0.25, srodekY, (srodekX + srodekX * 0.25 + szer) - (srodekX + srodekX * 0.25), (srodekY + 10) - srodekY, WHITE);
	a_h.drawLine(srodekX + srodekX * 0.7, srodekY, srodekX + srodekX * 0.25, srodekY, BLACK);							 // górna linia pozioma
	a_h.drawLine(srodekX + srodekX * 0.7, srodekY + szer, srodekX + srodekX * 0.25 + szer, srodekY + szer, BLACK);		 // dolna linia pozioma
	a_h.drawLine(srodekX + srodekX * 0.7, srodekY, srodekX + srodekX * 0.7, srodekY + szer, BLACK);						 // lewa linia pionowa
	a_h.drawLine(srodekX + srodekX * 0.25 + szer, srodekY + szer, srodekX + srodekX * 0.25 + szer, srodekY + 10, BLACK); // prawa linia pionowa mniejsza
	a_h.drawLine(srodekX + srodekX * 0.25, srodekY, srodekX + srodekX * 0.25, srodekY + 10, BLACK);						 // lewa linia pionowa mniejsza
	a_h.drawLine(srodekX + srodekX * 0.25 + szer, srodekY + 10, srodekX + srodekX * 0.25, srodekY + 10, BLACK);			 // linia pozioma mniejsza

	// Środek
	a_h.fillRect(srodekX - szer, srodekY, 2 * szer, 2 * szer, WHITE);
	a_h.drawRect(srodekX - szer, srodekY, 2 * szer, 2 * szer, BLACK);
	display.drawBitmap(width - 150, 0, a_h.getBuffer(), a_h.width(), a_h.height(), WHITE, BLACK);
}

void guiTask(void *parameters)
{
	for (;;)
	{
		if (button != NONE)
		{
			display.clearDisplay();
		}

		if (!settings_flag)
		{
			switch (button)
			{
			case BUTTON_1:
				button = NONE;
				loopFunction = mainScreen;
				break;

			case BUTTON_2:
				button = NONE;
				loopFunction = artificialHorizon;
				break;

			case BUTTON_3:
				button = NONE;
				loopFunction = screenTest;
				break;

			case BUTTON_4:
				button = NONE;
				lastLoopFunction = loopFunction;
				loopFunction = NULL;
				settings_flag = true;
				break;
			}
		}
		else
		{
			switch (button)
			{
			// Move down
			case BUTTON_2:
				button = NONE;
				currentMenu->selectedElement++;
				if (currentMenu->selectedElement >= currentMenu->elementCount)
				{
					currentMenu->selectedElement = currentMenu->elementCount - 1;
				}
				break;
			// Move up
			case BUTTON_1:
				button = NONE;
				if (currentMenu->selectedElement > 0)
				{
					currentMenu->selectedElement--;
				}
				break;

			// Select
			case BUTTON_3:
				button = NONE;
				if (currentMenu->elements[currentMenu->selectedElement].subMenu != NULL)
				{
					// if there is a submenu, change to submenu
					currentMenu = currentMenu->elements[currentMenu->selectedElement].subMenu;
				}
				else if (currentMenu->elements[currentMenu->selectedElement].function != NULL)
				{
					// if there is an action, setup current action and current parameter
					// currentMenu->items[currentMenu->selected].action(currentMenu->items[currentMenu->selected].param);
					currentFunction = currentMenu->elements[currentMenu->selectedElement].function;
					currentParameters = currentMenu->elements[currentMenu->selectedElement].parameters;
					eFunctionType currentFunctionType = currentMenu->elements[currentMenu->selectedElement].functionType;
					if (currentFunctionType == SINGLE)
					{
						currentFunction(currentParameters);
					}
					else if (currentFunctionType == LOOP)
					{
						while (button == NONE)
						{
							currentFunction(currentParameters);
						}
					}
				}
				break;

			case BUTTON_4:
				button = NONE;
				if (currentMenu == &settingsMenu)
				{
					display.clearDisplay();
					loopFunction = lastLoopFunction;
					settings_flag = false;
				}
				if (currentMenu->parent != NULL)
				{
					currentMenu = currentMenu->parent;
				}
				break;
			}
			displayMenu();
		}
		if (loopFunction != NULL)
		{
			loopFunction();
		}
		display.refresh();
		vTaskDelay((1 / FPS) / portTICK_PERIOD_MS);
	}
}