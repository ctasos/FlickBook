#include "UIManager.h"
#include "EpubParser.h"

extern EpubParser epubParser;
extern SDHandler sdHandler;
extern LibraryManager libraryManager;
extern SettingsManager settingsManager;

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
            // Before handling the image, ensure any pending text is flushed
            if (flushCurrentLine(i))
                return; // a break occurred while flushing text
            if (!word.isEmpty())
            {
                currentLine += word + " ";
                word = "";
                if (flushCurrentLine(i))
                    return;
            }

            // Parse image tag and resolve path
            int closingQuote = text.indexOf("\"", i + 10);
            if (closingQuote == -1)
                continue;
            String imgPath = text.substring(i + 10, closingQuote);
            int tagEndIndex = closingQuote + 2; // position after closing quote and trailing character(s)

            String fullImgPath = "/library/" + libraryManager.getCurrentBook() + "/" + pagePath + imgPath;
            fullImgPath = sdHandler.normalizePath(fullImgPath);

            int imgW, imgH;
            sdHandler.getImageDimensions(fullImgPath, imgW, imgH);

            // If image is taller than the page height, ensure it's rendered starting on a fresh page.
            if (imgH > height)
            {
                // If we're not at the top of the page, break now and start next page from the image tag
                if (cursorY != startY)
                {
                    onPageBreak((size_t)(i - startIdx));
                    return;
                }
                // We are at the top: render the oversized image anyway
                onRenderImage(fullImgPath, cursorX, cursorY, imgW, imgH);
                cursorY += imgH;
                remainingHeight = 0;                                  // no more space on this page
                consumedFromInput = (size_t)(tagEndIndex - startIdx); // advance past the image tag for next section
                onPageBreak(consumedFromInput);
                return;
            }

            // Image fits within a page but not the remaining space: break to next page starting at image
            if (remainingHeight < imgH)
            {
                onPageBreak((size_t)(i - startIdx));
                return;
            }

            // Render image on current page
            onRenderImage(fullImgPath, cursorX, cursorY, imgW, imgH);
            cursorY += imgH;
            remainingHeight -= imgH;

            // Advance parser index to after the image tag and continue
            i = tagEndIndex;
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
    display->setTextColor(settingsManager.getFgColor());
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

    // Try to load cached sections from storage
    std::vector<size_t> cachedSections;
    if (libraryManager.loadCurrentPageSections(cachedSections) && !cachedSections.empty())
    {
        sectionStarts = cachedSections;
        return (int)sectionStarts.size();
    }

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
                // TODO: There is still a small bug sometime loosing a section when there are in page images
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
    // Persist computed sections for future loads
    libraryManager.saveCurrentPageSections(sectionStarts);
    return (int)sectionStarts.size();
}
