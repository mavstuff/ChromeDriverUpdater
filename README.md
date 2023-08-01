# ChromeDriverUpdater

This application is intended to keep the actual ChromeDriver depending on the Chrome version currently installed.

Due to the constant Chrome updated, the chromedriver should be also updated.

ChromeDriverUpdater checks installed Chrome version and current Chromedriver.
If the versions are different, the app downloads the corresponding chromediver from the update site.

## Updates
2023-08-01:
- support Chrome URLs >= 115 (Chrome for Testing JSON endpoints)

## TO DO LIST
- Multiplatform
- Universal algorithm to find Chrome Path
- Move to cURL lib
 
(c) 2022-2023 Artem Moroz <artem.moroz@gmail.com>