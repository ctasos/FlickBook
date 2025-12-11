#include "UIManager.h"

extern EpubParser epubParser;
extern SettingsManager settingsManager;
extern SDHandler sdHandler;
extern LibraryManager libraryManager;

extern const GFXfont FreeSerif12pt7b;
extern const GFXfont FreeSerif18pt7b;
extern const GFXfont FreeSans12pt7b;
extern const GFXfont FreeSans13pt7b;
extern const GFXfont FreeSans18pt7b;
extern const GFXfont Bookerly14pt7b;
extern const GFXfont Bookerly18pt7b;
extern const GFXfont Amazon_Ember_V212pt7b;
extern const GFXfont Amazon_Ember_V213pt7b;
extern const GFXfont Amazon_Ember_V218pt7b;
extern const GFXfont Amazon_Ember_Light14pt7b;
extern const GFXfont Amazon_Ember_Light18pt7b;
static const GFXfont *FONT_PRIM_SMALL = &Amazon_Ember_V212pt7b;
static const GFXfont *FONT_PRIM_DEFAULT = &Amazon_Ember_V212pt7b;
static const GFXfont *FONT_PRIM_LARGE = &Amazon_Ember_V218pt7b;
static const GFXfont *FONT_ALT_SMALL = &FreeSans12pt7b;
static const GFXfont *FONT_ALT_DEFAULT = &FreeSans13pt7b;
static const GFXfont *FONT_ALT_LARGE = &FreeSans18pt7b;

UIManager::UIManager(Inkplate *display, bool grayscale) : display(display), currentScreen(0), previousScreen(0), grayscale(grayscale), scrollIndex(0), currentSection(0), currentSectionIndex(0), previousSectionIndex(0), nextSectionIndex(0), totalSections(0), pageContent(""), pagePath("") {}

void UIManager::init()
{
  display->begin();
  setFont(FONT_PRIM, FONT_SIZE_DEFAULT);
  display->setTextWrap(false); // Important for proper rendering of long text
  // display->setFullUpdateThreshold(PARTIAL_UPDATE_LIMIT_N);
  // Try touchscreen initialization up to 5 times
  bool tsInitialized = false;
  for (int i = 0; i < 5; i++)
  {
    if (display->tsInit(true))
    {
      Serial.println("Touchscreen init ok");
      tsInitialized = true;
      break;
    }
    else
    {
      Serial.printf("Touchscreen init failed (attempt %d/5)\n", i + 1);
      delay(500); // Small delay before retrying
    }
  }

  if (!tsInitialized)
  {
    Serial.println("Touchscreen failed to initialize after 5 attempts.");
  }
  display->setRotation(1); // Portrait mode
  renderScreen(MAIN_SCREEN);

  snakeGame = new SnakeGame(display, [this]()
                            {
      inSnakeMode = false;
      renderScreen(currentScreen, false, true); });
}

void UIManager::setFont(uint8_t fontFamily, uint8_t fontSize)
{
  switch (fontFamily)
  {
  case FONT_PRIM:

    switch (fontSize)
    {
    case FONT_SIZE_SMALL:
      display->setFont(FONT_PRIM_SMALL);
      display->setTextSize(1);
      break;
    case FONT_SIZE_DEFAULT:
      display->setFont(FONT_PRIM_DEFAULT);
      display->setTextSize(1);
      break;
    case FONT_SIZE_LARGE:
      display->setFont(FONT_PRIM_LARGE);
      display->setTextSize(1);
      break;
    default:
      display->setFont(FONT_PRIM_DEFAULT);
      display->setTextSize(1);
      break;
    }

    break;

  case FONT_ALT:

    switch (fontSize)
    {
    case FONT_SIZE_SMALL:
      display->setFont(FONT_ALT_SMALL);
      display->setTextSize(1);
      break;
    case FONT_SIZE_DEFAULT:
      display->setFont(FONT_ALT_DEFAULT);
      display->setTextSize(1);
      break;
    case FONT_SIZE_LARGE:
      display->setFont(FONT_ALT_LARGE);
      display->setTextSize(1);
      break;
    default:
      display->setFont(FONT_ALT_DEFAULT);
      display->setTextSize(1);
      break;
    }

    break;
  default:
    display->setFont(FONT_PRIM_DEFAULT);
    display->setTextSize(1);
    break;
  }
}
void UIManager::handleTouch()
{
  if (inSnakeMode)
  {
    snakeGame->update();
    uint8_t n;
    uint16_t x[2], y[2];
    if (display->tsAvailable() && (n = display->tsGetData(x, y)))
    {
      snakeGame->onTouch(x[0], y[0]);
    }
    return;
  }
  if (display->tsAvailable())
  {
    uint8_t n;
    uint16_t x[2], y[2];
    n = display->tsGetData(x, y);

    if (n != 0)
    {
      // Touch down or move
      Serial.printf("%d finger%c ", n, n > 1 ? 's' : '\0');
      for (uint8_t i = 0; i < n; i++)
        Serial.printf("X=%d Y=%d ", x[i], y[i]);
      Serial.println();

      if (!isTouchActive)
      {
        Serial.println("New touch start");
        // New touch start
        isTouchActive = true;
        touchStartTime = millis();
        longPressTriggered = false;
        touchStartX = x[0];
        touchStartY = y[0];
      }
      else
      {
        // Touch is still active, check for long press
        unsigned long now = millis();
        if (!longPressTriggered && (now - touchStartTime > LONG_TOUCH_DUR))
        {
          longPressTriggered = true;
          handleLongTouch(touchStartX, touchStartY);
        }
      }
    }
    else
    {
      // Touch release
      Serial.println("Touch Release");
      if (isTouchActive)
      {
        Serial.println("Touch Release with Touch Active");
        unsigned long now = millis();
        int dx = lastTouchX - touchStartX;
        int dy = lastTouchY - touchStartY;

        if (!longPressTriggered && (now - touchStartTime > LONG_TOUCH_DUR))
        {
          longPressTriggered = true;
          handleLongTouch(touchStartX, touchStartY);
        }
        else if (!longPressTriggered)
        {
          // Check for swipe
          if (abs(dx) > SWIPE_THRESHOLD || abs(dy) > SWIPE_THRESHOLD)
          {
            if (abs(dx) > abs(dy))
            {
              // Horizontal swipe
              if (dx > 0)
              {
                handleSwipeLeft(touchStartX, touchStartY);
              }
              else
              {
                handleSwipeRight(touchStartX, touchStartY);
              }
            }
            else
            {
              // Vertical swipe
              if (dy > 0)
              {
                handleSwipeUp(touchStartX, touchStartY);
              }
              else
              {
                handleSwipeDown(touchStartX, touchStartY);
              }
            }
          }
          else
          {
            // No swipe — treat as short touch
            handleShortTouch(touchStartX, touchStartY);
          }
        }

        isTouchActive = false;
      }
    }

    // Store last known touch position for swipe detection
    if (n != 0)
    {
      lastTouchX = x[0];
      lastTouchY = y[0];
    }
  }
}

void UIManager::handleShortTouch(uint16_t x, uint16_t y)
{
  Serial.printf("Short touch at X=%d Y=%d\n", x, y);
  if (handleTouchMenu(x, y, TOUCH_TYPE_SHORT))
  {
    // handled
  }
  else if (handleTouchReadingHeader(x, y, TOUCH_TYPE_SHORT))
  {
    // handled
  }
  else if (handleTouchMainHeader(x, y, TOUCH_TYPE_SHORT))
  {
    // handled
  }
  else if (handleTouchBookList(x, y, TOUCH_TYPE_SHORT))
  {
    // handled
  }
}

void UIManager::handleLongTouch(uint16_t x, uint16_t y)
{
  // For now, just log
  Serial.printf("Long touch at X=%d Y=%d\n", x, y);
  if (handleTouchMenu(x, y, TOUCH_TYPE_LONG))
  {
    // handled
  }
  else if (handleTouchReadingHeader(x, y, TOUCH_TYPE_LONG))
  {
    // handled
  }
  else if (handleTouchMainHeader(x, y, TOUCH_TYPE_LONG))
  {
    // handled
  }
  else if (handleTouchBookList(x, y, TOUCH_TYPE_LONG))
  {
    // handled
  }
}

void UIManager::handleSwipeLeft(uint16_t x, uint16_t y)
{
  if (handleTouchReadingPage(x, y, TOUCH_TYPE_SLEFT))
  {
    // handled
  }
}

void UIManager::handleSwipeRight(uint16_t x, uint16_t y)
{
  if (handleTouchReadingPage(x, y, TOUCH_TYPE_SRIGHT))
  {
    // handled
  }
}

void UIManager::handleSwipeUp(uint16_t x, uint16_t y)
{
  Serial.println("Swipe Up");
  if (handleTouchBookList(x, y, TOUCH_TYPE_SUP))
  {
    // handled
  }
  else if (handleTouchReadingPage(x, y, TOUCH_TYPE_SUP))
  {
    // handled
  }
}

void UIManager::handleSwipeDown(uint16_t x, uint16_t y)
{
  Serial.println("Swipe Down");
  if (handleTouchBookList(x, y, TOUCH_TYPE_SDOWN))
  {
    // handled
  }
  else if (handleTouchReadingPage(x, y, TOUCH_TYPE_SDOWN))
  {
    // handled
  }
}

// void UIManager::renderPage(const String& text) {
void UIManager::renderScreen(uint8_t s, bool partial_update, bool force)
{
  if (s == currentScreen and !force)
  {
    return;
  }
  Serial.println("Rendering Screen");
  // Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  previousScreen = currentScreen;
  currentScreen = s;
  display->clearDisplay();

  switch (currentScreen)
  {
  case MAIN_SCREEN:
    renderBookList();
    renderMenu();
    renderMainHeader();

    break;
  case READING_SCREEN:

    renderResult = renderTextBlockSection(pageContent, BOOK_PAGE_X, BOOK_PAGE_Y, BOOK_PAGE_W, BOOK_PAGE_H, currentSection, 0);
    renderMenu();
    renderReadingHeader();

    // Serial.println(renderResult.printedText);

    break;
  case SETTINGS_SCREEN:
    renderSettings();
    renderMenu();
    renderMainHeader();
    break;
  default:
    // statements
    break;
  }
  if (partial_update)
  {
    //   Serial.println("Partial Update");
    if (partial_update_counter < PARTIAL_UPDATE_LIMIT_N)
    {
      display->partialUpdate();
      partial_update_counter++;
    }
    else
    {
      display->display();
      partial_update_counter = 0;
    }
  }
  else
  {
    display->display();
    partial_update_counter = 0;
  }
}

void UIManager::loadSectionContent()
{
  renderLoadingIcon();
  pageContent = libraryManager.getCurrentPageContent(sectionStarts[currentSection]);
  pagePath = libraryManager.getCurrentPagePath();
  // totalSections = sectionsForTextBlock(pageContent, BOOK_PAGE_X, BOOK_PAGE_Y, BOOK_PAGE_W, BOOK_PAGE_H);
  // totalSections = sectionsForTextBlock(BOOK_PAGE_X, BOOK_PAGE_Y, BOOK_PAGE_W, BOOK_PAGE_H);
  // Serial.println(pageContent);
  // Serial.println("page path is: " + pagePath);
  // Serial.println("section out of total: " + String(currentSection) + " / " + String(totalSections - 1));
}

void UIManager::loadPageSections()
{
  renderLoadingIcon();
  // pageContent = libraryManager.getCurrentPageContent(currentSectionIndex);
  // pagePath = libraryManager.getCurrentPagePath();
  // totalSections = sectionsForTextBlock(pageContent, BOOK_PAGE_X, BOOK_PAGE_Y, BOOK_PAGE_W, BOOK_PAGE_H);
  totalSections = sectionsForTextBlock(BOOK_PAGE_X, BOOK_PAGE_Y, BOOK_PAGE_W, BOOK_PAGE_H);
  // Serial.println(pageContent);
  // Serial.println("page path is: " + pagePath);
  Serial.println("section out of total: " + String(currentSection) + " / " + String(totalSections - 1));
}

void UIManager::renderMenu(bool partial_update)
{

  display->fillRoundRect(MENU_ITEM_HOME[0], MENU_ITEM_HOME[1], MENU_ITEM_HOME[2], MENU_ITEM_HOME[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
  drawIcon(MENU_ITEM_ICON[0], MENU_ITEM_HOME[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_HOME[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);

  if (GRAYSCALE)
  {
    display->fillRoundRect(MENU_ITEM_BACKLIGHT[0], MENU_ITEM_BACKLIGHT[1], MENU_ITEM_BACKLIGHT[2], MENU_ITEM_BACKLIGHT[3], MENU_ITEM_RADIUS, settingsManager.getBacklight() / 10); // Arguments are: start X, start Y, size X, size Y, radius, color (if grayscale 0-6)
    drawIcon(MENU_ITEM_ICON[1], MENU_ITEM_BACKLIGHT[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_BACKLIGHT[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, settingsManager.getBacklight() < 30);
  }
  else
  {
    display->fillRoundRect(MENU_ITEM_BACKLIGHT[0], MENU_ITEM_BACKLIGHT[1], MENU_ITEM_BACKLIGHT[2], MENU_ITEM_BACKLIGHT[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
    if (settingsManager.getBacklight() > 0)
    {
      display->fillRoundRect(MENU_ITEM_BACKLIGHT[0] + 5, MENU_ITEM_BACKLIGHT[1] + MENU_ITEM_BACKLIGHT[3] - int(MENU_ITEM_BACKLIGHT[3] * settingsManager.getBacklight() / BACKLIGHT_L5), MENU_ITEM_BACKLIGHT[2] / 8, int(MENU_ITEM_BACKLIGHT[3] * settingsManager.getBacklight() / BACKLIGHT_L5), MENU_ITEM_RADIUS, WHITE);
    }
    else
    {
      display->fillRoundRect(MENU_ITEM_BACKLIGHT[0], MENU_ITEM_BACKLIGHT[1] + 1, MENU_ITEM_BACKLIGHT[2] / 4, MENU_ITEM_BACKLIGHT[3] - 2, MENU_ITEM_RADIUS, BLACK);
    }
    drawIcon(MENU_ITEM_ICON[1], MENU_ITEM_BACKLIGHT[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_BACKLIGHT[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);
  }

  if (currentScreen == READING_SCREEN)
  {
    display->fillRoundRect(MENU_ITEM_START[0], MENU_ITEM_START[1], MENU_ITEM_START[2], MENU_ITEM_START[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
    drawIcon(MENU_ITEM_ICON[2], MENU_ITEM_START[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_START[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);

    display->fillRoundRect(MENU_ITEM_BACK[0], MENU_ITEM_BACK[1], MENU_ITEM_BACK[2], MENU_ITEM_BACK[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
    drawIcon(MENU_ITEM_ICON[3], MENU_ITEM_BACK[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_BACK[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);

    display->fillRoundRect(MENU_ITEM_FW[0], MENU_ITEM_FW[1], MENU_ITEM_FW[2], MENU_ITEM_FW[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
    drawIcon(MENU_ITEM_ICON[4], MENU_ITEM_FW[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_FW[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);

    display->fillRoundRect(MENU_ITEM_UP[0], MENU_ITEM_UP[1], MENU_ITEM_UP[2], MENU_ITEM_UP[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
    drawIcon(MENU_ITEM_ICON[5], MENU_ITEM_UP[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_UP[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);

    display->fillRoundRect(MENU_ITEM_DOWN[0], MENU_ITEM_DOWN[1], MENU_ITEM_DOWN[2], MENU_ITEM_DOWN[3], MENU_ITEM_RADIUS, BLACK); // Arguments are: start X, start Y, size X, size Y, radius, color
    drawIcon(MENU_ITEM_ICON[6], MENU_ITEM_DOWN[0] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, MENU_ITEM_DOWN[1] + (MENU_ITEM_SIZE - MENU_ICON_SIZE) / 2, true);
  }
  if (partial_update)
  {
    //   Serial.println("Partial Update");
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
}

void UIManager::renderReadingHeader(bool partial_update)
{
  display->fillRect(HEADER[0], HEADER[1], HEADER[2], HEADER[3], BLACK);

  setFont(FONT_ALT, FONT_SIZE_SMALL);
  display->setTextColor(WHITE);
  display->setCursor(15, 20);
  String bookTitle = libraryManager.getTitle();
  if (bookTitle.length() > LIST_ITEM_MAX_CHARS)
  {
    bookTitle = bookTitle.substring(0, LIST_ITEM_MAX_CHARS - 3) + "..."; // Truncate and add '...'
  }
  display->print(bookTitle);

  display->setCursor(15, display->getCursorY() + NORMAL_LINE_HEIGHT);
  display->print(String(libraryManager.getCurrentPage() + 1) + "." + String(currentSection) + "/" + String(libraryManager.getTotalPage()) + "." + String(totalSections - 1));
  drawBattery(true);

  display->drawRoundRect(READING_HEADER_ITEM_RELOAD[0], READING_HEADER_ITEM_RELOAD[1], READING_HEADER_ITEM_RELOAD[2], READING_HEADER_ITEM_RELOAD[3], HEADER_ITEM_RADIUS, WHITE); // Arguments are: start X, start Y, size X, size Y, radius, color
  drawIcon(READING_HEADER_ITEM_ICON[0], READING_HEADER_ITEM_RELOAD[0] + (HEADER_ITEM_SIZE - HEADER_ICON_SIZE) / 2, READING_HEADER_ITEM_RELOAD[1] + (HEADER_ITEM_SIZE - HEADER_ICON_SIZE) / 2, true);

  if (partial_update)
  {
    //   Serial.println("Partial Update");
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
}

void UIManager::renderMainHeader(bool partial_update)
{
  display->fillRect(HEADER[0], HEADER[1], HEADER[2], HEADER[3], BLACK);
  setFont(FONT_ALT, FONT_SIZE_LARGE);
  display->setTextColor(WHITE);
  display->setCursor(15, 45);
  display->print("The Best eBook Reader!");
  drawBattery(true);

  display->drawRoundRect(MAIN_HEADER_ITEM_SETTINGS[0], MAIN_HEADER_ITEM_SETTINGS[1], MAIN_HEADER_ITEM_SETTINGS[2], MAIN_HEADER_ITEM_SETTINGS[3], HEADER_ITEM_RADIUS, WHITE); // Arguments are: start X, start Y, size X, size Y, radius, color
  drawIcon(MAIN_HEADER_ITEM_ICON[0], MAIN_HEADER_ITEM_SETTINGS[0] + (HEADER_ITEM_SIZE - HEADER_ICON_SIZE) / 2, MAIN_HEADER_ITEM_SETTINGS[1] + (HEADER_ITEM_SIZE - HEADER_ICON_SIZE) / 2, true);

  if (partial_update)
  {
    //   Serial.println("Partial Update");
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
}

uint8_t UIManager::getCurrentScreen()
{
  return currentScreen;
}

uint8_t UIManager::getPreviousScreen()
{
  return previousScreen;
}

void UIManager::flushTS(uint8_t d)
{

  uint16_t x[2], y[2];
  // delay(100);
  while (display->tsAvailable())
  {
    Serial.println("Flushing touchscreen...");
    display->tsGetData(x, y);
    delay(d);
  }
}

void UIManager::renderBookList(bool partial_update)
{
  std::vector<String> bookList = libraryManager.getLibrary();

  setFont(FONT_PRIM, FONT_SIZE_LARGE);
  display->setTextColor(BLACK);
  display->setCursor(LIST_TITLE_X, LIST_TITLE_Y);
  display->print("MY LIBRARY:");
  int maxScroll = max(0, (int)bookList.size() - LIST_MAX_FILES);

  // Printing book list
  for (int i = 0; i < LIST_MAX_FILES && (scrollIndex + i) < bookList.size(); i++)
  {
    int y = LIST_START_Y + i * LIST_ITEM_HEIGHT;
    display->drawRect(LIST_START_X, y, LIST_WIDTH, LIST_ITEM_HEIGHT, BLACK);

    // Get the file name and truncate if necessary
    String bookName = bookList[scrollIndex + i];
    StaticJsonDocument<4096> bookUserData = libraryManager.fetchBookUserData(bookName);
    String author = bookUserData["author"];
    String pages = bookUserData["pages"];
    int lastPage = bookUserData["lastPage"].as<int>();
    String itemTitle = bookName;
    String itemProgress = "(" + String(lastPage + 1) + "/" + pages + ")";
    if (itemTitle.length() > LIST_ITEM_MAX_CHARS)
    {
      itemTitle = itemTitle.substring(0, LIST_ITEM_MAX_CHARS - 3) + "..."; // Truncate and add '...'
    }

    display->setCursor(LIST_START_X + 10, y + 2 * LIST_ITEM_HEIGHT / 5);
    setFont(FONT_PRIM, FONT_SIZE_LARGE);
    display->print(itemTitle);
    setFont(FONT_PRIM, FONT_SIZE_DEFAULT);
    display->setCursor(LIST_START_X + 10, y + 4 * LIST_ITEM_HEIGHT / 5);
    display->print("by " + author);
    setFont(FONT_ALT, FONT_SIZE_DEFAULT);
    display->setCursor(LIST_WIDTH - itemProgress.length() * 10, y + 4 * LIST_ITEM_HEIGHT / 5);
    display->print(itemProgress);
  }

  // Drawing scrollbar
  if (bookList.size() > LIST_MAX_FILES)
  {
    int scrollbarAreaHeight = (TOT_H - LIST_START_Y - MENU_ITEM_SIZE - 2 * LIST_ARROW_SIZE); // Reserve space for arrows
    int scrollbarHeight = scrollbarAreaHeight * LIST_MAX_FILES / bookList.size();
    int scrollbarY = LIST_START_Y + (scrollbarAreaHeight - scrollbarHeight) * scrollIndex / maxScroll;
    display->fillRect(TOT_W - 2 * LIST_SCROLLBAR_WIDTH - 4, scrollbarY, LIST_SCROLLBAR_WIDTH, scrollbarHeight, BLACK);

    // Up arrow
    display->fillTriangle(LIST_UP_ARROW_DRAW[0][0], LIST_UP_ARROW_DRAW[0][1],
                          LIST_UP_ARROW_DRAW[1][0], LIST_UP_ARROW_DRAW[1][1],
                          LIST_UP_ARROW_DRAW[2][0], LIST_UP_ARROW_DRAW[2][1], BLACK);
    display->drawRoundRect(LIST_UP_ARROW_TOUCH[0], LIST_UP_ARROW_TOUCH[1], LIST_UP_ARROW_TOUCH[2], LIST_UP_ARROW_TOUCH[3], LIST_ARROW_RADIUS, BLACK);
    // Down arrow
    display->fillTriangle(LIST_DOWN_ARROW_DRAW[0][0], LIST_DOWN_ARROW_DRAW[0][1],
                          LIST_DOWN_ARROW_DRAW[1][0], LIST_DOWN_ARROW_DRAW[1][1],
                          LIST_DOWN_ARROW_DRAW[2][0], LIST_DOWN_ARROW_DRAW[2][1], BLACK);
    display->drawRoundRect(LIST_DOWN_ARROW_TOUCH[0], LIST_DOWN_ARROW_TOUCH[1], LIST_DOWN_ARROW_TOUCH[2], LIST_DOWN_ARROW_TOUCH[3], LIST_ARROW_RADIUS, BLACK);
  }

  if (partial_update)
  {
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
}

bool UIManager::handleTouchBookList(uint16_t x, uint16_t y, uint8_t touch_type)
{
  // if (!display->tsAvailable()) return;
  // uint16_t x, y;
  // display->tsGetData(&x, &y);
  if (currentScreen == MAIN_SCREEN)
  {
    std::vector<String> bookList = libraryManager.getLibrary();

    // Checking for Scrollbar arrow touches
    if (x > LIST_UP_ARROW_TOUCH[0])
    {
      // Up arrow
      if (y > LIST_UP_ARROW_TOUCH[1] and y < (LIST_UP_ARROW_TOUCH[1] + LIST_UP_ARROW_TOUCH[3]))
      {
        if (touch_type == TOUCH_TYPE_SHORT)
        {
          int newScrollIndex = max(0, scrollIndex - 3);
          if (newScrollIndex != scrollIndex)
          {
            scrollIndex = newScrollIndex;
            renderScreen(MAIN_SCREEN, true, true);
          }
          flushTS(30);
          return true;
        }
        // Down arrow
      }
      else if (y > LIST_DOWN_ARROW_TOUCH[1] and y < (LIST_DOWN_ARROW_TOUCH[1] + LIST_DOWN_ARROW_TOUCH[3]))
      {
        if (touch_type == TOUCH_TYPE_SHORT)
        {
          int newScrollIndex = min((int)bookList.size() - LIST_MAX_FILES, scrollIndex + 3);
          if (newScrollIndex != scrollIndex)
          {
            scrollIndex = newScrollIndex;
            renderScreen(MAIN_SCREEN, true, true);
          }
          flushTS(30);
          return true;
        }
      }

      // Checking for list item touches
    }
    else if (x > LIST_START_X and x < (LIST_START_X + LIST_WIDTH))
    {
      if (y > LIST_START_Y and y < (LIST_START_Y + (LIST_MAX_FILES + 1) * LIST_ITEM_HEIGHT))
      {
        int index = (y - LIST_START_Y) / LIST_ITEM_HEIGHT;
        if (index >= 0 && index < LIST_MAX_FILES && (scrollIndex + index) < bookList.size())
        {
          if (touch_type == TOUCH_TYPE_SHORT)
          {
            Serial.print("Selected file: ");
            Serial.println(bookList[scrollIndex + index]);
            flushTS(30);
            libraryManager.loadCurrentBook(bookList[scrollIndex + index]);
            currentSection = libraryManager.getCurrentSection();
            // currentSectionIndex = libraryManager.getCurrentSectionIndex();
            // nextSectionIndex = currentSectionIndex;
            loadPageSections();
            loadSectionContent();
            renderScreen(READING_SCREEN);
            return true;
          }
          else if (touch_type == TOUCH_TYPE_SDOWN)
          {
            int newScrollIndex = min((int)bookList.size() - LIST_MAX_FILES, scrollIndex + 3);
            if (newScrollIndex != scrollIndex)
            {
              scrollIndex = newScrollIndex;
              renderScreen(MAIN_SCREEN, true, true);
            }
            flushTS(30);
            return true;
          }
          else if (touch_type == TOUCH_TYPE_SUP)
          {
            int newScrollIndex = max(0, scrollIndex - 3);
            if (newScrollIndex != scrollIndex)
            {
              scrollIndex = newScrollIndex;
              renderScreen(MAIN_SCREEN, true, true);
            }
            flushTS(30);
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool UIManager::handleTouchMenu(uint16_t x, uint16_t y, uint8_t touch_type)
{
  if ((x >= MENU_ITEM_HOME[0]) and (y >= MENU_ITEM_HOME[1]) and (x <= (MENU_ITEM_HOME[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_HOME[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: HOME");
      renderScreen(MAIN_SCREEN);
      return true;
    }
  }
  else if ((currentScreen == READING_SCREEN) and (x >= MENU_ITEM_START[0]) and (y >= MENU_ITEM_START[1]) and (x <= (MENU_ITEM_START[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_START[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: START");
      libraryManager.setCurrentPage(0);
      currentSection = 0;
      // previousSectionIndex = 0;
      // currentSectionIndex = 0;
      libraryManager.setCurrentSection(currentSection);
      // libraryManager.setCurrentSectionIndex(currentSectionIndex);
      loadPageSections();
      loadSectionContent();
      renderScreen(READING_SCREEN, false, true);
      flushTS(50);
      return true;
    }
  }
  else if ((x >= MENU_ITEM_BACKLIGHT[0]) and (y >= MENU_ITEM_BACKLIGHT[1]) and (x <= (MENU_ITEM_BACKLIGHT[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_BACKLIGHT[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: BACKLIGHT");
      settingsManager.incBacklight();
      if (currentScreen == SETTINGS_SCREEN)
      {
        renderScreen(SETTINGS_SCREEN, true, true);
      }
      else
      {
        renderMenu(true);
      }
      // delay(100);
      flushTS(50);
      return true;
    }
    else if (touch_type == TOUCH_TYPE_LONG)
    {
      Serial.println("Long Touched Menu Item: BACKLIGHT");
      settingsManager.setBacklight(0, true);
      renderMenu(true);
      // delay(100);
      // flushTS(50); DONT FLUSH AFTER LONG TOUCH TO ALLOW FINGER RELEASE DETECTION
      return true;
    }
  }
  else if ((currentScreen == READING_SCREEN) and (x >= MENU_ITEM_BACK[0]) and (y >= MENU_ITEM_BACK[1]) and (x <= (MENU_ITEM_BACK[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_BACK[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: BACK");
      if (libraryManager.prevPage())
      {
        currentSection = 0;
        // previousSectionIndex = 0;
        // currentSectionIndex = 0;
        libraryManager.setCurrentSection(currentSection);
        // libraryManager.setCurrentSectionIndex(currentSectionIndex);
        loadPageSections();
        loadSectionContent();
        renderScreen(READING_SCREEN, true, true);
      }
      flushTS(50);
      return true;
    }
  }
  else if ((currentScreen == READING_SCREEN) and (x >= MENU_ITEM_FW[0]) and (y >= MENU_ITEM_FW[1]) and (x <= (MENU_ITEM_FW[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_FW[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: FW");
      if (libraryManager.nextPage())
      {
        currentSection = 0;
        // previousSectionIndex = 0;
        // currentSectionIndex = 0;
        libraryManager.setCurrentSection(currentSection);
        // libraryManager.setCurrentSectionIndex(currentSectionIndex);
        loadPageSections();
        loadSectionContent();
        renderScreen(READING_SCREEN, true, true);
      }
      flushTS(50);
      return true;
    }
  }
  else if ((currentScreen == READING_SCREEN) and (x >= MENU_ITEM_UP[0]) and (y >= MENU_ITEM_UP[1]) and (x <= (MENU_ITEM_UP[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_UP[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: UP");
      currentSection--;
      // previousSectionIndex = currentSectionIndex; TODO
      // currentSectionIndex = previousSectionIndex;
      if (currentSection < 0)
      {
        currentSection = 0;
      }
      else
      {
        loadSectionContent();
        renderScreen(READING_SCREEN, true, true);
      }
      libraryManager.setCurrentSection(currentSection);
      // libraryManager.setCurrentSectionIndex(currentSectionIndex);

      flushTS(50);
      return true;
    }
  }
  else if ((currentScreen == READING_SCREEN) and (x >= MENU_ITEM_DOWN[0]) and (y >= MENU_ITEM_DOWN[1]) and (x <= (MENU_ITEM_DOWN[0] + MENU_ITEM_SIZE)) and (y <= (MENU_ITEM_DOWN[1] + MENU_ITEM_SIZE)))
  {
    if (touch_type == TOUCH_TYPE_SHORT)
    {
      Serial.println("Touched Menu Item: DOWN");
      currentSection++;
      // previousSectionIndex = currentSectionIndex;
      // currentSectionIndex = nextSectionIndex;
      if (currentSection >= totalSections)
      {
        currentSection = totalSections - 1;
      }
      else
      {
        loadSectionContent();
        renderScreen(READING_SCREEN, true, true);
      }
      libraryManager.setCurrentSection(currentSection);
      // libraryManager.setCurrentSectionIndex(currentSectionIndex);
      flushTS(50);
      return true;
    }
  }
  return false;
}

bool UIManager::handleTouchReadingHeader(uint16_t x, uint16_t y, uint8_t touch_type)
{
  if (currentScreen == READING_SCREEN)
  {
    if ((x >= READING_HEADER_ITEM_RELOAD[0]) and (y >= READING_HEADER_ITEM_RELOAD[1]) and (x <= (READING_HEADER_ITEM_RELOAD[0] + HEADER_ITEM_SIZE)) and (y <= (READING_HEADER_ITEM_RELOAD[1] + HEADER_ITEM_SIZE)))
    {
      if (touch_type == TOUCH_TYPE_SHORT)
      {
        Serial.println("Touched Header Item: RELOAD");
        epubParser.parseEpubMetadata(libraryManager.getCurrentBook());
        libraryManager.initBookUserData();
        currentSection = 0;
        // previousSectionIndex = 0;
        // currentSectionIndex = 0;
        libraryManager.setCurrentSection(currentSection);
        // libraryManager.setCurrentSectionIndex(currentSectionIndex);
        loadPageSections();
        loadSectionContent();
        renderScreen(READING_SCREEN, false, true);
        // delay(100);
        flushTS(50);
        return true;
      }
    }
  }
  return false;
}

bool UIManager::handleTouchReadingPage(uint16_t x, uint16_t y, uint8_t touch_type)
{
  if (currentScreen == READING_SCREEN)
  {
    if ((x >= BOOK_PAGE_X) and (y >= BOOK_PAGE_Y) and (y <= BOOK_PAGE_Y + BOOK_PAGE_H))
    {
      if (touch_type == TOUCH_TYPE_SDOWN)
      {
        Serial.println("Touched Reading Page: Swiped Down");
        currentSection++;
        // previousSectionIndex = currentSectionIndex;
        // currentSectionIndex = nextSectionIndex;
        if (currentSection >= totalSections)
        {
          currentSection = totalSections - 1;
        }
        else
        {
          loadSectionContent();
          renderScreen(READING_SCREEN, true, true);
        }
        libraryManager.setCurrentSection(currentSection);
        // libraryManager.setCurrentSectionIndex(currentSectionIndex);
        flushTS(50);

        return true;
      }
      else if (touch_type == TOUCH_TYPE_SUP)
      {
        Serial.println("Touched Reading Page: Swiped Up");
        currentSection--;
        // previousSectionIndex = currentSectionIndex; TODO
        // currentSectionIndex = previousSectionIndex;
        if (currentSection < 0)
        {
          currentSection = 0;
        }
        else
        {
          loadSectionContent();
          renderScreen(READING_SCREEN, true, true);
        }
        libraryManager.setCurrentSection(currentSection);
        // libraryManager.setCurrentSectionIndex(currentSectionIndex);
        flushTS(50);

        return true;
      }
      else if (touch_type == TOUCH_TYPE_SLEFT)
      {
        Serial.println("Touched Reading Page: Swiped Left");
        if (libraryManager.prevPage())
        {
          currentSection = 0;
          // previousSectionIndex = 0;
          // currentSectionIndex = 0;
          libraryManager.setCurrentSection(currentSection);
          // libraryManager.setCurrentSectionIndex(currentSectionIndex);
          loadPageSections();
          loadSectionContent();
          renderScreen(READING_SCREEN, true, true);
        }
        flushTS(50);

        return true;
      }
      else if (touch_type == TOUCH_TYPE_SRIGHT)
      {
        Serial.println("Touched Reading Page: Swiped Right");
        if (libraryManager.nextPage())
        {
          currentSection = 0;
          // previousSectionIndex = 0;
          // currentSectionIndex = 0;
          libraryManager.setCurrentSection(currentSection);
          // libraryManager.setCurrentSectionIndex(currentSectionIndex);
          loadPageSections();
          loadSectionContent();
          renderScreen(READING_SCREEN, true, true);
        }
        flushTS(50);

        return true;
      }
    }
  }
  return false;
}

bool UIManager::handleTouchMainHeader(uint16_t x, uint16_t y, uint8_t touch_type)
{
  if (currentScreen == MAIN_SCREEN)
  {
    if ((x >= MAIN_HEADER_ITEM_SETTINGS[0]) and (y >= MAIN_HEADER_ITEM_SETTINGS[1]) and (x <= (MAIN_HEADER_ITEM_SETTINGS[0] + HEADER_ITEM_SIZE)) and (y <= (MAIN_HEADER_ITEM_SETTINGS[1] + HEADER_ITEM_SIZE)))
    {
      if (touch_type == TOUCH_TYPE_SHORT)
      {
        Serial.println("Touched Header Item Settings: Short Touch");
        renderScreen(SETTINGS_SCREEN);
        flushTS(50);
        return true;
      }
      else if (touch_type == TOUCH_TYPE_LONG)
      {
        Serial.println("Touched Header Item Settings: Long Touch");
        inSnakeMode = true;
        snakeGame->begin();
        return true;
      }
    }
  }
  return false;
}

void UIManager::drawBattery(bool invert)
{
  setFont(FONT_ALT, FONT_SIZE_SMALL);
  display->setTextColor(WHITE);
  float battery_perc = settingsManager.getBatteryPerc();
  drawIcon(ICON_BATTERY, TOT_W - ICON_BATTERY_W, 10, invert);
  display->setCursor(TOT_W - 63, 35);
  display->print(battery_perc, 0);
}

void UIManager::renderLoadingMsg(const String &message, bool partial_update)
{

  // Draw background box
  display->fillRoundRect(LOADING_MSG_X, LOADING_MSG_Y, LOADING_MSG_W, LOADING_MSG_H, 10, WHITE);
  display->drawRoundRect(LOADING_MSG_X, LOADING_MSG_Y, LOADING_MSG_W, LOADING_MSG_H, 10, BLACK);

  // Draw loading text
  setFont(FONT_ALT, FONT_SIZE_LARGE);
  display->setTextColor(BLACK);
  int textX = LOADING_MSG_X + (LOADING_MSG_W - message.length() * 14) / 2;
  int textY = LOADING_MSG_Y + LOADING_MSG_H / 1.5;
  display->setCursor(textX, textY);
  display->print(message);

  // Optionally a simple spinner animation (3 dots)
  // if(PARTIAL_UPDATE_ALLOWED) {
  //   for (int i = 0; i < 3; i++) {
  //       display->fillCircle(boxX + boxW - 30 + i * 8, boxY + boxH/2, 2, BLACK);
  //       delay(150);
  //       display->partialUpdate();
  //   }
  // }

  if (partial_update)
  {
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
  else
  {
    display->display();
  }
}

void UIManager::renderLoadingIcon(bool partial_update)
{
  drawIcon(HEADER_ITEM_LOADING_ICON, HEADER_ITEM_LOADING[0] + (HEADER_ITEM_SIZE - HEADER_ICON_SIZE) / 2, HEADER_ITEM_LOADING[1] + (HEADER_ITEM_SIZE - HEADER_ICON_SIZE) / 2, true);

  if (partial_update)
  {
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
  else
  {
    display->display();
  }
}

void UIManager::renderSettings(bool partial_update)
{

  display->setTextColor(BLACK);
  setFont(FONT_ALT, FONT_SIZE_DEFAULT);
  display->drawRect(SETTINGS_PAGE_X, SETTINGS_PAGE_Y, SETTINGS_PAGE_W, SETTINGS_PAGE_H, BLACK);

  display->fillRect(SETTINGS_TAB_1[0], SETTINGS_TAB_1[1], SETTINGS_TAB_1[2], SETTINGS_TAB_1[3], BLACK);
  drawIcon(SETTINGS_TAB_ICON[0], SETTINGS_TAB_1[0] + (SETTINGS_TAB_SIZE - SETTINGS_TAB_ICON_SIZE) / 2, SETTINGS_TAB_1[1] + (SETTINGS_TAB_SIZE - SETTINGS_TAB_ICON_SIZE) / 2, true);
  display->drawRect(SETTINGS_TAB_2[0], SETTINGS_TAB_2[1], SETTINGS_TAB_2[2], SETTINGS_TAB_2[3], BLACK);
  drawIcon(SETTINGS_TAB_ICON[1], SETTINGS_TAB_2[0] + (SETTINGS_TAB_SIZE - SETTINGS_TAB_ICON_SIZE) / 2, SETTINGS_TAB_2[1] + (SETTINGS_TAB_SIZE - SETTINGS_TAB_ICON_SIZE) / 2);
  display->drawRect(SETTINGS_TAB_3[0], SETTINGS_TAB_3[1], SETTINGS_TAB_3[2], SETTINGS_TAB_3[3], BLACK);
  drawIcon(SETTINGS_TAB_ICON[2], SETTINGS_TAB_3[0] + (SETTINGS_TAB_SIZE - SETTINGS_TAB_ICON_SIZE) / 2, SETTINGS_TAB_3[1] + (SETTINGS_TAB_SIZE - SETTINGS_TAB_ICON_SIZE) / 2);

  display->setCursor(SETTINGS_ITEM_1[0], SETTINGS_ITEM_1[1]);
  display->print("Backlight");
  display->setCursor(SETTINGS_PAGE_X + SETTINGS_PAGE_W - 50, SETTINGS_ITEM_1[1]);
  display->print(settingsManager.getBacklight());
  display->drawThickLine(SETTINGS_SEP_1[0], SETTINGS_SEP_1[1], SETTINGS_SEP_1[2], SETTINGS_SEP_1[3], BLACK, SETTINGS_SEP_1[4]);

  display->setCursor(SETTINGS_ITEM_2[0], SETTINGS_ITEM_2[1]);
  display->print("Gestures");
  if (settingsManager.getGestures())
  {
    drawIcon(SETTINGS_ITEM_STATUS_ICON[1], SETTINGS_PAGE_X + SETTINGS_PAGE_W - 70, SETTINGS_ITEM_2[1] - SETTINGS_ITEM_STATUS_ICON_SIZE / 2 - FONT_SIZE_DEFAULT_PX / 2);
  }
  else
  {
    drawIcon(SETTINGS_ITEM_STATUS_ICON[0], SETTINGS_PAGE_X + SETTINGS_PAGE_W - 70, SETTINGS_ITEM_2[1] - SETTINGS_ITEM_STATUS_ICON_SIZE / 2 - FONT_SIZE_DEFAULT_PX / 2);
  }
  display->drawThickLine(SETTINGS_SEP_2[0], SETTINGS_SEP_2[1], SETTINGS_SEP_2[2], SETTINGS_SEP_2[3], BLACK, SETTINGS_SEP_2[4]);

  display->setCursor(SETTINGS_ITEM_3[0], SETTINGS_ITEM_3[1]);
  display->print("Webserver");
  if (settingsManager.getWebserver())
  {
    drawIcon(SETTINGS_ITEM_STATUS_ICON[1], SETTINGS_PAGE_X + SETTINGS_PAGE_W - 70, SETTINGS_ITEM_3[1] - SETTINGS_ITEM_STATUS_ICON_SIZE / 2 - FONT_SIZE_DEFAULT_PX / 2);
  }
  else
  {
    drawIcon(SETTINGS_ITEM_STATUS_ICON[0], SETTINGS_PAGE_X + SETTINGS_PAGE_W - 70, SETTINGS_ITEM_3[1] - SETTINGS_ITEM_STATUS_ICON_SIZE / 2 - FONT_SIZE_DEFAULT_PX / 2);
  }

  if (partial_update)
  {
    //   Serial.println("Partial Update");
    if (PARTIAL_UPDATE_ALLOWED)
      display->partialUpdate();
    else
      display->display();
  }
}

/**
 * @brief     Get name of the pucture, create path and draw image on the screen.
 *
 * @return    0 if there is an error, 1 if the image is drawn.
 */
bool UIManager::drawIcon(String iconName, int x, int y, bool invert)
{

  SdFile iconFile;
  String iconsFolder = ICONS_FOLDER;
  String iconFileName = iconName;
  if (iconFileName.lastIndexOf(".") == -1)
  {
    iconFileName = iconFileName + ".png";
  }

  String iconPath = iconsFolder + iconFileName;

  if (!iconFile.open(iconPath.c_str(), O_RDONLY))
  {
    // Serial.println(iconName_str);
    Serial.println("Failed to load icon");
  }

  // Draw the image on the screen
  if (!display->drawImage(iconPath.c_str(), x, y, 1, invert))
  {
    // Close folder and file
    iconFile.close();

    // Return 0 to signalize an error
    return false;
  }

  return 1;
}

void UIManager::processTextBlock(
    const String &text, int startX, int startY, int width, int height,
    std::function<void(const String &, int, int, int)> onPrintLine,
    std::function<void(const String &, int, int, int, int)> onRenderImage,
    std::function<void(size_t)> onPageBreak,
    int startIdx)
{
  int cursorX = startX;
  int cursorY = startY;
  int normalLineHeight = NORMAL_LINE_HEIGHT;
  int headerLineHeight = HEADER_LINE_HEIGHT;
  int remainingHeight = height;

  String currentLine = "";
  String word = "";
  bool inHeader = false;
  bool pendingFontReset = false;
  // Track how many raw characters from 'text' have been consumed relative to startIdx.
  // This aligns sectionStarts with EpubParser::extractTextFromElement targetStart.
  size_t consumedFromInput = 0;

  // Serial.println("ProcessTextBlock Input txt: "+String(text));
  auto flushCurrentLine = [&](int idx)
  {
    int lineHeight = inHeader ? headerLineHeight : normalLineHeight;
    if (!currentLine.isEmpty())
    {
      if (remainingHeight < lineHeight)
      {
        // Notify with raw characters consumed so far from the source string
        onPageBreak(consumedFromInput);
        // Early return: stop processing further characters after page break
        return true; // signal break occurred
      }
      consumedFromInput = idx;
      onPrintLine(currentLine, cursorX, cursorY, lineHeight);
      cursorY += lineHeight;
      remainingHeight -= lineHeight;
      currentLine = "";
    }
    return false; // no break
  };

  for (size_t i = startIdx; i < text.length(); i++)
  {
    // Normalize consumedFromInput at the start of each iteration
    // consumedFromInput = i - startIdx;
    if (text.substring(i, i + 10) == "<img src=\"")
    {
      if (flushCurrentLine(i))
        return; // break occurred
      if (!word.isEmpty())
      {
        currentLine += word + " ";
        word = "";
        if (flushCurrentLine(i))
          return;
      }
      int closingQuote = text.indexOf("\"", i + 10);
      if (closingQuote == -1)
        continue;

      String imgPath = text.substring(i + 10, closingQuote);
      i = closingQuote + 2;
      // consumedFromInput = i - startIdx; // we've consumed up to after the image tag
      imgPath = "/library/" + libraryManager.getCurrentBook() + "/" + pagePath + imgPath;
      imgPath = sdHandler.normalizePath(imgPath);

      int imgW, imgH;
      sdHandler.getImageDimensions(imgPath, imgW, imgH);

      if (remainingHeight < imgH)
      {
        Serial.printf("[processTextBlock] PageBreak on image: consumed=%u i=%u startIdx=%u\n", (unsigned)consumedFromInput, (unsigned)i, (unsigned)startIdx);
        onPageBreak(consumedFromInput);
        return; // early exit on page break
      }
      onRenderImage(imgPath, cursorX, cursorY, imgW, imgH);
      cursorY += imgH;
      remainingHeight -= imgH;
      continue;
    }

    if (text.substring(i, i + 3) == "<h>")
    {
      flushCurrentLine(i);
      inHeader = true;
      setFont(FONT_PRIM, FONT_SIZE_LARGE);
      i += 2;
      // consumedFromInput = i - startIdx;
      continue;
    }

    if (text.substring(i, i + 4) == "</h>")
    {
      pendingFontReset = true;
      i += 3;
      // consumedFromInput = i - startIdx;
      continue;
    }

    char c = text[i];
    int lineHeight = inHeader ? headerLineHeight : normalLineHeight;

    if (c == ' ' || c == '\n')
    {
      int16_t x1, y1;
      uint16_t wordWidth, h;
      display->getTextBounds(currentLine + word, 0, 0, &x1, &y1, &wordWidth, &h);
      if (wordWidth > width)
      {
        if (flushCurrentLine(i - word.length()))
          return;
      }

      currentLine += word + " ";
      word = "";

      if (c == '\n')
      {
        if (flushCurrentLine(i))
          return;
      }
    }
    else
    {
      // Serial.println("Consumed character: "+String(c));
      word += c;
      // consumedFromInput = i + 1 - startIdx; // include this character
    }

    if (pendingFontReset && word.isEmpty() && currentLine.isEmpty())
    {
      if (remainingHeight < headerLineHeight)
      {
        Serial.printf("[processTextBlock] PageBreak on header reset: consumed=%u i=%u startIdx=%u\n", (unsigned)consumedFromInput, (unsigned)i, (unsigned)startIdx);
        onPageBreak(consumedFromInput);
        return; // early exit
      }
      cursorY += headerLineHeight;
      remainingHeight -= headerLineHeight;
      setFont(FONT_PRIM, FONT_SIZE_DEFAULT);
      inHeader = false;
      pendingFontReset = false;
    }
  }

  if (!word.isEmpty())
    currentLine += word + " ";
  flushCurrentLine(text.length()); // last line; ignore break signal here intentionally
  // At end, we don't force page break; caller can interpret final page char count if needed
}

TextRenderResult UIManager::renderTextBlockSection(
    const String &text, int startX, int startY, int width, int height, int pageNum, int startIdx)
{
  setFont(FONT_PRIM, FONT_SIZE_DEFAULT);

  int currentPage = 0;
  bool done = false;
  display->setTextColor(BLACK);
  bool printing = false;

  String printedText = "";
  int textStartIdx = -1;
  int textEndIdx = -1;
  TextRenderResult result;

  processTextBlock(
      text, startX, startY, width, height,
      [&](const String &line, int x, int y, int lineHeight)
      {
        // if (currentPage == pageNum) {
        if (!done)
        {
          if (textStartIdx == -1)
            textStartIdx = startIdx;
          display->setCursor(x, y);
          display->print(line);
          printing = true;
          printedText += line + "\n";
        }
        // Serial.println(line);
        startIdx += line.length() + 1; // +1 for space or newline separation
      },
      [&](const String &path, int x, int y, int w, int h)
      {
        // if (currentPage == pageNum) {
        if (!done)
        {
          renderImage(path, x, y);
          printing = true;
        }
        // Images don't count towards text index increments
      },
      [&](size_t consumedFromInput)
      {
        // Page break occurred; we stop rendering further lines.
        done = true;
        // result.endIndex += startIdx;
        result.endIndex += consumedFromInput;
        // Could record printedCharsBeforeBreak if needed (e.g., result.pageCharCount)
      },
      startIdx // pass startIdx down into processTextBlock
  );

  textEndIdx = startIdx;

  result.printedText = printedText;
  result.startIndex = textStartIdx;
  // previousSectionIndex = currentSectionIndex;
  // currentSectionIndex = result.startIndex;
  nextSectionIndex = result.endIndex + 1;
  // Serial.println("previousSectionIndex: " + String(previousSectionIndex));
  // Serial.println("currentSectionIndex: " + String(currentSectionIndex));
  // Serial.println("nextSectionIndex: " + String(nextSectionIndex));
  // Serial.println("printed text: " + String(printedText));

  return result;
}

int UIManager::sectionsForTextBlock(int startX, int startY, int width, int height)
{
  // Streaming implementation: use epubParser.getPageContent(book,page,start,len) to walk the entire page
  // and simulate layout with processTextBlock over each accumulated slice. We avoid building the full page string.
  sectionStarts.clear();
  // Ensure font matches renderTextBlockSection for consistent measurements
  setFont(FONT_PRIM, FONT_SIZE_DEFAULT);
  // Ensure pagePath is set so image dimensions resolve correctly during layout
  pagePath = libraryManager.getCurrentPagePath();
  int currentPage = libraryManager.getCurrentPage();
  size_t globalStart = 0;        // start offset for next slice request
  const size_t SLICE_LEN = 2048; // tune slice size
  String accumulated = "";       // unused with direct slice processing
  sectionStarts.push_back(0);    // first section

  // We will feed chunks into processTextBlock, but we need a way to stop mid-buffer where a page break occurs.
  // Strategy: run processTextBlock completely on accumulated, then discard consumed part.
  // For simplicity, we will reset after each chunk; page indices approximate raw character positions.
  // NOTE: This assumes tags (<h>, <img>) are included; their characters count in raw offsets.

  bool end = false;
  while (!end)
  {
    String slice = epubParser.getPageContent(libraryManager.getCurrentBook(), currentPage, sectionStarts.back(), SLICE_LEN);
    if (slice.isEmpty())
    {
      end = true;
      break;
    }
    // accumulated += slice; // append new slice
    // Serial.println(slice);
    // Layout this accumulated block
    processTextBlock(
        slice, startX, startY, width, height,
        [&](const String &line, int x, int y, int lineHeight)
        {
          // Rendering callback not used for index calculation here
          // Serial.println(line);
        },
        [&](const String &path, int x, int y, int w, int h)
        {
          // Image callback not used for index calculation here
        },
        [&](size_t consumedFromInput)
        {
          // consumedFromInput is the number of characters from this slice/page before break.
          // Serial.println("consumedFromInput: " + String(consumedFromInput));
          sectionStarts.push_back(sectionStarts.back() + consumedFromInput);
        });

    // Advance start for next slice based on latest sectionStarts
    globalStart = sectionStarts.back();
    // If less than requested slice length, we've reached end
    if (slice.length() < SLICE_LEN)
      end = true;
    // Accumulated buffer fully processed; clear to avoid re-processing
    // accumulated = ""; // we rely solely on processedChars continuing from previous
  }
  return (int)sectionStarts.size();
}

void UIManager::renderImage(const String &imgPath, int x, int y, bool invert)
{
  Serial.printf("Rendering image at %d, %d: %s\n", x, y, imgPath.c_str());

  SdFile imgFile;
  if (!imgFile.open(imgPath.c_str(), O_RDONLY))
  {
    Serial.println("Image not found!");
    return;
  }

  int imgW, imgH;
  if (!sdHandler.getImageDimensions(imgPath, imgW, imgH))
  {
    Serial.println("Failed to get image dimensions.");
    imgFile.close();
    return;
  }
  // TJpgDec->setJpgScale(4);
  display->drawImage(imgPath.c_str(), x, y, 1, invert);
  imgFile.close();
  return;
}
