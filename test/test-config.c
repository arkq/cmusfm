/*
 * cmusfm - test-config.c
 * SPDX-FileCopyrightText: 2026 Arkadiusz Bokowy and contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <unistd.h>

#include <check.h>

#include "../src/cmusfm-config.h"

/* Global variable used in the config code. */
const char * cmusfm_config_file;

/* The file created on the disk by this unit test. */
#define TEST_CONFIG_FILE "test-config.conf"

START_TEST(test_defaults) {

	FILE * f;
	ck_assert_ptr_nonnull(f = fopen(TEST_CONFIG_FILE, "w"));
	ck_assert_int_eq(fclose(f), 0);

	struct cmusfm_config conf;
	ck_assert_int_eq(cmusfm_config_read(TEST_CONFIG_FILE, &conf), 0);

	ck_assert_str_eq(conf.user_name, "");
	ck_assert_str_eq(conf.session_key, "");
	ck_assert_str_eq(conf.format_localfile, "^(?A.+) - (?T.+)\\.[^.]+$");
	ck_assert_str_eq(conf.format_shoutcast, "^(?A.+) - (?T.+)$");
	ck_assert_uint_eq(conf.nowplaying_localfile, true);
	ck_assert_uint_eq(conf.nowplaying_shoutcast, true);
	ck_assert_uint_eq(conf.submit_localfile, true);
	ck_assert_uint_eq(conf.submit_shoutcast, true);
	ck_assert_str_eq(conf.service_api_url, "https://ws.audioscrobbler.com/2.0/");
	ck_assert_str_eq(conf.service_auth_url, "https://www.last.fm/api/auth");
#if ENABLE_LIBNOTIFY
	ck_assert_str_eq(conf.format_coverfile, "^folder\\.jpg$");
	ck_assert_uint_eq(conf.notification, false);
#endif

}

START_TEST(test_config_read) {

	FILE * f;
	ck_assert_ptr_nonnull(f = fopen(TEST_CONFIG_FILE, "w"));
	/* Check if keys without a value are handled gracefully. */
	ck_assert_int_eq(fprintf(f, "user\n"), 5);
	/* Check if keys with empty value are handled gracefully. */
	ck_assert_int_eq(fprintf(f, "user=\n"), 6);
	/* Check if random keys are handled gracefully. */
	ck_assert_int_eq(fprintf(f, "non-existing-key = \"yes\"\n"), 25);
	/* Check if user-defined values are read from valid keys. */
	ck_assert_int_eq(fprintf(f, "user = \"test\"\n"), 14);
	ck_assert_int_eq(fprintf(f, "key = \"secret\"\n"), 15);
	ck_assert_int_eq(fprintf(f, "format-localfile = \".+\"\n"), 24);
	ck_assert_int_eq(fprintf(f, "format-shoutcast = \".+\"\n"), 24);
	ck_assert_int_eq(fprintf(f, "format-coverfile = \"cover\"\n"), 27);
	ck_assert_int_eq(fprintf(f, "now-playing-localfile=\"yes\"\n"), 28);
	ck_assert_int_eq(fprintf(f, "now-playing-shoutcast=\"no\"\n"), 27);
	ck_assert_int_eq(fprintf(f, "submit-localfile=     \"yes\"\n"), 28);
	ck_assert_int_eq(fprintf(f, "submit-shoutcast     =\"yes\"\n"), 28);
	ck_assert_int_eq(fprintf(f, "service-api-url = \"http://api.co\"\n"), 34);
	ck_assert_int_eq(fprintf(f, "service-auth-url = \"http://auth.co\"\n"), 36);
	/* Parser expects "yes" for true, nothing else. */
	ck_assert_int_eq(fprintf(f, "notification = \"y\"\n"), 19);
	ck_assert_int_eq(fclose(f), 0);

	struct cmusfm_config conf;
	ck_assert_int_eq(cmusfm_config_read(TEST_CONFIG_FILE, &conf), 0);

	ck_assert_str_eq(conf.user_name, "test");
	ck_assert_str_eq(conf.session_key, "secret");
	ck_assert_str_eq(conf.format_localfile, ".+");
	ck_assert_str_eq(conf.format_shoutcast, ".+");
	ck_assert_uint_eq(conf.nowplaying_localfile, true);
	ck_assert_uint_eq(conf.nowplaying_shoutcast, false);
	ck_assert_uint_eq(conf.submit_localfile, true);
	ck_assert_uint_eq(conf.submit_shoutcast, true);
	ck_assert_str_eq(conf.service_api_url, "http://api.co");
	ck_assert_str_eq(conf.service_auth_url, "http://auth.co");
#if ENABLE_LIBNOTIFY
	ck_assert_str_eq(conf.format_coverfile, "cover");
	ck_assert_uint_eq(conf.notification, false);
#endif

	ck_assert_int_eq(unlink(TEST_CONFIG_FILE), 0);

	/* Check case when the config file does not exist. */
	ck_assert_int_eq(cmusfm_config_read(TEST_CONFIG_FILE, &conf), -1);

}

START_TEST(test_config_write) {

	struct cmusfm_config conf = { 0 };
	strncpy(conf.user_name, "test", sizeof(conf.user_name) - 1);
	strncpy(conf.session_key, "secret", sizeof(conf.session_key) - 1);
	conf.submit_localfile = true;

	ck_assert_int_eq(cmusfm_config_write(TEST_CONFIG_FILE, &conf), 0);

	FILE * f;
	char buffer[1024] = "";
	ck_assert_ptr_nonnull(f = fopen(TEST_CONFIG_FILE, "r"));
	ck_assert_int_gt(fread(buffer, 1, sizeof(buffer), f), 0);
	ck_assert_int_eq(fclose(f), 0);

	ck_assert_str_eq(buffer,
	                 "# authentication\n"
	                 "user = \"test\"\n"
	                 "key = \"secret\"\n"
	                 "\n"
	                 "# regular expressions for name parsers\n"
	                 "format-localfile = \"\"\n"
	                 "format-shoutcast = \"\"\n"
#if ENABLE_LIBNOTIFY
	                 "format-coverfile = \"\"\n"
#endif
	                 "\n"
	                 "now-playing-localfile = \"no\"\n"
	                 "now-playing-shoutcast = \"no\"\n"
	                 "submit-localfile = \"yes\"\n"
	                 "submit-shoutcast = \"no\"\n"
#if ENABLE_LIBNOTIFY
	                 "notification = \"no\"\n"
#endif
	                 "\n"
	                 "# scrobbling service\n"
	                 "service-api-url = \"\"\n"
	                 "service-auth-url = \"\"\n");

}

static void tc_teardown(void) {
	/* Make sure that we will not leave any leftovers. */
	unlink(TEST_CONFIG_FILE);
}

int main(void) {

	Suite * s = suite_create(__FILE__);
	TCase * tc = tcase_create(__FILE__);
	SRunner * sr = srunner_create(s);

	suite_add_tcase(s, tc);
	tcase_add_checked_fixture(tc, NULL, tc_teardown);

	tcase_add_test(tc, test_defaults);
	tcase_add_test(tc, test_config_read);
	tcase_add_test(tc, test_config_write);

	srunner_run_all(sr, CK_ENV);
	int nf = srunner_ntests_failed(sr);

	srunner_free(sr);
	return nf == 0 ? 0 : 1;
}
