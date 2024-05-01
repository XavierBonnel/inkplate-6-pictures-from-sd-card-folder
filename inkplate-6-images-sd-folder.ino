/*
   Inkplate6_Image_Frame_From_SD example for Soldered Inkplate6
   For this example you will need a micro USB cable, Inkplate6 and a SD card loaded with images.
   Select "e-radionica Inkplate6" or "Soldered Inkplate6" from Tools -> Board menu.
   Don't have "e-radionica Inkplate6" or "Soldered Inkplate6" option? Follow our tutorial and add it:
   https://soldered.com/learn/add-inkplate-6-board-definition-to-arduino-ide/

   You can open .bmp, .jpeg, or .png files that have a color depth of 1-bit (BW bitmap), 4-bit, 8-bit and
   24 bit, but there are some limitations of the library. It will skip images that can't be drawn.
   Make sure that the image has a resolution smaller than 800x600 or otherwise it won't fit on the screen.
   Format your SD card in standard FAT file format.

   This example will show you how you can make slideshow images from an SD card. Put your images on
   the SD card in a file and specify the file path in the sketch. If you don't want to wait defined delay time,
   you can press the wake button and the next image will be loaded on the screen. It will take some time until 
   the image will be loaded.

   Want to learn more about Inkplate? Visit www.inkplate.io
   Looking to get support? Write on our forums: https://forum.soldered.com/
   12 April 2023 by Soldered
*/

// Precautionary error check: Ensure the correct board is selected in the Arduino IDE
#if !defined(ARDUINO_ESP32_DEV) && !defined(ARDUINO_INKPLATE6V2)
#error "Wrong board selection for this example, please select e-radionica Inkplate6 or Soldered Inkplate6 in the boards menu."
#endif

/******************CHANGE HERE***********************/
// Set the time between changing 2 images in seconds
#define SECS_BETWEEN_PICTURES 30  // Change to 30 seconds

// Path to the folder with pictures (e.g., there is a folder called images on the SD card)
const char folderPath[] = "/images/"; // NOTE: Must end with /

/****************************************************/

#include "Inkplate.h"            // Include Inkplate library to the sketch
Inkplate display(INKPLATE_3BIT); // Create an object on Inkplate library and also set library into 3 Bit mode
SdFile folder, file;             // Create SdFile objects used for accessing files on SD card

// Last image index stored in RTC RAM that stores variable even if deep sleep is used
RTC_DATA_ATTR uint16_t lastImageIndex = 0;

void setup()
{
    display.begin();             // Init Inkplate library (you should call this function ONLY ONCE)
    display.clearDisplay();      // Clear frame buffer of display
    display.setCursor(0, 0);     // Set the cursor on the beginning of the screen
    display.setTextColor(BLACK); // Set text color to black
    display.setTextSize(5);      // Scale text to be five times bigger then original (5x7 px)
}

void loop()
{
    // If the folder is empty print a message and go to the sleep
    if (!getFileCount())
    {
        display.println("The folder is empty");
        display.display();
        deepSleep();
    }

    // Open directory with pictures
    if (folder.open(folderPath))
    {
        openLastFile(); // Open the last opened file if it's not the beginning of the file

        // If it's the beginning of the file, just open the next file
        if (!file.openNext(&folder, O_RDONLY))
        {
            lastImageIndex = 0; // If it can't open the next file, there is an end of the file so set the index of the last file to 0
        }
        else
        {
            lastImageIndex = file.dirIndex(); // Save the index of the last opened file
            skipHidden(); // Skip hidden files and subdirectories

            if (!displayImage()) // Get name of the picture, create path and draw image on the screen
            {
                return; // Reset the loop if there is an error displaying the image
            }

            file.close(); // Close the file
        }
        folder.close(); // Close the folder
    }
    else
    {
        display.printf("Error opening folder! Make sure \nthat you have entered the proper \nname and add / to the end "
                       "of the \npath");
        display.display();
        deepSleep();
    }

    if (lastImageIndex != 0)
    {
        esp_sleep_enable_timer_wakeup(SECS_BETWEEN_PICTURES * 1000000LL); // Set EPS32 to be woken up in specified seconds
        deepSleep(); // Go to the deep sleep
    }
}

int getFileCount()
{
    if (!display.sdCardInit())
    {
        display.println("SD Card error!");
        display.display();
        deepSleep();
    }
    else
    {
        int fileCount = 0;
        if (folder.open(folderPath))
        {
            while (file.openNext(&folder, O_READ))
            {
                if (!file.isHidden())
                {
                    fileCount++;
                }
                file.close();
            }
            folder.close();
        }
        else
        {
            display.println("The folder doesn't exist");
            display.display();
            deepSleep();
        }
        return fileCount;
    }
}

void deepSleep()
{
    display.sdCardSleep();
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, LOW); // Enable wakeup from deep sleep on GPIO 36 (wake button)
    esp_deep_sleep_start();
}

void openLastFile()
{
    if (lastImageIndex != 0)
    {
        if (file.open(&folder, lastImageIndex, O_READ))
        {
            file.close(); // Close the file so the code below can open the next one
        }
    }
}

bool displayImage()
{
    char pictureName[100];
    file.getName(pictureName, sizeof(pictureName));

    char path[100];
    strcpy(path, folderPath);
    char *picturePath = strcat(path, pictureName);

    if (!display.drawImage(picturePath, 0, 0, 1, 0))
    {
        file.close();
        folder.close();
        return 0;
    }

    display.display();
    return 1;
}

void skipHidden()
{
    while (file.isHidden() || file.isSubDir())
    {
        file.close();
        if (!file.openNext(&folder, O_RDONLY))
        {
            lastImageIndex = 0;
        }
        else
        {
            lastImageIndex = file.dirIndex();
        }
    }
}
