/* errorlog.h - error.log writing, friendly message boxes, platform start-up. */
#ifndef ERRORLOG_H
#define ERRORLOG_H

/* Prepares the process: makes the folder of the program the working folder
 * (so "shaders/" and "error.log" are found no matter how it was started),
 * enables sharp rendering on high-DPI screens and a crash reporter (Windows). */
void bhs_platform_init(void);

/* Appends a line to error.log (created on first use) and to stderr. */
void bhs_log(const char *fmt, ...);

/* Logs the message and shows it to the user in a message box (Windows) or on
 * stderr (other systems). The text should be written for a beginner. */
void bhs_fatal(const char *title, const char *fmt, ...);

#endif
