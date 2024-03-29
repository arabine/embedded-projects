#ifndef TCLIE_H
#define TCLIE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "tcli.h"

#ifndef TCLIE_ENABLE_USERS
/**
 * If users should be used.
 */
#define TCLIE_ENABLE_USERS 1
#endif

#ifndef TCLIE_ENABLE_USERNAMES
/**
 * If usernames should be used with users (otherwise only passwords are used).
 */
#define TCLIE_ENABLE_USERNAMES 1
#endif

#ifndef TCLIE_LOGIN_ATTEMPTS
/**
 * Number of login attempts before a username or password is rejected.
 */
#define TCLIE_LOGIN_ATTEMPTS 3
#endif

#ifndef TCLIE_PATTERN_MATCH
/**
 * Disable/enable pattern matching.
 */
#define TCLIE_PATTERN_MATCH 1
#endif

#ifndef TCLIE_PATTERN_MATCH_MAX_TOKEN
/**
 * Maximum number of tokens to use for pattern matching tokenization. Required
 * size depends on pattern complexities.
 */
#define TCLIE_PATTERN_MATCH_MAX_TOKENS TCLI_MAX_TOKENS
#endif

#ifndef TCLIE_PATTEN_MATCH_BUF_LEN
/**
 * Size of static buffer used to store tab-completions matches from patterns.
 */
#define TCLIE_PATTEN_MATCH_BUF_LEN 64
#endif

#ifndef TCLIE_SUCCESS_FORMAT
/**
 * Format for success output messages.
 */
#define TCLIE_SUCCESS_FORMAT TCLI_COLOR_GREEN
#endif

#ifndef TCLIE_FAILURE_FORMAT
/**
 * Format for failure output messages.
 */
#define TCLIE_FAILURE_FORMAT TCLI_COLOR_RED
#endif

#ifndef TCLIE_COMMAND_FORMAT
/**
 * Format used when listing command names in help command.
 */
#define TCLIE_COMMAND_FORMAT                                                   \
	(TCLI_FORMAT_BOLD TCLI_FORMAT_UNDERLINE TCLI_COLOR_CYAN)
#endif

#ifndef TCLIE_USAGE_FORMAT
/**
 * Format used when listing usage patterns in help command.
 */
#define TCLIE_USAGE_FORMAT (TCLI_COLOR_CYAN)
#endif

#ifndef TCLIE_OPTION_FORMAT
/**
 * Format used when listing command options in help command.
 */
#define TCLIE_OPTION_FORMAT (TCLI_FORMAT_ITALIC TCLI_COLOR_CYAN)
#endif

typedef tcli_out_fn_t tclie_out_fn_t;
typedef tcli_sigint_fn_t tclie_sigint_fn_t;

typedef int (*tclie_cmd_fn_t)(void *arg, int argc, const char **argv);

#if TCLIE_PATTERN_MATCH
typedef struct tclie_cmd_opt {
	char short_opt;
	const char *long_opt;
	const char *desc;
	const char *pattern;
} tclie_cmd_opt_t;

typedef struct tclie_cmd_opts {
	const tclie_cmd_opt_t *option;
	size_t count;
} tclie_cmd_opts_t;
#endif

// Command definition
typedef struct tclie_cmd {
	const char *name;  // Command
	tclie_cmd_fn_t fn; // Callback function
#if TCLIE_ENABLE_USERS
	unsigned min_user_level; // Minimum user level required for execution
#endif
	const char *desc; // Command description
#if TCLIE_PATTERN_MATCH
	const char *pattern;
	tclie_cmd_opts_t options;
#endif
} tclie_cmd_t;

typedef void (*tclie_pre_cmd_fn_t)(void *arg, int argc, const char **argv);
typedef void (*tclie_post_cmd_fn_t)(void *arg, int argc, const char **argv,
									int res);

typedef struct tclie_cmds {
	const tclie_cmd_t *cmds;
	size_t count;
} tclie_cmds_t;

#if TCLIE_ENABLE_USERS
typedef struct tclie_user {
#if TCLIE_ENABLE_USERNAMES
	const char *name; // Username
#endif
	const char *password; // Password
	unsigned level;		  // User level used for restricting command access.
} tclie_user_t;

typedef enum tclie_login_state {
	TCLIE_LOGIN_IDLE = 0,
#if TCLIE_ENABLE_USERNAMES
	TCLIE_LOGIN_USERNAME,
#endif
	TCLIE_LOGIN_PASSWORD
} tclie_login_state_t;

typedef struct tclie_login {
	tclie_login_state_t state;
#if TCLIE_ENABLE_USERNAMES
	size_t target_user;
#endif
	unsigned attempt;
} tclie_login_t;

typedef struct tclie_users {
	const tclie_user_t *users;
	size_t count;
	tclie_login_t login;
	unsigned level;
} tclie_users_t;
#endif

typedef struct tclie {
	tcli_t tcli;
	tclie_cmds_t cmd;
#if TCLIE_ENABLE_USERS
	tclie_users_t user;
#endif
	tclie_out_fn_t out;
	tclie_pre_cmd_fn_t pre_cmd;
	tclie_post_cmd_fn_t post_cmd;
	tclie_sigint_fn_t sigint;
	void *arg;
#if TCLIE_PATTERN_MATCH && TCLI_COMPLETE
	struct {
		char buf[TCLIE_PATTEN_MATCH_BUF_LEN];
		size_t buf_len;
	} complete;
#endif
} tclie_t;

/**
 * Initializes the instance.
 * @param tclie Instance pointer.
 * @param out Output callback function.
 * @param arg User data to be passed to callback functions.
 */
void tclie_init(tclie_t *tclie, tclie_out_fn_t out, void *arg);

#if TCLIE_ENABLE_USERS

/**
 * Register users.
 * @param tclie Instance pointer.
 * @param users Pointer to user definition array.
 * @param count Number of users in specified array.
 * @return True on success, false on failure.
 */
bool tclie_reg_users(tclie_t *tclie, const tclie_user_t *users, size_t count);

#endif

/**
 * Register commands.
 * @param tclie Instance pointer.
 * @param cmds Pointer to command definition array.
 * @param count Number of commands in specified array.
 * @return True on success, false on failure.
 */
bool tclie_reg_cmds(tclie_t *tclie, const tclie_cmd_t *cmds, size_t count);

/**
 * Pass input to be processed.
 * @param tclie Instance pointer.
 * @param c Character to process.
 */
void tclie_input_char(tclie_t *tclie, char c);

/**
 * Pass string input to be processed.
 * @param tclie Instance pointer.
 * @param str Null-terminated string to process.
 */
void tclie_input_str(tclie_t *tclie, const char *str);

/**
 * Pass data input to be processed.
 * @param tclie Instance pointer.
 * @param buf Pointer to data buffer.
 * @param len Data length.
 */
void tclie_input(tclie_t *tclie, const void *buf, size_t len);

/**
 * Set callback function for output.
 * @param tclie Instance pointer.
 * @param out Output callback function.
 */
void tclie_set_out(tclie_t *tclie, tclie_out_fn_t out);

/**
 * Set user data for callback functions.
 * @param tclie Instance pointer.
 * @param arg Pointer to user data.
 */
void tclie_set_arg(tclie_t *tclie, void *arg);

/**
 * Set echo mode for output.
 * @param tclie Instance pointer.
 * @param mode Echo mode.
 */
void tclie_set_echo(tclie_t *tclie, tcli_echo_mode_t mode);

/**
 * Set callback function for SIGINT (ctrl+c).
 * @param tclie Instance pointer.
 * @param sigint Callback function.
 */
void tclie_set_sigint(tclie_t *tclie, tclie_sigint_fn_t sigint);

/**
 * Set pre-command callback function to be called before command execution.
 * @param tclie Instance pointer.
 * @param pre_cmd Callback function.
 */
void tclie_set_pre_cmd(tclie_t *tclie, tclie_pre_cmd_fn_t pre_cmd);

/**
 * Set post-command callback function to be called after command execution.
 * @param tclie Instance pointer.
 * @param pre_cmd Callback function.
 */
void tclie_set_post_cmd(tclie_t *tclie, tclie_post_cmd_fn_t post_cmd);

/**
 * Set default prompt string.
 * @param tclie Instance pointer.
 * @param prompt Prompt string.
 */
void tclie_set_prompt(tclie_t *tclie, const char *prompt);

/**
 * Set error prompt string (used when previous command failed).
 * @param tclie Instance pointer.
 * @param error_prompt Prompt string.
 */
void tclie_set_error_prompt(tclie_t *tclie, const char *error_prompt);

#if TCLI_HISTORY_BUF_LEN > 0

/**
 * Set history mode for recording entered lines.
 * @param tclie Instance pointer.
 * @param mode History mode.
 */
void tclie_set_hist(tclie_t *tclie, tcli_history_mode_t mode);

/**
 * Set search prompt (used when backwards-searching).
 * @param tclie Instance pointer.
 * @param search_prompt Prompt string.
 */
void tclie_set_search_prompt(tclie_t *tclie, const char *search_prompt);

#endif

#if TCLIE_ENABLE_USERS

/**
 * Set current user level (login as specific user level).
 * @param tclie Instance pointer.
 * @param user_level User level.
 */
void tclie_set_user_level(tclie_t *tclie, unsigned user_level);

/**
 * Get current user level.
 * @param tclie Instance pointer.
 * @return Current user level or 0 on failure.
 */
unsigned tclie_get_user_level(const tclie_t *tclie);
#endif

/**
 * Logs a string without disturbing the current prompt.
 * @param tclie Instance pointer.
 * @param str String.
 */
void tclie_log(tclie_t *tclie, const char *str);

/**
 * Logs formatted data from variable argument list without disturbing the
 * current prompt.
 * @param tclie Instance pointer.
 * @param buf Buffer to hold formatted data.
 * @param len Buffer length.
 * @param format Format string.
 * @param arg Variable arguments list.
 * @return On success, the total number of characters written, else -1.
 */
int tclie_log_vprintf(tclie_t *tclie, char *buf, size_t len, const char *format,
					  va_list arg);

/**
 * Logs formatted string without disturbing the current prompt.
 * @param tclie Instance pointer.
 * @param buf Buffer to hold formatted data.
 * @param len Buffer length.
 * @param format Format string.
 * @param ... Arguments depending on format string.
 * @return On success, the total number of characters written, else -1.
 */
int tclie_log_printf(tclie_t *tclie, char *buf, size_t len, const char *format,
					 ...);

/**
 * Flushes output buffer (if used).
 * @param tclie Instance pointer.
 */
void tclie_flush(tclie_t *tclie);

/**
 * Outputs a string through the instance output callback function. May be
 * buffered.
 * @param tclie Instance pointer.
 * @param str String to output.
 */
void tclie_out(tclie_t *tclie, const char *str);

/**
 * Outputs data from variable argument list without disturbing the
 * current prompt. May be buffered.
 * @param tcli Instance pointer.
 * @param buf Buffer to hold formatted data.
 * @param len Buffer length.
 * @param format Format string.
 * @param arg Variable arguments list.
 * @return On success, the total number of characters written, else -1.
 */
int tclie_out_vprintf(tclie_t *tclie, char *buf, size_t len, const char *format,
					  va_list arg);

/**
 * Outputs formatted string without disturbing the current prompt. May be
 * buffered.
 * @param tcli Instance pointer.
 * @param buf Buffer to hold formatted data.
 * @param len Buffer length.
 * @param format Format string.
 * @param ... Arguments depending on format string.
 * @return On success, the total number of characters written, else -1.
 */
int tclie_out_printf(tclie_t *tclie, char *buf, size_t len, const char *format,
					 ...);

/**
 * Clears the current screen output.
 * @param tclie Instance pointer.
 */
void tclie_clear_screen(tclie_t *tclie);

#ifdef __cplusplus
}
#endif


#endif
